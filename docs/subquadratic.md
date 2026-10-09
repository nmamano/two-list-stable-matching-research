# Anonymous subquadratic draft

An anonymous author supplied *A Subquadratic Algorithm for Two-List Stable
Matching*. The [PDF](../paper/twolist-subquadratic.pdf) is external work and is
excluded from the repository's MIT license.

**Status (2026-10-09): Theorem 4.11 and Proposition A.3 are proved and
independently confirmed (P, A+).** This covers the structural projection theorem
and unique minimum-seam characterization. Section 5's packed implementation
and running-time claim remain unaudited (A0). The draft claims deterministic
O(n^2 sqrt(log log n / log n)) time on a word RAM.

The draft unfolds the cycle of four type groups into a path of 4n+2 sheets.
Repeated mutual-first-choice elimination produces a matching on that path.
The audited structural theorem states that the middle sheet projects to the unique stable matching
with the fewest pairs between X-men and Y-women. A stationary slice, end bounds,
and a splicing argument support this projection. Packed lookup tables batch
eliminations to obtain the claimed running time.

`verify/cover_check.py` implements simple elimination in O(nR) time for R sheets.
It does not implement the packed batching and cannot validate the running-time
claim. On 2026-10-09, this check found a stable, uniquely minimum-seam matching
on 3,000 seeded random instances with n<=7, including 178 with multiple stable
matchings (**T; implementation check, A0**). The original check also found stable
outputs on several named families. This producer check is separate from the
independent audit below. Run `make test-full-cover` to repeat the
3,000-instance experiment; `make test` runs a small subset.

The independent review on 2026-10-09 checked the structural proof, including
Section 4's end bound, and Proposition A.3. A separate standard-library checker
used neither this repository's cover implementation nor other producer code.
It checked all 2,372 normalized instances through n=3 and 400 seeded random
instances with n=4..7: 2,947 stable matchings, including 175 inputs with multiple
stable matchings. Six larger saved inputs through n=128 also passed checks of
both proposing endpoints and direct stability. These are finite checks that
support the proof review (T, A+), not a proof from samples. Their saved-input
family labels were not re-audited. The independent check is not part of the
public test suite. No packed implementation or running-time audit was done.

Our rotation, label, and restricted proposal-model results are separate from
this draft. D2 gives a linear rotation-count bound; that alone does not give a
subquadratic algorithm. B14 concerns a limited unit-charge model and gives no
contradiction to the draft's batching method.

Independent notes by an anonymous researcher overlap F4, the failures of label
closure, and the refutation of F7. No text or code from that notes package is
included. Neither those notes nor the draft is work by this repository's team.
