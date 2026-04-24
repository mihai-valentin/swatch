#ifndef SWATCH_RENDER_H
#define SWATCH_RENDER_H

#include <stdio.h>
#include "parse.h"

typedef enum {
    SWATCH_COLOR_NONE      = 0,
    SWATCH_COLOR_XTERM256  = 1,
    SWATCH_COLOR_TRUECOLOR = 2
} swatch_color_mode_t;

typedef struct {
    int                 width;
    int                 height;
    char                fill;
    int                 label;
    swatch_color_mode_t mode;
} swatch_render_opts_t;

/*
 * Render a color swatch to `out`.
 *
 * Behavior by mode:
 *   SWATCH_COLOR_NONE      : writes "#RRGGBB (WxH)\n" in uppercase hex.
 *   SWATCH_COLOR_TRUECOLOR : writes "\x1b[48;2;R;G;Bm" + (fill x width) +
 *                            "\x1b[0m\n", repeated `height` times.
 *   SWATCH_COLOR_XTERM256  : same block shape using SGR 48;5;N with the
 *                            nearest match in the 6x6x6 color cube.
 *
 * If opts.label is non-zero, after the block one additional line is written:
 * "#RRGGBB\n" in uppercase hex.
 */
void swatch_render(rgb_t color, swatch_render_opts_t opts, FILE *out);

#endif
