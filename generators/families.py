# Instance families for family construction. Build from named agents and orders.
from sm import make, X, Y


def build(men, women, aord, bord, cord, dord, mtype, wtype):
    mi = {m: i for i, m in enumerate(men)}; wi = {w: i for i, w in enumerate(women)}
    n = len(men); assert len(women) == n
    a = [0] * n; b = [0] * n; c = [0] * n; d = [0] * n
    for r, m in enumerate(aord): a[mi[m]] = r
    for r, m in enumerate(bord): b[mi[m]] = r
    for r, w in enumerate(cord): c[wi[w]] = r
    for r, w in enumerate(dord): d[wi[w]] = r
    I = make(a, b, [mtype[m] for m in men], c, d, [wtype[w] for w in women])
    return I, mi, wi


def chain(k):
    """Alternating displacement chain: y_-1 -> p_0 -> x_0 -> q_0 -> y_0 -> p_1 -> x_1 -> ..."""
    xs = [('x', i) for i in range(k + 1)]
    ys = [('y', i) for i in range(-1, k + 1)]
    ps = [('p', i) for i in range(k + 1)]
    qs = [('q', i) for i in range(k + 1)]
    z = ('z', 0)
    men = xs + ys; women = ps + qs + [z]
    aord = ys + xs[::-1]
    bord = xs + ys[::-1]
    cord = ps[::-1] + qs + [z]
    dord = qs[::-1] + ps + [z]
    mtype = {m: (X if m[0] == 'x' else Y) for m in men}
    wtype = {w: (X if w[0] in 'pz' else Y) for w in women}
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def crossed(k, e=1):
    """k X-men x_i, k+e Y-men y_i, k X-women u_i, k+e Y-women v_i.
    a: x.. then y..; b: y.. then x..; c: v_1..v_k, u.., v_{k+1..}; d: u.., v...
    X-men want Y-women first (who want Y-men); Y-men want X-women first (who want X-men)."""
    xs = [('x', i) for i in range(k)]; ys = [('y', i) for i in range(k + e)]
    us = [('u', i) for i in range(k)]; vs = [('v', i) for i in range(k + e)]
    men = xs + ys; women = us + vs
    aord = xs + ys; bord = ys + xs
    cord = vs[:k] + us + vs[k:]; dord = us + vs
    mtype = {m: (X if m[0] == 'x' else Y) for m in men}
    wtype = {w: (X if w[0] == 'u' else Y) for w in women}
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def ring2(k, R):
    """Two-region ring with interleaved priorities (forces many cross-type switches).
    Men: X-men X_1..X_k, Y-men Y_1..Y_k and ys (a-worst Y). Women: U (X-type, u_0..u_{k-1}),
    V (Y-type, v_0..v_{k-1}), tail t. X-men: c = V, U, t. Y-men: d = U, V, t.
    Needs k >= 2R + 1. Forces 2R^2 cross-type switches in every GS order (see notes)."""
    assert k >= 2 * R + 1 and R >= 1
    Xm = {i: ('X', i) for i in range(1, k + 1)}
    Ym = {i: ('Y', i) for i in range(1, k + 1)}
    ys = ('Y', 0)
    Z = {i: Ym[k - i + 1] for i in range(1, R + 1)}  # i-th Y-man to enter V
    aord = []
    for j in range(1, R + 1):           # Y_1, X_R, Y_2, X_{R-1}, ..., Y_R, X_1
        aord += [Ym[j], Xm[R - j + 1]]
    aord += [Ym[j] for j in range(R + 1, k + 1)]
    aord += [Xm[i] for i in range(R + 1, k + 1)]
    aord += [ys]
    bord = []
    for j in range(R):                  # X_k, Z_R, X_{k-1}, Z_{R-1}, ..., X_{k-R+1}, Z_1
        bord += [Xm[k - j], Z[R - j]]
    bord += [ys]
    bord += [Xm[i] for i in range(k - R, 0, -1)]   # X_{k-R}, ..., X_1
    bord += [Ym[j] for j in range(1, k - R + 1)]
    men = [Xm[i] for i in range(1, k + 1)] + [Ym[i] for i in range(1, k + 1)] + [ys]
    assert sorted(aord) == sorted(men) and sorted(bord) == sorted(men)
    U = [('u', i) for i in range(k)]; V = [('v', i) for i in range(k)]; t = ('t', 0)
    women = U + V + [t]
    cord = V + U + [t]; dord = U + V + [t]
    mtype = {m: (X if m[0] == 'X' else Y) for m in men}
    wtype = {w: (X if w[0] in 'ut' else Y) for w in women}
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def gring(k, g):
    """Three-region ring: V (Y-women, prio b) -> G (alternating woman types, entered only by
    X-men; a = b on X-men, so G is a sorted buffer) -> U (X-women, prio a) -> V.
    c-order: V, G, U, t. d-order: U, V, t, G. X-men: P_1..P_{k-1}, Q_1..Q_g, xs (k+g);
    Y-men: Y_1..Y_k, ys (k+1). Each round puts a new X-man at the top of G and shifts all g
    G-men by one woman of the other type (men-side switches)."""
    P = [('P', i) for i in range(1, k)]; Q = [('Q', i) for i in range(1, g + 1)]; xs = ('x', 0)
    Ym = [('Y', i) for i in range(1, k + 1)]; ys = ('y', 0)
    men = P + Q + [xs] + Ym + [ys]
    aord = P + Q + [xs] + Ym + [ys]
    bord = Ym + [ys] + P + Q + [xs]
    U = [('u', i) for i in range(k)]; V = [('v', i) for i in range(k)]
    G = [('g', i) for i in range(g)]; t = ('t', 0)
    women = U + V + G + [t]
    cord = V + G + U + [t]; dord = U + V + [t] + G
    mtype = {m: (X if m[0] in 'PQx' else Y) for m in men}
    wtype = {w: (X if w[0] in 'ut' else Y) for w in U + V + [t]}
    for i, w in enumerate(G):
        wtype[w] = X if i % 2 == 0 else Y
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def ring3(k, R, g):
    """ring2's U/V interleaving (women-side switches) + a G delay line between V and U on the
    X-route (men-side switches). a = b on X-men. Needs k >= 2R+1, g >= R.
    c-order: V, G, U, t. d-order: U, V, t, G."""
    assert k >= 2 * R + 1 and g >= R and R >= 1
    Xm = {i: ('X', i) for i in range(1, k + 1)}; Q = {i: ('Q', i) for i in range(1, g + 1)}
    Ym = {i: ('Y', i) for i in range(1, k + 1)}; ys = ('Y', 0)
    E = {1: Xm[1]}
    for j in range(2, R + 1):
        E[j] = Q[g - j + 2]
    Z = {j: Ym[k - j + 1] for j in range(1, R + 1)}
    aord = [Xm[i] for i in range(k, 1, -1)] + [Q[i] for i in range(1, g - R + 2)]
    for j in range(1, R + 1):
        aord += [Ym[j], E[R - j + 1]]
    aord += [Ym[j] for j in range(R + 1, k + 1)] + [ys]
    bord = []
    for j in range(R):
        bord += [Xm[k - j], Z[R - j]]
    bord += [ys] + [Xm[i] for i in range(k - R, 1, -1)] + [Q[i] for i in range(1, g + 1)] + [Xm[1]]
    bord += [Ym[j] for j in range(1, k - R + 1)]
    men = [Xm[i] for i in range(1, k + 1)] + [Q[i] for i in range(1, g + 1)] + [Ym[i] for i in range(1, k + 1)] + [ys]
    assert sorted(aord) == sorted(men) and sorted(bord) == sorted(men), (len(aord), len(bord), len(men))
    U = [('u', i) for i in range(k)]; V = [('v', i) for i in range(k)]
    G = [('g', i) for i in range(g)]; t = ('t', 0)
    women = U + V + G + [t]
    cord = V + G + U + [t]; dord = U + V + [t] + G
    mtype = {m: (X if m[0] in 'XQ' else Y) for m in men}
    wtype = {w: (X if w[0] in 'ut' else Y) for w in U + V + [t]}
    for i, w in enumerate(G):
        wtype[w] = X if i % 2 == 0 else Y
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def transpose(I):
    """Swap the sides: men become women (a->c, b->d, types kept). Men-proposing GS on the
    transpose = women-proposing GS on I."""
    return make(I['c'], I['d'], I['tw'], I['a'], I['b'], I['tm'])


