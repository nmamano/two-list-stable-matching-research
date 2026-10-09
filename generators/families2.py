# Laced ring families (family construction, 2026-10-05): ring regions whose women are interleaved in the OTHER
# man type's list with X-type G-women, so that no region is an interval of both c and d (no big
# RBGS unit). See docs/summary.md "Laced rings".
from sm import X, Y
from families import build


def laced_gring(k, ny=None):
    """gring(k, g) with G = k X-women G1 (laced into U's d-range), k X-women G2 (laced into V's d-range)
    and ny Y-women G3 (after t in d). g = 2k + ny. c-order: V, G, U, t (G in c: G1_i, G2_i, G3_i, ...).
    d-order: u_0, G1_{k-1}, u_1, G1_{k-2}, ..., v_0, G2_{k-1}, v_1, ..., t, G3 reversed.
    X-men: P_1..P_{k-1}, Q_1..Q_g, xs. Y-men: Y_1..Y_k, ys. a = P, Q, xs, Y, ys; b = Y, ys, P, Q, xs."""
    if ny is None: ny = k
    g = 2 * k + ny
    P = [('P', i) for i in range(1, k)]; Q = [('Q', i) for i in range(1, g + 1)]; xs = ('x', 0)
    Ym = [('Y', i) for i in range(1, k + 1)]; ys = ('y', 0)
    men = P + Q + [xs] + Ym + [ys]
    aord = P + Q + [xs] + Ym + [ys]
    bord = Ym + [ys] + P + Q + [xs]
    U = [('u', i) for i in range(k)]; V = [('v', i) for i in range(k)]
    G1 = [('g1', i) for i in range(k)]; G2 = [('g2', i) for i in range(k)]; G3 = [('g3', i) for i in range(ny)]
    t = ('t', 0)
    Gc = []
    for i in range(max(k, ny)):
        if i < k: Gc += [G1[i], G2[i]]
        if i < ny: Gc.append(G3[i])
    women = U + V + G1 + G2 + G3 + [t]
    cord = V + Gc + U + [t]
    dord = []
    for i in range(k): dord += [U[i], G1[k - 1 - i]]
    for i in range(k): dord += [V[i], G2[k - 1 - i]]
    dord += [t] + G3[::-1]
    mtype = {m: (X if m[0] in 'PQx' else Y) for m in men}
    wtype = {w: (Y if w[0] in ('v', 'g3') else X) for w in women}
    I, mi, wi = build(men, women, aord, bord, cord, dord, mtype, wtype)
    return I, mi, wi, dict(U=[wi[w] for w in U], V=[wi[w] for w in V], G=[wi[w] for w in Gc], t=wi[t])


def lace(base, laces):
    """Spread the laces evenly between the elements of base (one lace after about every
    len(base)/len(laces) elements; never two laces next to each other if len(laces) <= len(base))."""
    out = []; L = len(laces); B = len(base); j = 0
    for i, e in enumerate(base):
        out.append(e)
        while j < L and (j + 1) * B <= (i + 1) * L:
            out.append(laces[j]); j += 1
    out += laces[j:]
    return out


