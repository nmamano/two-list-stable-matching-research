# auto-research/

Shared results from agents working on this repository, including failed experiments and negative results.
Read [../auto-research.md](../auto-research.md) first. It describes the problem, the current state, and how
to verify a claim.

## Layout

One subfolder per research track:

| Folder | Track |
| --- | --- |
| [runtime-audit/](runtime-audit/) | Audit or implement Section 5 of the subquadratic draft. |
| [polynomial-speedup/](polynomial-speedup/) | An O(n^(2-e)) or near-linear algorithm. |
| [lower-bounds/](lower-bounds/) | Lower bounds for wider models, or conditional hardness. |
| [structure/](structure/) | Open structural questions. |
| [formal-proofs/](formal-proofs/) | Machine-checked proofs of audited results. |

Each track folder has a README with context. Read it, and the notes already in the folder, before you start.

## One result per entry

- A single note: `<track>/YYYY-MM-DD-<handle>-<short-slug>.md`
- A note with code or data: `<track>/YYYY-MM-DD-<handle>-<short-slug>/README.md`, plus its files
  (for example `code/`, `instances/`, `check.py`).

Use the date you finish, your GitHub handle, and a slug of a few words. Do not edit another contributor's entry.
To correct or extend it, add a new entry that links to it.

The note must state: the base commit, the question and approach, the outcome (positive, negative or
inconclusive, with the status key of [../docs/summary.md](../docs/summary.md) for each claim), the models and
harness, the cost if known, and one reproducible command with its expected output. See
[../auto-research.md](../auto-research.md), section "How to share results", for the full list.

Instance files should use the keyword/order format of [../docs/code.md](../docs/code.md), so that
`build/reference check` can read them.

## Relation to docs/

[../docs/](../docs/) holds the reviewed summary of results. New work goes here. If a result is accepted, the
maintainer updates `docs/` and the README.
