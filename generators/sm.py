# family construction small tools (Python 3, standard library only).
# Instance: dict with n, a, b, tm (men), c, d, tw (women). Ranks 0 = best. Types 0 = X, 1 = Y.
# X-man ranks women by c, Y-man by d. X-woman ranks men by a, Y-woman by b.
import random, itertools, heapq
from collections import deque

X, Y = 0, 1


def make(a, b, tm, c, d, tw):
    n = len(a)
    assert sorted(a) == list(range(n)) and sorted(b) == list(range(n))
    assert sorted(c) == list(range(n)) and sorted(d) == list(range(n))
    return dict(n=n, a=list(a), b=list(b), tm=list(tm), c=list(c), d=list(d), tw=list(tw))


def rand_inst(n, rng=random, px=0.5, pw=0.5):
    def perm():
        p = list(range(n)); rng.shuffle(p); return p
    return make(perm(), perm(), [0 if rng.random() < px else 1 for _ in range(n)],
                perm(), perm(), [0 if rng.random() < pw else 1 for _ in range(n)])


def men_lists(I):
    n = I['n']
    byc = sorted(range(n), key=lambda w: I['c'][w])
    byd = sorted(range(n), key=lambda w: I['d'][w])
    return [byc if I['tm'][m] == X else byd for m in range(n)]


def wrank(I, w, m):
    return I['a'][m] if I['tw'][w] == X else I['b'][m]


def mrank(I, m, w):
    return I['c'][w] if I['tm'][m] == X else I['d'][w]


def gs_reference(I):
    """Plain O(n^2) men-proposing GS. Returns mu: man -> woman."""
    n = I['n']; L = men_lists(I)
    ptr = [0] * n; holder = [None] * n
    free = list(range(n))
    while free:
        m = free.pop()
        w = L[m][ptr[m]]; ptr[m] += 1
        h = holder[w]
        if h is None:
            holder[w] = m
        elif wrank(I, w, m) < wrank(I, w, h):
            holder[w] = m; free.append(h)
        else:
            free.append(m)
    mu = [None] * n
    for w in range(n):
        mu[holder[w]] = w
    return mu


def is_stable(I, mu):
    n = I['n']; inv = [None] * n
    for m in range(n):
        inv[mu[m]] = m
    for m in range(n):
        for w in range(n):
            if w != mu[m] and mrank(I, m, w) < mrank(I, m, mu[m]) and wrank(I, w, m) < wrank(I, w, inv[w]):
                return False
    return True


def all_stable(I):
    n = I['n']
    return [list(p) for p in itertools.permutations(range(n)) if is_stable(I, list(p))]


def bulk_step(I, L, holder, ptr, m):
    """Free man m proposes from ptr[m] until accepted. Returns (w, displaced)."""
    p = ptr[m]
    while True:
        w = L[m][p]
        h = holder[w]
        if h is None or wrank(I, w, m) < wrank(I, w, h):
            ptr[m] = p
            holder[w] = m
            return w, h
        p += 1


def min_cost_over_orders(I, cost='A', limit=2_000_000):
    """Exact min over all GS proposal orders of: 'A' = accepted proposals, or
    'S' = women-side cross-type switches (a woman's held man changes type), 'M' = men-side
    switches (a displaced man is next accepted by a woman of the other type). Dijkstra over states."""
    n = I['n']; L = men_lists(I)
    start = (tuple([None] * n), tuple([0] * n))
    dist = {start: 0}
    pq = [(0, 0, start)]; cnt = 0
    while pq:
        dcur, _, st = heapq.heappop(pq)
        if dist[st] < dcur:
            continue
        holder, ptr = st
        held = set(h for h in holder if h is not None)
        if len(held) == n:
            return dcur, len(dist)
        for m in range(n):
            if m in held:
                continue
            hl = list(holder); pt = list(ptr)
            w, h = bulk_step(I, L, hl, pt, m)
            if h is not None:
                pt[h] += 1
            pm = L[m][ptr[m] - 1] if ptr[m] > 0 else None
            sw_w = 1 if (h is not None and I['tm'][h] != I['tm'][m]) else 0
            sw_m = 1 if (pm is not None and I['tw'][pm] != I['tw'][w]) else 0
            if cost == 'A':
                wgt = 1
            elif cost == 'S':
                wgt = sw_w
            elif cost == 'M':
                wgt = sw_m
            else:
                raise ValueError(cost)
            ns = (tuple(hl), tuple(pt))
            nd = dcur + wgt
            if ns not in dist or dist[ns] > nd:
                dist[ns] = nd; cnt += 1
                heapq.heappush(pq, (nd, cnt, ns))
        if len(dist) > limit:
            raise RuntimeError('state limit')
    raise RuntimeError('no terminal')


def gs_order(I, rule='fifo', rng=None):
    """GS with a next-free-man rule; returns (mu, A, switches, displacements)."""
    n = I['n']; L = men_lists(I)
    holder = [None] * n; ptr = [0] * n
    A = S = D = 0
    if rule == 'fifo':
        q = deque(range(n)); pop = q.popleft; push = q.append
    elif rule == 'lifo':
        q = list(range(n)); pop = q.pop; push = q.append
    elif rule == 'xfirst':
        q = [(I['tm'][m], m) for m in range(n)]; heapq.heapify(q)
        pop = lambda: heapq.heappop(q)[1]; push = lambda m: heapq.heappush(q, (I['tm'][m], m))
    elif rule == 'yfirst':
        q = [(-I['tm'][m], m) for m in range(n)]; heapq.heapify(q)
        pop = lambda: heapq.heappop(q)[1]; push = lambda m: heapq.heappush(q, (-I['tm'][m], m))
    elif rule == 'random':
        q = list(range(n))
        def pop():
            i = rng.randrange(len(q)); q[i], q[-1] = q[-1], q[i]; return q.pop()
        push = q.append
    while q:
        m = pop()
        w, h = bulk_step(I, L, holder, ptr, m)
        A += 1
        if h is not None:
            D += 1; ptr[h] += 1
            if I['tm'][h] != I['tm'][m]:
                S += 1
            push(h)
    mu = [None] * n
    for w in range(n):
        mu[holder[w]] = w
    return mu, A, S, D


def phase_gs(I):
    """Type-alternating GS: X-phase = all free X-men run until no free X-man; then Y-phase; ...
    Returns (mu, number of phases that did work)."""
    n = I['n']; L = men_lists(I)
    holder = [None] * n; ptr = [0] * n
    free = {X: [m for m in range(n) if I['tm'][m] == X], Y: [m for m in range(n) if I['tm'][m] == Y]}
    phases = 0; t = X
    while free[X] or free[Y]:
        if free[t]:
            phases += 1
            while free[t]:
                m = free[t].pop()
                w, h = bulk_step(I, L, holder, ptr, m)
                if h is not None:
                    ptr[h] += 1
                    free[I['tm'][h]].append(h)
        t = 1 - t
    mu = [None] * n
    for w in range(n):
        mu[holder[w]] = w
    return mu, phases
