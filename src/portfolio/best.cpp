// structured solver structured solver: prefix split (P1), then per block the cheapest of
// C9 (4 variants: Y-men, X-men, Y-women, X-women runs) and C11 (women blocks, men blocks).
// Falls back to plain GS on a block when the smallest Tarski dimension exceeds --maxD.
// Usage: best instance.txt out_matching.txt [maxD=10] [reduce=1] [onesided=1]
// Tarski runs stop after evalCap = 20000 evaluations per block (then GS fallback).
// 2026-10-05: if D > maxD, a lockstep portfolio (portfolio.hpp): run-buffer implementation's RBGS(settle) x 44 orders,
// C3 x 4 (top/bottom, women/men sweeps), C8 x 2 starts; the least-work stable result wins.
// Build: g++ -O2 -o best best.cpp (includes ../rbgs.cpp read only, in a namespace).
// 2026-10-05: C13 one-sided prefix cuts (women-only or men-only) when the toolbox D > maxD.
// 2026-10-05: C12 reduction (forced propagation = iterated soulmate removal, O(n)) before the
// prefix split, applied again inside every prefix block (reduce=0 gives the old behavior).
// Prints per block: size, D of each method, chosen method, evaluations.
#define NO_FR_MAIN
#include "fewruns.cpp"
#include "lab_io.hpp"
#include "blocks_core.hpp"
#include "reduce.hpp"

// sub-instance on men set M and women set W (same size), ranks recomputed
Inst subInst(const Inst& I, const vector<int>& M, const vector<int>& W) {
  int k = M.size(); Inst J; J.n = k;
  auto rer = [&](const vector<int>& ids, const vector<int>& key) {
    vector<int> o(ids.size()); iota(o.begin(), o.end(), 0);
    sort(o.begin(), o.end(), [&](int x, int y) { return key[ids[x]] < key[ids[y]]; });
    vector<int> r(ids.size()); for (size_t i = 0; i < o.size(); i++) r[o[i]] = i; return r; };
  J.a = rer(M, I.a); J.b = rer(M, I.b); J.c = rer(W, I.c); J.d = rer(W, I.d);
  J.tm.resize(k); J.tw.resize(k);
  for (int i = 0; i < k; i++) { J.tm[i] = I.tm[M[i]]; J.tw[i] = I.tw[W[i]]; }
  return J;
}

// returns wife (man -> woman) of a stable matching of J, and fills method info
// one side has a single type: that side shares one list, serial dictatorship is stable.
// returns wife (man -> woman), or empty if both sides are mixed
vector<int> singleType(const Inst& J) {
  int n = J.n;
  bool wOne = count(J.tw.begin(), J.tw.end(), J.tw[0]) == n, mOne = count(J.tm.begin(), J.tm.end(), J.tm[0]) == n;
  vector<int> wife(n, -1);
  if (wOne) {  // all women rank men by one list: men in that order take their favorite free woman
    const vector<int>& key = J.tw[0] == 0 ? J.a : J.b;
    vector<int> men(n); iota(men.begin(), men.end(), 0); sort(men.begin(), men.end(), [&](int x, int y) { return key[x] < key[y]; });
    set<int> freeC, freeD; vector<int> byC(n), byD(n);
    for (int w = 0; w < n; w++) { freeC.insert(J.c[w]); freeD.insert(J.d[w]); byC[J.c[w]] = w; byD[J.d[w]] = w; }
    for (int m : men) { int w = J.tm[m] == 0 ? byC[*freeC.begin()] : byD[*freeD.begin()]; wife[m] = w; freeC.erase(J.c[w]); freeD.erase(J.d[w]); }
    return wife;
  }
  if (mOne) {  // all men rank women by one list: women in that order take their favorite free man
    const vector<int>& key = J.tm[0] == 0 ? J.c : J.d;
    vector<int> wom(n); iota(wom.begin(), wom.end(), 0); sort(wom.begin(), wom.end(), [&](int x, int y) { return key[x] < key[y]; });
    set<int> freeA, freeB; vector<int> byA(n), byB(n);
    for (int m = 0; m < n; m++) { freeA.insert(J.a[m]); freeB.insert(J.b[m]); byA[J.a[m]] = m; byB[J.b[m]] = m; }
    for (int w : wom) { int m = J.tw[w] == 0 ? byA[*freeA.begin()] : byB[*freeB.begin()]; wife[m] = w; freeA.erase(J.a[m]); freeB.erase(J.b[m]); }
    return wife;
  }
  return {};
}

