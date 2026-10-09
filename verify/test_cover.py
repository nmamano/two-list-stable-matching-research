"""Repeat the original finite cover experiment with a failing exit on mismatch."""
import itertools
import random
from cover_check import cover_solve, rand_inst, stable, seam
rng = random.Random(20261009)
multi = 0
for trial in range(3000):
    n = rng.randint(1,7)
    pi, sigma, mt, wt = rand_inst(n,rng)
    wife = cover_solve(n,pi,sigma,mt,wt)
    matches = [list(p) for p in itertools.permutations(range(n)) if stable(n,pi,sigma,mt,wt,list(p))]
    qmin = min(seam(mt,wt,p) for p in matches)
    minima = [p for p in matches if seam(mt,wt,p)==qmin]
    assert len(minima)==1 and wife==minima[0], (trial,n)
    multi += len(matches)>1
print(f'PASS: 3000 cover checks, n<=7, {multi} with multiple stable matchings')
