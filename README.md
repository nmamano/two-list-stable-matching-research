# Two-list stable matching in subquadratic time

**A stable matching of a two-list instance can be computed in
`O(n^2 sqrt(log log n / log n))` time, which is `o(n^2)`.** This breaks the quadratic
bound of Gale-Shapley for the problem.

In the two-list model, each of n men ranks the women by one of two master lists, and
each of n women ranks the men by one of two master lists, so the input has size O(n).
Kunnemann, Moeller, Paturi and Schneider (CSR 2016, Algorithmica 2019) asked whether
succinct preferences make stable matching easier, and they named this two-list case as
the natural starting point. To our knowledge, no `o(n^2)` algorithm for it was known
before.

- **Paper:** [paper/twolist-subquadratic.pdf](paper/twolist-subquadratic.pdf), by an anonymous author.
- **Summary and audit scope:** [docs/subquadratic.md](docs/subquadratic.md).

**How it works.** The four type groups (X-men, X-women, Y-men, Y-women) form a cycle of
acceptable pairs. The algorithm unrolls this cycle into a path of `4n + 2` copies of the
instance. On a path, mutual first choices always exist, so repeated elimination gives a
unique stable matching. The middle copy projects to a stable matching of the original
instance: the one with the fewest pairs between X-men and Y-women. A Four-Russians
packing batches the eliminations and saves a `sqrt(log n / log log n)` factor.

**Verification status (2026-10-09).** The structural theorem (Theorem 4.11) and the
minimum-seam characterization (Proposition A.3) were checked independently: a proof
review, plus exhaustive and random finite checks with separate code. The packed
implementation and its running-time analysis (Section 5) were not independently
audited. `verify/cover_check.py` runs the simple O(n^2) elimination on the `4n + 2`
copies and checks its output.

## Also in this repository

Supporting results from research done with AI agents (Claude and GPT models):

- **D1/D2, proved and independently audited (P, A+):** each rotation changes the
  number of X-man/X-woman pairs by one. There are at most
  `min(|MX|, |MY|, |WX|, |WY|) <= floor(n/2)` rotations, so at most `2^floor(n/2)`
  stable matchings. The bound is tight. See the [proof](docs/rotations.md).
- **F4, C4, C6, proved and audited (P, A+ / A~):** stable matchings have greedy
  structure and monotone label boundaries. The partner types of one woman type admit
  at most one stable completion.
- **C9/C11, proved and audited with stated costs (P, A~ / A+):** compressed fixed-point
  methods solve classes with few type runs or list breakpoints in near-linear time.
- **B11/B12/B14, proved and audited with model limits (P, A+ / A~):** explicit families
  force quadratic work for specified proposal and unit-charge models. These bounds do
  not apply to every algorithm, and the subquadratic algorithm above is outside them.

See the [result summary and status key](docs/summary.md) and the
[failed routes](docs/dead-routes.md).

![Stable label boundaries](docs/figures/staircases.png)

## Build and test

Requirements: Linux, GNU make, a C++17 compiler, and Python 3 (standard library).
No package installation is needed for the solvers or tests.

```sh
make
make test
build/reference solve instances/twin.txt build/matching.txt
build/reference check instances/twin.txt build/matching.txt
build/best instances/weave.txt build/matching.txt
python3 generators/generate.py ltwin 25 build/ltwin.txt
```

`make test` uses fixed seeds, brute force on small inputs, independent rotation
checks, label checks, and solver comparisons. It is intended to finish within two
minutes on a usual Linux development machine. `make test-full-cover` runs the
original 3,000-instance cover experiment separately. The optional figure script
needs matplotlib: `python3 docs/figures/staircases.py`.

## Files and formats

- `src/reference/`: instance parser, reference proposal solver, stability checks,
  random generation, and brute force.
- `src/portfolio/`: structured methods and the portfolio driver `best`.
- `src/rbgs/`: experimental run-buffer solver; prototype costs differ from theorem costs.
- `generators/`: named families, boundary weave, and decoy/glue transformations.
- `verify/`: independent standard-library checks and a simple cover implementation.
- `instances/`: six small generated examples; [format and commands](docs/code.md).
- `docs/`: result scope, rotation proof, and figure.
- `paper/`: anonymous external draft.

## Credit and license

The code and our documentation use the [MIT license](LICENSE).
The external draft `paper/twolist-subquadratic.pdf` retains its anonymous author's
rights and is **excluded** from that license.

The subquadratic algorithm and its paper are credited to an anonymous author.
Independent notes by an anonymous researcher overlap the greedy-label result F4,
the failures of label closure, and the refutation of F7. No text or code from
those notes is included.
