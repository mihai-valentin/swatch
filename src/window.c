#include "window.h"

#include <stddef.h>

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "swatch_window_fill_frame assumes a little-endian host (BGRA in memory == 0xFFRRGGBB as uint32_t)"
#endif

void swatch_window_fill_frame(uint8_t *pixels, int width, int height,
                              int bw, uint64_t *state) {
    if (pixels == NULL || width <= 0 || height <= 0 || state == NULL) {
        return;
    }

    /* 4-byte aligned in practice (malloc returns at least 8-byte alignment
     * on every supported target), but cast through void* to keep -Wcast-align
     * happy on stricter targets. */
    uint32_t *p32 = (uint32_t *)(void *)pixels;
    size_t total = (size_t)width * (size_t)height;

    if (bw) {
        for (size_t i = 0; i < total; i++) {
            uint64_t r = swatch_xorshift64(state);
            uint32_t v = (r & 1u)
                       ? (uint32_t)0xFFFFFFFFu  /* white: B=G=R=A=0xFF */
                       : (uint32_t)0xFF000000u; /* black: B=G=R=0, A=0xFF */
            p32[i] = v;
        }
    } else {
        for (size_t i = 0; i < total; i++) {
            uint64_t r = swatch_xorshift64(state);
            uint32_t b = (uint32_t)(r        & 0xFFu);
            uint32_t g = (uint32_t)((r >> 8) & 0xFFu);
            uint32_t rr = (uint32_t)((r >> 16) & 0xFFu);
            p32[i] = ((uint32_t)0xFFu << 24) | (rr << 16) | (g << 8) | b;
        }
    }
}
