#define _POSIX_C_SOURCE 200809L

#include "parse.h"
#include "render.h"
#include "noise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

static const char *prog_name(const char *argv0) {
    return (argv0 != NULL && argv0[0] != '\0') ? argv0 : "swatch";
}

static void print_usage(FILE *out, const char *prog) {
    fprintf(out,
        "Usage: %s <hex> [options]       Render a colored square for <hex>.\n"
        "       %s noise [options]       Random RGB white-noise animation.\n"
        "\n"
        "Subcommands:\n"
        "  noise          Fullscreen random-RGB white-noise animation until\n"
        "                 Ctrl+C (or --duration expires). Requires a TTY and\n"
        "                 a color mode other than 'none'.\n"
        "\n"
        "Options:\n"
        "  <hex>          #RRGGBB, RRGGBB, #RGB, or RGB (hex form only)\n"
        "  --size WxH     Block size in characters (default 6x3, each 1..200).\n"
        "                 In noise mode, overrides the animation area\n"
        "                 (default: probed from terminal, fallback 80x24).\n"
        "  --char CHAR    Single character used to fill the block (default ' ',\n"
        "                 hex form only)\n"
        "  --label        Print the normalized hex on a line below the block\n"
        "                 (hex form only)\n"
        "  --color MODE   Force color mode; one of auto (default), truecolor,\n"
        "                 256, none. Use 'truecolor' when auto-detection misses\n"
        "                 your terminal and shades snap to the xterm-256 cube.\n"
        "  --fps N        Frames per second for noise (1..60, default 15).\n"
        "  --duration N   Seconds to run noise (0..3600, default 0 = until\n"
        "                 Ctrl+C).\n"
        "  --bw           Black-and-white noise only (no color chroma, noise only).\n"
        "  --help         Print this help and exit 0\n"
        "  --version      Print version and exit 0\n"
        "\n"
        "Exit codes: 0 success, 2 malformed hex, 64 usage error.\n",
        prog, prog);
}

static int parse_size(const char *s, int *w_out, int *h_out) {
    if (s == NULL || *s == '\0') {
        return -1;
    }
    const char *x = strchr(s, 'x');
    if (x == NULL || x == s || *(x + 1) == '\0') {
        return -1;
    }

    char *endp = NULL;
    long w = strtol(s, &endp, 10);
    if (endp != x) {
        return -1;
    }
    long h = strtol(x + 1, &endp, 10);
    if (endp == x + 1 || *endp != '\0') {
        return -1;
    }
    if (w < 1 || w > 200 || h < 1 || h > 200) {
        return -1;
    }

    *w_out = (int)w;
    *h_out = (int)h;
    return 0;
}

static int parse_int_arg(const char *s, long min, long max, long *out) {
    if (s == NULL || *s == '\0') {
        return -1;
    }
    char *endp = NULL;
    long v = strtol(s, &endp, 10);
    if (endp == s || *endp != '\0') {
        return -1;
    }
    if (v < min || v > max) {
        return -1;
    }
    *out = v;
    return 0;
}

static swatch_color_mode_t resolve_color_mode(void) {
    if (isatty(fileno(stdout)) == 0) {
        return SWATCH_COLOR_NONE;
    }
    if (getenv("NO_COLOR") != NULL) {
        return SWATCH_COLOR_NONE;
    }

    const char *ct = getenv("COLORTERM");
    if (ct != NULL && (strstr(ct, "truecolor") != NULL || strstr(ct, "24bit") != NULL)) {
        return SWATCH_COLOR_TRUECOLOR;
    }

    const char *term = getenv("TERM");
    if (term != NULL && strstr(term, "direct") != NULL) {
        return SWATCH_COLOR_TRUECOLOR;
    }

    if (getenv("WT_SESSION") != NULL)       return SWATCH_COLOR_TRUECOLOR;
    if (getenv("KITTY_WINDOW_ID") != NULL)  return SWATCH_COLOR_TRUECOLOR;
    if (getenv("ALACRITTY_LOG") != NULL)    return SWATCH_COLOR_TRUECOLOR;
    if (getenv("KONSOLE_VERSION") != NULL)  return SWATCH_COLOR_TRUECOLOR;

    const char *tp = getenv("TERM_PROGRAM");
    if (tp != NULL) {
        if (strcmp(tp, "iTerm.app") == 0 ||
            strcmp(tp, "vscode") == 0 ||
            strcmp(tp, "Apple_Terminal") == 0 ||
            strcmp(tp, "WezTerm") == 0 ||
            strcmp(tp, "ghostty") == 0) {
            return SWATCH_COLOR_TRUECOLOR;
        }
    }

    const char *tem = getenv("TERMINAL_EMULATOR");
    if (tem != NULL && strncmp(tem, "JetBrains", 9) == 0) {
        return SWATCH_COLOR_TRUECOLOR;
    }

    return SWATCH_COLOR_XTERM256;
}

