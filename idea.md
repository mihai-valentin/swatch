# swatch — idea

## One-liner

Tiny C CLI that takes a hex color and prints a colored square in the terminal. Exists primarily as a **shawarma orchestration test-case**, not as a revenue product.

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

1. It's the portfolio's first C artifact — different discipline than the Node/TS projects, different stressors on shawarma's workers (header hygiene, `-Werror`, sanitizer runs, memory safety).
2. Zero-dep build means the worker agents cannot sidestep anything by reaching for a library.

## CLI surface

```text
swatch <hex> [--size WxH] [--char CHAR] [--label] [--help] [--version]
```

| Flag          | Default    | Meaning                                                          |
|---------------|------------|------------------------------------------------------------------|
| `<hex>`       | (required) | `#RRGGBB`, `RRGGBB`, `#RGB`, or `RGB`                            |
| `--size WxH`  | `6x3`      | Block dimensions in characters                                   |
| `--char CHAR` | `' '`      | Character to fill the block with (ANSI background still applies) |
| `--label`     | off        | Print the normalized hex on a line below the block               |
| `--help`      | —          | Print usage and exit 0                                           |
| `--version`   | —          | Print `swatch X.Y.Z` and exit 0                                  |

## Color mode resolution (at startup)

1. If stdout is not a TTY → `none`.
2. Else if `getenv("NO_COLOR")` is set (any value) → `none`.
3. Else if `getenv("COLORTERM")` contains `truecolor` or `24bit` → `truecolor`.
4. Else → `xterm256` (best-effort downgrade; 6×6×6 cube nearest-match).

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
