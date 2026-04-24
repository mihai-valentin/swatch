#ifndef SWATCH_NOISE_H
#define SWATCH_NOISE_H

#include <stdio.h>
#include "render.h"

typedef struct {
    int                 width;
    int                 height;
    int                 fps;
    int                 duration_seconds;
    swatch_color_mode_t mode;
} swatch_noise_opts_t;

/*
 * Run the noise animation.
 *
 * width/height <= 0 → probe terminal via TIOCGWINSZ, fall back to 80x24.
 * fps 0 → default 15; otherwise clamped to 1..60.
 * duration_seconds 0 → run until SIGINT/SIGTERM; otherwise clamped to 1..3600.
 *
 * Returns 0 on clean exit, 64 on refusal
 * (SWATCH_COLOR_NONE, or out == stdout with non-TTY stdout).
 */
int swatch_noise_run(swatch_noise_opts_t opts, FILE *out);

/*
 * Emit one frame of random-RGB white noise to `out`, prefixed with
 * a cursor-home escape (\x1b[H). Does NOT clear the screen, hide/show
 * the cursor, or flush — the caller is responsible for those.
 *
 * `rng` is invoked to produce pseudorandom bytes; callers pass a seeded
 * source. For TRUECOLOR, rng() is called 3 times per cell (R, G, B); for
 * XTERM256, rng() is also called 3 times per cell and the bytes are mapped
 * into the 6x6x6 xterm color cube. For SWATCH_COLOR_NONE nothing is
 * emitted (no escapes) so that accidental invocation cannot corrupt
 * terminals that can't handle them.
 */
void swatch_noise_draw_frame(swatch_color_mode_t mode, int width, int height,
                             unsigned (*rng)(void), FILE *out);

#endif
