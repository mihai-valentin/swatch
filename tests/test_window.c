#include "../src/window.h"
#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Replicate the production xorshift64 here so tests can predict exact
 * bytes for a given seed. If the production algorithm ever changes, this
 * helper must change to match — that asymmetry is on purpose; it forces
 * any algorithmic change to also re-acknowledge the tests. */
static uint64_t test_xs64(uint64_t *s) {
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

static void test_fill_frame_truecolor_1x1(void) {
    uint64_t s = 0xDEADBEEFCAFEBABEULL;
    uint64_t r = test_xs64(&s);
    uint8_t  b_byte = (uint8_t)(r        & 0xFFu);
    uint8_t  g_byte = (uint8_t)((r >> 8) & 0xFFu);
    uint8_t  r_byte = (uint8_t)((r >> 16) & 0xFFu);

    uint64_t state = 0xDEADBEEFCAFEBABEULL;
    uint8_t buf[4] = {0};
    swatch_window_fill_frame(buf, 1, 1, 0, &state);

    ASSERT_EQ(buf[0], b_byte, "BGRA[0] = blue byte derived from xorshift64");
    ASSERT_EQ(buf[1], g_byte, "BGRA[1] = green byte");
    ASSERT_EQ(buf[2], r_byte, "BGRA[2] = red byte");
    ASSERT_EQ(buf[3], 0xFF,   "BGRA[3] = alpha 0xFF");
}

static void test_fill_frame_truecolor_3x2(void) {
    const int W = 3, H = 2;
    uint8_t expected[3 * 2 * 4];
    uint64_t s = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < W * H; i++) {
        uint64_t r = test_xs64(&s);
        expected[i * 4 + 0] = (uint8_t)(r        & 0xFFu);
        expected[i * 4 + 1] = (uint8_t)((r >> 8) & 0xFFu);
        expected[i * 4 + 2] = (uint8_t)((r >> 16) & 0xFFu);
        expected[i * 4 + 3] = 0xFF;
    }

    uint64_t state = 0x123456789ABCDEF0ULL;
    uint8_t buf[3 * 2 * 4] = {0};
    swatch_window_fill_frame(buf, W, H, 0, &state);

    ASSERT_TRUE(memcmp(buf, expected, sizeof(buf)) == 0,
                "3x2 truecolor BGRA buffer matches xorshift64-derived bytes");
}

static void test_fill_frame_bw_picks_only_black_or_white(void) {
    const int W = 4, H = 4;
    uint8_t buf[4 * 4 * 4] = {0};
    uint64_t state = 99;
    swatch_window_fill_frame(buf, W, H, 1, &state);

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

static void test_fill_frame_bw_low_bit_picks_color(void) {
    /* For the same seed, the first two pixels should be picked by the
     * low bit of two successive xorshift64 outputs. */
    uint64_t s = 7;
    unsigned bit0 = (unsigned)(test_xs64(&s) & 1u);
    unsigned bit1 = (unsigned)(test_xs64(&s) & 1u);

    uint64_t state = 7;
    uint8_t buf[2 * 1 * 4] = {0};
    swatch_window_fill_frame(buf, 2, 1, 1, &state);

    uint8_t v0 = bit0 ? 0xFF : 0x00;
    uint8_t v1 = bit1 ? 0xFF : 0x00;

    ASSERT_EQ(buf[0], v0,   "cell 0 blue");
    ASSERT_EQ(buf[1], v0,   "cell 0 green");
    ASSERT_EQ(buf[2], v0,   "cell 0 red");
    ASSERT_EQ(buf[3], 0xFF, "cell 0 alpha");
    ASSERT_EQ(buf[4], v1,   "cell 1 blue");
    ASSERT_EQ(buf[5], v1,   "cell 1 green");
    ASSERT_EQ(buf[6], v1,   "cell 1 red");
    ASSERT_EQ(buf[7], 0xFF, "cell 1 alpha");
}

static void test_fill_frame_alpha_always_opaque(void) {
    const int W = 8, H = 8;
    uint8_t buf[8 * 8 * 4];
    memset(buf, 0x5A, sizeof(buf));
    uint64_t state = 2026;
    swatch_window_fill_frame(buf, W, H, 0, &state);

    for (int i = 0; i < W * H; i++) {
        ASSERT_EQ(buf[i * 4 + 3], 0xFF, "alpha byte is 0xFF for every pixel");
    }
}

static void test_fill_frame_deterministic_for_fixed_seed(void) {
    const int W = 5, H = 4;
    uint8_t buf_a[5 * 4 * 4] = {0};
    uint8_t buf_b[5 * 4 * 4] = {0};

    uint64_t state_a = 0xCAFEF00DDEADBEEFULL;
    swatch_window_fill_frame(buf_a, W, H, 0, &state_a);

    uint64_t state_b = 0xCAFEF00DDEADBEEFULL;
    swatch_window_fill_frame(buf_b, W, H, 0, &state_b);

    ASSERT_TRUE(memcmp(buf_a, buf_b, sizeof(buf_a)) == 0,
                "two fills with the same starting state produce identical bytes");
    ASSERT_TRUE(state_a == state_b,
                "state advances identically for identical inputs");
}

int main(void) {
    RUN_TEST(test_fill_frame_truecolor_1x1);
    RUN_TEST(test_fill_frame_truecolor_3x2);
    RUN_TEST(test_fill_frame_bw_picks_only_black_or_white);
    RUN_TEST(test_fill_frame_bw_low_bit_picks_color);
    RUN_TEST(test_fill_frame_alpha_always_opaque);
    RUN_TEST(test_fill_frame_deterministic_for_fixed_seed);
    TEST_SUMMARY();
}
