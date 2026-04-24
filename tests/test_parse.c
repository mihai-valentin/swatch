#include "../src/parse.h"
#include "test.h"

static void test_six_char_with_hash(void) {
    rgb_t rgb = {0, 0, 0};
    int rc = swatch_parse_hex("#00AF4C", &rgb);
    ASSERT_EQ(rc, 0, "return value");
    ASSERT_EQ(rgb.r, 0x00, "red");
    ASSERT_EQ(rgb.g, 0xAF, "green");
    ASSERT_EQ(rgb.b, 0x4C, "blue");
}

static void test_six_char_without_hash(void) {
    rgb_t rgb = {0, 0, 0};
    int rc = swatch_parse_hex("FF00FF", &rgb);
    ASSERT_EQ(rc, 0, "return value");
    ASSERT_EQ(rgb.r, 0xFF, "red");
    ASSERT_EQ(rgb.g, 0x00, "green");
    ASSERT_EQ(rgb.b, 0xFF, "blue");
}

static void test_three_char_with_hash(void) {
    rgb_t rgb = {0, 0, 0};
    int rc = swatch_parse_hex("#0AF", &rgb);
    ASSERT_EQ(rc, 0, "return value");
    ASSERT_EQ(rgb.r, 0x00, "red");
    ASSERT_EQ(rgb.g, 0xAA, "green");
    ASSERT_EQ(rgb.b, 0xFF, "blue");
}

static void test_three_char_without_hash(void) {
    rgb_t rgb = {0, 0, 0};
    int rc = swatch_parse_hex("abc", &rgb);
    ASSERT_EQ(rc, 0, "return value");
    ASSERT_EQ(rgb.r, 0xAA, "red");
    ASSERT_EQ(rgb.g, 0xBB, "green");
    ASSERT_EQ(rgb.b, 0xCC, "blue");
}

static void test_uppercase_and_lowercase_mix(void) {
    rgb_t rgb = {0, 0, 0};
    int rc = swatch_parse_hex("#aAfF00", &rgb);
    ASSERT_EQ(rc, 0, "return value");
    ASSERT_EQ(rgb.r, 0xAA, "red");
    ASSERT_EQ(rgb.g, 0xFF, "green");
    ASSERT_EQ(rgb.b, 0x00, "blue");
}

static void test_reject_wrong_length(void) {
    rgb_t rgb = {0, 0, 0};
    ASSERT_EQ(swatch_parse_hex("#12", &rgb), -1, "length 2");
    ASSERT_EQ(swatch_parse_hex("#1234", &rgb), -1, "length 4");
    ASSERT_EQ(swatch_parse_hex("#1234567", &rgb), -1, "length 7");
}

static void test_reject_non_hex(void) {
    rgb_t rgb = {0, 0, 0};
    ASSERT_EQ(swatch_parse_hex("#ZZZZZZ", &rgb), -1, "ZZZZZZ");
    ASSERT_EQ(swatch_parse_hex("#GGHHII", &rgb), -1, "GGHHII");
    ASSERT_EQ(swatch_parse_hex("#00AF4G", &rgb), -1, "00AF4G");
}

static void test_reject_null_input(void) {
    rgb_t rgb = {0, 0, 0};
    ASSERT_EQ(swatch_parse_hex(NULL, &rgb), -1, "null input");
}

static void test_reject_null_output(void) {
    ASSERT_EQ(swatch_parse_hex("#000000", NULL), -1, "null output");
}

static void test_reject_embedded_hash(void) {
    rgb_t rgb = {0, 0, 0};
    ASSERT_EQ(swatch_parse_hex("00#F4C", &rgb), -1, "embedded hash");
}

static void test_reject_whitespace(void) {
    rgb_t rgb = {0, 0, 0};
    ASSERT_EQ(swatch_parse_hex(" #00AF4C", &rgb), -1, "leading space");
    ASSERT_EQ(swatch_parse_hex("#00AF4C ", &rgb), -1, "trailing space");
    ASSERT_EQ(swatch_parse_hex("00 AF4C", &rgb), -1, "embedded space");
}

static void test_out_untouched_on_failure(void) {
    rgb_t rgb = {0x11, 0x22, 0x33};
    int rc = swatch_parse_hex("#ZZZ", &rgb);
    ASSERT_EQ(rc, -1, "return value");
    ASSERT_EQ(rgb.r, 0x11, "red untouched");
    ASSERT_EQ(rgb.g, 0x22, "green untouched");
    ASSERT_EQ(rgb.b, 0x33, "blue untouched");
}

int main(void) {
    RUN_TEST(test_six_char_with_hash);
    RUN_TEST(test_six_char_without_hash);
    RUN_TEST(test_three_char_with_hash);
    RUN_TEST(test_three_char_without_hash);
    RUN_TEST(test_uppercase_and_lowercase_mix);
    RUN_TEST(test_reject_wrong_length);
    RUN_TEST(test_reject_non_hex);
    RUN_TEST(test_reject_null_input);
    RUN_TEST(test_reject_null_output);
    RUN_TEST(test_reject_embedded_hash);
    RUN_TEST(test_reject_whitespace);
    RUN_TEST(test_out_untouched_on_failure);
    TEST_SUMMARY();
}
