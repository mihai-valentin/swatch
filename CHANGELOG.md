# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-04-24

### Added

- `swatch noise` subcommand — fullscreen random-RGB white-noise animation; honors `--color`, `--size`; new `--fps` (1..60, default 15) and `--duration` (0..3600, 0 = run until Ctrl+C) flags. Refuses with exit 64 if stdout is not a TTY or color mode is `none`.
- `swatch noise --bw` flag for black-and-white-only (1-bit) noise; palette is pure black/white per cell, 50/50 random.

### Fixed

- `swatch noise` no longer scrolls the terminal: uses the alternate screen buffer (ANSI `\x1b[?1049h` / `\x1b[?1049l`) for the duration of the animation, so scrollback is preserved and Ctrl+C returns the user to their original shell state.

## [0.1.1] - 2026-04-24

### Fixed

- `make install` on macOS: the `install -D` flag was GNU-specific and fails on BSD `install` (macOS) where `-D` takes a path argument. Replaced with `mkdir -p $(DESTDIR)$(PREFIX)/bin && install -m 755 ...` — portable across GNU coreutils and BSD install, and adds `DESTDIR` support for staged/packaged installs.

## [0.1.0] - 2026-04-24

### Added

- Hex color parser accepting `#RRGGBB`, `RRGGBB`, `#RGB`, and `RGB`. Malformed input exits 2 with a stderr message.
- ANSI renderer with three modes: truecolor (24-bit), xterm256 (6×6×6 cube nearest-match), and none (plain `#RRGGBB (WxH)` output). Mode is auto-selected from stdout TTY, `NO_COLOR`, `COLORTERM`, `TERM` (`*-direct`), and vendor-specific env (`WT_SESSION`, `KITTY_WINDOW_ID`, `ALACRITTY_LOG`, `KONSOLE_VERSION`, `TERM_PROGRAM`, `TERMINAL_EMULATOR`) — covers Windows Terminal, Kitty, Alacritty, Konsole, iTerm2, Apple Terminal, WezTerm, Ghostty, VS Code, and JetBrains IDE terminals that don't set `COLORTERM`.
- CLI flags: `--size WxH` (default 6×3, each 1..200), `--char CHAR`, `--label`, `--color MODE` (`auto` / `truecolor` / `256` / `none`), `--help`, `--version`.
- Makefile targets: `all`, `debug`, `test`, `install`, `clean`, `dist`, `help`. Release build uses `-std=c11 -Wall -Wextra -Wpedantic -Werror -O2`.
- Unit tests: 12 parser tests + 12 renderer tests. All tests run under `-fsanitize=address,undefined`.
- Zero external dependencies — libc + POSIX only (`getopt_long`, `isatty`, `getenv`, `fmemopen`).

[0.2.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.2.0
[0.1.1]: https://github.com/mihai-valentin/swatch/releases/tag/v0.1.1
[0.1.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.1.0
