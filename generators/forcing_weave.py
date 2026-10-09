"""Constrained perturbations of C3-race-hard seeds, no global SA."""
import json,pathlib,subprocess,random


def change(I,kind,t,keys,seed):
    J={k:list(v) if isinstance(v,list) else v for k,v in I.items()};rng=random.Random(seed)
    if t==0:return J
    if kind=='types':
        for types in ('tm','tw'):
            groups=[[i for i,v in enumerate(I[types]) if v==b] for b in (0,1)]
            for group in groups:
                for x in rng.sample(group,min(t,len(group))):J[types][x]^=1
        return J
    for key in keys:
        typ=I['tm' if key in 'ab' else 'tw'];order=sorted(range(I['n']),key=I[key].__getitem__)
        if kind=='spread':
            xs=[x for x in order if typ[x]==0];ys=[x for x in order if typ[x]==1]
            count=min(t,len(xs),len(ys))
            # Spread crossings across the full pure ranges, with independent offsets.
            xp=[xs[(2*j+1)*len(xs)//(2*count)] for j in range(count)]
            yp=[ys[(2*j+1)*len(ys)//(2*count)] for j in range(count)]
            rng.shuffle(yp)
            for x,y in zip(xp,yp):J[key][x],J[key][y]=J[key][y],J[key][x]
        elif kind=='boundary':
            boundaries=[i for i in range(1,I['n']) if typ[order[i]]!=typ[order[i-1]]]
            used=set()
            for b in boundaries:
                for j in range(t):
                    lo=b-1-j;hi=b+j
                    if lo<0 or hi>=I['n'] or lo in used or hi in used:continue
                    if j%2==0:
                        x,y=order[lo],order[hi];J[key][x],J[key][y]=J[key][y],J[key][x];used.update((lo,hi))
        else:raise ValueError(kind)
    return J

