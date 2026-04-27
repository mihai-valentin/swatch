#define _POSIX_C_SOURCE 200809L

#if (defined(__linux__) || defined(__unix__)) && !defined(__APPLE__) && defined(SWATCH_HAVE_X11)

#include "window.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t s_stop = 0;

static void on_signal(int sig) {
    (void)sig;
    s_stop = 1;
}

static uint64_t seed_state(void) {
    uint64_t s = ((uint64_t)time(NULL) << 32) ^ (uint64_t)getpid()
               ^ (uint64_t)(uintptr_t)&s;
    if (s == 0) s = 0x9E3779B97F4A7C15ULL;
    return s;
}

static void advance_deadline(struct timespec *deadline, long interval_ns) {
    deadline->tv_nsec += interval_ns;
    while (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_nsec -= 1000000000L;
        deadline->tv_sec  += 1;
    }
}

int swatch_window_run(swatch_window_opts_t opts) {
    int width  = opts.width  > 0 ? opts.width  : 640;
    int height = opts.height > 0 ? opts.height : 480;
    if (width < 64 || width > 3840 || height < 64 || height > 2160) {
        fprintf(stderr,
                "swatch window: --size out of range "
                "(width 64..3840, height 64..2160; got %dx%d)\n",
                width, height);
        return 64;
    }

    int fps = opts.fps;
    if (fps <= 0) fps = 15;
    if (fps > 120) fps = 120;

    int duration = opts.duration_seconds;
    if (duration < 0) duration = 0;
    if (duration > 3600) duration = 3600;

    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        const char *disp_env = getenv("DISPLAY");
        fprintf(stderr,
                "swatch window: cannot open X display (DISPLAY=%s) "
                "(on Wayland, ensure XWayland is available)\n",
                disp_env != NULL ? disp_env : "");
        return 1;
    }

    int      screen = DefaultScreen(dpy);
    Window   root   = RootWindow(dpy, screen);
    Visual  *visual = DefaultVisual(dpy, screen);
    int      depth  = DefaultDepth(dpy, screen);

    Window win = XCreateSimpleWindow(
        dpy, root, 0, 0,
        (unsigned)width, (unsigned)height,
        0, 0, 0);

    XStoreName(dpy, win, "swatch noise");

    /* Advertise sane bounds but let the user (and the WM) resize / maximize /
     * full-screen freely. We re-fill the pixel buffer at whatever size the
     * window settles on after each ConfigureNotify. */
    XSizeHints hints;
    memset(&hints, 0, sizeof(hints));
    hints.flags      = PMinSize;
    hints.min_width  = 64;
    hints.min_height = 64;
    XSetWMNormalHints(dpy, win, &hints);

    XSelectInput(dpy, win, ExposureMask | StructureNotifyMask);

    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete, 1);

    XMapWindow(dpy, win);
    XFlush(dpy);

    if (depth != 24 && depth != 32) {
        fprintf(stderr,
                "swatch window: warning — unsupported X visual depth %d "
                "(expected 24 or 32); colors may render incorrectly\n",
                depth);
    }

    size_t pixel_bytes = (size_t)width * (size_t)height * 4u;
    uint8_t *pixels = (uint8_t *)malloc(pixel_bytes);
    if (pixels == NULL) {
        fprintf(stderr, "swatch window: out of memory (%zu bytes)\n", pixel_bytes);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }

    XImage *img = XCreateImage(
        dpy, visual, (unsigned)depth, ZPixmap, 0,
        (char *)pixels, (unsigned)width, (unsigned)height,
        32, width * 4);
    if (img == NULL) {
        fprintf(stderr, "swatch window: XCreateImage failed\n");
        free(pixels);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    uint64_t rng_state = seed_state();

    long total_frames = (duration > 0) ? ((long)duration * (long)fps) : -1;
    long frames_drawn = 0;
    long interval_ns = 1000000000L / fps;

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);

    GC gc = DefaultGC(dpy, screen);

    while (!s_stop) {
        int resized_to_w = 0;
        int resized_to_h = 0;
        while (XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == ClientMessage &&
                (Atom)ev.xclient.data.l[0] == wm_delete) {
                s_stop = 1;
                break;
            }
            if (ev.type == ConfigureNotify) {
                int nw = ev.xconfigure.width;
                int nh = ev.xconfigure.height;
                if (nw > 0 && nh > 0 && (nw != width || nh != height)) {
                    resized_to_w = nw;
                    resized_to_h = nh;
                }
            }
        }
        if (s_stop) break;

        if (resized_to_w > 0 && resized_to_h > 0) {
            /* Detach + free the old image/buffer, allocate at the new size. */
            img->data = NULL;
            XDestroyImage(img);
            free(pixels);

            width  = resized_to_w;
            height = resized_to_h;
            pixel_bytes = (size_t)width * (size_t)height * 4u;
            pixels = (uint8_t *)malloc(pixel_bytes);
            if (pixels == NULL) {
                fprintf(stderr,
                        "swatch window: out of memory on resize (%zu bytes)\n",
                        pixel_bytes);
                XDestroyWindow(dpy, win);
                XCloseDisplay(dpy);
                return 1;
            }
            img = XCreateImage(
                dpy, visual, (unsigned)depth, ZPixmap, 0,
                (char *)pixels, (unsigned)width, (unsigned)height,
                32, width * 4);
            if (img == NULL) {
                fprintf(stderr,
                        "swatch window: XCreateImage failed on resize\n");
                free(pixels);
                XDestroyWindow(dpy, win);
                XCloseDisplay(dpy);
                return 1;
            }
        }

        swatch_window_fill_frame(pixels, width, height, opts.bw, &rng_state);
        XPutImage(dpy, win, gc, img, 0, 0, 0, 0,
                  (unsigned)width, (unsigned)height);
        XFlush(dpy);

        /* Absolute-deadline pacing: target the next frame at deadline += interval,
         * not "sleep interval after work". If the previous frame ran long the
         * sleep returns immediately and we render again at the work-rate ceiling. */
        advance_deadline(&deadline, interval_ns);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);

        frames_drawn++;
        if (total_frames >= 0 && frames_drawn >= total_frames) break;
    }

    /* XDestroyImage frees the data pointer it was given. We allocated
     * `pixels` ourselves and want to free it with free(), so detach
     * before the destroy call. */
    img->data = NULL;
    XDestroyImage(img);
    free(pixels);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}

#endif

#if (defined(__linux__) || defined(__unix__)) && !defined(__APPLE__) && !defined(SWATCH_HAVE_X11)

#include "window.h"
#include <stdio.h>

int swatch_window_run(swatch_window_opts_t opts) {
    (void)opts;
    fputs("swatch window: built without X11 support "
          "(libx11-dev missing at compile time)\n", stderr);
    return 1;
}

#endif
