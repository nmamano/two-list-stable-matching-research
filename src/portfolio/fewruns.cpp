// structured solver, C9: few type runs in the men's lists => Tarski search in low dimension.
// The decisions of all women depend on the Y-sweep only through the a-list block index of its
// min-a man and the b-list block index of its min-b man (+ the time it becomes empty).
// These are monotone step functions given by D thresholds; F = Psi o Phi is monotone on [0,n+1]^D.
// Usage: fewruns n trials seed runsA runsB   (runs = number of type runs in that list)
#define NO_ROT_MAIN
#include "rot.cpp"
#include "alt_core.hpp"

struct FewRuns {
  const Inst& I; int n;
  Sweeper S;
  vector<int> blkA, blkB;        // global run index of each man in the a-list / b-list
  vector<int> yRunsA, yRunsB;    // run indices (in order) of Y-man runs in a-list / b-list
  vector<int> xRunsA, xRunsB;
  long evals = 0, cap = LONG_MAX;  // cap: give up (return false) after this many evaluations
  FewRuns(const Inst& I_) : I(I_), n(I_.n), S(I_) {
    blkA.assign(n, 0); blkB.assign(n, 0);
    auto runs = [&](const vector<int>& ord, vector<int>& blk, vector<int>& yr, vector<int>& xr) {
      int r = -1, prev = -1;
      for (int k = 0; k < n; k++) {
        int m = ord[k];
        if (I.tm[m] != prev) { r++; prev = I.tm[m]; (I.tm[m] ? yr : xr).push_back(r); }
        blk[m] = r;
      }
    };
    runs(S.byA, blkA, yRunsA, xRunsA); runs(S.byB, blkB, yRunsB, xRunsB);
    // position of each man's run inside the run list of his own type (for O(1) profile updates)
    runPosA.assign(n, 0); runPosB.assign(n, 0);
    for (int m = 0; m < n; m++) {
      const vector<int>& ra = I.tm[m] ? yRunsA : xRunsA; const vector<int>& rb = I.tm[m] ? yRunsB : xRunsB;
      runPosA[m] = lower_bound(ra.begin(), ra.end(), blkA[m]) - ra.begin();
      runPosB[m] = lower_bound(rb.begin(), rb.end(), blkB[m]) - rb.begin();
    }
  }
  vector<int> runPosA, runPosB;
  // A sweep's "profile": thresholds when its min-a enters each later run of its type in the
  // a-list, same for b, and the time it becomes empty. Encoded as vector<int> of size D.
  int dims(int t) const { const vector<int>& ra = t ? yRunsA : xRunsA; const vector<int>& rb = t ? yRunsB : xRunsB;
    const vector<int>& oa = t ? xRunsA : yRunsA;
    if (ra.empty() || oa.empty()) return 1 << 28;  // one man type is empty: variant not usable
    return (int)ra.size() - 1 + (int)rb.size() - 1 + 1; }
  // value of the step function at time s from thresholds
  static int stepval(const vector<int>& runs, const vector<int>& th, int off, int cnt, int s) {
    int k = 0;
    for (int j = 0; j < cnt; j++) if (th[off + j] <= s) k++;
    return runs[k];
  }
  // run one sweep of type t over women in its order, using the other sweep's profile `oth`.
  // returns own profile.
  // O(n + D) sweep (independent reviewer's remark applied 2026-10-05): the other profile is bucket-decoded once,
  // and the own profile is filled by monotone run pointers.
  vector<int> sweep(int t, const vector<int>& oth, vector<int>* husb = nullptr) {
    evals++;
    int o = 1 - t;
    const vector<int>& oRA = o ? yRunsA : xRunsA; const vector<int>& oRB = o ? yRunsB : xRunsB;
    int oa = (int)oRA.size() - 1, ob = (int)oRB.size() - 1;
    int oEmpty = oth[oa + ob];
    // decode: cntA[s] = number of a-thresholds <= s (any order of the thresholds is allowed)
    vector<int> cntA(n + 2, 0), cntB(n + 2, 0);
    for (int j = 0; j < oa; j++) if (oth[j] <= n) cntA[oth[j]]++;
    for (int j = 0; j < ob; j++) if (oth[oa + j] <= n) cntB[oth[oa + j]]++;
    for (int s = 1; s <= n; s++) { cntA[s] += cntA[s - 1]; cntB[s] += cntB[s - 1]; }
    const vector<int>& tRA = t ? yRunsA : xRunsA; const vector<int>& tRB = t ? yRunsB : xRunsB;
    int ta = (int)tRA.size() - 1, tb = (int)tRB.size() - 1;
    vector<int> prof(ta + tb + 1, n + 1);
    Quad Q; Q.init(I, S.byA, S.byB, t);
    const vector<int>& ord = t == 0 ? S.byC : S.byD;
    int curA = 0, curB = 0; bool empty = false;
    auto record = [&](int s) {  // state valid from time s
      if (empty) return;
      int ma = Q.minA(), mb = Q.minB();
      if (ma == -1) {  // empty = beyond every run
        for (int j = curA + 1; j <= ta; j++) prof[j - 1] = s;
        for (int j = curB + 1; j <= tb; j++) prof[ta + j - 1] = s;
        prof[ta + tb] = s; empty = true; return;
      }
      int ia = runPosA[ma], ib = runPosB[mb];
      for (; curA < ia; curA++) prof[curA] = s;
      for (; curB < ib; curB++) prof[ta + curB] = s;
    };
    record(0);
    for (int s = 0; s < n; s++) {
      int w = ord[s], wt = I.tw[w];
      int own = Q.best(wt);
      if (own != -1) {
        int os = o == 0 ? I.c[w] : I.d[w];
        bool take;
        if (os >= oEmpty) take = true;
        else {
          int oblk = wt == 0 ? oRA[cntA[os]] : oRB[cntB[os]];
          int myblk = wt == 0 ? blkA[own] : blkB[own];
          take = myblk < oblk;
        }
        if (take) { Q.take(wt, own); if (husb) (*husb)[w] = own; }
      }
      record(s + 1);
    }
    return prof;
  }
  vector<int> F(const vector<int>& thY) { return sweep(1, sweep(0, thY)); }

