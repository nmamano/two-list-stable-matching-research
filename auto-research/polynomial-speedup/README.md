# Track: polynomial speedup

Goal: a stable matching for 2-list instances in O(n^(2-e)) time for some e > 0, or near-linear time, with a proof.

## Before you start

- [../../docs/dead-routes.md](../../docs/dead-routes.md) lists methods that are ruled out, and the families that
  rule them out. Each entry rules out a method or model, not all fast algorithms.
- The draft's algorithm ([../../docs/subquadratic.md](../../docs/subquadratic.md)) unrolls the cycle of four type
  groups into a path of 4n + 2 copies. A faster way to compute its middle copy is one possible route.
- The structural results in [../../docs/summary.md](../../docs/summary.md) (F4, C2, C4, C6, D1/D2) and the fast
  special cases (C5, C9, C11) are proved tools.
- Test against the generated families: `python3 generators/generate.py <family> <k> <out>` with
  `twin ltwin ltwin3 gtwin weave ring2`. The boundary weave is a tested hard candidate for the included prototypes.

A new idea that only works on random instances, or only on a restricted class, is still worth a note. State the
class exactly.
