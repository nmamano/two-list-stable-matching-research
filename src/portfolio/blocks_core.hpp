// C11 block solver (women blocks with equal order in c and d) and k-block generator.
// 2026-10-05 v2: guessed states chosen as a minimum feedback arc set of the block port graph
// (X-edge from the c-predecessor block, Y-edge from the d-predecessor block), idea from run-buffer implementation
// (D3); exact subset DP for <= 20 blocks, else the better of c-order and d-order processing.
// Quadrant queries via segment trees: O(n log n) per evaluation.
#pragma once

struct QuadIndex {  // men of type t; first man in a-order with a >= A and b >= B (and mirror)
  int n, t, sz; vector<int> treeA, treeB, manAtA, manAtB;  // max b over a-order, max a over b-order
  const Inst* I;
  void build(const Inst& I_, int t_) {
    I = &I_; n = I_.n; t = t_; sz = 1; while (sz < n) sz <<= 1;
    treeA.assign(2 * sz, -1); treeB.assign(2 * sz, -1); manAtA.assign(n, -1); manAtB.assign(n, -1);
    for (int m = 0; m < n; m++) if (I_.tm[m] == t) {
      manAtA[I_.a[m]] = m; manAtB[I_.b[m]] = m;
      treeA[sz + I_.a[m]] = I_.b[m]; treeB[sz + I_.b[m]] = I_.a[m];
    }
    for (int i = sz - 1; i >= 1; i--) { treeA[i] = max(treeA[2 * i], treeA[2 * i + 1]); treeB[i] = max(treeB[2 * i], treeB[2 * i + 1]); }
  }
  // first position p >= lo with tree value >= v; -1 if none
  static int first(const vector<int>& tr, int sz, int lo, int v) {
    if (lo >= sz) return -1;
    // walk: check right-side subtrees from leaf lo upward, then descend
    int i = lo + sz;
    if (tr[i] >= v) return lo;
    while (i > 1) {
      if ((i & 1) == 0 && tr[i + 1] >= v) { i = i + 1; while (i < sz) i = tr[2 * i] >= v ? 2 * i : 2 * i + 1; return i - sz; }
      i >>= 1;
    }
    return -1;
  }
  int minA(int A, int B) const { if (A > n - 1) return -1; int p = first(treeA, sz, max(A, 0), max(B, 0)); return p < 0 || p >= n ? -1 : manAtA[p]; }
  int minB(int A, int B) const { if (B > n - 1) return -1; int p = first(treeB, sz, max(B, 0), max(A, 0)); return p < 0 || p >= n ? -1 : manAtB[p]; }
};

