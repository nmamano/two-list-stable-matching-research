// C12: surely-decided reduction (run-buffer implementation's red-team idea, 2026-10-05).
// Run the 2-frontier forced propagation (C6/C7) with both woman types flexible until it stalls.
// Every decision holds in every stable matching, and at a stall every decided woman has taken a man.
// The residual instance (undecided women, men still in the two quadrants) has exactly the
// stable matchings of the full instance restricted to it (proof in docs/summary.md, C12).
#pragma once

struct PropResult {
  vector<int> husb;          // husb[w] = man taken by decided woman w, -1 if undecided
  vector<int> resM, resW;    // residual men / women (local ids, increasing)
  bool complete = false;     // true if every woman is decided
};

PropResult propagate(const Inst& J) {
  int n = J.n; Sweeper S(J);
  Quad Q[2]; Q[0].init(J, S.byA, S.byB, 0); Q[1].init(J, S.byA, S.byB, 1);
  const vector<int>* ord[2] = {&S.byC, &S.byD};
  vector<int> lab(n, -1);
  PropResult R; R.husb.assign(n, -1);
  int p[2] = {0, 0};
  while (p[0] < n || p[1] < n) {
    bool moved = false;
    for (int k = 0; k < 2; k++) {
      while (p[k] < n) {
        int w = (*ord[k])[p[k]], wt = J.tw[w];
        int own = Q[k].best(wt);
        if (lab[w] != -1) {  // decided by the other frontier
          if (lab[w] == k && own != -1) { R.husb[w] = own; Q[k].take(wt, own); }
          p[k]++; moved = true; continue;
        }
        int opt = Q[1 - k].best(wt);  // optimistic bound on her other option
        if (own != -1 && (opt == -1 || wr(J, w, own) < wr(J, w, opt))) {
          lab[w] = k; R.husb[w] = own; Q[k].take(wt, own); p[k]++; moved = true;
        } else if (own == -1) {
          lab[w] = 1 - k; p[k]++; moved = true;
        } else break;
      }
    }
    if (!moved) break;
  }
  if (p[0] == n && p[1] == n) { R.complete = true; return R; }
  for (int w = 0; w < n; w++) if (lab[w] == -1) R.resW.push_back(w);
  for (int m = 0; m < n; m++) {
    int t = J.tm[m];
    if (J.a[m] > Q[t].al && J.b[m] > Q[t].be) R.resM.push_back(m);
  }
  // at a stall both quadrants are nonempty, so no woman was decided with an empty option
  for (int w = 0; w < n; w++) if (lab[w] != -1 && R.husb[w] == -1) { fprintf(stderr, "propagate: pending forced woman at stall\n"); exit(1); }
  if (R.resM.size() != R.resW.size()) { fprintf(stderr, "propagate: residual sizes differ\n"); exit(1); }
  return R;
}
