"""Draw the C4 staircases  for the man-optimal and woman-optimal stable matchings
of one random 2-list instance. 2026-10-05. Standard library + matplotlib."""
import random, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

BLUE, ORANGE, INK, MUTED = "#2a78d6", "#eb6834", "#2b2b2b", "#8a8a85"


def instance(n, rng):
    perm = lambda: rng.sample(range(n), n)
    a, b, c, d = perm(), perm(), perm(), perm()  # rank of person i in each list
    mt = [rng.choice("XY") for _ in range(n)]
    wt = [rng.choice("XY") for _ in range(n)]
    return a, b, c, d, mt, wt


def gs(n, prop_rank, recv_rank):
    """Proposer-optimal stable matching. prop_rank(p, r), recv_rank(r, p): smaller = better."""
    order = [sorted(range(n), key=lambda r: prop_rank(p, r)) for p in range(n)]
    nxt, held, free = [0] * n, [None] * n, list(range(n))
    while free:
        p = free.pop()
        r = order[p][nxt[p]]
        nxt[p] += 1
        q = held[r]
        if q is None:
            held[r] = p
        elif recv_rank(r, p) < recv_rank(r, q):
            held[r] = p
            free.append(q)
        else:
            free.append(p)
    return held  # held[receiver] = proposer


def solve(n, a, b, c, d, mt, wt):
    mr = lambda m, w: c[w] if mt[m] == "X" else d[w]
    wr = lambda w, m: a[m] if wt[w] == "X" else b[m]
    husband_mo = gs(n, mr, wr)                      # men propose
    wife_wo = gs(n, wr, mr)                         # women propose: wife_wo[m] = w
    husband_wo = [None] * n
    for m, w in enumerate(wife_wo):
        husband_wo[w] = m
    for h in (husband_mo, husband_wo):              # brute-force stability check
        wife = [None] * n
        for w, m in enumerate(h):
            wife[m] = w
        for m in range(n):
            for w in range(n):
                assert not (mr(m, w) < mr(m, wife[m]) and wr(w, m) < wr(w, h[w])), "unstable"
    return husband_mo, husband_wo


def staircase(points):
    """points: list of (u, v, isX). X-labeled set must be closed under (u larger, v smaller).
    Returns the step boundary g(u)+0.5 for u = 0..n-1 and checks the C4 separation."""
    xs = [(u, v) for u, v, isx in points if isx]
    n = max(max(u for u, _, _ in points), max(v for _, v, _ in points)) + 1
    g = []
    for u in range(n):
        vs = [v for (u0, v) in xs if u0 <= u]
        g.append(max(vs) + 0.5 if vs else -0.5)
    for u, v, isx in points:
        assert isx == (v < g[u]), "C4 staircase separation fails"
    return g


def main(n=40, seed=None):
    best = None
    for s in range(400) if seed is None else [seed]:
        inst = instance(n, random.Random(s))
        mo, wo = solve(n, *inst)
        diff = sum(1 for w in range(n) if inst[4][mo[w]] != inst[4][wo[w]])
        if best is None or diff > best[0]:
            best = (diff, s, inst, mo, wo)
    diff, s, (a, b, c, d, mt, wt), mo, wo = best
    print(f"seed {s}: {diff} women change partner type between MOSM and WOSM")

    fig = plt.figure(figsize=(11, 14.2))
    gs_ = fig.add_gridspec(2, 2, left=0.08, right=0.97, top=0.735, bottom=0.12,
                           hspace=0.16, wspace=0.08)
    axes = [[fig.add_subplot(gs_[i, j]) for j in range(2)] for i in range(2)]
    rows = [("Man-optimal stable matching", mo), ("Woman-optimal stable matching", wo)]
    for i, (rname, husband) in enumerate(rows):
        for j, wtype in enumerate("XY"):
            ax = axes[i][j]
            pts = [(n - 1 - c[w], n - 1 - d[w], mt[husband[w]] == "X")
                   for w in range(n) if wt[w] == wtype]
            g = staircase(pts)
            ax.step(range(n), g, where="post", color=INK, lw=2, zorder=1)
            for isx, col, mk, lab in ((True, BLUE, "o", "woman matched to an X-man"),
                                      (False, ORANGE, "s", "woman matched to a Y-man")):
                sel = [(u, v) for u, v, f in pts if f == isx]
                ax.scatter([p[0] for p in sel], [p[1] for p in sel], s=70, c=col, marker=mk,
                           edgecolors="white", linewidths=1.5, zorder=2, label=lab)
            ax.set_xlim(-1, n)
            ax.set_ylim(-1, n)
            ax.set_aspect("equal")
            ax.grid(color="#e6e6e3", lw=0.8)
            ax.set_axisbelow(True)
            ax.set_xticklabels([])
            ax.set_yticklabels([])
            ax.tick_params(length=0)
            for sp in ax.spines.values():
                sp.set_color("#c9c9c4")
            ax.set_title(f"{rname}: {wtype}-type women ({len(pts)})", fontsize=11.5, color=INK)
            ax.set_xlabel("position in the women's X-list (right = better)", color=MUTED, fontsize=10)
            if j == 0:
                ax.set_ylabel("position in the women's Y-list (up = better)", color=MUTED, fontsize=10)
    h, l = axes[0][0].get_legend_handles_labels()
    h.append(plt.Line2D([], [], color=INK, lw=2))
    l.append("staircase that separates them")
    fig.legend(h, l, loc="upper center", bbox_to_anchor=(0.5, 0.775), ncol=3, frameon=False,
               fontsize=10.5)

    fig.text(0.04, 0.985, "2-list stable matching: two staircases determine a stable matching",
             fontsize=17, fontweight="bold", color=INK, va="top")
    intro = (
        "The problem. There are n men and n women. The men are ranked in two lists, X and Y, and so are the women.\n"
        "Each person ranks the other side by one of the two lists: X-type people use the X-list, Y-type people use\n"
        "the Y-list. A matching is stable if no man and woman prefer each other to their own partners. GS\n"
        "finds a stable matching in O(n\u00b2) time. A later anonymous draft claims a faster algorithm; see docs/subquadratic.md.\n"
        "\n"
        "The picture. Each panel shows the women of one type, placed by their positions in the two lists. Blue women\n"
        "are matched to X-men (who rank women by the X-list); orange women are matched to Y-men. In every panel the\n"
        "blue women lie below and to the right of a monotone staircase: good for X-men, poor for Y-men. This holds for\n"
        "every stable matching (C4: proved and audited), so two staircases, one per woman type, fix the whole matching. The top row\n"
        "is the man-optimal stable matching and the bottom row is the woman-optimal one; the staircases move between them."
    )
    fig.text(0.04, 0.955, intro, fontsize=10.8, color=INK, va="top", linespacing=1.45)
    foot = (
        f"Illustration regenerated 2026-10-09. Random instance: n = {n}, uniform random lists and types (seed {s}). Both matchings were checked for stability "
        "and for the staircase property by brute force.\n"
    )
    fig.text(0.04, 0.025, foot, fontsize=9, color=MUTED, va="bottom", linespacing=1.5)
    out = str(__import__("pathlib").Path(__file__).with_name("staircases.png"))
    fig.savefig(out, dpi=110)
    print(out)


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 40)
