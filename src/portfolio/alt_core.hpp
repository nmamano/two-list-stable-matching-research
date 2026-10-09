// Shared: quadrant pointers, one-type completion (C6), alternating iteration (C8).
#pragma once
struct Quad {  // remaining men of type t as a quadrant; pointers to min-a and min-b
  const Inst* I; const vector<int>* byA; const vector<int>* byB; int t, n;
  int al = -1, be = -1, pa = 0, pb = 0;
  void init(const Inst& I_, const vector<int>& A, const vector<int>& B, int t_) {
    I = &I_; byA = &A; byB = &B; t = t_; n = I_.n; al = be = -1; pa = pb = 0; norm();
  }
  void norm() {
    while (pa < n && (I->tm[(*byA)[pa]] != t || I->b[(*byA)[pa]] <= be || pa <= al)) pa++;
    while (pb < n && (I->tm[(*byB)[pb]] != t || I->a[(*byB)[pb]] <= al || pb <= be)) pb++;
  }
  int minA() const { return pa < n ? (*byA)[pa] : -1; }
  int minB() const { return pb < n ? (*byB)[pb] : -1; }
  int best(int wtype) const { return wtype == 0 ? minA() : minB(); }
  void take(int wtype, int m) { if (wtype == 0) al = I->a[m]; else be = I->b[m]; norm(); }
};

// returns false on deadlock/infeasible; lab[w] = 0 (X-man) / 1 (Y-man); opt[k][w] = option in sweep k
bool complete(const Inst& I, const Sweeper& S, const vector<int>& fixedLab, int flex,
              vector<int>& lab, vector<int>& part) {
  int n = I.n;
  Quad Q[2];
  Q[0].init(I, S.byA, S.byB, 0); Q[1].init(I, S.byA, S.byB, 1);
  const vector<int>* ord[2] = {&S.byC, &S.byD};
  lab.assign(n, -1); part.assign(n, -1);
  int p[2] = {0, 0};
  while (p[0] < n || p[1] < n) {
    bool moved = false;
    for (int k = 0; k < 2; k++) {
      while (p[k] < n) {
        int w = (*ord[k])[p[k]], wt = I.tw[w];
        int own = Q[k].best(wt);
        if (lab[w] != -1) {
          if (lab[w] == k && own != -1) { part[w] = own; Q[k].take(wt, own); }
          p[k]++; moved = true; continue;
        }
        if (wt != flex) {
          lab[w] = fixedLab[w];
          if (lab[w] == k && own != -1) { part[w] = own; Q[k].take(wt, own); }
          p[k]++; moved = true; continue;
        }
        int opt = Q[1 - k].best(wt);
        if (own != -1 && (opt == -1 || wr(I, w, own) < wr(I, w, opt))) {
          lab[w] = k; part[w] = own; Q[k].take(wt, own); p[k]++; moved = true;
        } else if (own == -1) {
          lab[w] = 1 - k; p[k]++; moved = true;
        } else break;
      }
    }
    if (!moved) return false;
  }
  return true;
}

bool stable_brute(const Inst& I, const vector<int>& husb) {  // husb[w] = man
  int n = I.n;
  vector<int> wife(n, -1);
  for (int w = 0; w < n; w++) { if (husb[w] < 0 || wife[husb[w]] != -1) return false; wife[husb[w]] = w; }
  for (int m = 0; m < n; m++) for (int w = 0; w < n; w++)
    if (wife[m] != w && mr(I, m, w) < mr(I, m, wife[m]) && wr(I, w, m) < wr(I, w, husb[w])) return false;
  return true;
}

// returns iterations (>0) or -1 on failure; husb output
int alt_iter(const Inst& I, int start, vector<int>& husb, int maxit = 100000) {
  int n = I.n; Sweeper S(I);
  vector<int> LY(n, start), lab, lab2, part;
  for (int it = 1; it <= maxit; it++) {
    if (!complete(I, S, LY, 0, lab, part)) return -1;
    if (!complete(I, S, lab, 1, lab2, part)) return -1;
    bool same = true;
    for (int w = 0; w < n; w++) if (I.tw[w] == 1 && lab2[w] != LY[w]) same = false;
    if (same) { husb = part; return it; }
    for (int w = 0; w < n; w++) if (I.tw[w] == 1) LY[w] = lab2[w];
  }
  return -2;
}

