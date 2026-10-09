"""Independent rotations from full preference successor cycles, 2026-10-05."""
from check import *
from collections import deque
from pathlib import Path
from itertools import combinations


def load(path):
    fields={line.split()[0]:line.split()[1:] for line in Path(path).read_text().splitlines() if line and not line.startswith('#')}
    return Instance(*(ranks(list(map(int,fields[k]))) for k in ('men_x_ranking','men_y_ranking','women_x_ranking','women_y_ranking')), *([int(t=='Y') for t in ''.join(fields[k])] for k in ('men_preferences','women_preferences')))


def exposed(I,wife):
    husband=ranks(wife);dest={};edge={}
    for m in range(I.n):
        for w in I.orders[m][I.mr[m][wife[m]]+1:]:
            if I.wr[w][m]<I.wr[w][husband[w]]:
                dest[m]=w;edge[m]=husband[w];break
    cycles=[];done=set()
    for start in range(I.n):
        path=[];where={};m=start
        while m in edge and m not in where and m not in done:
            where[m]=len(path);path.append(m);m=edge[m]
        if m in where:cycles.append(path[where[m]:])
        done.update(path)
    return [(cyc,{m:dest[m] for m in cyc}) for cyc in cycles]


def structural(I,wife,cyc,move):
    n=len(cyc);women=[wife[m] for m in cyc]
    # Boundaries A: X->Y partners; B: Y->X partners.
    As=[wife[cyc[j]] for j in range(n) if I.mt[cyc[j]]==0 and I.mt[cyc[j-1]]==1]
    Bs=[wife[cyc[j]] for j in range(n) if I.mt[cyc[j]]==1 and I.mt[cyc[j-1]]==0]
    assert As and len(As)==len(Bs)
    role={w:'A' for w in As}|{w:'B' for w in Bs};points=As+Bs
    for r,first in ((I.c,'A'),(I.d,'B')):
        ordered=sorted(points,key=r.__getitem__)
        assert ''.join(role[w] for w in ordered)==('AB' if first=='A' else 'BA')*len(As)
        assert all(I.wt[ordered[j]]==I.wt[ordered[j+1]] for j in range(1,len(ordered)-1,2))
        assert I.wt[ordered[0]]!=I.wt[ordered[-1]]
    for t in (0,1):
        chain=sorted([w for w in points if I.wt[w]==t],key=I.c.__getitem__)
        assert [I.d[w] for w in chain]==sorted(I.d[w] for w in chain)
    delta=sum(I.wt[w]==0 for w in Bs)-sum(I.wt[w]==0 for w in As)
    assert abs(delta)==1
    after=list(wife)
    for m,w in move.items():after[m]=w
    actual=sum(I.mt[m]==I.wt[w]==0 for m,w in enumerate(after))-sum(I.mt[m]==I.wt[w]==0 for m,w in enumerate(wife))
    assert delta==actual and I.stable(after)
    return delta,len(As),tuple(after)


def lattice(I,stats):
    initial=I.gs()[0];reached={initial:frozenset()};queue=deque([initial]);ids={};deltas=[];runs=[]
    while queue:
        wife=queue.popleft()
        for cyc,move in exposed(I,wife):
            key=tuple(sorted((m,wife[m],move[m]) for m in cyc))
            delta,run_count,after=structural(I,wife,cyc,move)
            if key not in ids:ids[key]=len(ids);deltas.append(delta);runs.append(run_count)
            ri=ids[key];used=reached[wife]|{ri};assert ri not in reached[wife]
            if after in reached:assert reached[after]==used
            else:reached[after]=used;queue.append(after)
            stats['exposed_edges']+=1
    stats['instances']+=1;stats['stable_states']+=len(reached);stats['rotations']+=len(ids);stats['max_runs']=max([stats['max_runs']]+runs)
    return reached,ids,deltas


