"""Small reproducible checks. Uses independent Python matching code."""
from pathlib import Path
from itertools import permutations, product
import random
import subprocess
import sys
import tempfile
from check import Instance, ranks, structure
from rotations import load
from rotation_count import audit, C
from cover_check import cover_solve, read_lab, stable, seam

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'generators'))
from generate import generate, format_instance


def run(*args):
    return subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                          capture_output=True, text=True, timeout=30)


def labels(I, p, t):
    h = ranks(p)
    return tuple(I.mt[h[w]] for w in range(I.n) if I.wt[w] == t)


# Exhaustive normalized instances through n=3 exercise many small rotations.
for n in range(1,4):
    for b,d in product(permutations(range(n)), repeat=2):
        for mt,wt in product(product((0,1),repeat=n),repeat=2):
            audit(Instance(range(n),b,range(n),d,mt,wt), brute=True)

rng = random.Random(895413)
with tempfile.TemporaryDirectory(dir=ROOT/'build') as tmp:
    tmp = Path(tmp)
    run(ROOT/'build/reference', 'selftest')
    for name, k in [('twin',8),('ltwin',5),('ltwin3',5),('gtwin',13),('weave',32),('ring2',7)]:
        path = ROOT/'instances'/f'{name}.txt'
        assert path.read_text() == format_instance(generate(name, k)), name
        I = load(path)
        audit(I)
        run(ROOT/'build/reference', 'solve', path, tmp/'ref.txt')
        ref = tuple(map(int, (tmp/'ref.txt').read_text().split()))
        assert I.stable(ref) and ref == I.gs()[0]
        for maxd in (0, 10):
            run(ROOT/'build/best', path, tmp/'best.txt', maxd)
            p = tuple(map(int, (tmp/'best.txt').read_text().split()))
            assert sorted(p) == list(range(I.n)) and I.stable(p)
            run(ROOT/'build/reference', 'check', path, tmp/'best.txt')
        run(ROOT/'build/rbgs', 'evallab', path)
        n, pi, sigma, mt, wt = read_lab(path)
        assert stable(n, pi, sigma, mt, wt, cover_solve(n, pi, sigma, mt, wt))
    for n in range(1, 8):
        for trial in range(12):
            I = Instance(*(rng.sample(range(n), n) for _ in range(4)),
                         *([rng.randrange(2) for _ in range(n)] for _ in range(2)))
            stable_set = [p for p in permutations(range(n)) if I.stable(p)]
            assert stable_set
            for p in stable_set: assert structure(I, p)
            for t in (0,1): assert len({labels(I,p,t) for p in stable_set}) == len(stable_set), 'C6 uniqueness'
            audit(I, brute=True)
            data = dict(n=n, a=I.a, b=I.b, c=I.c, d=I.d, tm=I.mt, tw=I.wt)
            path = tmp/'random.txt'; path.write_text(format_instance(data))
            for maxd in (0,10):
                run(ROOT/'build/best', path, tmp/'best.txt', maxd)
                p = tuple(map(int,(tmp/'best.txt').read_text().split()))
                assert p in stable_set
            n, pi, sigma, mt, wt = read_lab(path)
            p = cover_solve(n, pi, sigma, mt, wt)
            minima = [q for q in stable_set if seam(mt,wt,q)==min(seam(mt,wt,r) for r in stable_set)]
            assert len(minima)==1 and tuple(p)==minima[0]
    (tmp/'invalid.txt').write_text('0 0\n')
    bad = subprocess.run([ROOT/'build/reference','check',ROOT/'instances/twin.txt',tmp/'invalid.txt'],capture_output=True)
    assert bad.returncode != 0
print('PASS: six families; 84 random brute-force/label/cover/solver checks; independent rotations:', dict(C))
