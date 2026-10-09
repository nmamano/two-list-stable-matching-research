# Glue family construction's gauntlet(R) with r random swaps of list entries across block boundaries
# (positions within distance <= 3 of a boundary, random list). rank-array format. (2026-10-05)
import sys, random
import families
R, r, seed, out = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
I = families.gauntlet(R); n = I['n']; rng = random.Random(seed)
# block boundaries = prefix cuts of the unglued instance
cuts = []
ma = sorted(range(n), key=lambda m: I['a'][m]); wc = sorted(range(n), key=lambda w: I['c'][w])
mb = mw = -1
for k in range(1, n):
    mb = max(mb, I['b'][ma[k - 1]]); mw = max(mw, I['d'][wc[k - 1]])
    if mb == k - 1 and mw == k - 1: cuts.append(k)
J = {k: (list(v) if isinstance(v, list) else v) for k, v in I.items()}
for _ in range(r):
    key = rng.choice('abcd'); cut = rng.choice(cuts)
    i = cut - rng.randint(1, 3); j = cut + rng.randint(0, 2)
    order = sorted(range(n), key=lambda p: J[key][p])
    x, y = order[i], order[j]; J[key][x], J[key][y] = J[key][y], J[key][x]
with open(out, 'w') as f:
    f.write(f'{n}\n')
    for k in ('a', 'b', 'c', 'd', 'tm', 'tw'): f.write(' '.join(map(str, J[k])) + '\n')
print('n', n, 'cuts', cuts)
