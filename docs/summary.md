# Research summary

Snapshot of results and independent review on 2026-10-05. The later anonymous
[external draft](subquadratic.md) is separate. No new proof claim is made here.

**P**: proved in the research notes. **S**: proof sketch or argument.
**T**: finite test or measurement (2026-10-05 unless another date is given).
**C**: conjecture. **A+**: independently confirmed. **A~**: confirmed only with
stated scope. **A-**: refuted. **A0**: not independently audited.
A proved result need not have its complete proof reproduced in this repository;
the full D1/D2 proof is included. Test samples are not proofs.

## Model

There are n men and n women. Each man has ranks a and b; each woman has ranks c
and d. Every rank array is a permutation of 0 through n-1. Type-X men use c,
type-Y men use d; type-X women use a, type-Y women use b. MOSM and WOSM denote
the man-optimal and woman-optimal stable matchings. A woman's label is her
partner's type. A counts accepted proposals, not all attempted proposals.

## Structure

| ID | Result | Proof and audit status |
|---|---|---|
| F1 | Stability verification in O(n log n). | P, A+ |
| F2 | Proposal solver with bulk skipping in O((n+A) log n). | P, A+; original tree code not audited |
| F3 | Each of the four type blocks is order-preserving. | P, A+ |
| F4 | Labels of all women fix a stable matching: each type executes its greedy sweep. | P, A+ |
| F5 | Held pairs of one man type form the greedy matching of the currently held pool. | P, A~; this pool definition is required |
| F6 | Stability equals choosing the best available man across the two suffix pools. | P, A+ |
| F7 | The former top-four closure claim is false. Soulmates must be matched; a man gets at least the woman who ranks him first. | Old claim A-; retained statements P, A+ |
| F8 | There can be 2^(n/2) stable matchings for even n. | P, A+ |
| C1 | Remaining men in a per-type greedy sweep form a quadrant in (a,b). | P, A+ |
| C2 | Stable matchings are exactly consistent labelings. | P, A+ |
| C3 | The sweep map is monotone on pairs of nested remaining-set paths; endpoint iteration reaches MOSM/WOSM. | P, A~; domain as stated; round bounds are only tested |
| C4 | Within each woman type, X-labels are up-left closed in the (c,d) plane. | P, A+; the claimed C3 height bound of n is refuted, A- |
| C5 | Reversed or disjoint-top lists on either side, or a shared master list: O(n log n). | P, A+ |
| C6 | Labels of one woman type determine at most one stable completion, constructed in O(n). | P, A~; arbitrary input labels require the final F6 check |
| C7 | Forced propagation stalls only at mixed-type favorite squares; the four first-stall label choices occur in four different size-3 unique-matching instances. | P, A+ |
| C12 | Forced propagation equals repeated soulmate removal, O(n). | P, A+ |
| P1 | A joint prefix cut splits independent subproblems; cuts are found in O(n). | P, A+ |
| C13 | A one-sided prefix cut also permits local composition. | P, A~; no global recursion cost bound |
| D-D1 | Every rotation changes the XX pair count by +1 or -1. | P, A+ |
| D2 / D-COUNT | Intersecting c- or d-hulls have the same sign; no person moves under both signs. Thus the rotation count is at most each of the four endpoint-label-change counts, hence at most min of the four type sizes. | P, A+; full proof in [rotations.md](rotations.md); full SEP partly open and not required |
| D-PL | A nonplanar rotation Hasse diagram exists, with a subdivision of K3,3 at n=28. | P, A+; old planarity claim A- |
| As-maj | Woman-label vectors need not be majority-closed (n=6, indecomposable); no exact existential 2-CNF plus one cardinality constraint (n=12). | P, A+ |
| B4-cert | MOSM certification by an acyclic women-side successor graph takes O(n log n). | P, A+ |

## Algorithms

