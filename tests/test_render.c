#define _POSIX_C_SOURCE 200809L

#include "../src/render.h"
#include "test.h"

#include <stdio.h>
#include <string.h>

static void render_to_buf(rgb_t color, swatch_render_opts_t opts,
                         char *buf, size_t buflen) {
    memset(buf, 0, buflen);
    FILE *f = fmemopen(buf, buflen, "w");
    if (f == NULL) {
        return;
    }
    swatch_render(color, opts, f);
    fclose(f);
}

static void test_none_plain(void) {
    rgb_t c = {0x00, 0xAF, 0x4C};
    swatch_render_opts_t opts = {6, 3, ' ', 0, SWATCH_COLOR_NONE};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "#00AF4C (6x3)\n", "none plain");
}

static void test_none_ignores_label(void) {
    rgb_t c = {0x00, 0xAF, 0x4C};
    swatch_render_opts_t opts = {6, 3, ' ', 1, SWATCH_COLOR_NONE};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "#00AF4C (6x3)\n", "none ignores label");
}

static void test_truecolor_6x3(void) {
    rgb_t c = {0xFF, 0x00, 0x00};
    swatch_render_opts_t opts = {6, 3, ' ', 0, SWATCH_COLOR_TRUECOLOR};
    char buf[256];
    render_to_buf(c, opts, buf, sizeof(buf));
    const char *row = "\x1b[48;2;255;0;0m      \x1b[0m\n";
    char expected[256];
    snprintf(expected, sizeof(expected), "%s%s%s", row, row, row);
    ASSERT_STR_EQ(buf, expected, "truecolor 6x3");
}

static void test_truecolor_custom_fill(void) {
    rgb_t c = {1, 2, 3};
    swatch_render_opts_t opts = {4, 1, '#', 0, SWATCH_COLOR_TRUECOLOR};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;2;1;2;3m####\x1b[0m\n", "truecolor custom fill");
}

static void test_truecolor_with_label(void) {
    rgb_t c = {0x00, 0xAF, 0x4C};
    swatch_render_opts_t opts = {2, 1, ' ', 1, SWATCH_COLOR_TRUECOLOR};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;2;0;175;76m  \x1b[0m\n#00AF4C\n",
                  "truecolor with label");
}

static void test_truecolor_zero_fill_becomes_space(void) {
    rgb_t c = {10, 20, 30};
    swatch_render_opts_t opts = {3, 1, '\0', 0, SWATCH_COLOR_TRUECOLOR};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;2;10;20;30m   \x1b[0m\n",
                  "zero fill becomes space");
}

static void test_xterm256_pure_red(void) {
    rgb_t c = {255, 0, 0};
    swatch_render_opts_t opts = {1, 1, ' ', 0, SWATCH_COLOR_XTERM256};
    char buf[64];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;5;196m \x1b[0m\n", "xterm256 pure red");
}

static void test_xterm256_pure_black(void) {
    rgb_t c = {0, 0, 0};
    swatch_render_opts_t opts = {1, 1, ' ', 0, SWATCH_COLOR_XTERM256};
    char buf[64];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;5;16m \x1b[0m\n", "xterm256 pure black");
}

static void test_xterm256_pure_white(void) {
    rgb_t c = {255, 255, 255};
    swatch_render_opts_t opts = {1, 1, ' ', 0, SWATCH_COLOR_XTERM256};
    char buf[64];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "\x1b[48;5;231m \x1b[0m\n", "xterm256 pure white");
}

static void test_unknown_mode_falls_back_to_none(void) {
    rgb_t c = {0x00, 0xAF, 0x4C};
    swatch_render_opts_t opts = {6, 3, ' ', 0, (swatch_color_mode_t)999};
    char buf[128];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "#00AF4C (6x3)\n", "unknown mode falls back to none");
}

static void test_guard_zero_width(void) {
    rgb_t c = {1, 2, 3};
    swatch_render_opts_t opts = {0, 3, ' ', 0, SWATCH_COLOR_TRUECOLOR};
    char buf[64];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "", "guard zero width");
}

static void test_guard_negative_height(void) {
    rgb_t c = {1, 2, 3};
    swatch_render_opts_t opts = {3, -1, ' ', 0, SWATCH_COLOR_TRUECOLOR};
    char buf[64];
    render_to_buf(c, opts, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "", "guard negative height");
}

int main(void) {
    RUN_TEST(test_none_plain);
    RUN_TEST(test_none_ignores_label);
    RUN_TEST(test_truecolor_6x3);
    RUN_TEST(test_truecolor_custom_fill);
    RUN_TEST(test_truecolor_with_label);
    RUN_TEST(test_truecolor_zero_fill_becomes_space);
    RUN_TEST(test_xterm256_pure_red);
    RUN_TEST(test_xterm256_pure_black);
    RUN_TEST(test_xterm256_pure_white);
    RUN_TEST(test_unknown_mode_falls_back_to_none);
    RUN_TEST(test_guard_zero_width);
    RUN_TEST(test_guard_negative_height);
    TEST_SUMMARY();
}
