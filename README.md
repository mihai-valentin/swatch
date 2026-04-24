# swatch

![CI](https://github.com/mihai-valentin/swatch/actions/workflows/ci.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

Tiny CLI that prints a colored square for a hex color.

```console
$ swatch "#00AF4C"
█████████
█████████
█████████
```

(Square is rendered with ANSI 24-bit background escapes — the blocks are the background-color of the given hex.)

## Usage

```bash
swatch "#00AF4C"                      # 6×3 block, default
swatch 00AF4C                         # '#' optional
swatch "#0AF"                         # short form expands to #00AAFF
swatch "#00AF4C" --size 10x4
swatch "#00AF4C" --char "#"
swatch "#00AF4C" --label              # adds '#00AF4C' under the block
swatch "#00AF4C" --color truecolor    # force 24-bit when auto-detection misses
swatch "#00AF4C" --color 256          # force xterm-256 cube
swatch "#00AF4C" --color none         # plaintext ("#00AF4C (6x3)")
```

### Color mode resolution

`auto` (default for `--color`) picks in this order:

1. stdout is not a TTY → `none`.
2. `NO_COLOR` set → `none`.
3. `COLORTERM` contains `truecolor` / `24bit` → `truecolor`.
4. `TERM` matches `*-direct` → `truecolor`.
5. Any of `WT_SESSION`, `KITTY_WINDOW_ID`, `ALACRITTY_LOG`, `KONSOLE_VERSION` set → `truecolor`.
6. `TERM_PROGRAM` ∈ { `iTerm.app`, `vscode`, `Apple_Terminal`, `WezTerm`, `ghostty` } → `truecolor`.
7. `TERMINAL_EMULATOR` starts with `JetBrains` → `truecolor`.
8. otherwise → `xterm256` (216-color cube; shades snap to the nearest {0, 95, 135, 175, 215, 255} step per channel).

If the heuristics miss your terminal and shades look flat, pass `--color truecolor` explicitly.

### Non-TTY / `NO_COLOR` fallback

If stdout is not a TTY, `NO_COLOR` is set, or the terminal is not truecolor-capable, `swatch` degrades gracefully:

- `truecolor` (default when `COLORTERM=truecolor` or `24bit`): 24-bit ANSI background.
- `xterm256` (fallback): nearest-color match in the 6×6×6 ANSI cube.
- `none` (when `NO_COLOR=1` or not a TTY): plain text — `#00AF4C (6x3)`.

## Build

```bash
make              # release build → ./swatch
make debug        # ASan + UBSan build
make test         # unit tests under sanitizers
make install      # -> $PREFIX/bin/swatch  (default PREFIX=/usr/local)
make clean
make dist         # source tarball
```

Requires a C11 compiler (`gcc` or `clang`) and `make`. No other dependencies.

## Exit codes

| Code | Meaning                                   |
|------|-------------------------------------------|
| 0    | Success                                   |
| 2    | Malformed hex input                       |
| 64   | Usage error (bad flag / missing argument) |

## Installation — pre-built binary

Release binaries are published to GitHub Releases on every `v*` tag.

```bash
# Linux x86_64
curl -fsSL https://github.com/mihai-valentin/swatch/releases/latest/download/swatch-v0.1.0-linux-x64.tar.gz | tar xz
sudo mv swatch /usr/local/bin/

# macOS (Apple Silicon or Intel)
curl -fsSL https://github.com/mihai-valentin/swatch/releases/latest/download/swatch-v0.1.0-macos-arm64.tar.gz | tar xz
sudo mv swatch /usr/local/bin/
```

Pre-built binaries only exist after the first `v*` tag has been pushed. Before then, use the source install below.

## Installation — from source

```bash
git clone https://github.com/mihai-valentin/swatch.git
cd swatch
make
sudo make install
```

Requires a C11 compiler (`gcc` or `clang`) and GNU `make`. No runtime dependencies.

## Animation — `swatch noise`

`swatch noise` renders a fullscreen random-RGB white-noise animation in the terminal until you press Ctrl+C (or `--duration` expires). It honors `--color` (required to be `truecolor` or `256`, not `none`), `--size WxH` (defaults to the terminal's current size, falling back to 80x24), and two noise-only flags: `--fps N` (1..60, default 15) and `--duration SECONDS` (0..3600, default 0 = run until Ctrl+C).

```bash
swatch noise                          # run until Ctrl+C at 15 fps
swatch noise --fps 30 --duration 5    # 30 fps, auto-stop after 5 seconds
swatch noise --color 256              # force xterm-256 cube mapping
swatch noise --size 60x20             # override the animation area
```

Requires a TTY on stdout and a color mode other than `none` — piping or redirecting output, or running with `--color none` / `NO_COLOR=1`, exits 64 with a stderr message.

Uses the alternate screen buffer, so Ctrl+C (or `--duration` expiry) leaves your scrollback intact.

## Contributing

swatch is deliberately tiny — open an issue before any non-trivial change so scope stays small.

Before submitting a PR: `make clean && make && make test` must pass locally. CI runs the same gate across `{gcc, clang} × {ubuntu, macos}`; if CI is green on your PR, you're done.