// Global work counter for the lab (unit: one person visited in one linear pass): sweeps and
// evaluations count the block size each, propagation / splits / D computation count the size,
// plain-GS fallback counts its proposals.
// g_best_work = work really done (portfolio: RBGS at prototype cost); the two deltas turn it into the
// ideal lockstep count (RBGS at unit-charge model cost n + Ch) and the winner-only count.
long g_best_work = 0, g_best_winwork = 0, g_best_lockdelta = 0;
vector<int> gsFallback(const Inst& J) { long p0 = g_gs_props; vector<int> w = gs_plain(J, true); g_best_work += g_gs_props - p0; return w; }

// C3 portfolio (fallback before GS): bulk sweep iteration from the top and from the bottom, on the
// instance (women sweeps) and on its dual (men sweeps), run round-robin, each capped at capR rounds.
// Any converged run is a fixed point of the C3 map, so a stable matching (C2, C3).
struct C3Run {
  Inst I; unique_ptr<Sweeper> S; bool dual; int rounds = 0; bool done = false;
  vector<int> xo, yo, yo2; vector<char> tx, ty;
  C3Run(const Inst& J, bool dual_, bool top) : I(dual_ ? dualInst(J) : J), dual(dual_) {
    int n = I.n; S.reset(new Sweeper(I)); xo.assign(n, -1); yo.assign(n, -1); yo2.assign(n, -1); tx.assign(n, 0); ty.assign(n, 0);
    if (!top) {  // bottom: no Y-man consumed, y(w) = her favorite Y-man overall
      int ma = -1, mb = -1;
      for (int k = 0; k < n && (ma == -1); k++) if (I.tm[S->byA[k]] == 1) ma = S->byA[k];
      for (int k = 0; k < n && (mb == -1); k++) if (I.tm[S->byB[k]] == 1) mb = S->byB[k];
      for (int w = 0; w < n; w++) yo[w] = I.tw[w] == 0 ? ma : mb;
    }
  }
  void step() {
    rounds++;
    S->sweep(0, yo, xo, tx); S->sweep(1, xo, yo2, ty);
    if (yo2 == yo) done = true; else yo = yo2;
  }
  vector<int> wife() {  // man -> woman of the original instance
    int n = I.n; vector<int> mu(n, -1);
    for (int w = 0; w < n; w++) mu[tx[w] ? xo[w] : yo2[w]] = w;  // mu: man of I -> woman of I
    if (!dual) return mu;
    vector<int> wf(n); for (int m = 0; m < n; m++) wf[mu[m]] = m;  // I-man = J-woman
    return wf;
  }
};
vector<int> c3portfolio(const Inst& J, int capR, string& info) {
  vector<unique_ptr<C3Run>> R;
  const char* nm[4] = {"C3 top", "C3 bottom", "C3 dual top", "C3 dual bottom"};
  for (int v = 0; v < 4; v++) R.emplace_back(new C3Run(J, v >= 2, v % 2 == 0));
  for (int r = 0; r < capR; r++)
    for (int v = 0; v < 4; v++) {
      R[v]->step();
      if (R[v]->done) {  // evals = rounds run over all four variants (one round = 2 sweeps, like one C9 eval)
        info += string(nm[v]) + " rounds " + to_string(R[v]->rounds) + " evals " + to_string(4 * r + v + 1);
        g_best_work += (long)(4 * r + v + 1) * J.n; return R[v]->wife(); }
    }
  g_best_work += (long)4 * capR * J.n;
  return {};
}

#include "portfolio.hpp"

