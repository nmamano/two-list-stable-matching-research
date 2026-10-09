"""Finite check of the 'opened cover' algorithm in paper/twolist-subquadratic.pdf (Theorem 4.11,
Prop. A.3). Naive O(nR) elimination, no Four-Russians. Standard library only. 2026-10-09.
Mapping: paper type 0 = X, type 1 = Y; sigma_0 = women X-list, sigma_1 = women Y-list,
pi_0 = men X-list, pi_1 = men Y-list. Seam pairs = (X-man, Y-woman)."""
import itertools, random, sys


def read_lab(path):
    d = {}
    for line in open(path):
        if line.startswith('#') or not line.strip():
            continue
        k, *v = line.split()
        d[k] = v
    n = int(d['n'][0])
    mx = list(map(int, d['men_x_ranking'])); my = list(map(int, d['men_y_ranking']))
    wx = list(map(int, d['women_x_ranking'])); wy = list(map(int, d['women_y_ranking']))
    mt = [0 if ch == 'X' else 1 for ch in d['men_preferences'][0]]
    wt = [0 if ch == 'X' else 1 for ch in d['women_preferences'][0]]
    return n, [mx, my], [wx, wy], mt, wt


def pos_of(perm):
    p = [0] * len(perm)
    for i, x in enumerate(perm):
        p[x] = i
    return p


def cover_solve(n, pi, sigma, mt, wt, R=None):
    """pi[i] = men list i (best first), sigma[j] = women list j. Returns wife[m] from sheet 2n+1."""
    if R is None:
        R = 4 * n + 2
    ppi = [pos_of(pi[0]), pos_of(pi[1])]
    psg = [pos_of(sigma[0]), pos_of(sigma[1])]
    # group kinds in path order per sheet: M0, W0, M1, W1
    kinds = ['M0', 'W0', 'M1', 'W1']
    # list used by each kind, and (left neighbour kind list, right neighbour kind list)
    own = {'M0': ('s', 0), 'W0': ('p', 0), 'M1': ('s', 1), 'W1': ('p', 1)}
    leftk = {'M0': 'W1', 'W0': 'M0', 'M1': 'W0', 'W1': 'M1'}
    rightk = {'M0': 'W0', 'W0': 'M1', 'M1': 'W1', 'W1': 'M0'}

    def position(kind_of_ranker, agent):
        side, i = own[kind_of_ranker]
        return psg[i][agent] if side == 's' else ppi[i][agent]

    K = 4 * R
    groups = []
    for a in range(R):
        for kd in kinds:
            if kd[0] == 'M':
                mem = [m for m in range(n) if mt[m] == int(kd[1])]
            else:
                mem = [w for w in range(n) if wt[w] == int(kd[1])]
            lpos = {x: position(leftk[kd], x) for x in mem}
            rpos = {x: position(rightk[kd], x) for x in mem}
            groups.append({'kind': kd, 'present': set(mem),
                           'lorder': sorted(mem, key=lambda x: lpos[x]),
                           'rorder': sorted(mem, key=lambda x: rpos[x]),
                           'lpos': lpos, 'rpos': rpos, 'li': 0, 'ri': 0})

    def lhead(k):
        g = groups[k]
        while g['li'] < len(g['lorder']) and g['lorder'][g['li']] not in g['present']:
            g['li'] += 1
        return g['lorder'][g['li']] if g['li'] < len(g['lorder']) else None

    def rhead(k):
        g = groups[k]
        while g['ri'] < len(g['rorder']) and g['rorder'][g['ri']] not in g['present']:
            g['ri'] += 1
        return g['rorder'][g['ri']] if g['ri'] < len(g['rorder']) else None

    def fav(k):
        best = None
        if k > 0:
            h = rhead(k - 1)
            if h is not None:
                best = (groups[k - 1]['rpos'][h], 'L', h)
        if k < K - 1:
            h = lhead(k + 1)
            if h is not None:
                c = (groups[k + 1]['lpos'][h], 'R', h)
                if best is None or c[0] < best[0]:
                    best = c
        return best

    match = {}  # (k, agent) -> (k', agent')
    work = list(range(K - 1))
    inq = [True] * (K - 1)
    while work:
        k = work.pop()
        inq[k] = False
        a = rhead(k); b = lhead(k + 1)
        if a is None or b is None:
            continue
        f1 = fav(k); f2 = fav(k + 1)
        if f1 and f2 and f1[1] == 'R' and f1[2] == b and f2[1] == 'L' and f2[2] == a:
            groups[k]['present'].discard(a); groups[k + 1]['present'].discard(b)
            match[(k, a)] = (k + 1, b); match[(k + 1, b)] = (k, a)
            for e in range(max(0, k - 2), min(K - 1, k + 3)):
                if not inq[e]:
                    inq[e] = True; work.append(e)
    z = 2 * n + 1 if R == 4 * n + 2 else R // 2
    wife = [None] * n
    for kd_i in (0, 2):  # M0^z, M1^z
        k = 4 * z + kd_i
        for m in range(n):
            if (k, m) in match:
                wife[m] = match[(k, m)][1]
    return wife


def stable(n, pi, sigma, mt, wt, wife):
    if None in wife or len(set(wife)) != n:
        return False
    husb = [0] * n
    for m, w in enumerate(wife):
        husb[w] = m
    ps = [pos_of(sigma[0]), pos_of(sigma[1])]; pp = [pos_of(pi[0]), pos_of(pi[1])]
    for m in range(n):
        for w in range(n):
            if ps[mt[m]][w] < ps[mt[m]][wife[m]] and pp[wt[w]][m] < pp[wt[w]][husb[w]]:
                return False
    return True


def seam(mt, wt, wife):
    return sum(1 for m, w in enumerate(wife) if mt[m] == 0 and wt[w] == 1)


def rand_inst(n, rng):
    return ([rng.sample(range(n), n) for _ in range(2)], [rng.sample(range(n), n) for _ in range(2)],
            [rng.randrange(2) for _ in range(n)], [rng.randrange(2) for _ in range(n)])


