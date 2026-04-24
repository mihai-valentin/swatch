#include "render.h"

#include <stdio.h>

static const int CUBE_STEPS[6] = {0, 95, 135, 175, 215, 255};

static int cube_index(uint8_t channel) {
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

static void write_fill(FILE *out, char fill, int width) {
    char c = (fill == '\0') ? ' ' : fill;
    for (int i = 0; i < width; i++) {
        fputc(c, out);
    }
}

static void write_block(FILE *out, const char *sgr_prefix, rgb_t color,
                        int use_truecolor, swatch_render_opts_t opts) {
    for (int row = 0; row < opts.height; row++) {
        if (use_truecolor) {
            fprintf(out, "\x1b[48;2;%u;%u;%um",
                    (unsigned)color.r, (unsigned)color.g, (unsigned)color.b);
        } else {
            fprintf(out, "%s", sgr_prefix);
        }
        write_fill(out, opts.fill, opts.width);
        fprintf(out, "\x1b[0m\n");
    }
    if (opts.label) {
        fprintf(out, "#%02X%02X%02X\n",
                (unsigned)color.r, (unsigned)color.g, (unsigned)color.b);
    }
}

void swatch_render(rgb_t color, swatch_render_opts_t opts, FILE *out) {
    if (out == NULL || opts.width <= 0 || opts.height <= 0) {
        return;
    }

    switch (opts.mode) {
    case SWATCH_COLOR_TRUECOLOR:
        write_block(out, NULL, color, 1, opts);
        return;
    case SWATCH_COLOR_XTERM256: {
        int r_idx = cube_index(color.r);
        int g_idx = cube_index(color.g);
        int b_idx = cube_index(color.b);
        int n = 16 + 36 * r_idx + 6 * g_idx + b_idx;
        char sgr[32];
        snprintf(sgr, sizeof(sgr), "\x1b[48;5;%dm", n);
        write_block(out, sgr, color, 0, opts);
        return;
    }
    case SWATCH_COLOR_NONE:
    default:
        fprintf(out, "#%02X%02X%02X (%dx%d)\n",
                (unsigned)color.r, (unsigned)color.g, (unsigned)color.b,
                opts.width, opts.height);
        return;
    }
}