long evalCap = 20000;  // Tarski evaluations per block before the GS fallback
vector<int> solveBlock(const Inst& J, int maxD, string& info) {
  int n = J.n;
  g_best_work += 7L * n;  // single-type test and the six D computations
  { vector<int> w1 = singleType(J); if (!w1.empty()) { info = "single-type side: serial dictatorship"; return w1; } }
  vector<Inst> V = {J, swapXY(J), dualInst(J), swapXY(dualInst(J))};
  const char* nm[6] = {"C9:Y-men", "C9:X-men", "C9:Y-women", "C9:X-women", "C11(FAS):women", "C11(FAS):men"};
  int D[6];
  for (int k = 0; k < 4; k++) { FewRuns FR(V[k]); D[k] = FR.dims(1); }
  { BlockSolver B(J); D[4] = B.dims(); }
  { BlockSolver B(V[2]); D[5] = B.dims(); }
  int best = min_element(D, D + 6) - D;
  char buf[256];
  snprintf(buf, sizeof buf, "D: Ymen %d Xmen %d Ywomen %d Xwomen %d C11w %d C11m %d -> ", D[0], D[1], D[2], D[3], D[4], D[5]);
  info = buf;
  vector<int> wife(n, -1), husb(n, -1);
  if (D[best] > maxD) {  // fallback portfolio in lockstep (portfolio.hpp); plain GS only if all fail
    PortfolioResult PR = portfolio(J);
    if (!PR.wife.empty()) {
      g_best_work += PR.actual; g_best_winwork += PR.winWork - PR.actual; g_best_lockdelta += PR.total - PR.actual;
      if (PR.rejected) info += "[" + to_string(PR.rejected) + " unstable candidate outputs rejected] ";
      info += "portfolio winner " + PR.winner + " work " + to_string(PR.winWork) + " lockstep total " + to_string(PR.total) +
              " evals " + to_string((PR.total + n - 1) / n);
      return PR.wife;
    }
    info += "portfolio failed, GS fallback"; return gsFallback(J);
  }
  info += nm[best];
  long ev = 0;
  if (best < 4) {
    const Inst& K = V[best]; FewRuns FR(K); FR.cap = evalCap; int d = FR.dims(1);
    vector<int> x(d), lo(d, 0), hi(d, n + 1);
    if (!FR.tarski(x, lo, hi, 0)) { info += " FAILED, GS"; return gsFallback(J); }
    vector<int> px = FR.sweep(0, x, &husb); FR.sweep(1, px, &husb); ev = FR.evals;
  } else {
    BlockSolver B(best == 4 ? J : V[2]); B.cap = evalCap;
    if (!B.solve(husb)) { info += " FAILED, GS"; return gsFallback(J); }
    ev = B.evals;
  }
  bool dualSide = (best == 2 || best == 3 || best == 5);
  if (!dualSide) { for (int w = 0; w < n; w++) wife[husb[w]] = w; } else { for (int w = 0; w < n; w++) wife[w] = husb[w]; }
  info += " evals " + to_string(ev);
  g_best_work += ev * n;
  return wife;
}

// P1 prefix split of J: list of (local men, local women) blocks in order
vector<pair<vector<int>, vector<int>>> prefixBlocks(const Inst& J) {
  int n = J.n;
  vector<int> byA(n), byB(n), byC(n), byD(n);
  for (int i = 0; i < n; i++) { byA[J.a[i]] = i; byB[J.b[i]] = i; byC[J.c[i]] = i; byD[J.d[i]] = i; }
  vector<int> cntM(n, 0), cntW(n, 0); int bothM = 0, bothW = 0, prev = 0;
  vector<pair<vector<int>, vector<int>>> out;
  for (int k = 0; k < n; k++) {
    if (++cntM[byA[k]] == 2) bothM++;
    if (++cntM[byB[k]] == 2) bothM++;
    if (++cntW[byC[k]] == 2) bothW++;
    if (++cntW[byD[k]] == 2) bothW++;
    if (bothM == k + 1 && bothW == k + 1) {
      out.push_back({vector<int>(byA.begin() + prev, byA.begin() + k + 1), vector<int>(byC.begin() + prev, byC.begin() + k + 1)});
      prev = k + 1;
    }
  }
  return out;
}

struct Stats { long decidedW = 0; int rounds = 0, maxDepth = 0, oneSided = 0; };
bool useReduce = true, useOneSided = true, quiet = false;

// smallest Tarski dimension of the toolbox on J (0 for a single-type side)
int toolboxD(const Inst& J) {
  if (!singleType(J).empty()) return 0;
  vector<Inst> V = {J, swapXY(J), dualInst(J), swapXY(dualInst(J))};
  int best = INT_MAX;
  for (int k = 0; k < 4; k++) { FewRuns FR(V[k]); best = min(best, FR.dims(1)); }
  { BlockSolver B(J); best = min(best, B.dims()); }
  { BlockSolver B(V[2]); best = min(best, B.dims()); }
  return best;
}

// One-sided prefix cut (C13): k with top-k women by c = top-k by d (side 0), or top-k men by a =
// top-k by b (side 1). Dummy people (padding) are last in their lists; a cut must leave a real
// person of that side on both sides of the cut. Returns the most balanced cut, {0, -1} if none.
pair<int, int> oneSidedCut(const Inst& J, const vector<char>& dM, const vector<char>& dW) {
  int n = J.n;
  vector<int> byA(n), byB(n), byC(n), byD(n);
  for (int i = 0; i < n; i++) { byA[J.a[i]] = i; byB[J.b[i]] = i; byC[J.c[i]] = i; byD[J.d[i]] = i; }
  int realM = n - count(dM.begin(), dM.end(), 1), realW = n - count(dW.begin(), dW.end(), 1);
  vector<int> cntM(n, 0), cntW(n, 0); int bothM = 0, bothW = 0;
  pair<int, int> best = {0, -1}; int bestScore = 0;
  for (int k = 1; k < n; k++) {
    if (++cntM[byA[k - 1]] == 2) bothM++;
    if (++cntM[byB[k - 1]] == 2) bothM++;
    if (++cntW[byC[k - 1]] == 2) bothW++;
    if (++cntW[byD[k - 1]] == 2) bothW++;
    int score = min(k, n - k);
    if (bothW == k && k < realW && score > bestScore) { bestScore = score; best = {k, 0}; }
    if (bothM == k && k < realM && score > bestScore) { bestScore = score; best = {k, 1}; }
  }
  return best;
}

