# Auto-research: guide for agents

This file is for AI agents, and the people who run them, who want to contribute to this repository.
It says what the maintainer, Nil Mamano, needs help with. Read it before you start. Then pick one
research track below. Do not spend your session on small incremental optimizations.

This file follows the auto-research convention proposed by Rohan Arun
([post](https://x.com/RohanArun/status/2110022929048056246)): an `auto-research.md` guide for agents,
plus an `auto-research/` folder where contributors share results, including negative ones.

## The problem

In 2-list stable matching there are n men and n women. Each man ranks all women by one of two master
lists, X or Y, and each woman ranks all men by one of two master lists. The input has size O(n), but
Gale-Shapley takes O(n^2) time. The question is how fast a stable matching can be computed. Kunnemann,
Moeller, Paturi and Schneider (CSR 2016, Algorithmica 2019; [arXiv:1510.06452](https://arxiv.org/abs/1510.06452))
asked whether succinct preferences make stable matching easier, and named this case as the natural starting
point. The blog post [2-List Stable Matching](https://nilmamano.com/blog/two-list-stable-matching) states the
problem with examples. [docs/summary.md](docs/summary.md) fixes the notation used in this repository.

## Current state

- Upper bound, partly audited: an anonymous draft, [paper/twolist-subquadratic.pdf](paper/twolist-subquadratic.pdf),
  claims a deterministic O(n^2 sqrt(log log n / log n)) algorithm on a word RAM. Its structural theorem
  (Theorem 4.11) and minimum-seam characterization (Proposition A.3) were independently confirmed on 2026-10-09
  (P, A+). Its packed implementation and running-time analysis (Section 5) were not independently audited (A0).
  The proof is not checked in Lean. Scope and audit details: [docs/subquadratic.md](docs/subquadratic.md).
- Open: an O(n^(2-e)) algorithm for some e > 0, or a near-linear one. No such algorithm is known.
- Lower bounds: only for specified models. Explicit families force quadratic work for proposal and unit-charge
  models (B2, B3, B6, B11-B14). They do not apply to every algorithm, and the draft's algorithm is outside them.
  B4 (S, A~): under NSETH, fast verification blocks specified deterministic fine-grained SAT reductions.
  See [docs/summary.md](docs/summary.md).
- Structure: each rotation changes the number of X-man/X-woman pairs by one, so there are at most 2^floor(n/2)
  stable matchings, and this is tight ([docs/rotations.md](docs/rotations.md)). Several restricted classes
  (few type runs, few list breakpoints, shared or reversed lists) are solved in near-linear time (C5, C9, C11).
- Failed routes: [docs/dead-routes.md](docs/dead-routes.md). Read it before you try an algorithm.

## What counts as a contribution

In order of value to the maintainer:

1. An independent audit of Section 5 of the draft: the packed implementation and its running time. A found
   error counts as much as a confirmation.
2. A polynomially faster algorithm, O(n^(2-e)), or even a near-linear one, with a proof.
3. A lower bound for a model that is wider than the existing ones, or a conditional hardness result.
4. A proof of an open structural question that helps (1) to (3).
5. A machine-checked proof of an audited result.
6. A negative result: a proved limit of a method, or a hard family for a stated class of algorithms.

Not wanted: shaving lower-order factors from the existing bound without a new idea, tuning the prototype
solvers, more benchmark tables without a claim, and cosmetic rewrites of existing files.

## How to verify

Requirements: Linux, GNU make, a C++17 compiler, and Python 3 (standard library). Commands run from the
repository root ([docs/code.md](docs/code.md) has the formats).

```sh
make
make test                      # the CI suite, meant to finish within two minutes
make test-full-cover           # 3,000-instance check of the draft's simple cover elimination
python3 generators/generate.py weave 32 build/weave.txt   # families: twin ltwin ltwin3 gtwin weave ring2
build/reference solve build/weave.txt build/matching.txt
build/reference check build/weave.txt build/matching.txt
build/reference enum <instance>   # brute force, n <= 9
```

- A new algorithm: implement it, check its output with `build/reference check` on generated families and
  random instances, and compare with brute force for small n. Tests support a proof. They do not replace it.
- A running-time claim needs a proof. Iteration counts, unit charges and work counters are different cost
  measures, and none of them is a measured runtime ([docs/summary.md](docs/summary.md)).
- A claim counts as audited only after a check with separate code, as in [verify/check.py](verify/check.py),
  which imports no producer code. Say whether your checker shares code with the code it checks.
- Label each claim with the status key of [docs/summary.md](docs/summary.md): P, S, T or C, and A+, A~, A- or A0.
  Your own new claims are A0 until someone else audits them.
- The repository has no Lean project. A formal proof must bring its own setup and build command.

## Research tracks

| Track | Folder | Goal |
| --- | --- | --- |
| Runtime audit | [auto-research/runtime-audit/](auto-research/runtime-audit/) | Audit or implement Section 5 of the draft (packing and running time). |
| Polynomial speedup | [auto-research/polynomial-speedup/](auto-research/polynomial-speedup/) | An O(n^(2-e)) or near-linear algorithm. |
| Lower bounds | [auto-research/lower-bounds/](auto-research/lower-bounds/) | Lower bounds for wider models, or conditional hardness. |
| Structure | [auto-research/structure/](auto-research/structure/) | Open structural questions listed in docs/summary.md. |
| Formal proofs | [auto-research/formal-proofs/](auto-research/formal-proofs/) | Machine-checked proofs of Theorem 4.11, Proposition A.3, or D1/D2. |

## How to share results

Put every result under `auto-research/<track>/`, including failed experiments and negative results, so that
other agents do not repeat them. Use one of two forms:

- One note: `auto-research/<track>/YYYY-MM-DD-<handle>-<short-slug>.md`
- A note with code or data: `auto-research/<track>/YYYY-MM-DD-<handle>-<short-slug>/README.md`, plus its files.

The note must include:

- Base commit of this repository.
- The question, and the approach you took.
- Outcome: positive, negative, or inconclusive, with a status label for each claim.
- Model or models and harness (for example, Claude Code with a named model), and what the human did.
- Cost, if you know it: tokens or dollars, wall time, CPU.
- One reproducible command, run from the repository root, and its expected output.
- For experiments: the instance families and sizes, seeds, and what was measured.
  "No counterexample found" is not a proof.
- Links to prior work you build on, in `auto-research/` and in `docs/`.

Open one pull request per result. It should add only files under `auto-research/<track>/`. If you also change
code elsewhere, for example a fix to a solver or checker, say so in the PR description and keep that change
minimal; `make test` must still pass. Do not edit `docs/summary.md`, `docs/subquadratic.md` or the README.
The maintainer updates them after review. The paper PDF is external work: do not modify it.
