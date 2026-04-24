# swatch — idea

## One-liner

Tiny C CLI that takes a hex color and prints a colored square in the terminal. Exists primarily as a **shawarma orchestration test-case**.

## Purpose

`swatch` is a deliberately tiny greenfield artifact whose only job is to exercise the shawarma multi-agent flow end-to-end:

- A planner has to stage four tasks in dependency order.
- Two workers have to run in parallel (parser + renderer) against a shared interface (headers written in task 1).
- A verifier has to validate C build discipline: `-Wall -Wextra -Wpedantic -Werror`, ASan/UBSan-clean, no memory leaks, tests pass.
- A final worker integrates everything in `main.c`.

Success = shawarma orchestrates the above autonomously and the resulting binary passes the acceptance checks below.

## Stack

C11, `gcc`/`clang`, plain `Makefile`. No external dependencies — `getopt_long` for flags, `isatty()`/`getenv()` from libc for environment probes. Tests via a tiny hand-rolled assert macro in `tests/test.h`.

Chosen because:

1. First C project wired through this workflow — different discipline than the Node/TS projects, different stressors on shawarma's workers (header hygiene, `-Werror`, sanitizer runs, memory safety).
2. Zero-dep build means the worker agents cannot sidestep anything by reaching for a library.

## CLI surface

```text
swatch <hex> [--size WxH] [--char CHAR] [--label] [--help] [--version]
```

| Flag           | Default    | Meaning                                                          |
|----------------|------------|------------------------------------------------------------------|
| `<hex>`        | (required) | `#RRGGBB`, `RRGGBB`, `#RGB`, or `RGB`                            |
| `--size WxH`   | `6x3`      | Block dimensions in characters                                   |
| `--char CHAR`  | `' '`      | Character to fill the block with (ANSI background still applies) |
| `--label`      | off        | Print the normalized hex on a line below the block               |
| `--color MODE` | `auto`     | Force color mode: `auto` / `truecolor` / `256` / `none`          |
| `--help`       | —          | Print usage and exit 0                                           |
| `--version`    | —          | Print `swatch X.Y.Z` and exit 0                                  |

## Color mode resolution (at startup)

Auto-detection order:

1. Not a TTY → `none`.
2. `NO_COLOR` set → `none`.
3. `COLORTERM` contains `truecolor` / `24bit` → `truecolor`.
4. `TERM` matches `*-direct` → `truecolor`.
5. Any of `WT_SESSION`, `KITTY_WINDOW_ID`, `ALACRITTY_LOG`, `KONSOLE_VERSION` set → `truecolor`.
6. `TERM_PROGRAM` ∈ { `iTerm.app`, `vscode`, `Apple_Terminal`, `WezTerm`, `ghostty` } → `truecolor`.
7. `TERMINAL_EMULATOR` starts with `JetBrains` → `truecolor`.
8. otherwise → `xterm256` (best-effort; 6×6×6 cube nearest-match).

`--color MODE` short-circuits the whole chain. Accepted values: `auto` (default, runs the chain above), `truecolor`, `256`, `none`.

### Noise subcommand (v0.2.0 candidate)

```text
swatch noise [--size WxH] [--color MODE] [--fps N] [--duration SECONDS]
```

`swatch noise` renders a fullscreen random-RGB white-noise animation and runs until `SIGINT` / `SIGTERM` (or `--duration` expires). It is an additive subcommand — the existing `swatch <hex>` form is unchanged.

| Flag              | Default     | Meaning                                                                |
|-------------------|-------------|------------------------------------------------------------------------|
| `--size WxH`      | auto        | Animation area; probed via `TIOCGWINSZ`, fall back to `80x24`          |
| `--color MODE`    | `auto`      | Required to resolve to `truecolor` or `256` — `none` is refused        |
| `--fps N`         | `15`        | Frames per second, clamped to `1..60`                                  |
| `--duration N`    | `0`         | Seconds to run; `0` means "until Ctrl+C", otherwise clamped to `1..3600` |

Requirements:

- stdout must be a TTY — piping/redirecting exits `64`.
- Color mode must not be `none` — exits `64`.
- `SIGINT` / `SIGTERM` trigger a clean exit: restore cursor, reset SGR, exit `0`.
- An `atexit` hook restores the cursor even if the process dies some other way.

## Architecture (deliberately split across 4 shawarma tasks)

```text
┌─────────────────────────────────────────────┐
│  task 1: scaffold                            │
│   Makefile, src/parse.h, src/render.h,       │
│   tests/test.h, .gitignore                   │
│   (defines the shared interfaces)            │
└────────────────┬─────────────────────────────┘
                 │
        ┌────────┴─────────┐
        ▼                  ▼
┌───────────────┐  ┌───────────────┐
│ task 2: parser│  │ task 3: render│
│ src/parse.c   │  │ src/render.c  │
│ tests/test_   │  │ tests/test_   │
│    parse.c    │  │   render.c    │
└───────┬───────┘  └───────┬───────┘
        └────────┬─────────┘
                 ▼
     ┌───────────────────────┐
     │ task 4: cli glue      │
     │ src/main.c, README.md │
     └───────────────────────┘
```

## Acceptance criteria

- `make clean && make` produces `./swatch` with zero warnings under `-Wall -Wextra -Wpedantic -Werror`.
- `make test` passes under `-fsanitize=address,undefined` (no leaks, no UB).
- `./swatch "#00AF4C"` on a truecolor terminal prints a 6×3 green block.
- `NO_COLOR=1 ./swatch "#00AF4C"` prints `#00AF4C (6x3)\n`.
- `./swatch nope` exits 2 with a clear error on stderr.
- `./swatch --help` exits 0; `./swatch --version` exits 0.
- `make install PREFIX=/tmp/swatch-test` lands a working binary at `/tmp/swatch-test/bin/swatch`.

## Out of scope

- No packaging (Homebrew / deb / rpm). GitHub Releases publishes tagged linux and macos tarballs via `.github/workflows/release.yml`.
- No configuration file, no theme presets, no palette mode, no image output.
- No Windows port (Unix-only — POSIX `getopt_long` + `isatty`).
- No man page. `--help` output is the documentation.
- No npm wrapper. This is a C tool; install via `make install`.

## Success signal

- Shawarma completes all four tasks end-to-end autonomously (no human intervention required at the worker or verifier stage).
- The planner correctly stages them (task 1 before 2/3; 4 last).
- Total wall-clock time ≤ 40 minutes; total cost ≤ $10 per shawarma run (per the cost table in the skill card).

Once the test-case goal is met, `swatch` has no further roadmap. It stays in the repo as a reference artifact and a regression target for shawarma.