// Padded first part of a women cut at k (C13): women W1 (= top-k by c and by d) plus dummy women
// after them in both lists; men S = men in the top-k of a OR of b (|S| >= k). Every stable matching
// matches X-type W1 women into top-k(a) and Y-type W1 women into top-k(b), so other men are not needed.
Inst padWomen(const Inst& J, const vector<int>& W1, const vector<int>& S) {
  int k = W1.size(), N = S.size(); Inst P; P.n = N;
  auto rer = [&](const vector<int>& key) { vector<int> o(N); iota(o.begin(), o.end(), 0);
    sort(o.begin(), o.end(), [&](int x, int y) { return key[S[x]] < key[S[y]]; });
    vector<int> r(N); for (int i = 0; i < N; i++) r[o[i]] = i; return r; };
  P.a = rer(J.a); P.b = rer(J.b); P.tm.resize(N); for (int i = 0; i < N; i++) P.tm[i] = J.tm[S[i]];
  P.c.resize(N); P.d.resize(N); P.tw.resize(N);
  for (int i = 0; i < N; i++) {
    if (i < k) { P.c[i] = J.c[W1[i]]; P.d[i] = J.d[W1[i]]; P.tw[i] = J.tw[W1[i]]; }
    else { P.c[i] = P.d[i] = i; P.tw[i] = 0; }
  }
  return P;
}

