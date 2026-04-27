#define _POSIX_C_SOURCE 200809L

#include "noise.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t swatch_noise_stop = 0;
static int swatch_noise_cursor_hidden = 0;
static int swatch_noise_altscreen_on = 0;

static const int CUBE_STEPS[6] = {0, 95, 135, 175, 215, 255};

static int cube_index(uint8_t channel) {
    int best = 0;
    int best_dist = CUBE_STEPS[0] - channel;
    if (best_dist < 0) best_dist = -best_dist;
    for (int i = 1; i < 6; i++) {
        int d = CUBE_STEPS[i] - channel;
        if (d < 0) d = -d;
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    return best;
}

static void handle_signal(int sig) {
    (void)sig;
    swatch_noise_stop = 1;
}

static void restore_cursor_atexit(void) {
    if (swatch_noise_cursor_hidden) {
        fputs("\x1b[0m\x1b[?25h", stderr);
        swatch_noise_cursor_hidden = 0;
    }
    if (swatch_noise_altscreen_on) {
        fputs("\x1b[?1049l", stderr);
        swatch_noise_altscreen_on = 0;
    }
}

static unsigned rand_unsigned(void) {
    return (unsigned)rand();
}

/*
 * Sleep until the given absolute CLOCK_MONOTONIC deadline.
 *
 * Linux/glibc has clock_nanosleep(TIMER_ABSTIME, ...); macOS shipped
 * clock_gettime in 10.12 but never added clock_nanosleep, so we fall back
 * to computing the remaining interval and nanosleep()ing that. Returns
 * immediately if the deadline is already in the past.
 *
 * Retries on EINTR so a non-stop signal (e.g. SIGWINCH) doesn't shorten
 * the frame; bails as soon as the stop flag fires so SIGINT/SIGTERM
 * still exit cleanly.
 */
static void sleep_until_monotonic(const struct timespec *deadline) {
#if defined(__APPLE__)
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long sec_diff  = (long)(deadline->tv_sec  - now.tv_sec);
    long nsec_diff = (long)(deadline->tv_nsec - now.tv_nsec);
    if (nsec_diff < 0) {
        sec_diff  -= 1;
        nsec_diff += 1000000000L;
    }
    if (sec_diff < 0 || (sec_diff == 0 && nsec_diff <= 0)) {
        return;
    }
    struct timespec rem = { .tv_sec = sec_diff, .tv_nsec = nsec_diff };
    while (nanosleep(&rem, &rem) == -1 && errno == EINTR && !swatch_noise_stop) {
        /* nanosleep wrote the unslept remainder into `rem`; resume */
    }
#else
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, deadline, NULL) == EINTR
           && !swatch_noise_stop) {
        /* interrupted by a non-stop signal — resume the wait */
    }
#endif
}

void swatch_noise_draw_frame(swatch_color_mode_t mode, int width, int height,
                             int bw, unsigned (*rng)(void), FILE *out) {
    if (out == NULL || width <= 0 || height <= 0 || rng == NULL) {
        return;
    }
    if (mode != SWATCH_COLOR_TRUECOLOR && mode != SWATCH_COLOR_XTERM256) {
        return;
    }

    fputs("\x1b[H", out);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (bw) {
                unsigned bit = rng() & 1u;
                if (mode == SWATCH_COLOR_TRUECOLOR) {
                    if (bit) {
                        fputs("\x1b[48;2;255;255;255m ", out);
                    } else {
                        fputs("\x1b[48;2;0;0;0m ", out);
                    }
                } else {
                    if (bit) {
                        fputs("\x1b[48;5;231m ", out);
                    } else {
                        fputs("\x1b[48;5;16m ", out);
                    }
                }
            } else {
                uint8_t r = (uint8_t)(rng() & 0xFFu);
                uint8_t g = (uint8_t)(rng() & 0xFFu);
                uint8_t b = (uint8_t)(rng() & 0xFFu);
                if (mode == SWATCH_COLOR_TRUECOLOR) {
                    fprintf(out, "\x1b[48;2;%u;%u;%um ",
                            (unsigned)r, (unsigned)g, (unsigned)b);
                } else {
                    int n = 16 + 36 * cube_index(r) + 6 * cube_index(g) + cube_index(b);
                    fprintf(out, "\x1b[48;5;%dm ", n);
                }
            }
        }
        if (row == height - 1) {
            fputs("\x1b[0m", out);
        } else {
            fputs("\x1b[0m\n", out);
        }
    }
}

static int probe_terminal_size(int *w_out, int *h_out) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        return -1;
    }
    if (ws.ws_col == 0 || ws.ws_row == 0) {
        return -1;
    }
    *w_out = (int)ws.ws_col;
    *h_out = (int)ws.ws_row;
    return 0;
}

int swatch_noise_run(swatch_noise_opts_t opts, FILE *out) {
    if (out == stdout && !isatty(fileno(stdout))) {
        fputs("swatch: noise requires a TTY\n", stderr);
        return 64;
    }

    if (opts.mode != SWATCH_COLOR_TRUECOLOR && opts.mode != SWATCH_COLOR_XTERM256) {
        fputs("swatch: noise requires a color terminal "
              "(try --color truecolor or unset NO_COLOR)\n", stderr);
        return 64;
    }

    int width = opts.width;
    int height = opts.height;
    if (width <= 0 || height <= 0) {
        int pw = 0, ph = 0;
        if (probe_terminal_size(&pw, &ph) == 0) {
            width = pw;
            height = ph;
        } else {
            width = 80;
            height = 24;
        }
    }

    int fps = opts.fps;
    if (fps <= 0) fps = 15;
    if (fps > 120) fps = 120;

    int duration = opts.duration_seconds;
    if (duration < 0) duration = 0;
    if (duration > 3600) duration = 3600;

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    atexit(restore_cursor_atexit);

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    fputs("\x1b[?25l\x1b[?1049h\x1b[H", out);
    fflush(out);
    swatch_noise_cursor_hidden = 1;
    swatch_noise_altscreen_on = 1;

    long total_frames = (duration > 0) ? ((long)duration * (long)fps) : -1;
    long frames_drawn = 0;
    long interval_ns = 1000000000L / fps;

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);

    while (!swatch_noise_stop && (total_frames < 0 || frames_drawn < total_frames)) {
        swatch_noise_draw_frame(opts.mode, width, height, opts.bw, rand_unsigned, out);
        fflush(out);

        /* Absolute-deadline pacing: target the next frame at deadline += interval,
         * not "sleep interval after work". When a frame runs long the next
         * sleep returns immediately and we render again at the work-rate ceiling. */
        deadline.tv_nsec += interval_ns;
        while (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_nsec -= 1000000000L;
            deadline.tv_sec  += 1;
        }
        sleep_until_monotonic(&deadline);

        frames_drawn++;
    }

    fputs("\x1b[0m\x1b[?25h\x1b[?1049l\n", out);
    fflush(out);
    swatch_noise_cursor_hidden = 0;
    swatch_noise_altscreen_on = 0;
    return 0;
}
