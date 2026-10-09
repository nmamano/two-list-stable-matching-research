# Red-team construction (2026-10-05): break the pure modules of a base instance F on BOTH sides with
# decoy pairs, and hide the decoys' inertness from frontier rules behind a cycles prefix block + glue.
# add_decoys(F, T): for each woman t in T insert a decoy woman z (type Y) right after t in c-order;
#   all decoy women come first in d-order (reversed); decoy men g (type Y) are b-top and a-bottom.
# Both sides: F2 = transpose(add_decoys(transpose(add_decoys(F, Tw)), Tm)).
# Then G = H (+) F2 (H = cycles block from file), glued by r swaps near the cuts.
# Usage: hide.py BASE CYCFILE r seed OUT [stride]   (BASE, CYCFILE in rank-array format)
import sys, random
import families
from sm import make
def load(p):
    L = open(p).read().split('\n'); a, b, c, d, tm, tw = [list(map(int, L[i].split())) for i in range(1, 7)]
    return make(a, b, tm, c, d, tw)
def order(r): return sorted(range(len(r)), key=lambda i: r[i])
def ranks(o):
    r = [0] * len(o)
    for i, x in enumerate(o): r[x] = i
    return r
def modules(I):
    """women in pure modules of size >= 2 (greedy QM-style pure modules), in c-order"""
    n = I['n']; wc = order(I['c']); out = []
    i = 0
    while i < n:
        mn = mx = I['d'][wc[i]]; jm = i
        for j in range(i + 1, n):
            if I['tw'][wc[j]] != I['tw'][wc[i]]: break
            mn = min(mn, I['d'][wc[j]]); mx = max(mx, I['d'][wc[j]])
            if mx - mn == j - i: jm = j
        if jm > i: out += wc[i:jm]   # all but the last woman of the module get a decoy after them
        i = jm + 1
    return out
def add_decoys(I, T):
    n0 = I['n']; K = len(T); n = n0 + K
    Z = list(range(n0, n)); G = list(range(n0, n)); pos = {w: i for i, w in enumerate(T)}
    newc = []
    for w in order(I['c']):
        newc.append(w)
        if w in pos: newc.append(Z[pos[w]])
    dord = Z[::-1] + order(I['d'])
    aord = order(I['a']) + G; bord = G + order(I['b'])
    return make(ranks(aord), ranks(bord), I['tm'] + [1] * K, ranks(newc), ranks(dord), I['tw'] + [1] * K)
base, cyc, r, seed, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
stride = int(sys.argv[6]) if len(sys.argv) > 6 else 1
F = load(base)
F1 = add_decoys(F, modules(F)[::stride])
F1T = families.transpose(F1)
F2 = families.transpose(add_decoys(F1T, modules(F1T)[::stride]))
I = families.union(load(cyc), F2) if cyc != '-' else F2
n = I['n']; rng = random.Random(seed)
cuts = []; ma = order(I['a']); wc = order(I['c']); mb = mw = -1
for k in range(1, n):
    mb = max(mb, I['b'][ma[k - 1]]); mw = max(mw, I['d'][wc[k - 1]])
    if mb == k - 1 and mw == k - 1: cuts.append(k)
J = {k: (list(v) if isinstance(v, list) else v) for k, v in I.items()}
for _ in range(r):
    key = rng.choice('abcd'); cut = rng.choice(cuts)
    i = cut - rng.randint(1, 3); j = cut + rng.randint(0, 2)
    o = sorted(range(n), key=lambda p: J[key][p])
    x, y = o[i], o[j]; J[key][x], J[key][y] = J[key][y], J[key][x]
with open(out, 'w') as f:
    f.write(f'{n}\n')
    for k in ('a', 'b', 'c', 'd', 'tm', 'tw'): f.write(' '.join(map(str, J[k])) + '\n')
print('n', n, 'base', F['n'], 'after decoys', F2['n'], 'cuts', len(cuts))
