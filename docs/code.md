# Code and instance formats

All commands below run from the repository root. All IDs are zero-based. Rank
zero is best. The code assumes strict, complete lists and positive n.
The portfolio and run-buffer tools are research prototypes for valid inputs;
the reference parser validates keyword/order inputs.

## Keyword/order format

The example files, reference tool, and portfolio driver use seven fields:

```
n 2
men_x_ranking 0 1
men_y_ranking 1 0
women_x_ranking 0 1
women_y_ranking 1 0
men_preferences XY
women_preferences YX
```

The four ranking fields are **orders**: person IDs, best first. Each preference
string has one X or Y per person ID. A matching is n whitespace-separated woman
IDs, one for each man in ID order.

```sh
build/reference gen 20 123 build/random.txt
build/reference solve build/random.txt build/matching.txt
build/reference check build/random.txt build/matching.txt
```

Brute-force enumeration is limited to n<=9. Generate `twin 4` (n=8) to enumerate:

```sh
python3 generators/generate.py twin 4 build/tiny.txt
build/reference enum build/tiny.txt
build/best instances/weave.txt build/matching.txt 10 1 1
build/rbgs evallab instances/weave.txt
```

The optional portfolio arguments are maximum dimension (default 10), soulmate
reduction (1), and one-sided cuts (1). The run-buffer command reports unit
charges and checks its output against its reference optimum. Its counters are
not elapsed time. Internal module names preserve stable include paths.

## Reproduce the six checked-in instances

```sh
python3 generators/generate.py twin 8 instances/twin.txt
python3 generators/generate.py ltwin 5 instances/ltwin.txt
python3 generators/generate.py ltwin3 5 instances/ltwin3.txt
python3 generators/generate.py gtwin 13 instances/gtwin.txt
python3 generators/generate.py weave 32 instances/weave.txt
python3 generators/generate.py ring2 7 instances/ring2.txt
```

The parameter is k (or K), not n. Twin and weave use n=2k; laced twin uses
n=4K+2; ltwin3 uses n=4K+3; ring2 uses n=2k+1. The gtwin example uses J=K and
s=t=u=floor(K/2), with n=3K+2; it is a representative mixed twin, not a claim
that all gtwinr forcing side conditions hold for these parameters.
The weave swaps alternating pairs at type boundaries in all four lists of a
twin, with width n/16 rounded down. `make test` verifies byte-for-byte generation.

## Rank-array helpers

`hide.py` and `glue_many.py` use the rank-array format: first n, then six lines
`a b c d tm tw`, one array per line. Types are 0=X and 1=Y. These four arrays
are **ranks**, not best-first orders. `families.py`, `families2.py`, and `sm.py`
provide Python functions with this representation.

```sh
python3 generators/glue_many.py 4 2 123 build/glued.rank
python3 generators/hide.py build/glued.rank build/glued.rank 2 123 build/hidden.rank
build/rbgs build/glued.rank
```

The hide transformation adds decoys on both sides and joins a prefix instance.
The glue transformation swaps entries near existing block boundaries.

## Validation scope

The independent Python checker tests F3/F4/F6/C4 and C6's uniqueness implication
by brute force. D1/D2 checks enumerate exposed rotations and all reachable
stable states, compare with brute force, and check the proof's witness and hull
lemmas. The test driver compares compiled solver outputs with independent
blocking-pair checks. These checks are a small reproducible regression suite,
not a rerun of every archived audit and not a new independent audit of the
whole portfolio or its runtime.
