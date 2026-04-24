#define _POSIX_C_SOURCE 200809L

#include "../src/noise.h"
#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned test_rng_state = 0;

static void test_rng_seed(unsigned s) {
    test_rng_state = s;
}

static unsigned test_rng(void) {
    test_rng_state = test_rng_state * 1103515245u + 12345u;
    return test_rng_state;
}

static void draw_to_buf(swatch_color_mode_t mode, int w, int h,
                        char *buf, size_t buflen) {
    memset(buf, 0, buflen);
    FILE *f = fmemopen(buf, buflen, "w");
    if (f == NULL) {
        return;
    }
    swatch_noise_draw_frame(mode, w, h, test_rng, f);
    fclose(f);
}

static const int CUBE_STEPS[6] = {0, 95, 135, 175, 215, 255};

static int expected_cube_index(uint8_t channel) {
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

static void test_draw_frame_truecolor_1x1(void) {
    test_rng_seed(0);
    uint8_t r = (uint8_t)(test_rng() & 0xFFu);
    uint8_t g = (uint8_t)(test_rng() & 0xFFu);
    uint8_t b = (uint8_t)(test_rng() & 0xFFu);

    /* Last row drops the trailing newline — the alt-screen buffer that
     * swatch_noise_run enters contains scrolling on over-tall frames, and
     * omitting the final \n makes the pure-function frame emission
     * scroll-safe even outside the alt buffer. */
    char expected[128];
    snprintf(expected, sizeof(expected),
             "\x1b[H\x1b[48;2;%u;%u;%um \x1b[0m",
             (unsigned)r, (unsigned)g, (unsigned)b);

    test_rng_seed(0);
    char buf[256];
    draw_to_buf(SWATCH_COLOR_TRUECOLOR, 1, 1, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, expected, "truecolor 1x1 frame");
}

static void test_draw_frame_truecolor_3x2(void) {
    test_rng_seed(42);
    char expected[1024];
    size_t off = 0;
    off += (size_t)snprintf(expected + off, sizeof(expected) - off, "\x1b[H");
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 3; col++) {
            uint8_t r = (uint8_t)(test_rng() & 0xFFu);
            uint8_t g = (uint8_t)(test_rng() & 0xFFu);
            uint8_t b = (uint8_t)(test_rng() & 0xFFu);
            off += (size_t)snprintf(expected + off, sizeof(expected) - off,
                                    "\x1b[48;2;%u;%u;%um ",
                                    (unsigned)r, (unsigned)g, (unsigned)b);
        }
        /* Last row has no trailing newline (alt-screen scroll-safety). */
        if (row == 1) {
            off += (size_t)snprintf(expected + off, sizeof(expected) - off, "\x1b[0m");
        } else {
            off += (size_t)snprintf(expected + off, sizeof(expected) - off, "\x1b[0m\n");
        }
    }

    test_rng_seed(42);
    char buf[1024];
    draw_to_buf(SWATCH_COLOR_TRUECOLOR, 3, 2, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, expected, "truecolor 3x2 frame");
}

static void test_draw_frame_xterm256_1x1(void) {
    test_rng_seed(7);
    uint8_t r = (uint8_t)(test_rng() & 0xFFu);
    uint8_t g = (uint8_t)(test_rng() & 0xFFu);
    uint8_t b = (uint8_t)(test_rng() & 0xFFu);
    int n = 16 + 36 * expected_cube_index(r) + 6 * expected_cube_index(g)
          + expected_cube_index(b);

    /* Last row has no trailing newline (alt-screen scroll-safety). */
    char expected[128];
    snprintf(expected, sizeof(expected),
             "\x1b[H\x1b[48;5;%dm \x1b[0m", n);

    test_rng_seed(7);
    char buf[256];
    draw_to_buf(SWATCH_COLOR_XTERM256, 1, 1, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, expected, "xterm256 1x1 frame");
}

static void test_draw_frame_none_emits_nothing_or_safe(void) {
    test_rng_seed(0);
    char buf[256];
    draw_to_buf(SWATCH_COLOR_NONE, 4, 2, buf, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        ASSERT_TRUE(buf[i] != '\x1b', "no ANSI escape bytes in NONE-mode output");
    }
}

int main(void) {
    RUN_TEST(test_draw_frame_truecolor_1x1);
    RUN_TEST(test_draw_frame_truecolor_3x2);
    RUN_TEST(test_draw_frame_xterm256_1x1);
    RUN_TEST(test_draw_frame_none_emits_nothing_or_safe);
    TEST_SUMMARY();
}