// solve the sub-instance of I on men M and women W (ids of I, same size); writes wife[] (ids of I).
// dumM / dumW mark padding people of I (indexed by ids of I).
void solveRec(const Inst& I, vector<int> M, vector<int> W, vector<int>& wife, int maxD, Stats& st, int depth,
              const vector<char>& dumM, const vector<char>& dumW) {
  st.maxDepth = max(st.maxDepth, depth);
  if (useReduce && !M.empty()) {  // C12: forced propagation = iterated soulmate removal, to exhaustion
    st.rounds++;
    Inst J = subInst(I, M, W);
    PropResult R = propagate(J); g_best_work += 2L * J.n;  // sub-instance + propagation
    for (int w = 0; w < J.n; w++) if (R.husb[w] != -1) { wife[M[R.husb[w]]] = W[w]; st.decidedW++; }
    if (R.complete) return;
    vector<int> M2, W2; for (int m : R.resM) M2.push_back(M[m]); for (int w : R.resW) W2.push_back(W[w]);
    M = M2; W = W2;
  }
  if (M.empty()) return;
  Inst J = subInst(I, M, W);
  int n = J.n;
  auto blocks = prefixBlocks(J); g_best_work += 2L * n;
  if (blocks.size() > 1 && useReduce) {
    for (auto& bl : blocks) {
      vector<int> M2, W2; for (int m : bl.first) M2.push_back(M[m]); for (int w : bl.second) W2.push_back(W[w]);
      solveRec(I, M2, W2, wife, maxD, st, depth + 1, dumM, dumW);
    }
    return;
  }
  if (useOneSided && (g_best_work += 7L * n, toolboxD(J) > maxD)) {  // C13: one-sided cut, first part padded, then the rest
    vector<char> dM(n), dW(n);
    for (int i = 0; i < n; i++) { dM[i] = dumM[M[i]]; dW[i] = dumW[W[i]]; }
    auto [k, side] = oneSidedCut(J, dM, dW);
    if (k > 0) {
      st.oneSided++; g_best_work += 2L * n;  // cut search and padded instance
      vector<int> M2, W2;
      // a men cut is a women cut of the dual instance: work on Jc = J or dual(J)
      Inst Jc = side == 0 ? J : dualInst(J);
      const vector<char>& dWc = side == 0 ? dW : dM;  // flags of Jc's women / men
      const vector<char>& dMc = side == 0 ? dM : dW;
      vector<int> W1(k), S;
      for (int w = 0; w < n; w++) if (Jc.c[w] < k) W1[Jc.c[w]] = w;
      for (int m = 0; m < n; m++) if (Jc.a[m] < k || Jc.b[m] < k) S.push_back(m);
      int N = S.size();
      Inst P = padWomen(Jc, W1, S);
      vector<char> pdW(N, 1), pdM(N);
      for (int i = 0; i < k; i++) pdW[i] = dWc[W1[i]];
      for (int i = 0; i < N; i++) pdM[i] = dMc[S[i]];
      if (!quiet) printf("one-sided %s cut at %d of %d, padded part size %d (depth %d)\n", side == 0 ? "women" : "men", k, n, N, depth);
      vector<int> idsP(N); iota(idsP.begin(), idsP.end(), 0);
      vector<int> wifeP(N, -1);
      // P is in Jc orientation; solve it in the original orientation (dual again for a men cut)
      if (side == 0) solveRec(P, idsP, idsP, wifeP, maxD, st, depth + 1, pdM, pdW);
      else {
        Inst Pd = dualInst(P); vector<int> wd(N, -1);
        solveRec(Pd, idsP, idsP, wd, maxD, st, depth + 1, pdW, pdM);
        for (int i = 0; i < N; i++) wifeP[wd[i]] = i;  // wd: Pd-man (= P-woman) -> Pd-woman (= P-man)
      }
      // wifeP: P-man (index into S) -> P-woman (< k: Jc-woman W1[.])
      vector<char> usedS(n, 0), inW1(n, 0);
      for (int i = 0; i < k; i++) inW1[W1[i]] = 1;
      for (int i = 0; i < N; i++) if (wifeP[i] < k) {
        int jm = S[i], jw = W1[wifeP[i]];  // Jc ids
        usedS[jm] = 1;
        if (side == 0) wife[M[jm]] = W[jw]; else wife[M[jw]] = W[jm];  // in dual(J), Jc-woman = J-man
      }
      // residual: Jc-women not in W1, Jc-men not used
      if (side == 0) {
        for (int m = 0; m < n; m++) if (!usedS[m]) M2.push_back(M[m]);
        for (int w = 0; w < n; w++) if (!inW1[w]) W2.push_back(W[w]);
      } else {
        for (int w = 0; w < n; w++) if (!inW1[w]) M2.push_back(M[w]);
        for (int m = 0; m < n; m++) if (!usedS[m]) W2.push_back(W[m]);
      }
      solveRec(I, M2, W2, wife, maxD, st, depth + 1, dumM, dumW);
      return;
    }
  }
  string info;
  vector<int> wk = solveBlock(J, maxD, info);
  if (!quiet) printf("block size %d (depth %d): %s\n", n, depth, info.c_str());
  for (int i = 0; i < n; i++) wife[M[i]] = W[wk[i]];
}

#ifndef NO_BEST_MAIN
int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "usage: best INPUT OUTPUT [maxD] [reduce] [onesided]\n"); return 2; }
  g_best_work = 0; g_best_winwork = 0; g_best_lockdelta = 0; g_blockctor_work = 0;  // the lab calls main repeatedly (as best_main)
  Inst I = loadLab(argv[1]); int n = I.n;
  int maxD = argc > 3 ? atoi(argv[3]) : 10;
  useReduce = argc > 4 ? atoi(argv[4]) != 0 : true;
  vector<int> M(n), W(n); iota(M.begin(), M.end(), 0); iota(W.begin(), W.end(), 0);
  vector<int> wife(n, -1);
  useOneSided = argc > 5 ? atoi(argv[5]) != 0 : true;
  Stats st; vector<char> none(n, 0);
  solveRec(I, M, W, wife, maxD, st, 0, none, none);
  if (useReduce) printf("C12: %ld soulmate pairs removed, %d propagation calls; C13: %d one-sided cuts; recursion depth %d\n", st.decidedW, st.rounds, st.oneSided, st.maxDepth);
  g_best_work += g_blockctor_work;
  long W2 = g_best_work + g_best_winwork, W3 = g_best_work + g_best_lockdelta;
  printf("work units %ld (%.1f per person); with portfolio blocks counted at the winner's work only: %ld (%.1f per person); ideal lockstep: %ld (%.1f per person)\n",
         g_best_work, (double)g_best_work / max(1, n), W2, (double)W2 / max(1, n), W3, (double)W3 / max(1, n));
  FILE* f = fopen(argv[2], "w"); if (!f) { perror("output"); return 2; } for (int m = 0; m < n; m++) fprintf(f, "%d\n", wife[m]); fclose(f);
  return 0;  // needed: the lab renames main to best_main, where falling off the end is undefined
}
#endif
