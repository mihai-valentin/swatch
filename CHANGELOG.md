# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.1] - 2026-04-27

### Fixed

- macOS build: the v0.4.0 frame-pacing rework called `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`, but Apple shipped `clock_gettime` in macOS 10.12 without ever adding `clock_nanosleep`. The build failed with `error: call to undeclared function 'clock_nanosleep'`. Both `noise.c` and `window_cocoa.c` now compute the remaining interval against `CLOCK_MONOTONIC` and fall back to `nanosleep()` on `__APPLE__`. Linux/glibc still uses `clock_nanosleep(TIMER_ABSTIME)` directly.

## [0.4.0] - 2026-04-27

### Added

- `--fps` cap raised from 60 to 120 (range is now `1..120`, default still 15). Applies to both `swatch noise` and `swatch window`.

### Changed

- `swatch window` now uses an inlined xorshift64 PRNG (Marsaglia, 2003) instead of libc `rand()` for the per-pixel fill. Eliminates the per-call mutex in glibc/msvcrt rand and the function-pointer indirection that prevented inlining; per-frame fill at 1920×1080 drops from a `rand()`-bound ~100 ms to ~5 ms (measured `-O2`, ~20× faster on the fill alone). Pixel writes are now 32-bit BGRA stores (`uint32_t`) instead of four byte stores, which lets `-O2` emit a single MOV per pixel.
- Frame pacing across `swatch noise` and `swatch window` switched from `nanosleep(interval)` after work to absolute-deadline `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` (POSIX) and `QueryPerformanceCounter` + `Sleep` (Win32). Previously the cycle was `render + sleep`, so a 50 ms render at 60 fps target produced ~15 effective fps; now the target is honored up to the work-rate ceiling and frames don't compound lag. Most user-visible at full-screen, where the X11 path felt sluggish.
- The internal `swatch_window_fill_frame` signature changed from `unsigned (*rng)(void)` (function pointer) to `uint64_t *state` (caller-owned PRNG state). The pure helper remains testable with deterministic bytes — the unit tests replicate xorshift64 to assert exact output. No CLI behavior change; this is a purely internal reshape.
- BGRA fill assumes a little-endian host (compile-time `#error` on big-endian). All supported targets (x86_64, arm64) are little-endian.

## [0.3.1] - 2026-04-27

### Fixed

- `swatch window` now grows the animation when the window is resized, maximized, or full-screened. Previously the pixel buffer stayed at the initial `--size` (or 640x480), leaving the noise pinned to a small region in the upper-left of a larger window. X11 listens for `ConfigureNotify` and reallocates the buffer + `XImage` at the new size; Win32 switches to `WS_OVERLAPPEDWINDOW` and handles `WM_SIZE` to realloc the DIB; Cocoa adds `NSWindowStyleMaskResizable` and sets the image view's `imageScaling` to `NSImageScaleAxesIndependently` so the noise stretches with the view.

## [0.3.0] - 2026-04-27

### Added

- `swatch window` subcommand — runs the noise animation in a native OS window using only system GUI libraries: Win32 (`user32`/`gdi32`) on Windows, X11 (`libX11`) on Linux/Unix, Cocoa via `objc_msgSend` (from a pure-C source file, no Objective-C compilation) on macOS. Honors `--size WxH` (in pixels, default 640x480, width 64..3840, height 64..2160), `--fps`, `--duration`, `--bw`. `--color`, `--char`, `--label` are accepted but ignored in window mode.

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

[0.4.1]: https://github.com/mihai-valentin/swatch/releases/tag/v0.4.1
[0.4.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.4.0
[0.3.1]: https://github.com/mihai-valentin/swatch/releases/tag/v0.3.1
[0.3.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.3.0
[0.2.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.2.0
[0.1.1]: https://github.com/mihai-valentin/swatch/releases/tag/v0.1.1
[0.1.0]: https://github.com/mihai-valentin/swatch/releases/tag/v0.1.0
