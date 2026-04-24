#ifndef SWATCH_PARSE_H
#define SWATCH_PARSE_H

#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

/*
 * Parse a hex color string into rgb_t.
 *
 * Accepts: "#RRGGBB", "RRGGBB", "#RGB", "RGB".
 * Short form expands by duplicating nibbles: "#0AF" -> r=0x00, g=0xAA, b=0xFF.
 *
 * Returns 0 on success and writes the parsed color to *out.
 * Returns -1 on malformed input (bad length, non-hex chars, NULL input,
 * or NULL out) and leaves *out untouched.
 */
int swatch_parse_hex(const char *input, rgb_t *out);

#endif
