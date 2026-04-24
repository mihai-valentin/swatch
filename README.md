# swatch

![CI](REPLACE_WITH_REMOTE_URL/actions/workflows/ci.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

Tiny CLI that prints a colored square for a hex color.

```
$ swatch "#00AF4C"
█████████
█████████
█████████
```

(Square is rendered with ANSI 24-bit background escapes — the blocks are the background-color of the given hex.)

Part of the [xlnf](https://mihai-valentin.github.io/xlnf/) portfolio. Built as a shawarma orchestration test-case — see `idea.md`.

## Usage

```bash
swatch "#00AF4C"               # 6×3 block, default
swatch 00AF4C                  # '#' optional
swatch "#0AF"                  # short form expands to #00AAFF
swatch "#00AF4C" --size 10x4
swatch "#00AF4C" --char "#"
swatch "#00AF4C" --label       # adds '#00AF4C' under the block
```

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

| Code | Meaning |
|------|---------|
| 0 | Success |
| 2 | Malformed hex input |
| 64 | Usage error (bad flag / missing argument) |

## Installation — pre-built binary

Release binaries are published to GitHub Releases on every `v*` tag.

```bash
# Linux x86_64
curl -fsSL REPLACE_WITH_REMOTE_URL/releases/latest/download/swatch-v0.1.0-linux-x64.tar.gz | tar xz
sudo mv swatch /usr/local/bin/

# macOS (Apple Silicon or Intel)
curl -fsSL REPLACE_WITH_REMOTE_URL/releases/latest/download/swatch-v0.1.0-macos-arm64.tar.gz | tar xz
sudo mv swatch /usr/local/bin/
```

Pre-built binaries only exist after the first `v*` tag has been pushed. Before then, use the source install below.

## Installation — from source

```bash
git clone REPLACE_WITH_REMOTE_URL.git
cd swatch
make
sudo make install
```

Requires a C11 compiler (`gcc` or `clang`) and GNU `make`. No runtime dependencies.

## Contributing

swatch is deliberately tiny — open an issue before any non-trivial change so scope stays small.

Before submitting a PR: `make clean && make && make test` must pass locally. CI runs the same gate across `{gcc, clang} × {ubuntu, macos}`; if CI is green on your PR, you're done.