def union(I1, I2):
    """Block union: every list ranks all block-1 people before all block-2 people.
    Stable matchings = (stable matching of I1) + (stable matching of I2)."""
    n1, n2 = I1['n'], I2['n']
    a = I1['a'] + [n1 + r for r in I2['a']]; b = I1['b'] + [n1 + r for r in I2['b']]
    c = I1['c'] + [n1 + r for r in I2['c']]; d = I1['d'] + [n1 + r for r in I2['d']]
    return make(a, b, I1['tm'] + I2['tm'], c, d, I1['tw'] + I2['tw'])


def both_ways(F):
    """F (+) transpose(F): costly for men-proposing (block 1) and women-proposing (block 2)."""
    return union(F, transpose(F))


def swapXY(I):
    """Rename X <-> Y everywhere (a <-> b, c <-> d, all types flipped). Same instance up to names."""
    return make(I['b'], I['a'], [1 - t for t in I['tm']], I['d'], I['c'], [1 - t for t in I['tw']])


def union_all(blocks):
    J = blocks[0]
    for B in blocks[1:]:
        J = union(J, B)
    return J


def gauntlet(R, kc=None):
    """Block union ring3 (+) transpose(ring3) (+) chain: forced-hard for men- and women-proposing GS
    (ring3 blocks), slow for C3 from both ends and for C8 from both starts (chain block covers the
    start that ring3 does not), and many type runs in all four groups (C9's D grows with R)."""
    F = ring3(2 * R + 1, R, R)[0]
    C = chain(kc if kc is not None else 2 * R + 1)[0]
    return union_all([F, transpose(F), C])


