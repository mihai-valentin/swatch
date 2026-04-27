#ifndef SWATCH_WINDOW_H
#define SWATCH_WINDOW_H

#include <stdint.h>

typedef struct {
    int width;             /* pixels; 0 = default 640 */
    int height;            /* pixels; 0 = default 480 */
    int fps;               /* 1..60; 0 = default 15 */
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
 * Pure, testable per-frame pixel fill. Writes width*height*4 bytes to
 * `pixels` in BGRA8888 order (B, G, R, A=0xff per pixel) — matches Win32
 * BI_RGB DIBs, X11 ZPixmap on TrueColor 24/32-bit visuals, and Core
 * Graphics with kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst.
 *
 * If `bw` is zero: rng() is invoked three times per pixel for R/G/B
 * (low byte of each return value).
 *
 * If `bw` is non-zero: rng() is invoked exactly once per pixel; the low
 * bit of the return picks pure black (0,0,0) or pure white (255,255,255).
 */
void swatch_window_fill_frame(uint8_t *pixels, int width, int height,
                              int bw, unsigned (*rng)(void));

#endif
