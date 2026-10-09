"""Independent audit code. Python standard library only, 2026-10-05.

Uses rank arrays and single-proposal GS states; imports no producer code.
"""
from itertools import permutations, product
from functools import lru_cache
import json
import random


def ranks(order):
    r = [0] * len(order)
    for i, v in enumerate(order):
        r[v] = i
    return tuple(r)


class Instance:
    def __init__(self, a, b, c, d, mt, wt):
        self.a, self.b, self.c, self.d = map(tuple, (a, b, c, d))
        self.mt, self.wt = tuple(mt), tuple(wt)
        self.n = len(a)
        self.mr = [self.c if t == 0 else self.d for t in mt]
        self.wr = [self.a if t == 0 else self.b for t in wt]
        self.orders = [sorted(range(self.n), key=r.__getitem__) for r in self.mr]

    def stable(self, wife):
        husband = ranks(wife)
        return not any(self.mr[m][w] < self.mr[m][wife[m]] and
                       self.wr[w][m] < self.wr[w][husband[w]]
                       for m in range(self.n) for w in range(self.n))

    def step(self, state, m):
        ptr, held = map(list, state)
        w = self.orders[m][ptr[m]]
        ptr[m] += 1
        old = held[w]
        accepted = old < 0 or self.wr[w][m] < self.wr[w][old]
        switched = accepted and old >= 0 and self.mt[m] != self.mt[old]
        if accepted:
            held[w] = m
        return (tuple(ptr), tuple(held)), int(accepted), int(switched), w

    def free(self, state):
        held = set(state[1])
        return [m for m in range(self.n) if m not in held]

    def gs(self, key=None, rng=None, invariant=None, ring_k=None):
        state = ((0,) * self.n, (-1,) * self.n)
        counts = [0, 0]
        forced = None
        t2_state = None
        while self.free(state):
            free = self.free(state)
            m = rng.choice(free) if rng else min(free, key=key)
            w = self.orders[m][state[0][m]]
            if forced is None and ring_k is not None and self.mt[m] == 0 and w < ring_k:
                forced = [0, 0]
                t2_state = state
                assert len(free) == 1
            state, ac, sw, _ = self.step(state, m)
            counts[0] += ac
            counts[1] += sw
            if forced is not None:
                forced[0] += ac
                forced[1] += sw
            if invariant:
                invariant(self, state)
        wife = ranks(state[1])
        assert self.stable(wife)
        return wife, counts, forced, t2_state


def crossed(k):
    n = 2*k + 1
    x, y = list(range(k)), list(range(k, n))
    u, v = list(range(k)), list(range(k, 2*k))
    return Instance(ranks(x+y), ranks(y+x), ranks(v+u+[2*k]),
                    ranks(u+v+[2*k]), [0]*k+[1]*(k+1), [0]*k+[1]*(k+1))


def ring2(k, r):
    assert k >= 2*r+1
    # X_i=i-1; Y_i=k+i-1; ys=2k. This is translated from the note, not code.
    X = lambda i: i-1
    Y = lambda i: k+i-1
    ao = []
    for i in range(1, r+1):
        ao += [Y(i), X(r-i+1)]
    ao += [Y(i) for i in range(r+1, k+1)]
    ao += [X(i) for i in range(r+1, k+1)] + [2*k]
    bo = []
    for i in range(r):
        bo += [X(k-i), Y(k-r+i+1)]
    bo += [2*k] + [X(i) for i in range(k-r, 0, -1)]
    bo += [Y(i) for i in range(1, k-r+1)]
    assert sorted(ao) == sorted(bo) == list(range(2*k+1))
    u, v = list(range(k)), list(range(k, 2*k))
    return Instance(ranks(ao), ranks(bo), ranks(v+u+[2*k]),
                    ranks(u+v+[2*k]), [0]*k+[1]*(k+1), [0]*k+[1]*(k+1))


def minimum_cost(I, which):
    @lru_cache(None)
    def visit(state):
        choices = I.free(state)
        if not choices:
            return 0
        vals = []
        for m in choices:
            nxt, ac, sw, _ = I.step(state, m)
            vals.append((ac if which == 0 else sw) + visit(nxt))
        return min(vals)
    result = visit(((0,)*I.n, (-1,)*I.n))
    return result, visit.cache_info().currsize


def greedy(I, women, men, rank):
    free = set(men)
    pairs = {}
    for w in sorted(women, key=rank.__getitem__):
        m = min(free, key=I.wr[w].__getitem__)
        pairs[w] = m
        free.remove(m)
    return pairs


def held_invariant(I, state):
    for t in (0, 1):
        pairs = {w:m for w,m in enumerate(state[1]) if m >= 0 and I.mt[m] == t}
        assert pairs == greedy(I, pairs, pairs.values(), I.c if t == 0 else I.d)


def structure(I, wife):
    h = ranks(wife)
    stable = I.stable(wife)
    sweep = True
    for w in range(I.n):
        avail = [m for m in range(I.n) if I.mr[m][wife[m]] >= I.mr[m][w]]
        if min(avail, key=I.wr[w].__getitem__) != h[w]:
            sweep = False
    assert stable == sweep, 'F6'
    if not stable:
        return False
    for t, u in product((0,1), repeat=2):
        pairs = [(m,w) for m,w in enumerate(wife) if I.mt[m] == t and I.wt[w] == u]
        pairs.sort(key=lambda p: I.mr[p[0]][p[1]])
        rr = [I.wr[w][m] for m,w in pairs]
        assert rr == sorted(rr), 'F3'
    for t in (0,1):
        men = [m for m in range(I.n) if I.mt[m] == t]
        women = [wife[m] for m in men]
        assert greedy(I, women, men, I.c if t == 0 else I.d) == {w:h[w] for w in women}, 'F4'
    # C4 women-side staircase.
    for v,w in product(range(I.n), repeat=2):
        if I.wt[v] == I.wt[w] and I.mt[h[v]] == 0 and I.c[w] < I.c[v] and I.d[w] > I.d[v]:
            assert I.mt[h[w]] == 0, 'C4'
    return True


