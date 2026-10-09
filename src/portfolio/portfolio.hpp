// Fallback portfolio for best.cpp (portfolio update, 2026-10-05). Methods, all in one lockstep with one
// shared work counter (unit: one person visited in one linear pass):
//  - RBGS(settle): run-buffer implementation's run-buffer GS with settled-women deletion (rbgs.cpp runS,
//    live file, read only), 11 priority rules x {atomic, pool} x {men, women proposing} = 44 runs.
//    Work of one run = n + Ch (the run-buffer implementation cost: unit acceptances; each costs O(log n) time).
//  - C3: bulk sweep iteration from the top and from the bottom, women sweeps and men sweeps
//    (dual instance): 4 runs. Work of one round = n.
//  - C8: alternating exact completions from both starts (all Y-type women X-labeled / Y-labeled):
//    2 runs. Work of one iteration = 2n.
// Lockstep on real costs with doubling budgets (see portfolio() below); RBGS runs use run-buffer implementation's
// abort budget tc_rbgs::gAbortWork (added 2026-10-05) and are restarted with a doubled budget.
// Every candidate output is checked by an O(n) stability test before it can win.
#pragma once
#include <stdexcept>
namespace tc_rbgs {
#define main tc_rbgs_unused_main
#define exit(x) throw std::runtime_error("rbgs exit")
#include "../rbgs/rbgs.cpp"
#undef exit
#undef main
}  // namespace tc_rbgs

// O(n) stability test of wife (man -> woman): for each of the 4 (man type, woman type) pairs, a
// blocking pair is m, w with pos_m(w) < pos_m(mu(m)) and rank_w(m) < rank_w(mu(w)).
bool stableFast(const Inst& J, const vector<int>& wife) {
  int n = J.n; vector<int> husb(n, -1);
  for (int m = 0; m < n; m++) { if (wife[m] < 0 || wife[m] >= n || husb[wife[m]] != -1) return false; husb[wife[m]] = m; }
  for (int tm = 0; tm < 2; tm++) for (int tw = 0; tw < 2; tw++) {
    const vector<int>& P = tm == 0 ? J.c : J.d;  // men of type tm rank women by P
    const vector<int>& R = tw == 0 ? J.a : J.b;  // women of type tw rank men by R
    vector<int> best(n + 1, -1);  // best[u] = max over women w of type tw with P[w] < u of R[husb[w]]
    for (int w = 0; w < n; w++) if (J.tw[w] == tw) best[P[w] + 1] = max(best[P[w] + 1], R[husb[w]]);
    for (int u = 1; u <= n; u++) best[u] = max(best[u], best[u - 1]);
    for (int m = 0; m < n; m++) if (J.tm[m] == tm && best[P[wife[m]]] > R[m]) return false;
  }
  return true;
}

struct C8Run {  // alternating completions (C8), one iteration per step
  const Inst& I; Sweeper S; int n; vector<int> LY, lab, lab2, part; bool done = false, failed = false; int iters = 0;
  C8Run(const Inst& I_, int start) : I(I_), S(I_), n(I_.n), LY(I_.n, start) {}
  void step() {
    iters++;
    if (!complete(I, S, LY, 0, lab, part) || !complete(I, S, lab, 1, lab2, part)) { failed = true; return; }
    bool same = true;
    for (int w = 0; w < n; w++) if (I.tw[w] == 1 && lab2[w] != LY[w]) same = false;
    if (same) { done = true; return; }
    for (int w = 0; w < n; w++) if (I.tw[w] == 1) LY[w] = lab2[w];
  }
  vector<int> wife() { vector<int> wf(n, -1); for (int w = 0; w < n; w++) if (part[w] >= 0) wf[part[w]] = w; return wf; }
};

int rbMode = 1;  // 1: runT (the run-buffer implementation RB11 dead-for-type mode QMT, newest); 0: runS (RB7 settle)
int rbFast = 1;  // 1: run-buffer implementation's FAST=1 runT (one-pass dead declarations, lazy dead checks; 2026-10-05)