| ID | Bound and scope | Proof and audit status |
|---|---|---|
| C9 | With D=r_a+r_b-1 type runs for one group, nested binary search uses at most sum(j=1..D) ceil(log2(n+3))^j evaluations, each O(n+D): O(n log^D n) for fixed D. Choose the smallest dimension across four groups. | P, A~; linear sweep correction applied and source-audited |
| C11 / A-D3 | For k common-order blocks and a feedback arc set, D is twice the number of cut edges; evaluations take O(n log n). Exact setup O(k 2^k), implemented through k=20, heuristic above. | P, A+ with setup cost |
| RB1 | A pure module is a sorted buffer; final assignment is a serial dictatorship. | P, A+ |
| RB3 | Run-buffer GS has claimed time O((n+Ch) log n) per order. | P; full runtime A0; component RB1 confirmed and RB4 scoped |
| RB4 | Each (man,unit) pair is accepted at most once; unit acceptances <= nK. | P, A~; conversion to charges requires unit-cost charges |
| RB11 | Dead-for-type deletion is sound with native holders and units within a joint-prefix block. | P, A~; prototype runtime not audited |
| U1 | Budgeted bulk sweeps give model work O(nP), where P is type-batched phase depth. | P, A~; prototype runtime excluded |
| U2 | Phase-aligned units reach MOSM in at most P phases, with capped charge bound <= P. Runtime O(n(1+L_R) log n) needs O(log n) charges and amortized O(n log n) initialization, materialization, deadness checks, rebuilds, and bulk closures. | Phase/MOSM/capped charges P, A+; runtime conditional; efficient builder A0 |
| RB13 | MOSM-derived per-type deadness is sound. This is an oracle diagnostic. | P, A~; finite charge minima tested, A~; no all-rule bound |
| best.cpp | Reduction, prefix cuts, smallest-dimension C9/C11, then a budgeted portfolio of run-buffer methods, C3, C8, and a plain proposal fallback. | T, A0 as a whole; no new worst-case bound |

## Lower bounds and barriers

| ID | Bound and exact model | Proof and audit status |
|---|---|---|
| B2 | crossed(k), n=2k+1: every men-proposing order accepts at least (n^2+1)/2 proposals, tight. | P, A+ |
| B3 | ring2(k,(k-1)/2): every order needs at least (n-3)^2/8 women-side type switches, tight. | P, A+ |
| B6 | gring(k,k), even k: at least k^2+k men-side switches. ring3 has quadratic switches on both sides. | P plus counts, A~; parity limits required |
| B7 | F followed by its transpose forces F's cost in opposite proposing directions in the two blocks. | P, A~; replacement projection proof |
| B11 | twin(k), even k>=4, n=2k: exact minimum accepted proposals MP n^2/4-3n/4+1; WP n^2/4+n/4+1. | P, A+; C9 solves it with D=1 |
| B12 | ltwin(K), odd K>=3, n=4K+2, H=(K+1)/2: exact minima MP (H-1)(n-1)+K+1; WP H(n-1)+2K+2. No prefix cut or soulmates. Units <=5, with 5 only at K=5,9. | P, A~; unit-size correction |
| B13 | ltwin3(K), odd K>=5, n=4K+3: forcing bounds MP (n-1)+(H-2)(n-3)+K+1; WP (n-1)+(H-1)(n-4)+2H+2. | P; forcing dependency and B14 suffix counts audited; tight schedules and method table A0 |
| B14 | Unit-charged GS with pure modules/common runs in filtered lists, block-aware absolute deletion, arbitrary repartition, fixed charge per entry, and no bulk operations: MP >= (H-2)(n-3)/6, WP >= (H-1)(n-4)/6 on ltwin3. | P, A~; excludes relative validity skips, bulk sweeps, and oracles |
| B15 | gtwinr forcing holds for odd K>=13; costs measured only at K=13,25,49,99. | Forcing P, A~; costs T; no asymptotic bound |
| B4 | Under NSETH, fast verification blocks specified deterministic fine-grained SAT reductions for any stable matching or either optimum. | S, A~; total-search oracle reductions correct for every valid answer and with a query budget permitting verifier simulation; excludes randomized reductions and other hypotheses |

## Measurements and open scope

All measurements in this section are from **2026-10-05**.

The partial boundary weave has finite slow cases for the implemented methods.
At n=128 and 256, independent checks confirmed C3 counts 231 and 884 and C8
counts 23 and 44 (T, A~). At n=1024, an independent spot check reproduced C8=165
and a minimum of 41,480 static-unit charges over its specified 40 runs
(T, A~). This does not audit the full portfolio minimum. Iterations, unit
charges, and implementation work counters are different cost measures; their
minimum is not a measured runtime. Frontier/oracle equality on tested instances
does not imply an all-rule, all-partition, or asymptotic lower bound.

B17's later finite forcing-certificate argument is **P, A0** for its written
lemmas and checked finite sizes; extension to all sizes is **C, A0**. It is not
an audited general lower bound and is not used by this repository's headlines.

C14's continuum mean-field convergence theorem is **P, A+**. The finite random
instance O(log n) stopping theorem remains open. D4's quadratic total rotation
length is finite evidence (**T**) and an asymptotic conjecture (**C, A0**), not a
proved lower bound for lattice listing.

The shape of possible rotation posets, full SEP, and bounds beyond the stated
proposal models remain open in this research. The anonymous external draft
claims a general subquadratic algorithm. Its structural projection theorem
(Theorem 4.11, including the end bound) and unique minimum-seam characterization
(Proposition A.3) are independently confirmed (P, A+, 2026-10-09). Section 5's
packed implementation and running-time claim remain unaudited (A0).