def ltwin(K, m=None, M=None, rev=0):
    """Laced twin, both directions. Women: Q = q_1..q_K (Y), G (m, X), P = p_1..p_p (X, p = K+1+M), h (X).
    Men: x_1..x_X1 (X, X1 = K+m+1), R = r_1..r_M (Y), y_1..y_K (Y), g (X). n = 2K + m + M + 2.
    c: Q, G, P with h after p_{(K+1)//2}.  d: P laced with the first G-women, then Q laced with the
    other G-women, h after q_{K-1}.  a: x's, R, y's, g.  b: y's laced with the first R-men, then x's laced
    with the other R-men, g after x_{(K+1)//2}.  (MP ring: X-men Q -> G -> P, Y-men P -> Q; WP ring:
    X-women x's -> R -> y's, Y-women y's -> x's.)"""
    if m is None: m = K
    if M is None: M = K
    p = K + 1 + M; X1 = K + m + 1
    q = [('q', i) for i in range(1, K + 1)]; G = [('G', i) for i in range(1, m + 1)]
    P = [('p', i) for i in range(1, p + 1)]; h = ('h', 0)
    x = [('x', i) for i in range(1, X1 + 1)]; R = [('r', i) for i in range(1, M + 1)]
    y = [('y', i) for i in range(1, K + 1)]; g = ('g', 0)
    men = x + R + y + [g]; women = q + G + P + [h]
    hpos = (K + 1) // 2
    cord = q + G + P[:hpos] + [h] + P[hpos:]
    # split G between P's d-range and Q's d-range in proportion to their sizes
    gP = (m * p) // (p + K)
    Qd = q[:K - 1] + [h] + q[K - 1:]
    G1, G2 = (G[:gP][::-1], G[gP:][::-1]) if rev else (G[:gP], G[gP:])
    dord = lace(P, G1) + lace(Qd, G2)
    aord = x + R + y + [g]
    rY = (M * K) // (K + X1)
    xb = x[:hpos] + [g] + x[hpos:]
    R1, R2 = (R[:rY][::-1], R[rY:][::-1]) if rev else (R[:rY], R[rY:])
    bord = lace(y, R1) + lace(xb, R2)
    mtype = {mm: (X if mm[0] in 'xg' else Y) for mm in men}
    wtype = {w: (X if w[0] in 'Gph' else Y) for w in women}
    I, mi, wi = build(men, women, aord, bord, cord, dord, mtype, wtype)
    return I, mi, wi


def ltwin3(K, m=None, M=None, rev=1, hoff=1, gadget=True):
    """ltwin plus 'liveness keepers' (2026-10-05) against the dead-for-type rule:
    MP keeper: r_1 moved to a-rank 2 (just below x_1); his d-first woman p_1 keeps him (only x_1 is better,
    and x_1 stays in Q), so r_1 never settles and every G-lace (holding an x a-worse than r_1) stays alive
    for Y-men. WP keeper: new Y-woman s at d-rank 2 (after p_1) and new Y-man v (b-top, a-last); v keeps s
    (only p_1 is d-better); p_1 is held in WP by x_1 (p_1 is moved in c to just after Q, so she is x_1's
    c-best X-woman; x_1 is moved in b to the end of the first H x's, so the q's reach him only in the last
    WP round). h moves hoff places later in c (P's c-part loses p_1)."""
    if m is None: m = K
    if M is None: M = K
    p = K + 1 + M; X1 = K + m + 1; H = (K + 1) // 2
    q = [('q', i) for i in range(1, K + 1)]; G = [('G', i) for i in range(1, m + 1)]
    P = [('p', i) for i in range(1, p + 1)]; h = ('h', 0)
    x = [('x', i) for i in range(1, X1 + 1)]; R = [('r', i) for i in range(1, M + 1)]
    y = [('y', i) for i in range(1, K + 1)]; g = ('g', 0)
    s = ('s', 0); v = ('v', 0)
    men = x + R + y + [g] + ([v] if gadget else []); women = q + G + P + [h] + ([s] if gadget else [])
    Pc = P[1:] if gadget else P
    hp = H + (hoff if gadget else 0)
    cord = q + ([P[0]] if gadget else []) + G + Pc[:hp - (1 if gadget else 0)] + [h] + Pc[hp - (1 if gadget else 0):]
    if gadget: cord += [s]
    gP = (m * p) // (p + K)
    G1, G2 = (G[:gP][::-1], G[gP:][::-1]) if rev else (G[:gP], G[gP:])
    Qd = q[:K - 1] + [h] + q[K - 1:]
    if gadget:
        dord = [P[0], s] + lace(P[1:], G1) + lace(Qd, G2)
    else:
        dord = lace(P, G1) + lace(Qd, G2)
    if gadget:
        aord = [x[0], R[0]] + x[1:] + R[1:] + y + [g, v]
    else:
        aord = x + R + y + [g]
    rY = (M * K) // (K + X1)
    R1, R2 = (R[:rY][::-1], R[rY:][::-1]) if rev else (R[:rY], R[rY:])
    if gadget:
        xb = x[1:H] + [x[0], g] + x[H:]
        bord = [v] + lace(y, R1) + lace(xb, R2)
    else:
        xb = x[:H] + [g] + x[H:]
        bord = lace(y, R1) + lace(xb, R2)
    mtype = {mm: (X if mm[0] in 'xg' else Y) for mm in men}
    wtype = {w: (X if w[0] in 'Gph' else Y) for w in women}
    I, mi, wi = build(men, women, aord, bord, cord, dord, mtype, wtype)
    return I, mi, wi


