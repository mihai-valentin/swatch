#ifndef SWATCH_WINDOW_H
#define SWATCH_WINDOW_H

#include <stdint.h>

typedef struct {
    int width;             /* pixels; 0 = default 640 */
    int height;            /* pixels; 0 = default 480 */
    int fps;               /* 1..120; 0 = default 15 */
    int duration_seconds;  /* 0 = until window close, 1..3600 supported */
    int bw;                /* 0 = full color, non-zero = 1-bit black/white */
} swatch_window_opts_t;

/*
 * Open a native OS window and run the noise animation inside it.
 *
 * Backends are selected at compile time:
 *   _WIN32   -> src/window_win32.c (user32 + gdi32)
 *   __APPLE__-> src/window_cocoa.c (objc_msgSend, no .m source)
 *   else     -> src/window_x11.c   (libX11; falls back to a stub
 *                                   if SWATCH_HAVE_X11 is not defined)
 *
 * Returns 0 on clean exit, 64 on usage refusal (e.g. out-of-range size),
 * 1 on platform error (cannot open display, cannot create window, etc.).
 * Any error message is written to stderr.
 */
int swatch_window_run(swatch_window_opts_t opts);

/*
 * Fast inline xorshift64 PRNG (Marsaglia, 2003). State must be non-zero;
 * with non-zero seed the period is 2^64 - 1. ~1ns per call when inlined,
 * no mutex (unlike libc rand() / rand_r() with stdio locking on glibc).
 *
 * Header-inline so each backend's hot fill loop can inline this away
 * entirely under -O2.
 */
static inline uint64_t swatch_xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

/*
 * Pure, testable per-frame pixel fill. Writes width*height*4 bytes to
 * `pixels` in BGRA8888 little-endian (B, G, R, A=0xff per pixel) — matches
 * Win32 BI_RGB DIBs, X11 ZPixmap on TrueColor 24/32-bit visuals, and Core
 * Graphics with kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst.
 *
 * `state` is a caller-owned non-zero seed for swatch_xorshift64 (see above).
 * The state is advanced once per pixel; the function is fully deterministic
 * for a given starting state.
 *
 * Layout assumption: little-endian host. Storing as packed uint32_t with
 * value (0xFF<<24)|(R<<16)|(G<<8)|B yields the correct B,G,R,A byte order.
 *
 * If `bw` is zero: the low 24 bits of each xorshift64 result populate R/G/B.
 * If `bw` is non-zero: the low bit picks pure black (0,0,0) or white
 * (255,255,255).
 */
void swatch_window_fill_frame(uint8_t *pixels, int width, int height,
                              int bw, uint64_t *state);

#endif