def scramble_women(I, rng, frac=1.0):
    """Randomize the internal order of every maximal block of consecutive same-type women... no:
    of each women REGION, given as the maximal runs of women that are contiguous as a set in both
    c and d. Simple version used here: for each list (c, d) separately, shuffle the women inside
    each maximal run of equal type (a region of one woman type), with probability frac per run.
    The sets of positions of each run are kept, so region-level structure (and the ring lemma's
    counting) is kept; only internal orders change, independently in c and d."""
    n = I['n']; J = {k: (list(v) if isinstance(v, list) else v) for k, v in I.items()}
    for key in ('c', 'd'):
        order = sorted(range(n), key=lambda w: I[key][w])
        i = 0
        while i < n:
            j = i
            while j + 1 < n and I['tw'][order[j + 1]] == I['tw'][order[i]]:
                j += 1
            if rng.random() < frac:
                seg = order[i:j + 1]; rng.shuffle(seg); order[i:j + 1] = seg
            i = j + 1
        for r, w in enumerate(order):
            J[key][w] = r
    return J


def ring3c(k, R, g):
    """ring3 inside structured solver's chain-men subclass C10 (a = b inside each man type): as ring3, but
    Y_1..Y_{k-R} are moved to the TOP of the b-order, so the Y-men are a chain in a and b."""
    I, mi, wi = ring3(k, R, g)
    n = I['n']
    bord = sorted(range(n), key=lambda m: I['b'][m])
    top = [mi[('Y', j)] for j in range(1, k - R + 1)]
    rest = [m for m in bord if m not in set(top)]
    b = [0] * n
    for r, m in enumerate(top + rest): b[m] = r
    J = dict(I); J['b'] = b
    return J, mi, wi


def twin(k):
    """The 'twin' family (B11), n = 2k, k even: crossed on both sides at once,
    glued by an X-man g and an X-woman h."""
    assert k % 2 == 0 and k >= 2
    x = [('x', i) for i in range(1, k + 1)]; y = [('y', i) for i in range(1, k)]; g = ('g', 0)
    p = [('p', i) for i in range(1, k + 1)]; q = [('q', i) for i in range(1, k)]; h = ('h', 0)
    men = x + [g] + y; women = p + [h] + q
    h2 = k // 2
    cord = q + p[:h2] + [h] + p[h2:]
    dord = p + q[:k - 2] + [h] + q[k - 2:]
    aord = x + y + [g]
    bord = y + x[:h2] + [g] + x[h2:]
    mtype = {m: (X if m[0] in 'xg' else Y) for m in men}
    wtype = {w: (X if w[0] in 'ph' else Y) for w in women}
    return build(men, women, aord, bord, cord, dord, mtype, wtype)


def scramble_sets(I, groups, rng, keys=('c', 'd')):
    """For each list in keys, shuffle the internal order of each given group of people (a set of
    ids of that side), keeping the set of positions the group occupies in that list."""
    n = I['n']; J = {k: (list(v) if isinstance(v, list) else v) for k, v in I.items()}
    for key in keys:
        for grp in groups:
            pos = sorted(J[key][x] for x in grp)
            ids = list(grp); rng.shuffle(ids)
            for p, x in zip(pos, ids): J[key][x] = p
    return J
