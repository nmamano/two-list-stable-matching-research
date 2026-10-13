# Track: runtime audit of the subquadratic draft

Goal: an independent audit of Section 5 of [../../paper/twolist-subquadratic.pdf](../../paper/twolist-subquadratic.pdf):
the packed (Four-Russians) implementation and the claimed O(n^2 sqrt(log log n / log n)) running time.

## Status

See [../../docs/subquadratic.md](../../docs/subquadratic.md). Theorem 4.11 and Proposition A.3 are
independently confirmed (P, A+). Section 5 is not audited (A0). [../../verify/cover_check.py](../../verify/cover_check.py)
implements only the simple O(nR) elimination on R sheets. It does not implement the packing and cannot
validate the running time.

## Useful contributions

- A line-by-line check of the Section 5 analysis: the word-RAM model, table sizes, preprocessing cost, and how
  the batched eliminations add up. Say for each step whether you checked it.
- A packed implementation whose output agrees with `verify/cover_check.py` and with `build/reference check`.
  Timings are evidence for the implementation, not a proof of the bound.
- An error in the analysis, with its exact location, and whether it can be repaired.
