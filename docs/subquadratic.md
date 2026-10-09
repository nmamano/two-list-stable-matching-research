# Anonymous subquadratic draft

An anonymous author supplied *A Subquadratic Algorithm for Two-List Stable
Matching*. The [PDF](../paper/twolist-subquadratic.pdf) is external work and is
excluded from the repository's MIT license.

**Status: external theorem claim; full proof and runtime not independently
audited here (A0).** The draft claims deterministic
O(n^2 sqrt(log log n / log n)) time on a word RAM.

The draft unfolds the cycle of four type groups into a path of 4n+2 sheets.
Repeated mutual-first-choice elimination produces a matching on that path.
The draft claims that the middle sheet projects to the unique stable matching
with the fewest pairs between X-men and Y-women. A stationary slice, end bounds,
and a splicing argument support this projection. Packed lookup tables batch
eliminations to obtain the claimed running time.

`verify/cover_check.py` implements simple elimination in O(nR) time for R sheets.
It does not implement the packed batching and cannot validate the running-time
claim. On 2026-10-09, this check found a stable, uniquely minimum-seam matching
on 3,000 seeded random instances with n<=7, including 178 with multiple stable
matchings (**T; implementation check, A0**). The original check also found stable
outputs on several named families. Section 4's end bound and the packed
implementation remain unchecked. Run `make test-full-cover` to repeat the
3,000-instance experiment; `make test` runs a small subset.

Our rotation, label, and restricted proposal-model results are separate from
this draft. D2 gives a linear rotation-count bound; that alone does not give a
subquadratic algorithm. B14 concerns a limited unit-charge model and gives no
contradiction to the draft's batching method.

Independent notes by an anonymous researcher overlap F4, the failures of label
closure, and the refutation of F7. No text or code from that notes package is
included. Neither those notes nor the draft is work by this repository's team.
