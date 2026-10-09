"""Independent D2 lemma and count checks; standard library, 2026-10-05.
Uses independently written GS/successor code; never imports or runs producer code.
"""
from rotations import Instance, exposed, ranks, load
from itertools import product, permutations, combinations
from collections import Counter, deque
from pathlib import Path
import ast, json, random

C=Counter()
def witnesses(I,p):
    h=ranks(p); out={-1:set(),1:set()}
    for A in range(I.n):
        if I.mt[h[A]]!=1: continue
        for B in range(I.n):
            if I.mt[h[B]]!=0 or I.wt[A]==I.wt[B]:continue
            if I.c[A]<I.c[B] and I.d[B]<I.d[A]:
                out[2*I.wt[A]-1].update(range(I.c[A],I.c[B]+1))
    assert not out[-1]&out[1]
    C['witness_disjoint_states']+=1
    return out

def audit(I, brute=False):
    start=I.gs()[0]; todo=deque([start]); states={start:frozenset()}; rot={}; ws={start:witnesses(I,start)}
    while todo:
        p=todo.popleft(); h=ranks(p)
        for men,move in exposed(I,p):
            q=list(p)
            for m,w in move.items():q[m]=w
            q=tuple(q); assert I.stable(q)
            hq=ranks(q)
            A=[w for w in range(I.n) if I.mt[h[w]]==0 and I.mt[hq[w]]==1]
            B=[w for w in range(I.n) if I.mt[h[w]]==1 and I.mt[hq[w]]==0]
            sign=sum(I.wt[w]==0 for w in B)-sum(I.wt[w]==0 for w in A)
            assert abs(sign)==1
            cb=sorted(A+B,key=I.c.__getitem__); db=sorted(A+B,key=I.d.__getitem__)
            hc=set(range(I.c[cb[0]],I.c[cb[-1]]+1)); hd=set(range(I.d[db[0]],I.d[db[-1]]+1))
            # Lemma 2, every consecutive boundary gap has an endpoint-type witness.
            for u,v in zip(cb,cb[1:]):
                assert any(I.wt[a]==I.wt[cb[0]] and I.wt[b]==I.wt[cb[-1]] and I.c[a]<=I.c[u]<I.c[v]<=I.c[b] and I.d[b]<I.d[a] for a in A for b in B)
                C['configuration_gaps']+=1
            for a in A:
                if I.wt[a]==I.wt[cb[0]]:
                    nxt=next(w for w in cb if I.c[w]>I.c[a] and I.wt[w]!=I.wt[a])
                    assert I.d[nxt]<I.d[a] # Lemma 4, both sign forms.
                    assert I.c[a] not in ws[p][-sign] # Lemma 5.
                    C['boundary_exclusions']+=1
                b=next(w for w in cb if I.c[w]>I.c[a])
                if I.wt[a]!=I.wt[cb[0]] and I.wt[b]==I.wt[cb[0]]:
                    assert I.d[a]<I.d[b] # Lemma 3.
                    C['reverse_runs']+=1
            if q not in ws:ws[q]=witnesses(I,q)
            assert hc<=ws[q][sign]
            assert all(ws[p][s]<=ws[q][s] for s in (-1,1))
            key=tuple(sorted((m,p[m],q[m]) for m in men))
            rec=(sign,hc,hd,set(men),{p[m] for m in men})
            if key in rot:assert rot[key]==rec
            rot[key]=rec
            used=states[p]|{key}; assert key not in states[p]
            if q in states:assert states[q]==used
            else:states[q]=used;todo.append(q)
            C['rotation_edges']+=1
            C['multi_run_edges']+=int(len(A)>1)
    if brute:
        assert set(states)=={p for p in permutations(range(I.n)) if I.stable(p)}
        C['bruteforce_instances']+=1
    end=max(states,key=lambda p:len(states[p])); assert len(states[end])==len(rot)
    for sign,hc,hd,men,women in rot.values():assert hc<=ws[end][sign]
    for r,s in combinations(rot.values(),2):
        if r[1]&s[1] or r[2]&s[2]:assert r[0]==s[0];C['hull_intersections']+=1
        if r[3]&s[3] or r[4]&s[4]:assert r[0]==s[0];C['shared_person_pairs']+=1
    hs,he=ranks(start),ranks(end)
    changed=[sum(I.wt[w]==t and I.mt[hs[w]]!=I.mt[he[w]] for w in range(I.n)) for t in (0,1)]
    changed += [sum(I.mt[m]==t and I.wt[start[m]]!=I.wt[end[m]] for m in range(I.n)) for t in (0,1)]
    assert len(rot)<=min(changed)
    C['instances']+=1;C['rotations']+=len(rot); C['stable_states']+=len(states)

