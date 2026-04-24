# Sub-project — part of the xlnf workspace

This repository is **one sub-project** inside the `xlnf` meta-project. The root session at `../` (i.e. `/home/mihai/xlnf/`) administrates the broader workspace; this session implements this tool.

## Where to find meta-project context

When you need shared rules, conventions, or workspace context, read these files in the xlnf root:

| File | Purpose |
|------|---------|
| `../SUMMARY.md` | Vision and workspace index |
| `../CLAUDE.md` | Management rules: naming, branching, Shawarma workflow |
| `../.claude/skills/shawarma/SKILL.md` | Canonical Shawarma skill (also mirrored here under `.claude/skills/shawarma/`) |
| `../.agent-orch/` | Shared orchestration state (shawarma.db, task manifests) |
| `~/.gpt-export/exports/2026-04-15T08-17-46-740Z-neuro-with-shawarma/full.md` | Original source conversation for workspace decisions |

You may read from `../` freely. **Do not modify files in the xlnf root from this sub-session** — surface the change as a request to the root session instead.

## This sub-project

- **Concept:** see `./idea.md` (core idea, CLI surface, acceptance criteria, out-of-scope)
- **Role in the workspace:** Internal / test-case. `swatch` is a tiny C CLI used to exercise the shawarma multi-agent orchestration flow end-to-end.
- **Purpose:** validate that shawarma's planner correctly stages four dependent C tasks, that parallel workers respect a shared header interface, and that the verifier enforces `-Werror` + sanitizer discipline.

## Operating rules (inherited from root)

- Follow naming / branching / env-var conventions from `../CLAUDE.md`
- Default branch is `master` (matches the GitHub remote)
- swatch is a CLI — no Dockerfile, no health check, no stdout-logging ceremony. Install from source via `make install`. Tagged release binaries are published via `.github/workflows/release.yml` on `v*` pushes.
- Conventional commits
- Dispatch heavy lifting via Shawarma task manifests under `./.agent-orch/tasks/` — the skill card lives at `.claude/skills/shawarma/SKILL.md`
- Surface cross-workspace or architectural decisions back to the root session; do not decide them here

## Code style (C)

- C11 (`-std=c11`)
- Strict warnings: `-Wall -Wextra -Wpedantic -Werror`
- Debug target builds with `-fsanitize=address,undefined -g -O0`; release target with `-O2`
- Headers guarded with `#ifndef SWATCH_<NAME>_H`
- No external dependencies — libc + POSIX only
- Test harness is a tiny hand-rolled `ASSERT_*` / `RUN_TEST` set in `tests/test.h`; do NOT introduce Unity, Check, greatest, etc.

## Scope discipline

This session's scope is **this toy CLI only**. If shawarma's test-run surfaces improvements to the orchestration layer itself, those changes belong to `agent-orchestration-system`, not here. swatch is the stimulus, not the system under test.
