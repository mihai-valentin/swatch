# swatch

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