def retype(I, women_flip=(), men_flip=()):
    """Copy of I with the types of the given women / men flipped (lists unchanged)."""
    J = {k: (list(v) if isinstance(v, list) else v) for k, v in I.items()}
    for w in women_flip: J['tw'][w] ^= 1
    for m in men_flip: J['tm'][m] ^= 1
    return J


def mtwin(K, flip='PQxy', step=2):
    """ltwin(K, m=0, M=0) (twin core, no laces) with every step-th member of each listed region flipped to the
    other type: P, Q (women), x, y (men). Experiment for 'mixed non-chain regions'."""
    I, mi, wi = ltwin(K, 0, 0, 0)
    wf = []; mf = []
    p = K + 1
    if 'P' in flip: wf += [wi[('p', i)] for i in range(1, p + 1) if i % step == 0]
    if 'Q' in flip: wf += [wi[('q', i)] for i in range(1, K + 1) if i % step == 0]
    if 'x' in flip: mf += [mi[('x', i)] for i in range(1, K + 2) if i % step == 0]
    if 'y' in flip: mf += [mi[('y', i)] for i in range(1, K + 1) if i % step == 0]
    return retype(I, wf, mf), mi, wi


def mxtwin(K, J=None, yperm='rev', seed=1, hpos=None, gpos=None, pattern='alt'):
    """Mixed twin (2026-10-05 experiment): P = K+1 X-women and J Y-women interleaved (same internal order in
    c and d), Q = K Y-women, h (X). Men: K+1 x's (X), K+J y's (Y), g (X). a: x's, y's (order A), g.
    b: y's (order B), x's with g inside. c: Q, P (h inside), d: P, Q (h near the end).
    yperm: how B relates to A on the y's ('same', 'rev', 'rand')."""
    import random
    rng = random.Random(seed)
    if J is None: J = K
    q = [('q', i) for i in range(1, K + 1)]
    PX = [('p', i) for i in range(1, K + 2)]; PY = [('P', i) for i in range(1, J + 1)]
    h = ('h', 0)
    x = [('x', i) for i in range(1, K + 2)]; y = [('y', i) for i in range(1, K + J + 1)]; g = ('g', 0)
    # interleave P_X and P_Y
    P = lace(PX, PY) if pattern == 'alt' else PX + PY
    men = x + y + [g]; women = q + PX + PY + [h]
    if hpos is None: hpos = (len(P) + 1) // 2
    cord = q + P[:hpos] + [h] + P[hpos:]
    dord = P + q[:K - 1] + [h] + q[K - 1:]
    A = list(y)
    if yperm == 'same': B = list(y)
    elif yperm == 'rev': B = y[::-1]
    else: B = list(y); rng.shuffle(B)
    if gpos is None: gpos = (K + 1) // 2
    aord = x + A + [g]
    bord = B + x[:gpos] + [g] + x[gpos:]
    mtype = {mm: (X if mm[0] in 'xg' else Y) for mm in men}
    wtype = {w: (X if w[0] in 'ph' else Y) for w in women}
    I, mi, wi = build(men, women, aord, bord, cord, dord, mtype, wtype)
    return I, mi, wi


