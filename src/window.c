#include "window.h"

#include <stddef.h>

void swatch_window_fill_frame(uint8_t *pixels, int width, int height,
                              int bw, unsigned (*rng)(void)) {
    if (pixels == NULL || width <= 0 || height <= 0 || rng == NULL) {
        return;
    }

    size_t i = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r, g, b;
            if (bw) {
                uint8_t v = (rng() & 1u) ? (uint8_t)0xFFu : (uint8_t)0x00u;
                r = g = b = v;
            } else {
                r = (uint8_t)(rng() & 0xFFu);
                g = (uint8_t)(rng() & 0xFFu);
                b = (uint8_t)(rng() & 0xFFu);
            }
            pixels[i++] = b;
            pixels[i++] = g;
            pixels[i++] = r;
            pixels[i++] = (uint8_t)0xFFu;
        }
    }
}
