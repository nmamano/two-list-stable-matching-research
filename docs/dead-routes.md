# Failed routes and scope limits

Results from the route ledger, reconciled with independent review on 2026-10-05.
Status labels are defined in [summary.md](summary.md). These results rule out
specific methods or models. They do not rule out all fast algorithms.

| Route | Obstruction | Status |
|---|---|---|
| Single-proposal GS with bulk skipping and an improved next-proposer rule | B2 crossed forces quadratic accepted proposals for every order. | P, A+ |
| Pay for every women-side type switch | B3 ring2 forces quadratic switches. | P, A+ |
| Pay for every men-side switch, or either side | B6 gring/ring3. | P plus counts, A~ with parity limits |
| Change proposing direction | B7 direct sums, or indecomposable B11 twin. | P, A~ / A+ |
| Use one method on the whole instance | B8 gauntlet. Prefix splitting escapes this obstruction. | GS P, A+; other method counts T, larger counts A0 |
| Earlier portfolio with only joint prefix cuts | B9 glued gauntlet; one-sided cuts C13 later escaped it. | T, A0 |
| Iterate the monotone sweep map C3 from the top | B5 ring2: finite large round counts. | T; A+ only at n=7,11,15,19,31,43,63,103; no proved asymptotic bound |
| Alternate label completions C8 | Twin and laced twin take many rounds in tests. | T, A~; convergence itself unproved |
| Alternate proposal phases | B1 chain needs n+1 phases for that schedule. | P, A~ |
| Compress units with absolute pruning | B13/B14 laced twin with keepers. | P, A~; explicit unit-charge model only; excludes relative skips and bulk operations |
| Encode labels by exact majority-closed constraints | As-maj counterexamples. | P, A+ |
| Use fixed random priorities on identical lists | Accepted proposals equal n plus inversions, quadratic in expectation. | P, A+ |
| Assume rotation diagrams are planar | D-PL nonplanar witness. | Planarity A-; witness P, A+ |

The boundary weave remains a tested hard candidate for the included prototypes.
Finite charge counts do not establish hardness against every pruning rule or
unit partition. The [anonymous draft](subquadratic.md) uses an opened cover and
batching, outside the audited B14 model.
