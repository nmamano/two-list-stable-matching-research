// structured solver: bulk Tarski iteration of the sweep map (C3), O(n) per sweep (two-pointer quadrant).
// Usage: tarski n trials seed family
// family: 0 uniform; 1 men lists anti-corr noisy; 2 women lists anti-corr noisy; 3 both
// Prints rounds from top (-> MOSM) and from bottom (-> WOSM), checks vs plain GS.
#include <bits/stdc++.h>
using namespace std;

struct Inst {
  int n;
  vector<int> a, b, c, d, tm, tw;  // ranks and types (0 = X, 1 = Y)
};

Inst gen(int n, mt19937_64& rng, int fam) {
  Inst I; I.n = n;
  auto perm = [&]() { vector<int> p(n); iota(p.begin(), p.end(), 0); shuffle(p.begin(), p.end(), rng); return p; };
  I.a = perm(); I.b = perm(); I.c = perm(); I.d = perm();
  if (fam & 1) { for (int i = 0; i < n; i++) I.b[i] = n - 1 - I.a[i]; }
  if (fam & 2) { for (int i = 0; i < n; i++) I.d[i] = n - 1 - I.c[i]; }
  I.tm.resize(n); I.tw.resize(n);
  for (int i = 0; i < n; i++) { I.tm[i] = rng() & 1; I.tw[i] = rng() & 1; }
  return I;
}

// rank helpers
inline int mr(const Inst& I, int m, int w) { return I.tm[m] == 0 ? I.c[w] : I.d[w]; }
inline int wr(const Inst& I, int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; }

long g_gs_props = 0;  // proposals made by gs_plain (work counter for best.cpp)
vector<int> gs_plain(const Inst& I, bool menprop) {  // returns man -> woman
  int n = I.n;
  vector<vector<int>> P(n);
  for (int p = 0; p < n; p++) {
    P[p].resize(n); iota(P[p].begin(), P[p].end(), 0);
    if (menprop) sort(P[p].begin(), P[p].end(), [&](int x, int y) { return mr(I, p, x) < mr(I, p, y); });
    else sort(P[p].begin(), P[p].end(), [&](int x, int y) { return wr(I, p, x) < wr(I, p, y); });
  }
  vector<int> nxt(n, 0), held(n, -1), fr;
  for (int i = 0; i < n; i++) fr.push_back(i);
  while (!fr.empty()) {
    int p = fr.back(); fr.pop_back();
    int q = P[p][nxt[p]++]; g_gs_props++;
    int h = held[q];
    auto pref = [&](int x, int y) { return menprop ? wr(I, q, x) < wr(I, q, y) : mr(I, q, x) < mr(I, q, y); };
    if (h == -1) held[q] = p;
    else if (pref(p, h)) { held[q] = p; fr.push_back(h); }
    else fr.push_back(p);
  }
  vector<int> mu(n);
  if (menprop) { for (int w = 0; w < n; w++) mu[held[w]] = w; }
  else mu = held;
  return mu;
}

struct Sweeper {
  const Inst& I; int n;
  vector<int> byA, byB, byC, byD;  // men sorted by a, by b; women by c, by d
  Sweeper(const Inst& I_) : I(I_), n(I_.n) {
    byA.resize(n); byB.resize(n); byC.resize(n); byD.resize(n);
    for (int i = 0; i < n; i++) { byA[I.a[i]] = i; byB[I.b[i]] = i; byC[I.c[i]] = i; byD[I.d[i]] = i; }
  }
  // sweep over men of type t; women in c-order (t=0) or d-order (t=1).
  // other[w] = woman's option from the other sweep (-1 = none). take iff own option beats it.
  // writes opt[w] (her favorite remaining man of type t, -1 none) and took[w].
  void sweep(int t, const vector<int>& other, vector<int>& opt, vector<char>& took) {
    int al = -1, be = -1, pa = 0, pb = 0;
    const vector<int>& ord = t == 0 ? byC : byD;
    for (int k = 0; k < n; k++) {
      int w = ord[k];
      // min-a in quadrant
      while (pa < n && (I.tm[byA[pa]] != t || I.b[byA[pa]] <= be || pa <= al)) pa++;
      while (pb < n && (I.tm[byB[pb]] != t || I.a[byB[pb]] <= al || pb <= be)) pb++;
      int best = -1;
      if (I.tw[w] == 0) best = pa < n ? byA[pa] : -1;
      else best = pb < n ? byB[pb] : -1;
      opt[w] = best;
      bool tk = false;
      if (best != -1) {
        int o = other[w];
        tk = (o == -1) || wr(I, w, best) < wr(I, w, o);
      }
      took[w] = tk;
      if (tk) {
        if (I.tw[w] == 0) al = I.a[best]; else be = I.b[best];
      }
    }
  }
};

// returns rounds; mu output
int tarski(const Inst& I, bool fromTop, vector<int>& mu) {
  int n = I.n;
  Sweeper S(I);
  vector<int> xo(n), yo(n, -1), yo2(n);
  vector<char> tx(n), ty(n);
  if (!fromTop) {
    // bottom: nothing consumed in Y: y(w) = favorite Y-man overall
    vector<int> none(n, -1);
    // sweep with "never take" : other = special; emulate by computing opt with take disabled
    // simplest: run sweep with other = the man itself trick not available; do direct
    for (int w = 0; w < n; w++) {
      int best = -1;
      for (int m = 0; m < n; m++) if (I.tm[m] == 1 && (best == -1 || wr(I, w, m) < wr(I, w, best))) best = m;
      yo[w] = best;
    }
  }
  int rounds = 0;
  while (true) {
    rounds++;
    S.sweep(0, yo, xo, tx);
    S.sweep(1, xo, yo2, ty);
    if (yo2 == yo) break;
    yo = yo2;
  }
  mu.assign(n, -1);
  for (int w = 0; w < n; w++) {
    if (tx[w] && ty[w]) { fprintf(stderr, "both labels\n"); exit(1); }
    if (!tx[w] && !ty[w]) { fprintf(stderr, "no label\n"); exit(1); }
    mu[tx[w] ? xo[w] : yo2[w]] = w;
  }
  return rounds;
}

int main(int argc, char** argv) {
  int n = atoi(argv[1]), trials = atoi(argv[2]);
  mt19937_64 rng(atoll(argv[3]));
  int fam = argc > 4 ? atoi(argv[4]) : 0;
  bool check = n <= 2000;
  int mxT = 0, mxB = 0; double sT = 0, sB = 0;
  for (int t = 0; t < trials; t++) {
    Inst I = gen(n, rng, fam);
    vector<int> mT, mB;
    int rT = tarski(I, true, mT), rB = tarski(I, false, mB);
    if (check) {
      if (mT != gs_plain(I, true)) { printf("MISMATCH top\n"); return 1; }
      if (mB != gs_plain(I, false)) { printf("MISMATCH bottom\n"); return 1; }
    }
    mxT = max(mxT, rT); mxB = max(mxB, rB); sT += rT; sB += rB;
  }
  printf("n=%d fam=%d trials=%d rounds top avg %.2f max %d | bottom avg %.2f max %d\n", n, fam, trials, sT / trials, mxT, sB / trials, mxB);
  return 0;
}
