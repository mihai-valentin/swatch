#include "../src/window.h"
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

static void test_fill_frame_truecolor_1x1(void) {
    test_rng_seed(1);
    uint8_t r = (uint8_t)(test_rng() & 0xFFu);
    uint8_t g = (uint8_t)(test_rng() & 0xFFu);
    uint8_t b = (uint8_t)(test_rng() & 0xFFu);

    test_rng_seed(1);
    uint8_t buf[4] = {0};
    swatch_window_fill_frame(buf, 1, 1, 0, test_rng);

    ASSERT_EQ(buf[0], b,    "BGRA[0] = blue byte");
    ASSERT_EQ(buf[1], g,    "BGRA[1] = green byte");
    ASSERT_EQ(buf[2], r,    "BGRA[2] = red byte");
    ASSERT_EQ(buf[3], 0xFF, "BGRA[3] = alpha 0xFF");
}

static void test_fill_frame_truecolor_3x2(void) {
    const int W = 3, H = 2;
    uint8_t expected[3 * 2 * 4];
    test_rng_seed(42);
    for (int i = 0; i < W * H; i++) {
        uint8_t r = (uint8_t)(test_rng() & 0xFFu);
        uint8_t g = (uint8_t)(test_rng() & 0xFFu);
        uint8_t b = (uint8_t)(test_rng() & 0xFFu);
        expected[i * 4 + 0] = b;
        expected[i * 4 + 1] = g;
        expected[i * 4 + 2] = r;
        expected[i * 4 + 3] = 0xFF;
    }

    test_rng_seed(42);
    uint8_t buf[3 * 2 * 4] = {0};
    swatch_window_fill_frame(buf, W, H, 0, test_rng);

    ASSERT_TRUE(memcmp(buf, expected, sizeof(buf)) == 0,
                "3x2 truecolor BGRA buffer matches expected bytes");
}

static void test_fill_frame_bw_picks_only_black_or_white(void) {
    const int W = 4, H = 4;
    uint8_t buf[4 * 4 * 4] = {0};
    test_rng_seed(99);
    swatch_window_fill_frame(buf, W, H, 1, test_rng);

    for (int i = 0; i < W * H; i++) {
        uint8_t b = buf[i * 4 + 0];
        uint8_t g = buf[i * 4 + 1];
        uint8_t r = buf[i * 4 + 2];
        uint8_t a = buf[i * 4 + 3];
        int is_black = (r == 0   && g == 0   && b == 0);
        int is_white = (r == 255 && g == 255 && b == 255);
        ASSERT_TRUE(is_black || is_white, "bw pixel is pure black or pure white");
        ASSERT_EQ(a, 0xFF, "bw alpha is 0xFF");
    }
}

static void test_fill_frame_bw_uses_one_rng_call_per_pixel(void) {
    test_rng_seed(7);
    unsigned bit0 = test_rng() & 1u;
    unsigned bit1 = test_rng() & 1u;

    test_rng_seed(7);
    uint8_t buf[2 * 1 * 4] = {0};
    swatch_window_fill_frame(buf, 2, 1, 1, test_rng);

    uint8_t v0 = bit0 ? 0xFF : 0x00;
    uint8_t v1 = bit1 ? 0xFF : 0x00;

    ASSERT_EQ(buf[0], v0, "cell 0 blue");
    ASSERT_EQ(buf[1], v0, "cell 0 green");
    ASSERT_EQ(buf[2], v0, "cell 0 red");
    ASSERT_EQ(buf[3], 0xFF, "cell 0 alpha");
    ASSERT_EQ(buf[4], v1, "cell 1 blue");
    ASSERT_EQ(buf[5], v1, "cell 1 green");
    ASSERT_EQ(buf[6], v1, "cell 1 red");
    ASSERT_EQ(buf[7], 0xFF, "cell 1 alpha");
}

static void test_fill_frame_alpha_always_opaque(void) {
    const int W = 8, H = 8;
    uint8_t buf[8 * 8 * 4];
    memset(buf, 0x5A, sizeof(buf));
    test_rng_seed(2026);
    swatch_window_fill_frame(buf, W, H, 0, test_rng);

    for (int i = 0; i < W * H; i++) {
        ASSERT_EQ(buf[i * 4 + 3], 0xFF, "alpha byte is 0xFF for every pixel");
    }
}

int main(void) {
    RUN_TEST(test_fill_frame_truecolor_1x1);
    RUN_TEST(test_fill_frame_truecolor_3x2);
    RUN_TEST(test_fill_frame_bw_picks_only_black_or_white);
    RUN_TEST(test_fill_frame_bw_uses_one_rng_call_per_pixel);
    RUN_TEST(test_fill_frame_alpha_always_opaque);
    TEST_SUMMARY();
}