struct PortfolioResult { vector<int> wife; string winner; long winWork = 0, total = 0, actual = 0; int rejected = 0; };
// True lockstep on REAL costs (2026-10-05, with run-buffer implementation's abort budget gAbortWork in runS/runT):
// phases with budget B = 4n, 8n, 16n, ...; in a phase every steppable run (C3 x4, C8 x2) advances
// until it has used B, and every unfinished RBGS run is restarted from scratch with gAbortWork = B
// (it aborts once (Ch + rebuilds) n > B). The first phase in which some run finishes with a stable
// output ends the race; the winner is the finished run with the least work.
// Work of an RBGS run = max(gRealWork, (Ch + rebuilds + 1) n) (prototype cost; an aborted attempt
// costs B). actual = all work really done (restarts included, at most about 2x the lockstep);
// total = sum over runs of min(own work so far, winner work) (the ideal lockstep figure).
PortfolioResult portfolio(const Inst& J) {
  if (rbFast) setenv("FAST", "1", 1); else unsetenv("FAST");
  int n = J.n;
  PortfolioResult PR; long best = LONG_MAX;
  tc_rbgs::Inst A; A.n = n; A.a = J.a; A.b = J.b; A.c = J.c; A.d = J.d; A.tm = J.tm; A.tw = J.tw;
  tc_rbgs::Inst T = A; T.a = A.c; T.b = A.d; T.c = A.a; T.d = A.b; T.tm = A.tw; T.tw = A.tm;
  const char* pols[] = {"lifo", "fifo", "pa", "pb", "pmin", "pmax", "ra", "rb", "rmin", "rmax", "prand"};
  struct RBRun { int dir; const char* pol; int at; bool live = true; long spent = 0; };
  vector<RBRun> RB;
  for (int dir = 0; dir < 2; dir++) for (const char* p : pols) for (int at = 0; at < 2; at++) RB.push_back({dir, p, at});
  vector<unique_ptr<C3Run>> C3; vector<unique_ptr<C8Run>> C8;
  const char* c3nm[4] = {"C3 top", "C3 bottom", "C3 dual top", "C3 dual bottom"};
  for (int v = 0; v < 4; v++) C3.emplace_back(new C3Run(J, v >= 2, v % 2 == 0));
  for (int s = 0; s < 2; s++) C8.emplace_back(new C8Run(J, s));
  int K = 6; vector<long> used(K, 0); vector<char> live(K, 1);
  auto offer = [&](long w, const vector<int>& wf, const string& name) {
    if (!stableFast(J, wf)) { PR.rejected++; return; }
    if (w < best) { best = w; PR.wife = wf; PR.winner = name; }
  };
  for (long B = 4L * n; best == LONG_MAX; B *= 2) {
    for (int k = 0; k < K; k++) {
      while (live[k] && used[k] < B) {
        if (k < 4) {
          C3[k]->step(); used[k] += n;
          if (C3[k]->done) { live[k] = 0; offer(used[k], C3[k]->wife(), string(c3nm[k]) + " rounds " + to_string(C3[k]->rounds)); }
        } else {
          C8Run& R = *C8[k - 4]; R.step(); used[k] += 2L * n;
          if (R.failed) live[k] = 0;  // deadlock: never finishes
          else if (R.done) { live[k] = 0; offer(used[k], R.wife(), string("C8 start ") + (k == 4 ? "X" : "Y") + " iterations " + to_string(R.iters)); }
        }
      }
    }
    for (RBRun& Q : RB) {
      if (!Q.live) continue;
      long long rb = 0;
      try {
        tc_rbgs::rng.seed(12345); tc_rbgs::gAbortWork = B;
        vector<int> h; int nu; int sw;
        tc_rbgs::Res r = rbMode == 0 ? tc_rbgs::runS(Q.dir == 0 ? A : T, Q.pol, Q.at, h, nu, rb, sw) : tc_rbgs::runT(Q.dir == 0 ? A : T, Q.pol, Q.at, h, nu, rb);
        long w = max((long)tc_rbgs::gRealWork, (long)(r.Ch + rb + 1) * n);
        Q.spent += w; Q.live = false;
        vector<int> wf(n, -1);  // h[w] = man (men proposing); in T, h[m] = woman
        if (Q.dir == 0) { for (int x = 0; x < n; x++) if (h[x] >= 0) wf[h[x]] = x; } else { for (int m = 0; m < n; m++) wf[m] = h[m]; }
        offer(w, wf, string(rbMode ? "RB(deadT) " : "RB(settle) ") + (Q.dir ? "women" : "men") + "-proposing " + Q.pol + (Q.at ? "/atomic" : "/pool"));
      } catch (std::exception&) { Q.spent += B; }  // aborted at the budget (or an internal error)
    }
    if (B > (1L << 40)) break;  // safety
  }
  tc_rbgs::gAbortWork = LLONG_MAX;
  if (best == LONG_MAX) return PR;
  PR.winWork = best;
  for (int k = 0; k < K; k++) { PR.actual += used[k]; PR.total += min(used[k], best); }
  for (RBRun& Q : RB) { PR.actual += Q.spent; PR.total += min(Q.spent, best); }
  return PR;
}