static int parse_color_arg(const char *s, swatch_color_mode_t *mode_out, int *is_auto_out) {
    if (s == NULL) return -1;
    if (strcmp(s, "auto") == 0) {
        *is_auto_out = 1;
        return 0;
    }
    *is_auto_out = 0;
    if (strcmp(s, "truecolor") == 0 || strcmp(s, "24bit") == 0) {
        *mode_out = SWATCH_COLOR_TRUECOLOR;
        return 0;
    }
    if (strcmp(s, "256") == 0 || strcmp(s, "xterm256") == 0) {
        *mode_out = SWATCH_COLOR_XTERM256;
        return 0;
    }
    if (strcmp(s, "none") == 0 || strcmp(s, "off") == 0) {
        *mode_out = SWATCH_COLOR_NONE;
        return 0;
    }
    return -1;
}

int main(int argc, char **argv) {
    const char *prog = prog_name(argc > 0 ? argv[0] : NULL);

    int width = 6;
    int height = 3;
    int size_specified = 0;
    char fill = ' ';
    int label = 0;
    int color_forced = 0;
    swatch_color_mode_t forced_mode = SWATCH_COLOR_NONE;
    int fps_opt = 0;
    int duration_opt = 0;
    int bw_opt = 0;

    enum { OPT_SIZE = 256, OPT_CHAR, OPT_LABEL, OPT_HELP, OPT_VERSION, OPT_COLOR,
           OPT_FPS, OPT_DURATION, OPT_BW };
    static const struct option longopts[] = {
        {"size",     required_argument, 0, OPT_SIZE},
        {"char",     required_argument, 0, OPT_CHAR},
        {"label",    no_argument,       0, OPT_LABEL},
        {"color",    required_argument, 0, OPT_COLOR},
        {"fps",      required_argument, 0, OPT_FPS},
        {"duration", required_argument, 0, OPT_DURATION},
        {"bw",       no_argument,       0, OPT_BW},
        {"help",     no_argument,       0, OPT_HELP},
        {"version",  no_argument,       0, OPT_VERSION},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
        switch (c) {
        case OPT_SIZE:
            if (parse_size(optarg, &width, &height) != 0) {
                fprintf(stderr, "swatch: invalid --size '%s' (expected WxH, each 1..200)\n",
                        optarg != NULL ? optarg : "");
                return 64;
            }
            size_specified = 1;
            break;
        case OPT_CHAR:
            if (optarg == NULL || optarg[0] == '\0' || optarg[1] != '\0') {
                fprintf(stderr, "swatch: --char requires exactly one character\n");
                return 64;
            }
            fill = optarg[0];
            break;
        case OPT_LABEL:
            label = 1;
            break;
        case OPT_COLOR: {
            int is_auto = 0;
            swatch_color_mode_t m = SWATCH_COLOR_NONE;
            if (parse_color_arg(optarg, &m, &is_auto) != 0) {
                fprintf(stderr, "swatch: invalid --color '%s' (expected auto|truecolor|256|none)\n",
                        optarg != NULL ? optarg : "");
                return 64;
            }
            if (!is_auto) {
                color_forced = 1;
                forced_mode = m;
            } else {
                color_forced = 0;
            }
            break;
        }
        case OPT_FPS: {
            long v = 0;
            if (parse_int_arg(optarg, 1, 60, &v) != 0) {
                fprintf(stderr, "swatch: invalid --fps '%s' (expected integer 1..60)\n",
                        optarg != NULL ? optarg : "");
                return 64;
            }
            fps_opt = (int)v;
            break;
        }
        case OPT_DURATION: {
            long v = 0;
            if (parse_int_arg(optarg, 0, 3600, &v) != 0) {
                fprintf(stderr, "swatch: invalid --duration '%s' (expected integer 0..3600)\n",
                        optarg != NULL ? optarg : "");
                return 64;
            }
            duration_opt = (int)v;
            break;
        }
        case OPT_BW:
            bw_opt = 1;
            break;
        case OPT_HELP:
            print_usage(stdout, prog);
            return 0;
        case OPT_VERSION:
            printf("swatch 0.1.0\n");
            return 0;
        case '?':
        default:
            print_usage(stderr, prog);
            return 64;
        }
    }

    if (optind < argc && strcmp(argv[optind], "noise") == 0) {
        optind++;
        if (optind != argc) {
            fprintf(stderr, "swatch: 'noise' takes no positional arguments\n");
            return 64;
        }
        swatch_noise_opts_t n_opts = {
            .width            = size_specified ? width : 0,
            .height           = size_specified ? height : 0,
            .fps              = fps_opt,
            .duration_seconds = duration_opt,
            .bw               = bw_opt,
            .mode             = color_forced ? forced_mode : resolve_color_mode()
        };
        return swatch_noise_run(n_opts, stdout);
    }

    int positional = argc - optind;
    if (positional != 1) {
        print_usage(stderr, prog);
        return 64;
    }

    const char *hex = argv[optind];
    rgb_t color;
    if (swatch_parse_hex(hex, &color) != 0) {
        fprintf(stderr, "swatch: malformed hex '%s'\n", hex);
        return 2;
    }

    swatch_render_opts_t opts = {
        .width = width,
        .height = height,
        .fill = fill,
        .label = label,
        .mode = color_forced ? forced_mode : resolve_color_mode()
    };
    swatch_render(color, opts, stdout);
    return 0;
}
