---
name: shawarma
description: |
  Orchestrate multi-agent development tasks using Shawarma.
  Use when the user wants to parallelize independent implementation tasks,
  run complex multi-file changes with verification, or distribute work
  across specialized agents (planner, distributor, worker, verifier).
  Trigger when: user mentions "shawarma", wants to run multiple tasks in parallel,
  needs multi-agent orchestration, or asks to distribute development work.
---

# Shawarma Task Orchestrator

You are helping the user orchestrate development tasks using Shawarma, a multi-agent system available as the `shawarma` CLI.

## When to use Shawarma

Shawarma adds orchestration overhead (task management, worktree setup, export, conflict resolution). It pays off **only** when parallelism benefits outweigh that overhead.

**✅ Good fit — use Shawarma when:**
- **10+ truly independent tasks** that can run in parallel
- **Large codebases** (5k+ LOC, 100+ files) with well-separated concerns
- **Multi-package monorepos** where agents work in different packages without conflicts
- **Batch refactors** across many files (renames, migrations, API upgrades)
- Complex changes that benefit from the worker → verifier pipeline

**❌ Bad fit — use a direct Claude Code session instead when:**
- **Small projects** (<3k LOC, <50 files) — the orchestration overhead exceeds the parallelism benefit
- **Fewer than 5 independent tasks** — not enough work to justify task management, export, and conflict resolution
- **Tightly coupled changes** where most tasks would touch the same files and cause merge conflicts
- **Exploratory/iterative work** where requirements aren't well-defined upfront

**Rough cost/time comparison:**
| Approach | Typical cost | Typical time | Best for |
|----------|-------------|-------------|----------|
| Direct CC session | ~$3-5 | 30-40 min | Small projects, <5 tasks, exploratory work |
| Shawarma | ~$10-15 | 60+ min | 10+ parallel tasks, large codebases |

**💡 Lightweight alternative for small projects:**
For small projects where you still want Shawarma's verification value, run a single task in guided mode (`shawarma --task <id> --mode guided`). This gives you the worker → verifier pipeline for one task without the full orchestration overhead of parallel execution, task management, and export. The skeptic review runs automatically when confidence scoring triggers it — it is not a standalone CLI flag.

## Task definition format

Create task JSON files in `.agent-orch/tasks/` in the project directory:

```json
{
  "id": "unique-task-id",
  "projectId": "project-name",
  "title": "Short descriptive title",
  "description": "Detailed description of what to implement and why.\n\nInclude context the worker agent needs.",
  "requirements": [
    "Specific requirement 1 — be precise about what files to create/modify",
    "Specific requirement 2 — include expected behavior",
    "Verify the project compiles with tsc --noEmit and all existing tests still pass"
  ],
  "allowedPaths": ["src/file1.ts", "src/file2.ts", "tests/file1.test.ts"],
  "forbiddenPaths": [],
  "priority": 5,
  "status": "pending",
  "maxRetries": 2
}
```

### Task writing guidelines

- **id**: kebab-case, unique. Use a prefix for the project (e.g., `myapp-auth-login`)
- **requirements**: Be specific and actionable. Each requirement should be independently verifiable. Always include a compilation/test check as the last requirement.
- **allowedPaths**: List only the files the worker should touch. This enforces scope via hooks.
- **priority**: Higher number = runs first (9 = high, 5 = normal, 1 = low)
- **status**: Use `pending` for tasks ready to run, `blocked` for tasks with unmet dependencies

## Workflow

### 1. Define tasks
Create `.agent-orch/tasks/*.json` files in the target project, one per task.

### 2. Import and run
```bash
# Option A: Import project (copies source into Shawarma's managed area)
shawarma --import-project /path/to/project --parallel --report

# Option B: Work directly on an external repo (no copy — worktrees branch from repo in-place)
shawarma --repo /path/to/project --planned --report

# Or step by step:
shawarma --import-project /path/to/project   # or --repo /path/to/project
shawarma --project <project-id> --parallel --report
```

**`--repo` vs `--import-project`**: Use `--repo` when you want agents to work directly on the original repo (worktrees are created from its `.git`). Use `--import-project` when you want an isolated copy. `--repo` avoids the copy overhead and makes export unnecessary — changes merge directly into the repo's main branch.

### 3. Check results
After the run completes, check the summary output. Failed tasks are reset to `pending` for retry.

To re-run failed tasks:
```bash
shawarma --project <project-id> --parallel --report
```

### 4. Export changes back
```bash
shawarma --export-project <project-id>
```
This applies git patches from Shawarma's project copy back to the source repo.

## Common flags

| Flag | Purpose |
|------|---------|
| `--parallel` | Run all pending tasks concurrently in isolated worktrees (capped at 4) |
| `--concurrency <n>` | Max parallel pipelines (1..4; `1` runs sequentially; applies to `--parallel` and `--planned`) |
| `--planned` | Use planner agent to stage tasks by dependency order |
| `--report` | Print cost/token metrics after run |
| `--project <id>` | Filter to tasks from a specific project |
| `--task <id>` | Run a single specific task |
| `--mode direct\|guided\|full` | Force execution depth |
| `--worker-model <alias>` | Override worker model (e.g. `sonnet` for cheaper runs) |
| `--repo <path>` | Work directly on an external repo (no copy, worktrees from repo) |
| `--import-project <path>` | Import an external project (copy) |
| `--export-project <id>` | Export Shawarma commits back to source via git patches |
| `--archive-done` | Bulk-archive every task currently in `done` status, then exit |
| `--auto-tasks` | Auto-create pending tasks from ≥80% confidence post-worker recommendations |
| `--daemon --project <id>` | Watch for new pending tasks (poll every 2s) |

## Execution modes

Shawarma auto-selects the pipeline depth based on task complexity:

- **direct**: Worker only — for simple/low-risk tasks (docs, config, small fixes)
- **guided**: Worker → Verifier — default for most tasks
- **full**: Distributor → Worker → Verifier — for complex tasks with many requirements (may split into subtasks)

## Handling dependencies between tasks

For tasks that depend on each other, use staged execution:

1. Set dependent tasks to `status: "blocked"`
2. Run independent tasks first with `--parallel`
3. After completion, update blocked tasks to `pending`
4. Run again

Or use `--planned` which lets the planner agent figure out the stages automatically.

## Handling failures

- **Merge conflicts**: Sibling subtasks touching same files. Re-run and they'll self-heal.
- **Worker timeout**: Task too complex for 600s. Split into subtasks.
- **Verifier failure**: Worker didn't meet requirements. Auto-retried with feedback.
- **Lock conflict**: Two tasks need same files. Sequential re-run resolves it.

## Tips

- Keep tasks focused — 3-6 requirements each. Large tasks get split by the distributor anyway.
- Include test requirements — "Add tests in tests/foo.test.ts" helps the verifier validate.
- Use specific allowedPaths — broad paths like `["."]` disable scope enforcement.
- Check `shawarma --report` output for cost tracking after runs.
- Once a wave of tasks finishes, run `shawarma --archive-done` to hide completed tasks from future listings (DB row stays; re-archivable is a no-op).
- For offline learning consolidation (lessons → skills → anti-pattern promotion), run `npm run agent:sleep` from inside the shawarma repo — there is no `--sleep` CLI flag.
