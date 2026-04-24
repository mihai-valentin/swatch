#ifndef SWATCH_TEST_H
#define SWATCH_TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Minimal zero-dependency assert harness.
 *
 * File-local static counter — each test binary has one TU that includes this
 * header (the test_*.c file that defines main), which is where the counter
 * lives. Source files under test (src/<name>.c) must not include this header.
 */
static int swatch_test_fails_ = 0;

#define ASSERT_EQ(actual, expected, msg) do {                                \
    long long _swt_a = (long long)(actual);                                  \
    long long _swt_e = (long long)(expected);                                \
    if (_swt_a != _swt_e) {                                                  \
        fprintf(stderr, "FAIL %s:%d %s got=%lld want=%lld\n",                \
                __FILE__, __LINE__, (msg), _swt_a, _swt_e);                  \
        swatch_test_fails_++;                                                \
        return;                                                              \
    }                                                                        \
} while (0)

#define ASSERT_STR_EQ(actual_ptr, expected_cstr, msg) do {                   \
    const char *_swt_a = (actual_ptr);                                       \
    const char *_swt_e = (expected_cstr);                                    \
    if (_swt_a == NULL || _swt_e == NULL || strcmp(_swt_a, _swt_e) != 0) {   \
        fprintf(stderr, "FAIL %s:%d %s got=\"%s\" want=\"%s\"\n",            \
                __FILE__, __LINE__, (msg),                                   \
                _swt_a ? _swt_a : "(null)",                                  \
                _swt_e ? _swt_e : "(null)");                                 \
        swatch_test_fails_++;                                                \
        return;                                                              \
    }                                                                        \
} while (0)

#define ASSERT_TRUE(cond, msg) do {                                          \
    if (!(cond)) {                                                           \
        fprintf(stderr, "FAIL %s:%d %s got=false want=true\n",               \
                __FILE__, __LINE__, (msg));                                  \
        swatch_test_fails_++;                                                \
        return;                                                              \
    }                                                                        \
} while (0)

#define ASSERT_FALSE(cond, msg) do {                                         \
    if (cond) {                                                              \
        fprintf(stderr, "FAIL %s:%d %s got=true want=false\n",               \
                __FILE__, __LINE__, (msg));                                  \
        swatch_test_fails_++;                                                \
        return;                                                              \
    }                                                                        \
} while (0)

#define RUN_TEST(fn) do {                                                    \
    int _swt_before = swatch_test_fails_;                                    \
    fprintf(stdout, "RUN %s\n", #fn);                                        \
    fn();                                                                    \
    if (swatch_test_fails_ == _swt_before) {                                 \
        fprintf(stdout, "OK %s\n", #fn);                                     \
    } else {                                                                 \
        fprintf(stdout, "FAIL %s\n", #fn);                                   \
    }                                                                        \
} while (0)

#define TEST_SUMMARY() do {                                                  \
    fprintf(stdout, "TOTAL FAILS: %d\n", swatch_test_fails_);                \
    return swatch_test_fails_ == 0 ? 0 : 1;                                  \
} while (0)

#endif
