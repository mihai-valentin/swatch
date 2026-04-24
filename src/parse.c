#include "parse.h"

#include <ctype.h>
#include <string.h>

static int hex_nibble(char c) {
    if (!isxdigit((unsigned char)c)) {
        return -1;
    }
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return 10 + (c - 'A');
}

int swatch_parse_hex(const char *input, rgb_t *out) {
    if (input == NULL || out == NULL) {
        return -1;
    }

    const char *p = input;
    if (*p == '#') {
        p++;
    }

    size_t len = strlen(p);
    if (len != 3 && len != 6) {
        return -1;
    }

    int nibbles[6];
    for (size_t i = 0; i < len; i++) {
        int n = hex_nibble(p[i]);
        if (n < 0) {
            return -1;
        }
        nibbles[i] = n;
    }

    uint8_t r, g, b;
    if (len == 3) {
        r = (uint8_t)((nibbles[0] << 4) | nibbles[0]);
        g = (uint8_t)((nibbles[1] << 4) | nibbles[1]);
        b = (uint8_t)((nibbles[2] << 4) | nibbles[2]);
    } else {
        r = (uint8_t)((nibbles[0] << 4) | nibbles[1]);
        g = (uint8_t)((nibbles[2] << 4) | nibbles[3]);
        b = (uint8_t)((nibbles[4] << 4) | nibbles[5]);
    }

    out->r = r;
    out->g = g;
    out->b = b;
    return 0;
}