def mxtwin2(K, J=None, qstep=0, xperm='same', ystep=0, seed=1, yperm='rev'):
    """mxtwin plus: every qstep-th Q-woman flipped to X-type; b-order on the x's permuted ('same', 'rev',
    'rand'); every ystep-th y flipped to X-type (0 = none)."""
    import random
    rng = random.Random(seed)
    I, mi, wi = mxtwin(K, J, yperm=yperm, seed=seed)
    n = I['n']; J = K if J is None else J
    wf = [wi[('q', i)] for i in range(1, K + 1) if qstep and i % qstep == 0]
    mf = [mi[('y', i)] for i in range(1, K + J + 1) if ystep and i % ystep == 0]
    I = retype(I, wf, mf)
    if xperm != 'same':
        xs = [mi[('x', i)] for i in range(1, K + 2)]
        pos = sorted(I['b'][m] for m in xs)
        order = xs[::-1] if xperm == 'rev' else rng.sample(xs, len(xs))
        for p_, m in zip(pos, order): I['b'][m] = p_
    return I, mi, wi


def gtwin(K, J, s, t, u, xperm='rev', qperm='same', seed=1, hpos=None, gpos=None):
    """General mixed twin (2026-10-05). Women: Q (K: s X-type + K-s Y-type, interleaved), P (K+1+J: P_X X-type and
    P_Y Y-type interleaved, same internal order in c and d), h (X). Men: XR (K+1-u X-men + t Y-men, interleaved),
    YR (u X-men + K+J-t Y-men, interleaved, same internal order in a and b), g (X).
    Counts satisfy both twin forcing lemmas: |MX| = |Q|+2, |MY| = |P|-1, |WX| = |XR|+1, |WY| = |YR|.
    c: Q, P (h after position hpos); d: P, Q (h before the last q). a: XR, YR, g; b: YR, XR (permuted: xperm) with g
    after position gpos. qperm: order of Q in d relative to c."""
    import random
    rng = random.Random(seed)
    p = K + 1 + J
    PX = K + 1 - u + t - s; PY = p - PX
    assert PX >= 0 and PY >= 0 and 0 <= s <= K and 0 <= u <= K + 1 and 0 <= t <= p - 1
    def mix(nx, ny, tag):          # interleave nx X-type and ny Y-type names
        xs = [(tag + 'X', i) for i in range(nx)]; ys = [(tag + 'Y', i) for i in range(ny)]
        return lace(xs, ys) if nx >= ny else lace(ys, xs)
    Q = mix(s, K - s, 'q'); P = mix(PX, PY, 'p'); h = ('h', 0)
    XR = mix(K + 1 - u, t, 'x'); YR = mix(u, K + J - t, 'y'); g = ('g', 0)
    if hpos is None: hpos = (p + 1) // 2
    if gpos is None: gpos = (len(XR) + 1) // 2
    cord = Q + P[:hpos] + [h] + P[hpos:]
    Qd = Q if qperm == 'same' else (Q[::-1] if qperm == 'rev' else rng.sample(Q, len(Q)))
    dord = P + Qd[:-1] + [h] + Qd[-1:]
    XRb = XR[::-1] if xperm == 'rev' else (XR if xperm == 'same' else rng.sample(XR, len(XR)))
    aord = XR + YR + [g]
    bord = YR + XRb[:gpos] + [g] + XRb[gpos:]
    men = XR + YR + [g]; women = Q + P + [h]
    mtype = {m: (X if m[0].endswith('X') or m[0] == 'g' else Y) for m in men}
    wtype = {w: (X if w[0].endswith('X') or w[0] == 'h' else Y) for w in women}
    return build(men, women, aord, bord, cord, dord, mtype, wtype)