long g_blockctor_work = 0;  // real cost of the feedback-arc-set setup (subset DP), for best.cpp
struct BlockSolver {
  const Inst& I; int n; Sweeper S;
  vector<vector<int>> blocks;      // women of each block in c-order (= d-order inside)
  vector<int> predX, predY;        // c-predecessor / d-predecessor block (-1 if first)
  vector<int> order;               // processing order (topological after cuts)
  vector<int> cutX, cutY;          // guess index of the cut X-edge / Y-edge into block b, or -1
  QuadIndex QI[2];
  long evals = 0, cap = LONG_MAX;  // cap: give up (return false) after this many evaluations
  BlockSolver(const Inst& I_) : I(I_), n(I_.n), S(I_) {
    vector<int> blk(n, -1);
    for (int k = 0; k < n; k++) {
      int w = S.byC[k];
      if (k > 0) { int p = S.byC[k - 1]; if (I.d[w] == I.d[p] + 1) { blk[w] = blk[p]; blocks[blk[w]].push_back(w); continue; } }
      blk[w] = blocks.size(); blocks.push_back({w});
    }
    int K = blocks.size();
    predX.assign(K, -1); predY.assign(K, -1);
    for (int b = 1; b < K; b++) predX[b] = b - 1;  // blocks were created in c-order
    vector<int> dOrd;
    for (int k = 0; k < n; k++) { int w = S.byD[k]; if (dOrd.empty() || dOrd.back() != blk[w]) dOrd.push_back(blk[w]); }
    for (int i = 1; i < K; i++) predY[dOrd[i]] = dOrd[i - 1];
    // choose order minimizing cut edges (edges u->b with u placed after b)
    auto costOrder = [&](const vector<int>& ord) { vector<int> pos(K); for (int i = 0; i < K; i++) pos[ord[i]] = i; int c = 0;
      for (int b = 0; b < K; b++) { if (predX[b] >= 0 && pos[predX[b]] > pos[b]) c++; if (predY[b] >= 0 && pos[predY[b]] > pos[b]) c++; } return c; };
    g_blockctor_work += K <= 20 ? (1L << K) * K : (long)K;
    if (K <= 20) {
      vector<int> f(1 << K, INT_MAX), choice(1 << K, -1);
      f[0] = 0;
      for (int Sm = 0; Sm < (1 << K); Sm++) if (f[Sm] != INT_MAX)
        for (int b = 0; b < K; b++) if (!(Sm >> b & 1)) {
          int c = f[Sm] + (predX[b] >= 0 && !(Sm >> predX[b] & 1)) + (predY[b] >= 0 && !(Sm >> predY[b] & 1));
          int T = Sm | 1 << b;
          if (c < f[T]) { f[T] = c; choice[T] = b; }
        }
      order.assign(K, 0); int Sm = (1 << K) - 1;
      for (int i = K - 1; i >= 0; i--) { int b = choice[Sm]; order[i] = b; Sm ^= 1 << b; }
    } else {
      vector<int> oc(K); iota(oc.begin(), oc.end(), 0);
      order = costOrder(oc) <= costOrder(dOrd) ? oc : dOrd;
    }
    vector<int> pos(K); for (int i = 0; i < K; i++) pos[order[i]] = i;
    cutX.assign(K, -1); cutY.assign(K, -1); int g = 0;
    for (int i = 0; i < K; i++) { int b = order[i];
      if (predX[b] >= 0 && pos[predX[b]] > pos[b]) cutX[b] = g++;
      if (predY[b] >= 0 && pos[predY[b]] > pos[b]) cutY[b] = g++; }
    QI[0].build(I, 0); QI[1].build(I, 1);
  }
  int dims() const { int g = 0; for (size_t b = 0; b < blocks.size(); b++) g += (cutX[b] >= 0) + (cutY[b] >= 0); return 2 * g; }
  // state = tight corner (A, B): type-t men with a >= A, b >= B; (n+1, n+1) = empty
  vector<int> F(const vector<int>& gs, vector<int>* husb = nullptr) {
    evals++;
    int K = blocks.size();
    vector<array<int, 2>> endX(K), endY(K);
    for (int b : order) {
      array<int, 2> st[2];
      st[0] = predX[b] < 0 ? array<int, 2>{0, 0} : cutX[b] >= 0 ? array<int, 2>{gs[2 * cutX[b]], gs[2 * cutX[b] + 1]} : endX[predX[b]];
      st[1] = predY[b] < 0 ? array<int, 2>{0, 0} : cutY[b] >= 0 ? array<int, 2>{gs[2 * cutY[b]], gs[2 * cutY[b] + 1]} : endY[predY[b]];
      for (int w : blocks[b]) {
        int wt = I.tw[w], opt[2];
        for (int k = 0; k < 2; k++) opt[k] = wt == 0 ? QI[k].minA(st[k][0], st[k][1]) : QI[k].minB(st[k][0], st[k][1]);
        if (opt[0] == -1 && opt[1] == -1) continue;
        int k = (opt[1] == -1 || (opt[0] != -1 && wr(I, w, opt[0]) < wr(I, w, opt[1]))) ? 0 : 1;
        int m = opt[k];
        if (wt == 0) st[k][0] = I.a[m] + 1; else st[k][1] = I.b[m] + 1;
        if (husb) (*husb)[w] = m;
      }
      for (int k = 0; k < 2; k++) {  // tighten
        int ma = QI[k].minA(st[k][0], st[k][1]), mb = QI[k].minB(st[k][0], st[k][1]);
        array<int, 2> e = ma == -1 ? array<int, 2>{n + 1, n + 1} : array<int, 2>{I.a[ma], I.b[mb]};
        (k == 0 ? endX : endY)[b] = e;
      }
    }
    vector<int> out(gs.size());
    for (int b = 0; b < K; b++) {
      if (cutX[b] >= 0) { out[2 * cutX[b]] = endX[predX[b]][0]; out[2 * cutX[b] + 1] = endX[predX[b]][1]; }
      if (cutY[b] >= 0) { out[2 * cutY[b]] = endY[predY[b]][0]; out[2 * cutY[b] + 1] = endY[predY[b]][1]; }
    }
    return out;
  }
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
  bool solve(vector<int>& husb) {
    int D = dims();
    vector<int> x(D), lo(D, 0), hi(D, n + 1);
    if (!tarski(x, lo, hi, 0)) return false;
    husb.assign(n, -1);
    vector<int> fx = F(x, &husb);
    return fx == x;
  }
};

Inst genBlocks(int n, int K, mt19937_64& rng) {
  Inst I = gen(n, rng, 0);
  // women: K blocks of random sizes, random block permutation, identity inside
  vector<int> cut; for (int i = 1; i < n; i++) cut.push_back(i); shuffle(cut.begin(), cut.end(), rng);
  cut.resize(K - 1); sort(cut.begin(), cut.end());
  vector<int> sz; int prev = 0; for (int x : cut) { sz.push_back(x - prev); prev = x; } sz.push_back(n - prev);
  vector<int> perm(K); iota(perm.begin(), perm.end(), 0); shuffle(perm.begin(), perm.end(), rng);
  // c-order: blocks 0..K-1 in order; d-order: blocks in order perm
  vector<int> start(K); int s = 0; for (int b = 0; b < K; b++) { start[b] = s; s += sz[b]; }
  vector<int> womenOfC(n); iota(womenOfC.begin(), womenOfC.end(), 0); shuffle(womenOfC.begin(), womenOfC.end(), rng);
  for (int r = 0; r < n; r++) I.c[womenOfC[r]] = r;
  int dpos = 0;
  for (int b : perm) for (int j = 0; j < sz[b]; j++) I.d[womenOfC[start[b] + j]] = dpos++;
  return I;
}