  // Nested binary-search Tarski (nested binary search). Coords < k of x are fixed. Precondition:
  // the box [lo, hi] on coords >= k maps into itself. Finds x with F(x)_j = x_j for all j >= k.
  bool tarski(vector<int>& x, vector<int> lo, vector<int> hi, int k) {
    int D = x.size();
    if (k == D) return true;
    while (lo[k] <= hi[k]) {
      int m = (lo[k] + hi[k]) / 2;
      x[k] = m;
      if (evals > cap) return false;
      if (!tarski(x, lo, hi, k + 1)) return false;
      vector<int> fx = F(x);
      if (fx[k] == m) return true;
      if (fx[k] > m) { lo[k] = m + 1; for (int j = k + 1; j < D; j++) lo[j] = x[j]; }
      else { hi[k] = m - 1; for (int j = k + 1; j < D; j++) hi[j] = x[j]; }
    }
    return false;
  }
};

Inst genRuns(int n, mt19937_64& rng, int runsA, int runsB) {
  Inst I; I.n = n;
  auto perm = [&]() { vector<int> p(n); iota(p.begin(), p.end(), 0); shuffle(p.begin(), p.end(), rng); return p; };
  I.c = perm(); I.d = perm();
  I.tw.resize(n); for (auto& t : I.tw) t = rng() & 1;
  int nx = n / 2;
  I.tm.assign(n, 1); for (int m = 0; m < nx; m++) I.tm[m] = 0;
  // a type pattern with exactly R runs: split X-men into ceil(R/2) groups, Y into floor/ceil
  auto pattern = [&](int R) {
    int startX = rng() & 1;
    int gx = (R + (startX ? 1 : 0)) / 2 + (R % 2 == 0 ? 0 : 0);
    // runs alternate starting with startX? type 0 : 1
    int cx = 0, cy = 0;
    for (int r = 0; r < R; r++) { int t = (r % 2 == 0) == (bool)startX ? 0 : 1; if (t == 0) cx++; else cy++; }
    (void)gx;
    // random composition of nx into cx positive parts, n-nx into cy parts
    auto comp = [&](int tot, int parts) { vector<int> cut; for (int i = 1; i < tot; i++) cut.push_back(i); shuffle(cut.begin(), cut.end(), rng); cut.resize(parts - 1); sort(cut.begin(), cut.end()); vector<int> sz; int prev = 0; for (int x : cut) { sz.push_back(x - prev); prev = x; } sz.push_back(tot - prev); return sz; };
    vector<int> sx = comp(nx, cx), sy = comp(n - nx, cy);
    vector<int> xs, ys; for (int m = 0; m < nx; m++) xs.push_back(m); for (int m = nx; m < n; m++) ys.push_back(m);
    shuffle(xs.begin(), xs.end(), rng); shuffle(ys.begin(), ys.end(), rng);
    vector<int> order; int ix = 0, iy = 0, px = 0, py = 0;
    for (int r = 0; r < R; r++) { int t = (r % 2 == 0) == (bool)startX ? 0 : 1;
      if (t == 0) { for (int k = 0; k < sx[ix]; k++) order.push_back(xs[px++]); ix++; }
      else { for (int k = 0; k < sy[iy]; k++) order.push_back(ys[py++]); iy++; } }
    vector<int> rank(n); for (int k = 0; k < n; k++) rank[order[k]] = k; return rank;
  };
  I.a = pattern(runsA); I.b = pattern(runsB);
  return I;
}

#ifndef NO_FR_MAIN
int main(int argc, char** argv) {
  int n = atoi(argv[1]); long trials = atol(argv[2]); mt19937_64 rng(atoll(argv[3]));
  int ra = atoi(argv[4]), rb = atoi(argv[5]);
  long ok = 0, bad = 0, fail = 0; long totalEvals = 0; int maxD = 0;
  for (long t = 0; t < trials; t++) {
    Inst I = genRuns(n, rng, ra, rb);
    FewRuns FR(I);
    int D = FR.dims(1);
    maxD = max(maxD, D);
    vector<int> x(D), lo(D, 0), hi(D, n + 1);
    if (!FR.tarski(x, lo, hi, 0)) { fail++; continue; }
    vector<int> husb(n, -1);
    vector<int> px = FR.sweep(0, x, &husb);
    vector<int> py = FR.sweep(1, px, &husb);
    if (py != x) { fail++; continue; }
    totalEvals += FR.evals;
    if (n <= 4000 ? stable_brute(I, husb) : true) ok++; else bad++;
  }
  printf("n=%d runsA=%d runsB=%d D(max)=%d trials=%ld ok=%ld unstable=%ld fail=%ld avg sweeps=%.1f\n", n, ra, rb, maxD, trials, ok, bad, fail, (double)totalEvals / max(1L, ok + bad));
}
#endif
