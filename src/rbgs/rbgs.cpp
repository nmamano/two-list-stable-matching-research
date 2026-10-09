// rbgs.cpp: run-buffer GS (run-buffer implementation, 2026-10-05).
// Units = maximal runs of women that are consecutive in BOTH c-order and d-order (same relative
// order). Mode P: a run must also be pure (one woman type). Mode M: any types.
// Facts used (proved in docs/summary.md, "Run-buffer GS"): every man enters a unit at its first
// woman; the held women of a unit form a prefix; the unit accepts man m iff it has a free woman,
// or a(m) < max a over holders of its X-women, or b(m) < max b over holders of its Y-women.
// A pure unit is a sorted buffer (insert + pop max = O(log n)).
// Cost counters per GS order:
//   Au   = unit acceptances (= skip queries that find a unit; pure mode: time O((n+Au) log n))
//   Str  = sum over unit acceptances of the number of maximal adjacent stretches of the inner
//          GS chain (in pure units always 1)
//   Bnd  = sum over unit acceptances of the number of woman-type boundaries inside the shifted
//          positions (cost to keep adjacent-pair flags in a mixed unit)
//   Aex  = accepted proposals of the executed GS order (classic A)
// Usage: rbgs FILE... (rank-array format)  or  rbgs gen KIND n pxm pxw noise seed
#include "core.hpp"

vector<long long> gCustom;  // priority for policy "custom" (smaller first)
struct Res { long long Au = 0, Str = 0, Bnd = 0, Aex = 0, Ch = 0; };

struct Seg {  // first index >= p with mA[i] > x or mB[i] > y
  int N; vector<int> mA, mB;
  void init(int n) { N = 1; while (N < n) N <<= 1; mA.assign(2 * N, INT_MIN); mB.assign(2 * N, INT_MIN); }
  void set(int i, int va, int vb) {
    i += N; mA[i] = va; mB[i] = vb;
    for (i >>= 1; i; i >>= 1) mA[i] = max(mA[2 * i], mA[2 * i + 1]), mB[i] = max(mB[2 * i], mB[2 * i + 1]);
  }
  int find(int node, int l, int r, int p, int x, int y) {
    if (r <= p || (mA[node] <= x && mB[node] <= y)) return -1;
    if (r - l == 1) return l;
    int m = (l + r) / 2;
    int res = find(2 * node, l, m, p, x, y);
    if (res >= 0) return res;
    return find(2 * node + 1, m, r, p, x, y);
  }
  int first(int p, int x, int y) { return find(1, 0, N, p, x, y); }
};

const int INF = INT_MAX / 2;

// Unit modes: "P" pure runs, "M" runs (any types), "Q" pure modules (one woman type, an interval
// of the c-order AND of the d-order, internal orders arbitrary), "QM" per start position the
// longer of a pure module and a run. Kind 0 unit = sorted buffer (pure module or pure run):
// accepts m iff not full or key(m) < max key of holders; overflow = holder with max key.
// Kind 1 unit = mixed run: explicit inner GS (same internal order in c and d).
Res run(const Inst& I, const string& mode, const string& policy, bool atomic, vector<int>& heldOut, int& nunits) {
  int n = I.n;
  vector<int> wc(n), wd(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  vector<vector<int>> U;  // women of the unit (for kind 1: in common order)
  vector<int> kind, uid(n);
  bool useRun = mode == "P" || mode == "M" || mode == "QM";
  bool useMod = mode == "Q" || mode == "QM";
  for (int i = 0; i < n;) {
    // run from i
    int jr = i;
    if (useRun) {
      while (jr + 1 < n && I.d[wc[jr + 1]] == I.d[wc[jr]] + 1 && (mode != "P" ? true : I.tw[wc[jr + 1]] == I.tw[wc[i]])) jr++;
    }
    int jm = i;
    if (useMod) {
      int mn = I.d[wc[i]], mx = mn;
      for (int j = i + 1; j < n && I.tw[wc[j]] == I.tw[wc[i]]; j++) {
        mn = min(mn, I.d[wc[j]]); mx = max(mx, I.d[wc[j]]);
        if (mx - mn == j - i) jm = j;
      }
    }
    int j = max(jr, jm);
    vector<int> ws;
    for (int t = i; t <= j; t++) ws.push_back(wc[t]);
    bool pure = true;
    for (int w : ws) if (I.tw[w] != I.tw[ws[0]]) pure = false;
    for (int w : ws) uid[w] = U.size();
    U.push_back(ws);
    kind.push_back(pure ? 0 : 1);
    i = j + 1;
  }
  nunits = U.size();
  int K = U.size();
  vector<int> cnt(K, 0), sC(K), sD(K);
  for (int u = 0; u < K; u++) {
    sC[u] = INT_MAX; sD[u] = INT_MAX;
    for (int w : U[u]) sC[u] = min(sC[u], I.c[w]), sD[u] = min(sD[u], I.d[w]);
  }
  vector<int> held(n, -1);
  vector<set<pair<int, int>>> buf(K);  // kind 0: (key, man)
  Seg TC, TD;
  TC.init(n); TD.init(n);
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  auto refresh = [&](int u) {
    int ta = INT_MIN, tb = INT_MIN;
    int L = U[u].size();
    if (kind[u] == 0) {
      int mx = (int)buf[u].size() < L ? INF : buf[u].rbegin()->first;
      if (I.tw[U[u][0]] == 0) ta = mx; else tb = mx;
    } else if (cnt[u] < L) ta = tb = INF;
    else for (int w : U[u]) { if (I.tw[w] == 0) ta = max(ta, I.a[held[w]]); else tb = max(tb, I.b[held[w]]); }
    TC.set(sC[u], ta, tb); TD.set(sD[u], ta, tb);
  };
  for (int u = 0; u < K; u++) refresh(u);
  vector<int> ptr(n, 0);
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else if (policy == "ra") pr[m] = -I.a[m];
    else if (policy == "rb") pr[m] = -I.b[m];
    else if (policy == "rmin") pr[m] = -min(I.a[m], I.b[m]);
    else if (policy == "rmax") pr[m] = -max(I.a[m], I.b[m]);
    else if (policy == "custom") pr[m] = gCustom[m];
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  auto push = [&](int m) { if (policy == "lifo" || policy == "fifo") dq.push_back(m); else pq.push({pr[m], m}); };
  auto pop = [&]() -> int {
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() { return (policy == "lifo" || policy == "fifo") ? dq.empty() : pq.empty(); };
  for (int m = 0; m < n; m++) push(m);
  Res R;
  while (!empty()) {
    int m = pop();
    while (m >= 0) {
      bool X = I.tm[m] == 0;
      int pos = (X ? TC : TD).first(ptr[m], I.a[m], I.b[m]);
      if (pos < 0) { fprintf(stderr, "man %d unmatched: bug\n", m); exit(1); }
      int u = uid[X ? wc[pos] : wd[pos]];
      int L = U[u].size();
      R.Au++;
      int fallen = -1;
      if (kind[u] == 0) {
        int k = I.tw[U[u][0]] == 0 ? I.a[m] : I.b[m];
        buf[u].insert({k, m});
        R.Ch++; R.Aex++;
        if ((int)buf[u].size() > L) { fallen = prev(buf[u].end())->second; buf[u].erase(prev(buf[u].end())); }
      } else {
        bool chainOK = true;
        vector<pair<int, int>> pts;
        for (int w : U[u]) if (held[w] >= 0) pts.push_back({I.a[held[w]], I.b[held[w]]});
        pts.push_back({I.a[m], I.b[m]});
        sort(pts.begin(), pts.end());
        for (size_t i = 1; i < pts.size(); i++) if (pts[i].second < pts[i - 1].second) { chainOK = false; break; }
        R.Ch += chainOK ? 1 : L;
        int cur = m, last = -2, firstk = -1, lastk = -1;
        for (int k = 0; k < L; k++) {
          int w = U[u][k], h = held[w];
          if (h >= 0 && key(w, h) < key(w, cur)) continue;
          R.Aex++;
          if (k != last + 1) R.Str++;
          if (firstk < 0) firstk = k;
          lastk = k; last = k;
          held[w] = cur;
          if (h < 0) { cnt[u]++; cur = -1; break; }
          cur = h;
        }
        if (cur >= 0) fallen = cur;
        for (int k = firstk + 1; k <= lastk; k++) if (I.tw[U[u][k]] != I.tw[U[u][k - 1]]) R.Bnd++;
      }
      refresh(u);
      m = -1;
      if (fallen >= 0) {
        ptr[fallen] = (I.tm[fallen] == 0 ? sC[u] : sD[u]) + L;
        if (atomic) m = fallen; else push(fallen);
      }
    }
  }
  // final assignment inside kind-0 units: holders in key order, each takes the best remaining woman
  for (int u = 0; u < K; u++) if (kind[u] == 0) {
    vector<int> rem = U[u];
    for (auto& [k, m] : buf[u]) {
      int bi = -1;
      for (int i = 0; i < (int)rem.size(); i++) {
        int w = rem[i];
        if (bi < 0) { bi = i; continue; }
        int wb = rem[bi];
        if (I.tm[m] == 0 ? I.c[w] < I.c[wb] : I.d[w] < I.d[wb]) bi = i;
      }
      held[rem[bi]] = m; rem.erase(rem.begin() + bi);
    }
  }
  heldOut = held;
  return R;
}

// Budgeted RBGS (2026-10-05): run RBGS; whenever the unit work since the last bulk step reaches
// n, do a bulk step = complete an X-phase then a Y-phase (each = the unique outcome of a sub-market
// with a men master list, computable by one O(n log n) sweep; simulated explicitly here and charged
// n each). Work W = Ch + 2n * (number of bulk steps). gBulk = number of bulk steps.
long long gBulk;
// Abort budget and real-work counter for structured solver's lockstep portfolio (2026-10-05): runS and runT exit(2)
// when (Ch + rebuilds) * n > gAbortWork; gRealWork counts prototype operations (scans, rebuilds, steps).
long long gAbortWork = LLONG_MAX, gRealWork = 0;
vector<long long>* gUsize = nullptr;  // USIZE=1: histogram of unit sizes at entry (log2 buckets), runT
Res runB(const Inst& I, const string& mode, const string& policy, bool atomic, vector<int>& heldOut, int& nunits) {
  int n = I.n;
  vector<int> wc(n), wd(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  vector<vector<int>> U;  // women of the unit (for kind 1: in common order)
  vector<int> kind, uid(n);
  bool useRun = mode == "P" || mode == "M" || mode == "QM";
  bool useMod = mode == "Q" || mode == "QM";
  for (int i = 0; i < n;) {
    // run from i
    int jr = i;
    if (useRun) {
      while (jr + 1 < n && I.d[wc[jr + 1]] == I.d[wc[jr]] + 1 && (mode != "P" ? true : I.tw[wc[jr + 1]] == I.tw[wc[i]])) jr++;
    }
    int jm = i;
    if (useMod) {
      int mn = I.d[wc[i]], mx = mn;
      for (int j = i + 1; j < n && I.tw[wc[j]] == I.tw[wc[i]]; j++) {
        mn = min(mn, I.d[wc[j]]); mx = max(mx, I.d[wc[j]]);
        if (mx - mn == j - i) jm = j;
      }
    }
    int j = max(jr, jm);
    vector<int> ws;
    for (int t = i; t <= j; t++) ws.push_back(wc[t]);
    bool pure = true;
    for (int w : ws) if (I.tw[w] != I.tw[ws[0]]) pure = false;
    for (int w : ws) uid[w] = U.size();
    U.push_back(ws);
    kind.push_back(pure ? 0 : 1);
    i = j + 1;
  }
  nunits = U.size();
  int K = U.size();
  vector<int> cnt(K, 0), sC(K), sD(K);
  for (int u = 0; u < K; u++) {
    sC[u] = INT_MAX; sD[u] = INT_MAX;
    for (int w : U[u]) sC[u] = min(sC[u], I.c[w]), sD[u] = min(sD[u], I.d[w]);
  }
  vector<int> held(n, -1);
  vector<set<pair<int, int>>> buf(K);  // kind 0: (key, man)
  Seg TC, TD;
  TC.init(n); TD.init(n);
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  auto refresh = [&](int u) {
    int ta = INT_MIN, tb = INT_MIN;
    int L = U[u].size();
    if (kind[u] == 0) {
      int mx = (int)buf[u].size() < L ? INF : buf[u].rbegin()->first;
      if (I.tw[U[u][0]] == 0) ta = mx; else tb = mx;
    } else if (cnt[u] < L) ta = tb = INF;
    else for (int w : U[u]) { if (I.tw[w] == 0) ta = max(ta, I.a[held[w]]); else tb = max(tb, I.b[held[w]]); }
    TC.set(sC[u], ta, tb); TD.set(sD[u], ta, tb);
  };
  for (int u = 0; u < K; u++) refresh(u);
  vector<int> ptr(n, 0);
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else if (policy == "ra") pr[m] = -I.a[m];
    else if (policy == "rb") pr[m] = -I.b[m];
    else if (policy == "rmin") pr[m] = -min(I.a[m], I.b[m]);
    else if (policy == "rmax") pr[m] = -max(I.a[m], I.b[m]);
    else if (policy == "custom") pr[m] = gCustom[m];
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  auto push = [&](int m) { if (policy == "lifo" || policy == "fifo") dq.push_back(m); else pq.push({pr[m], m}); };
  auto pop = [&]() -> int {
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() { return (policy == "lifo" || policy == "fifo") ? dq.empty() : pq.empty(); };
  for (int m = 0; m < n; m++) push(m);
  Res R;
  gBulk = 0;
  long long since = 0, chLast = 0;
  auto materialize = [&](int u) {
    if (kind[u] != 0) return;
    for (int w : U[u]) held[w] = -1;
    vector<int> rem = U[u];
    for (auto& [k, m] : buf[u]) {
      int bi = 0;
      for (int i = 1; i < (int)rem.size(); i++)
        if (I.tm[m] == 0 ? I.c[rem[i]] < I.c[rem[bi]] : I.d[rem[i]] < I.d[rem[bi]]) bi = i;
      held[rem[bi]] = m; rem.erase(rem.begin() + bi);
    }
  };
  auto bulk = [&]() {
    vector<int> fr;
    while (!empty()) fr.push_back(pop());
    for (int u = 0; u < K; u++) materialize(u);
    // explicit pointers of held men: position of their woman (GS-consistent, see notes RB10)
    for (int w = 0; w < n; w++) if (held[w] >= 0) { int h = held[w]; ptr[h] = I.tm[h] == 0 ? I.c[w] : I.d[w]; }
    vector<int> pending[2];
    for (int m : fr) pending[I.tm[m]].push_back(m);
    for (int t : {0, 1}) {
      vector<int> q = pending[t]; pending[t].clear();
      while (!q.empty()) {
        int m = q.back(); q.pop_back();
        while (m >= 0) {
          int w = I.tm[m] == 0 ? wc[ptr[m]] : wd[ptr[m]];
          int h = held[w];
          if (h >= 0 && key(w, h) < key(w, m)) { ptr[m]++; continue; }
          held[w] = m;
          m = -1;
          if (h >= 0) { ptr[h]++; if (I.tm[h] == t) m = h; else pending[I.tm[h]].push_back(h); }
        }
      }
    }
    // rebuild unit state; free men (pending) go back to the pool with pointer = start of their unit
    for (int u = 0; u < K; u++) {
      cnt[u] = 0; buf[u].clear();
      for (int w : U[u]) if (held[w] >= 0) { cnt[u]++; if (kind[u] == 0) buf[u].insert({key(w, held[w]), held[w]}); }
      refresh(u);
    }
    for (int t : {0, 1}) for (int m : pending[t]) {
      int p = ptr[m]; int w = I.tm[m] == 0 ? wc[p] : wd[p]; int u = uid[w];
      ptr[m] = I.tm[m] == 0 ? sC[u] : sD[u];
      push(m);
    }
    gBulk++;
  };
  while (!empty()) {
    since += R.Ch - chLast; chLast = R.Ch;
    if (since >= n) { bulk(); since = 0; if (empty()) break; }
    int m = pop();
    while (m >= 0) {
      bool X = I.tm[m] == 0;
      int pos = (X ? TC : TD).first(ptr[m], I.a[m], I.b[m]);
      if (pos < 0) { fprintf(stderr, "man %d unmatched: bug\n", m); exit(1); }
      int u = uid[X ? wc[pos] : wd[pos]];
      int L = U[u].size();
      R.Au++;
      int fallen = -1;
      if (kind[u] == 0) {
        int k = I.tw[U[u][0]] == 0 ? I.a[m] : I.b[m];
        buf[u].insert({k, m});
        R.Ch++; R.Aex++;
        if ((int)buf[u].size() > L) { fallen = prev(buf[u].end())->second; buf[u].erase(prev(buf[u].end())); }
      } else {
        bool chainOK = true;
        vector<pair<int, int>> pts;
        for (int w : U[u]) if (held[w] >= 0) pts.push_back({I.a[held[w]], I.b[held[w]]});
        pts.push_back({I.a[m], I.b[m]});
        sort(pts.begin(), pts.end());
        for (size_t i = 1; i < pts.size(); i++) if (pts[i].second < pts[i - 1].second) { chainOK = false; break; }
        R.Ch += chainOK ? 1 : L;
        int cur = m, last = -2, firstk = -1, lastk = -1;
        for (int k = 0; k < L; k++) {
          int w = U[u][k], h = held[w];
          if (h >= 0 && key(w, h) < key(w, cur)) continue;
          R.Aex++;
          if (k != last + 1) R.Str++;
          if (firstk < 0) firstk = k;
          lastk = k; last = k;
          held[w] = cur;
          if (h < 0) { cnt[u]++; cur = -1; break; }
          cur = h;
        }
        if (cur >= 0) fallen = cur;
        for (int k = firstk + 1; k <= lastk; k++) if (I.tw[U[u][k]] != I.tw[U[u][k - 1]]) R.Bnd++;
      }
      refresh(u);
      m = -1;
      if (fallen >= 0) {
        ptr[fallen] = (I.tm[fallen] == 0 ? sC[u] : sD[u]) + L;
        if (atomic) m = fallen; else push(fallen);
      }
    }
  }
  // final assignment inside kind-0 units: holders in key order, each takes the best remaining woman
  for (int u = 0; u < K; u++) if (kind[u] == 0) {
    vector<int> rem = U[u];
    for (auto& [k, m] : buf[u]) {
      int bi = -1;
      for (int i = 0; i < (int)rem.size(); i++) {
        int w = rem[i];
        if (bi < 0) { bi = i; continue; }
        int wb = rem[bi];
        if (I.tm[m] == 0 ? I.c[w] < I.c[wb] : I.d[w] < I.d[wb]) bi = i;
      }
      held[rem[bi]] = m; rem.erase(rem.begin() + bi);
    }
  }
  heldOut = held;
  return R;
}

// Phased RBGS (2026-10-05): two pools by man type; the current type's pool is emptied (a phase),
// then the type switches. Overflow men of the other type wait for the next phase. Records the
// unit cost of every phase; hybrid cost = sum over phases of min(n, phase Ch): each phase may
// instead be done by one O(n log n) sweep (phase = men-master-list sub-market, unique outcome).
long long gPhases, gHybrid; int gStartType = 0;
Res runPh(const Inst& I, const string& mode, const string& policy, bool atomic, vector<int>& heldOut, int& nunits) {
  int n = I.n;
  vector<int> wc(n), wd(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  vector<vector<int>> U;  // women of the unit (for kind 1: in common order)
  vector<int> kind, uid(n);
  bool useRun = mode == "P" || mode == "M" || mode == "QM";
  bool useMod = mode == "Q" || mode == "QM";
  for (int i = 0; i < n;) {
    // run from i
    int jr = i;
    if (useRun) {
      while (jr + 1 < n && I.d[wc[jr + 1]] == I.d[wc[jr]] + 1 && (mode != "P" ? true : I.tw[wc[jr + 1]] == I.tw[wc[i]])) jr++;
    }
    int jm = i;
    if (useMod) {
      int mn = I.d[wc[i]], mx = mn;
      for (int j = i + 1; j < n && I.tw[wc[j]] == I.tw[wc[i]]; j++) {
        mn = min(mn, I.d[wc[j]]); mx = max(mx, I.d[wc[j]]);
        if (mx - mn == j - i) jm = j;
      }
    }
    int j = max(jr, jm);
    vector<int> ws;
    for (int t = i; t <= j; t++) ws.push_back(wc[t]);
    bool pure = true;
    for (int w : ws) if (I.tw[w] != I.tw[ws[0]]) pure = false;
    for (int w : ws) uid[w] = U.size();
    U.push_back(ws);
    kind.push_back(pure ? 0 : 1);
    i = j + 1;
  }
  nunits = U.size();
  int K = U.size();
  vector<int> cnt(K, 0), sC(K), sD(K);
  for (int u = 0; u < K; u++) {
    sC[u] = INT_MAX; sD[u] = INT_MAX;
    for (int w : U[u]) sC[u] = min(sC[u], I.c[w]), sD[u] = min(sD[u], I.d[w]);
  }
  vector<int> held(n, -1);
  vector<set<pair<int, int>>> buf(K);  // kind 0: (key, man)
  Seg TC, TD;
  TC.init(n); TD.init(n);
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  auto refresh = [&](int u) {
    int ta = INT_MIN, tb = INT_MIN;
    int L = U[u].size();
    if (kind[u] == 0) {
      int mx = (int)buf[u].size() < L ? INF : buf[u].rbegin()->first;
      if (I.tw[U[u][0]] == 0) ta = mx; else tb = mx;
    } else if (cnt[u] < L) ta = tb = INF;
    else for (int w : U[u]) { if (I.tw[w] == 0) ta = max(ta, I.a[held[w]]); else tb = max(tb, I.b[held[w]]); }
    TC.set(sC[u], ta, tb); TD.set(sD[u], ta, tb);
  };
  for (int u = 0; u < K; u++) refresh(u);
  vector<int> ptr(n, 0);
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else if (policy == "ra") pr[m] = -I.a[m];
    else if (policy == "rb") pr[m] = -I.b[m];
    else if (policy == "rmin") pr[m] = -min(I.a[m], I.b[m]);
    else if (policy == "rmax") pr[m] = -max(I.a[m], I.b[m]);
    else if (policy == "custom") pr[m] = gCustom[m];
    else pr[m] = rng() % (1LL << 40);
  }
  long long ctr = 0;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pools[2];
  auto push = [&](int m) {
    long long k = policy == "fifo" ? ctr++ : policy == "lifo" ? -(ctr++) : pr[m];
    pools[I.tm[m]].push({k, m});
  };
  for (int m = 0; m < n; m++) push(m);
  int cur = gStartType;
  gPhases = 0; gHybrid = 0;
  long long chPhase = 0, chStart = 0;
  Res R;
  while (!pools[0].empty() || !pools[1].empty()) {
    if (pools[cur].empty()) { if (chPhase) gHybrid += min<long long>(n, R.Ch - chStart); chPhase = 0; cur ^= 1; continue; }
    if (chPhase == 0 && chStart >= 0) { gPhases++; chStart = R.Ch; chPhase = 1; }
    int m = pools[cur].top().second; pools[cur].pop();
    while (m >= 0) {
      bool X = I.tm[m] == 0;
      int pos = (X ? TC : TD).first(ptr[m], I.a[m], I.b[m]);
      if (pos < 0) { fprintf(stderr, "man %d unmatched: bug\n", m); exit(1); }
      int u = uid[X ? wc[pos] : wd[pos]];
      int L = U[u].size();
      R.Au++;
      int fallen = -1;
      if (kind[u] == 0) {
        int k = I.tw[U[u][0]] == 0 ? I.a[m] : I.b[m];
        buf[u].insert({k, m});
        R.Ch++; R.Aex++;
        if ((int)buf[u].size() > L) { fallen = prev(buf[u].end())->second; buf[u].erase(prev(buf[u].end())); }
      } else {
        bool chainOK = true;
        vector<pair<int, int>> pts;
        for (int w : U[u]) if (held[w] >= 0) pts.push_back({I.a[held[w]], I.b[held[w]]});
        pts.push_back({I.a[m], I.b[m]});
        sort(pts.begin(), pts.end());
        for (size_t i = 1; i < pts.size(); i++) if (pts[i].second < pts[i - 1].second) { chainOK = false; break; }
        R.Ch += chainOK ? 1 : L;
        int cur = m, last = -2, firstk = -1, lastk = -1;
        for (int k = 0; k < L; k++) {
          int w = U[u][k], h = held[w];
          if (h >= 0 && key(w, h) < key(w, cur)) continue;
          R.Aex++;
          if (k != last + 1) R.Str++;
          if (firstk < 0) firstk = k;
          lastk = k; last = k;
          held[w] = cur;
          if (h < 0) { cnt[u]++; cur = -1; break; }
          cur = h;
        }
        if (cur >= 0) fallen = cur;
        for (int k = firstk + 1; k <= lastk; k++) if (I.tw[U[u][k]] != I.tw[U[u][k - 1]]) R.Bnd++;
      }
      refresh(u);
      m = -1;
      if (fallen >= 0) {
        ptr[fallen] = (I.tm[fallen] == 0 ? sC[u] : sD[u]) + L;
        if (atomic && I.tm[fallen] == cur) m = fallen; else push(fallen);
      }
    }
  }
  if (chPhase) gHybrid += min<long long>(n, R.Ch - chStart);
  // final assignment inside kind-0 units: holders in key order, each takes the best remaining woman
  for (int u = 0; u < K; u++) if (kind[u] == 0) {
    vector<int> rem = U[u];
    for (auto& [k, m] : buf[u]) {
      int bi = -1;
      for (int i = 0; i < (int)rem.size(); i++) {
        int w = rem[i];
        if (bi < 0) { bi = i; continue; }
        int wb = rem[bi];
        if (I.tm[m] == 0 ? I.c[w] < I.c[wb] : I.d[w] < I.d[wb]) bi = i;
      }
      held[rem[bi]] = m; rem.erase(rem.begin() + bi);
    }
  }
  heldOut = held;
  return R;
}

// RBGS with settled-women deletion (mode "QMS", measurement version, 2026-10-05).
// A woman is settled when every man she prefers to her holder is settled (= held by a settled
// woman); settled women reject every later proposal, so they leave the active orders, and units
// are rebuilt on the active women (greedy QM partition). Pure modules settle only as a whole:
// full and every unsettled man with key < max key of H lies in H. Free men's pointers are moved
// back to the start of the unit that contains their next active woman (the passed women of that
// unit reject them again: valid GS with repeated rejected proposals).
Res runS(const Inst& I, const string& policy, bool atomic, vector<int>& heldOut, int& nunits, long long& rebuilds, int& settledW) {
  int n = I.n;
  vector<int> wc(n), wd(n), ma(n), mb(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  for (int m = 0; m < n; m++) ma[I.a[m]] = m, mb[I.b[m]] = m;
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  vector<char> active(n, 1), settledM(n, 0), isHeld(n, 0);
  vector<int> held(n, -1), ptr(n, 0);
  vector<vector<int>> U;
  vector<int> kind, uid(n, -1), cnt, sC, sD, eC, eD;
  vector<set<pair<int, int>>> buf;
  Seg TC, TD;
  auto refresh = [&](int u) {
    int ta = INT_MIN, tb = INT_MIN;
    int L = U[u].size();
    if (kind[u] == 0) {
      int mx = (int)buf[u].size() < L ? INF : buf[u].rbegin()->first;
      if (I.tw[U[u][0]] == 0) ta = mx; else tb = mx;
    } else if (cnt[u] < L) ta = tb = INF;
    else for (int w : U[u]) { if (I.tw[w] == 0) ta = max(ta, I.a[held[w]]); else tb = max(tb, I.b[held[w]]); }
    TC.set(sC[u], ta, tb); TD.set(sD[u], ta, tb);
  };
  auto materialize = [&](int u) {  // kind 0: serial dictatorship over buf -> held[]
    if (kind[u] != 0) return;
    for (int w : U[u]) held[w] = -1;
    vector<int> rem = U[u];
    for (auto& [k, m] : buf[u]) {
      int bi = 0;
      for (int i = 1; i < (int)rem.size(); i++)
        if (I.tm[m] == 0 ? I.c[rem[i]] < I.c[rem[bi]] : I.d[rem[i]] < I.d[rem[bi]]) bi = i;
      held[rem[bi]] = m; rem.erase(rem.begin() + bi);
    }
  };
  gRealWork = 0;
  auto build = [&]() {
    gRealWork += n;
    vector<int> actC, dact(n, -1);
    for (int i = 0; i < n; i++) if (active[wc[i]]) actC.push_back(wc[i]);
    int r = 0;
    for (int i = 0; i < n; i++) if (active[wd[i]]) dact[wd[i]] = r++;
    U.clear(); kind.clear(); fill(uid.begin(), uid.end(), -1);
    int N = actC.size();
    for (int i = 0; i < N;) {
      int jr = i;
      while (jr + 1 < N && dact[actC[jr + 1]] == dact[actC[jr]] + 1) jr++;
      int jm = i, mn = dact[actC[i]], mx = mn;
      for (int j = i + 1; j < N && I.tw[actC[j]] == I.tw[actC[i]]; j++) {
        mn = min(mn, dact[actC[j]]); mx = max(mx, dact[actC[j]]);
        if (mx - mn == j - i) jm = j;
      }
      int j = max(jr, jm);
      vector<int> ws(actC.begin() + i, actC.begin() + j + 1);
      bool pure = true;
      for (int w : ws) if (I.tw[w] != I.tw[ws[0]]) pure = false;
      for (int w : ws) uid[w] = U.size();
      U.push_back(ws); kind.push_back(pure ? 0 : 1);
      i = j + 1;
    }
    int K = U.size();
    cnt.assign(K, 0); sC.assign(K, INT_MAX); sD.assign(K, INT_MAX); eC.assign(K, -1); eD.assign(K, -1);
    buf.assign(K, {});
    for (int u = 0; u < K; u++) {
      for (int w : U[u]) {
        sC[u] = min(sC[u], I.c[w]); sD[u] = min(sD[u], I.d[w]);
        eC[u] = max(eC[u], I.c[w] + 1); eD[u] = max(eD[u], I.d[w] + 1);
        if (held[w] >= 0) { cnt[u]++; if (kind[u] == 0) buf[u].insert({key(w, held[w]), held[w]}); }
      }
    }
    TC.init(n); TD.init(n);
    for (int u = 0; u < K; u++) refresh(u);
    // pointer reset for free men
    for (int m = 0; m < n; m++) {
      if (isHeld[m]) continue;
      bool X = I.tm[m] == 0;
      int p = ptr[m];
      while (p < n && !active[X ? wc[p] : wd[p]]) p++;
      if (p >= n) continue;
      int u = uid[X ? wc[p] : wd[p]];
      ptr[m] = X ? sC[u] : sD[u];
    }
  };
  build();
  nunits = U.size();
  if (getenv("DBG")) { fprintf(stderr, "K=%d N=%d\n", (int)U.size(), n); }
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else if (policy == "ra") pr[m] = -I.a[m];
    else if (policy == "rb") pr[m] = -I.b[m];
    else if (policy == "rmin") pr[m] = -min(I.a[m], I.b[m]);
    else if (policy == "rmax") pr[m] = -max(I.a[m], I.b[m]);
    else if (policy == "custom") pr[m] = gCustom[m];
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  auto push = [&](int m) { if (policy == "lifo" || policy == "fifo") dq.push_back(m); else pq.push({pr[m], m}); };
  auto pop = [&]() -> int {
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() { return (policy == "lifo" || policy == "fifo") ? dq.empty() : pq.empty(); };
  for (int m = 0; m < n; m++) push(m);
  Res R;
  rebuilds = 0; settledW = 0;
  vector<int> cntA(n + 1), cntB(n + 1);
  // joint prefix cuts: block j = men with a-rank (and b-rank) in [cs[j], cs[j+1]), women with c, d in it.
  // Men of earlier blocks never propose to women of later blocks (counting), so settledness of a
  // block-j woman whose holder is a block-j man only needs the block-j men below her holder.
  vector<int> bm(n), bw(n), cs = {0};
  { int mxb = -1, mxd = -1; for (int k = 1; k <= n; k++) { mxb = max(mxb, I.b[ma[k - 1]]); mxd = max(mxd, I.d[wc[k - 1]]); if (mxb == k - 1 && mxd == k - 1) cs.push_back(k); } }
  bool blockAware = !getenv("NOBLOCK");
  if (!blockAware) cs = {0, n};
  for (int j = 0; j + 1 < (int)cs.size(); j++) for (int r = cs[j]; r < cs[j + 1]; r++) bm[ma[r]] = j, bw[wc[r]] = j;
  auto settleCheck = [&]() -> bool {
    bool any = false;
    while (true) {
      gRealWork += n + (long long)U.size();
      cntA[0] = cntB[0] = 0;
      for (int r = 0; r < n; r++) cntA[r + 1] = cntA[r] + !settledM[ma[r]], cntB[r + 1] = cntB[r] + !settledM[mb[r]];
      bool ch = false;
      for (int u = 0; u < (int)U.size(); u++) {
        int L = U[u].size();
        if (kind[u] == 0) {
          if (!active[U[u][0]] || (int)buf[u].size() < L) continue;
          int mk = buf[u].rbegin()->first, j = bw[U[u][0]];
          bool same = true;
          for (int w : U[u]) if (bw[w] != j) same = false;
          for (auto& [k2, m2] : buf[u]) if (bm[m2] != j) same = false;
          if (!same) continue;
          int c = I.tw[U[u][0]] == 0 ? cntA[mk] - cntA[cs[j]] : cntB[mk] - cntB[cs[j]];
          if (c == L - 1) {
            materialize(u);
            for (int w : U[u]) { active[w] = 0; settledM[held[w]] = 1; settledW++; }
            ch = true;
          }
        } else {
          for (int w : U[u]) {
            if (!active[w] || held[w] < 0) continue;
            int h = held[w], j = bw[w];
            if (bm[h] != j) continue;
            int c = I.tw[w] == 0 ? cntA[I.a[h]] - cntA[cs[j]] : cntB[I.b[h]] - cntB[cs[j]];
            if (c == 0) { active[w] = 0; settledM[h] = 1; settledW++; ch = true; break; }
          }
        }
        if (ch) break;
      }
      if (!ch) break;
      any = true;
    }
    return any;
  };
  while (!empty()) {
    int m = pop();
    if (isHeld[m]) continue;
    while (m >= 0) {
      gRealWork++;
      if ((R.Ch + rebuilds) * (long long)n > gAbortWork) exit(2);
      bool X = I.tm[m] == 0;
      int pos = (X ? TC : TD).first(ptr[m], I.a[m], I.b[m]);
      if (pos < 0) { fprintf(stderr, "man %d unmatched: bug (S)\n", m); exit(1); }
      int u = uid[X ? wc[pos] : wd[pos]];
      if (u < 0) { fprintf(stderr, "inactive start: bug (S)\n"); exit(1); }
      int L = U[u].size();
      R.Au++;
      int fallen = -1;
      isHeld[m] = 1;
      if (kind[u] == 0) {
        buf[u].insert({key(U[u][0], m), m});
        R.Ch++; R.Aex++;
        if ((int)buf[u].size() > L) { fallen = prev(buf[u].end())->second; buf[u].erase(prev(buf[u].end())); }
      } else {
        bool chainOK = true;
        vector<pair<int, int>> pts;
        for (int w : U[u]) if (held[w] >= 0) pts.push_back({I.a[held[w]], I.b[held[w]]});
        pts.push_back({I.a[m], I.b[m]});
        sort(pts.begin(), pts.end());
        for (size_t i = 1; i < pts.size(); i++) if (pts[i].second < pts[i - 1].second) { chainOK = false; break; }
        R.Ch += chainOK ? 1 : L;
        int cur = m;
        for (int k = 0; k < L; k++) {
          int w = U[u][k], h = held[w];
          if (h >= 0 && key(w, h) < key(w, cur)) continue;
          R.Aex++;
          held[w] = cur;
          if (h < 0) { cnt[u]++; cur = -1; break; }
          cur = h;
        }
        if (cur >= 0) fallen = cur;
      }
      refresh(u);
      m = -1;
      if (fallen >= 0) {
        isHeld[fallen] = 0;
        ptr[fallen] = I.tm[fallen] == 0 ? eC[u] : eD[u];
      }
      if (settleCheck()) {
        for (int v = 0; v < (int)U.size(); v++) materialize(v);
        rebuilds++;
        build();
      }
      if (fallen >= 0) { if (atomic) m = fallen; else push(fallen); }
    }
  }
  for (int u = 0; u < (int)U.size(); u++) materialize(u);
  heldOut = held;
  return R;
}

// RBGS with per-type dead women (mode QMT, 2026-10-05). Woman w is dead for man type T when no
// unsettled type-T man (other than men held in her own unit) is better for her than her holder
// (block-aware counts). A dead-for-T woman rejects every later type-T proposal, so she is removed
// from the type-T walk only. Units: same-type sets that are contiguous among the alive-for-X
// women in c-order AND among the alive-for-Y women in d-order (RB1 holds: the women between them
// in a man's list reject him). Mixed runs only among women alive for both types.
vector<int> mosm(const Inst& I);
Res runT(const Inst& I, const string& policy, bool atomic, vector<int>& heldOut, int& nunits, long long& rebuilds) {
  int n = I.n;
  vector<array<int, 3>> decl;  // (woman, type, holder) at the time she was declared dead
  // joint prefix blocks first: units must not cross a block cut (the block-aware dead rule ignores
  // earlier-block men, who never reach later-block women; a unit across a cut would let them).
  vector<int> blkW(n);
  {
    vector<int> wcc(n), mac(n);
    for (int w = 0; w < n; w++) wcc[I.c[w]] = w;
    for (int m = 0; m < n; m++) mac[I.a[m]] = m;
    int mxb = -1, mxd = -1, cur = 0, last = 0;
    vector<int> cuts = {0};
    for (int k = 1; k <= n; k++) { mxb = max(mxb, I.b[mac[k - 1]]); mxd = max(mxd, I.d[wcc[k - 1]]); if (mxb == k - 1 && mxd == k - 1) cuts.push_back(k); }
    for (int j = 0; j + 1 < (int)cuts.size(); j++) for (int r = cuts[j]; r < cuts[j + 1]; r++) blkW[wcc[r]] = j;
    (void)cur; (void)last;
  }
  vector<int> wc(n), wd(n), ma(n), mb(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  for (int m = 0; m < n; m++) ma[I.a[m]] = m, mb[I.b[m]] = m;
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  vector<char> alive[2] = {vector<char>(n, 1), vector<char>(n, 1)};
  vector<char> settledM(n, 0), isHeld(n, 0);
  vector<int> held(n, -1), ptr(n, 0);
  vector<vector<int>> U;
  vector<int> kind, uid(n, -1), cnt, sP[2], eP[2];
  vector<set<pair<int, int>>> buf;
  Seg TC, TD;
  auto pos = [&](int T, int w) { return T == 0 ? I.c[w] : I.d[w]; };
  auto refresh = [&](int u) {
    int ta = INT_MIN, tb = INT_MIN;
    int L = U[u].size();
    if (kind[u] == 0) {
      int mx = (int)buf[u].size() < L ? INF : buf[u].rbegin()->first;
      if (I.tw[U[u][0]] == 0) ta = mx; else tb = mx;
    } else if (cnt[u] < L) ta = tb = INF;
    else for (int w : U[u]) { if (I.tw[w] == 0) ta = max(ta, I.a[held[w]]); else tb = max(tb, I.b[held[w]]); }
    if (sP[0][u] >= 0) TC.set(sP[0][u], ta, tb);
    if (sP[1][u] >= 0) TD.set(sP[1][u], ta, tb);
  };
  auto materialize = [&](int u) {  // serial dictatorship, O(L log L): sorted orders + forward pointers
    if (kind[u] != 0) return;
    for (int w : U[u]) held[w] = -1;
    if (buf[u].empty()) return;
    vector<int> byC = U[u], byD = U[u];
    sort(byC.begin(), byC.end(), [&](int x, int y) { return I.c[x] < I.c[y]; });
    sort(byD.begin(), byD.end(), [&](int x, int y) { return I.d[x] < I.d[y]; });
    size_t ic = 0, id = 0;
    for (auto& [k, m] : buf[u]) {
      vector<int>& ord = I.tm[m] == 0 ? byC : byD; size_t& ix = I.tm[m] == 0 ? ic : id;
      while (held[ord[ix]] >= 0) ix++;
      held[ord[ix]] = m;
    }
    gRealWork += U[u].size();
  };
  // positional aliveness: a woman matters for the type-T walk if she may accept a T-man, or holds a
  // T-man who may still be displaced (he then continues from her position).
  auto pa = [&](int T, int w) { return alive[T][w] || (held[w] >= 0 && I.tm[held[w]] == T && (alive[0][w] || alive[1][w])); };
  // ORACLE mode: a pure unit must have one matter pattern (RB1 needs every buffer man to walk
  // through every unit woman; oracle deadness only covers men who reach her in real GS).
  bool uniformPat = getenv("ORACLE") != nullptr;
  gRealWork = 0;
  auto build = [&]() {
    gRealWork += n;
    vector<int> aC, aD, rY(n, -1);
    vector<char> P0(n), P1(n);
    for (int w = 0; w < n; w++) P0[w] = pa(0, w), P1[w] = pa(1, w);
    for (int i = 0; i < n; i++) if (P0[wc[i]]) aC.push_back(wc[i]);
    for (int i = 0; i < n; i++) if (P1[wd[i]]) { rY[wd[i]] = aD.size(); aD.push_back(wd[i]); }
    U.clear(); kind.clear(); fill(uid.begin(), uid.end(), -1);
    int N = aC.size();
    for (int i = 0; i < N;) {
      int jr = i;  // run: alive for both, consecutive in aD, any types
      if (P1[aC[i]]) while (jr + 1 < N && blkW[aC[jr + 1]] == blkW[aC[i]] && P1[aC[jr + 1]] && rY[aC[jr + 1]] == rY[aC[jr]] + 1) jr++;
      int jm = i, mn = INT_MAX, mx = INT_MIN, cntY = 0;
      for (int j = i; j < N && I.tw[aC[j]] == I.tw[aC[i]] && blkW[aC[j]] == blkW[aC[i]] && (!uniformPat || P1[aC[j]] == P1[aC[i]]); j++) {
        if (P1[aC[j]]) { mn = min(mn, rY[aC[j]]); mx = max(mx, rY[aC[j]]); cntY++; }
        if (cntY == 0 || mx - mn + 1 == cntY) jm = j;
      }
      int j = max(jr, jm);
      vector<int> ws(aC.begin() + i, aC.begin() + j + 1);
      bool pure = true;
      for (int w : ws) if (I.tw[w] != I.tw[ws[0]]) pure = false;
      for (int w : ws) uid[w] = U.size();
      U.push_back(ws); kind.push_back(pure ? 0 : 1);
      i = j + 1;
    }
    for (int i = 0; i < (int)aD.size();) {  // women alive only for Y
      if (uid[aD[i]] >= 0) { i++; continue; }
      int j = i;
      while (j + 1 < (int)aD.size() && uid[aD[j + 1]] < 0 && I.tw[aD[j + 1]] == I.tw[aD[i]] && blkW[aD[j + 1]] == blkW[aD[i]]) j++;
      vector<int> ws(aD.begin() + i, aD.begin() + j + 1);
      for (int w : ws) uid[w] = U.size();
      U.push_back(ws); kind.push_back(0);
      i = j + 1;
    }
    int K = U.size();
    cnt.assign(K, 0); buf.assign(K, {});
    for (int T : {0, 1}) { sP[T].assign(K, -1); eP[T].assign(K, -1); }
    for (int u = 0; u < K; u++) {
      for (int w : U[u]) {
        for (int T : {0, 1}) if (T == 0 ? P0[w] : P1[w]) {
          if (sP[T][u] < 0 || pos(T, w) < sP[T][u]) sP[T][u] = pos(T, w);
          eP[T][u] = max(eP[T][u], pos(T, w) + 1);
        }
        if (held[w] >= 0) { cnt[u]++; if (kind[u] == 0) buf[u].insert({key(w, held[w]), held[w]}); }
      }
    }
    TC.init(n); TD.init(n);
    for (int u = 0; u < K; u++) refresh(u);
    for (int m = 0; m < n; m++) {
      if (isHeld[m]) continue;
      int T = I.tm[m], p = ptr[m];
      while (p < n && !(T == 0 ? P0[wc[p]] : P1[wd[p]])) p++;
      if (p >= n) continue;
      ptr[m] = sP[T][uid[T == 0 ? wc[p] : wd[p]]];
    }
  };
  // joint prefix cuts (block-aware counts, as in runS)
  vector<int> bm(n), bw(n), cs = {0};
  { int mxb = -1, mxd = -1; for (int k = 1; k <= n; k++) { mxb = max(mxb, I.b[ma[k - 1]]); mxd = max(mxd, I.d[wc[k - 1]]); if (mxb == k - 1 && mxd == k - 1) cs.push_back(k); } }
  for (int j = 0; j + 1 < (int)cs.size(); j++) for (int r = cs[j]; r < cs[j + 1]; r++) bm[ma[r]] = j, bw[wc[r]] = j;
  build();
  nunits = U.size();
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else if (policy == "ra") pr[m] = -I.a[m];
    else if (policy == "rb") pr[m] = -I.b[m];
    else if (policy == "rmin") pr[m] = -min(I.a[m], I.b[m]);
    else if (policy == "rmax") pr[m] = -max(I.a[m], I.b[m]);
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  // policy "phase" (PHSTAT, 2026-10-05): type-batched order = pure phase iteration of RB10 (pop men of
  // the current type until none is pending, then switch); records unit work per phase.
  deque<int> phq[2]; int tau = 0; vector<long long> phW; long long phStart = 0; Res R;
  bool phaseP = policy == "phase";
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  auto push = [&](int m) { if (phaseP) phq[I.tm[m]].push_back(m); else if (policy == "lifo" || policy == "fifo") dq.push_back(m); else pq.push({pr[m], m}); };
  auto pop = [&]() -> int {
    if (phaseP) {
      if (phq[tau].empty()) { tau ^= 1; phW.push_back(R.Ch - phStart); phStart = R.Ch; }
      int m = phq[tau].front(); phq[tau].pop_front(); return m;
    }
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() { if (phaseP) return phq[0].empty() && phq[1].empty(); return (policy == "lifo" || policy == "fifo") ? dq.empty() : pq.empty(); };
  for (int m = 0; m < n; m++) push(m);
  // (Res R declared above)
  rebuilds = 0;
  // cnt[T][key-kind][r]: unsettled type-T men with a-rank (kk=0) or b-rank (kk=1) < r
  vector<vector<int>> cA(2, vector<int>(n + 1)), cB(2, vector<int>(n + 1));
  // ORACLE=1 (measurement only, 2026-10-05): exact static deadness from the MOSM mu.
  // theta[T][w] = key_w(mu(w)) if mu(w) has type T, else min key_w over type-T men m who strictly
  // prefer w to mu(m) (the only type-T men who ever propose to w), INF if none.
  // w is dead for T iff key_w(holder) <= theta[T][w]: every later T-proposer is no better.
  bool oracle = getenv("ORACLE") != nullptr;
  // FAST=1 (2026-10-05): one dead-check pass declares all certifiable women (default restarts after each
  // change), and dead checks run lazily, after every n charges (default: after every step). Sound: a
  // later or batched declaration only declares less at each moment. Results may differ from the default.
  bool fastT = getenv("FAST") != nullptr, traceT = getenv("TRACE") != nullptr, validT = getenv("VALID") != nullptr, usizeT = getenv("USIZE") != nullptr;
  long long lastCheck = 0;
  vector<int> theta[2] = {vector<int>(n, INT_MAX), vector<int>(n, INT_MAX)};
  if (oracle) {
    vector<int> mu = mosm(I), wifeOf(n);
    for (int w = 0; w < n; w++) wifeOf[mu[w]] = w;
    for (int w = 0; w < n; w++) theta[I.tm[mu[w]]][w] = key(w, mu[w]);
    for (int m = 0; m < n; m++) {
      int T = I.tm[m], pm = pos(T, wifeOf[m]);
      for (int p = 0; p < pm; p++) { int w = T == 0 ? wc[p] : wd[p]; if (I.tm[mu[w]] != T) theta[T][w] = min(theta[T][w], key(w, m)); }
    }
  }
  auto deadCheckOracle = [&]() -> bool {
    bool any = false, ch = true;
    while (ch) {
      ch = false;
      gRealWork += n + (long long)U.size();
      for (int u = 0; u < (int)U.size(); u++) {
        if (kind[u] == 0 && U[u].size() > 1) {
          if ((int)buf[u].size() < (int)U[u].size()) continue;
          int mk = buf[u].rbegin()->first;
          for (int T : {0, 1}) {
            bool al = false; int th = INT_MAX;
            for (int w : U[u]) { if (alive[T][w]) al = true; th = min(th, theta[T][w]); }
            if (al && mk <= th) { for (int w : U[u]) alive[T][w] = 0; ch = true; }
          }
          bool deadAll = true;
          for (int w : U[u]) if (alive[0][w] || alive[1][w]) deadAll = false;
          if (deadAll) { materialize(u); for (int w : U[u]) settledM[held[w]] = 1; }
          continue;
        }
        materialize(u);
        for (int w : U[u]) {
          if (held[w] < 0) continue;
          int h = held[w];
          for (int T : {0, 1}) if (alive[T][w] && key(w, h) <= theta[T][w]) { alive[T][w] = 0; ch = true; decl.push_back({w, T, h}); }
          if (!alive[0][w] && !alive[1][w]) settledM[h] = 1;
        }
      }
      if (ch) any = true;
    }
    return any;
  };
  auto deadCheck = [&]() -> bool {
    if (oracle) return deadCheckOracle();
    bool any = false;
    while (true) {
      gRealWork += 2LL * n + (long long)U.size();
      for (int T : {0, 1}) {
        cA[T][0] = cB[T][0] = 0;
        for (int r = 0; r < n; r++) {
          cA[T][r + 1] = cA[T][r] + (!settledM[ma[r]] && I.tm[ma[r]] == T);
          cB[T][r + 1] = cB[T][r] + (!settledM[mb[r]] && I.tm[mb[r]] == T);
        }
      }
      bool ch = false;
      for (int u = 0; u < (int)U.size() && (fastT || !ch); u++) {
        if (kind[u] == 0 && U[u].size() > 1) {  // pure module: unit-level test on the max holder
          int L = U[u].size();
          if ((int)buf[u].size() < L) continue;
          int j = bw[U[u][0]]; bool same = true;
          for (int w : U[u]) if (bw[w] != j) same = false;
          for (auto& [k2, m2] : buf[u]) if (bm[m2] != j) same = false;
          if (!same) continue;
          bool X = I.tw[U[u][0]] == 0;
          int mk = buf[u].rbegin()->first;
          for (int T : {0, 1}) {
            bool al = false;
            for (int w : U[u]) if (alive[T][w]) al = true;
            if (!al) continue;
            int c = X ? cA[T][mk] - cA[T][cs[j]] : cB[T][mk] - cB[T][cs[j]];
            if (c == 0) { for (int w : U[u]) alive[T][w] = 0; ch = true; }
          }
          bool deadAll = true;
          for (int w : U[u]) if (alive[0][w] || alive[1][w]) deadAll = false;
          if (deadAll) { materialize(u); for (int w : U[u]) settledM[held[w]] = 1; }
          continue;
        }
        materialize(u);  // singleton or run: explicit per-woman holder
        for (int w : U[u]) {
          if (held[w] < 0) continue;
          int h = held[w], j = bw[w];
          if (bm[h] != j) continue;
          for (int T : {0, 1}) {
            if (!alive[T][w]) continue;
            int c = I.tw[w] == 0 ? cA[T][I.a[h]] - cA[T][cs[j]] : cB[T][I.b[h]] - cB[T][cs[j]];
            if (c == 0) { alive[T][w] = 0; ch = true; decl.push_back({w, T, h}); if (traceT) fprintf(stderr, "  dead: w%d for %c holder %d (unit %d)\n", w, "XY"[T], h, u); }
          }
          if (!alive[0][w] && !alive[1][w]) settledM[h] = 1;
          if (ch && !fastT) break;
        }
      }
      if (!ch) break;
      any = true;
    }
    return any;
  };
  long long since = 0, chLast = 0;
  bool useBudget = getenv("TBUDGET") != nullptr;
  gBulk = 0;
  auto bulk = [&]() {  // phase X then phase Y, explicit (charged 2n), then rebuild
    vector<int> fr;
    while (!empty()) { int m = pop(); if (!isHeld[m]) fr.push_back(m); }
    for (int u = 0; u < (int)U.size(); u++) materialize(u);
    for (int w = 0; w < n; w++) if (held[w] >= 0) { int h = held[w]; ptr[h] = I.tm[h] == 0 ? I.c[w] : I.d[w]; }
    vector<int> pending[2];
    for (int m : fr) pending[I.tm[m]].push_back(m);
    for (int t : {0, 1}) {
      vector<int> q = pending[t]; pending[t].clear();
      while (!q.empty()) {
        int m = q.back(); q.pop_back();
        while (m >= 0) {
          int w = I.tm[m] == 0 ? wc[ptr[m]] : wd[ptr[m]];
          int h = held[w];
          if (h >= 0 && key(w, h) < key(w, m)) { ptr[m]++; continue; }
          held[w] = m; isHeld[m] = 1;
          m = -1;
          if (h >= 0) { isHeld[h] = 0; ptr[h]++; if (I.tm[h] == t) m = h; else pending[I.tm[h]].push_back(h); }
        }
      }
    }
    build();  // buffers from the explicit holders first (materialize() reads the buffers)
    if (deadCheck()) { for (int v = 0; v < (int)U.size(); v++) materialize(v); build(); }
    for (int t : {0, 1}) for (int m : pending[t]) push(m);
    gBulk++;
  };
  // PAU (env PAU=1 with policy "phase", RB14 U2): when the current phase has used n unit charges, finish the
  // phase by one explicit tau-only closure (the RB10 sweep, charged n), then rebuild units and dead sets.
  bool pauCap = phaseP && getenv("PAU") != nullptr;
  long long pauSweeps = 0;
  auto closePhase = [&](int t) {
    vector<int> pending[2];
    for (int T : {0, 1}) { for (int m : phq[T]) if (!isHeld[m]) pending[T].push_back(m); phq[T].clear(); }
    for (int u = 0; u < (int)U.size(); u++) materialize(u);
    for (int w = 0; w < n; w++) if (held[w] >= 0) { int h = held[w]; ptr[h] = I.tm[h] == 0 ? I.c[w] : I.d[w]; }
    vector<int> q = pending[t]; pending[t].clear();
    while (!q.empty()) {
      int m = q.back(); q.pop_back();
      while (m >= 0) {
        int w = I.tm[m] == 0 ? wc[ptr[m]] : wd[ptr[m]];
        int h = held[w];
        if (h >= 0 && key(w, h) < key(w, m)) { ptr[m]++; continue; }
        held[w] = m; isHeld[m] = 1; m = -1;
        if (h >= 0) { isHeld[h] = 0; ptr[h]++; if (I.tm[h] == t) m = h; else pending[I.tm[h]].push_back(h); }
      }
    }
    build();
    if (deadCheck()) { for (int v = 0; v < (int)U.size(); v++) materialize(v); build(); }
    for (int T : {0, 1}) for (int m : pending[T]) phq[T].push_back(m);
    R.Ch += n; pauSweeps++;
  };
  while (!empty()) {
    if (pauCap && !phq[tau].empty() && R.Ch - phStart >= n) { closePhase(tau); if (empty()) break; }
    if (useBudget) { since += R.Ch - chLast; chLast = R.Ch; if (since >= n) { bulk(); since = 0; if (empty()) break; } }
    int m = pop();
    if (isHeld[m]) continue;
    while (m >= 0) {
      int m0 = m;
      gRealWork++;
      if ((R.Ch + rebuilds) * (long long)n > gAbortWork) exit(2);
      int T = I.tm[m];
      int p = (T == 0 ? TC : TD).first(ptr[m], I.a[m], I.b[m]);
      if (p < 0) { fprintf(stderr, "man %d unmatched: bug (T)\n", m); exit(1); }
      int u = uid[T == 0 ? wc[p] : wd[p]];
      if (u < 0 || sP[T][u] != p) { fprintf(stderr, "bad unit start: bug (T)\n"); exit(1); }
      int L = U[u].size();
      R.Au++;
      if (usizeT) { static vector<long long> hist(64); int b = 0; while ((1 << (b + 1)) <= L) b++; hist[b]++; gUsize = &hist; }
      int fallen = -1;
      isHeld[m] = 1;
      if (kind[u] == 0) {
        buf[u].insert({key(U[u][0], m), m});
        R.Ch++; R.Aex++;
        if ((int)buf[u].size() > L) { fallen = prev(buf[u].end())->second; buf[u].erase(prev(buf[u].end())); }
      } else {
        bool chainOK = true;
        vector<pair<int, int>> pts;
        for (int w : U[u]) if (held[w] >= 0) pts.push_back({I.a[held[w]], I.b[held[w]]});
        pts.push_back({I.a[m], I.b[m]});
        sort(pts.begin(), pts.end());
        for (size_t i = 1; i < pts.size(); i++) if (pts[i].second < pts[i - 1].second) { chainOK = false; break; }
        R.Ch += chainOK ? 1 : L;
        int cur = m;
        for (int k = 0; k < L; k++) {
          int w = U[u][k], h = held[w];
          if (h >= 0 && key(w, h) < key(w, cur)) continue;
          R.Aex++;
          held[w] = cur;
          if (h < 0) { cnt[u]++; cur = -1; break; }
          cur = h;
        }
        if (cur >= 0) fallen = cur;
      }
      refresh(u);
      m = -1;
      if (fallen >= 0) { isHeld[fallen] = 0; ptr[fallen] = eP[I.tm[fallen]][u]; }
      if (traceT) { fprintf(stderr, "step: man %d -> unit %d {", m0, u); for (int w : U[u]) fprintf(stderr, " w%d", w); fprintf(stderr, " } kind %d fallen %d\n", kind[u], fallen); }
      if (validT) {  // GS-state validity: passed women hold better men
        for (int v = 0; v < (int)U.size(); v++) materialize(v);
        vector<int> wifeOf(n, -1);
        for (int w = 0; w < n; w++) if (held[w] >= 0) wifeOf[held[w]] = w;
        for (int x = 0; x < n; x++) {
          int lim = wifeOf[x] >= 0 ? pos(I.tm[x], wifeOf[x]) : ptr[x];
          if (wifeOf[x] < 0 && x == fallen) lim = ptr[x];
          for (int q = 0; q < lim; q++) {
            int w = I.tm[x] == 0 ? wc[q] : wd[q];
            if (held[w] < 0 || key(w, x) < key(w, held[w])) { fprintf(stderr, "INVALID after step man %d unit %d: man %d passed w%d (holder %d)\n", m0, u, x, w, held[w]); exit(2); }
          }
        }
      }
      if ((!fastT || R.Ch - lastCheck >= n) && (lastCheck = R.Ch, deadCheck())) {
        for (int v = 0; v < (int)U.size(); v++) materialize(v);
        rebuilds++;
        build();
        if (getenv("TRACE")) { fprintf(stderr, "alive:"); for (int w = 0; w < n; w++) fprintf(stderr, " w%d:%d%d%s", w, (int)alive[0][w], (int)alive[1][w], held[w] >= 0 ? (I.tm[held[w]] ? "y" : "x") : "-"); fprintf(stderr, "\n"); }
        if (getenv("TRACE")) { fprintf(stderr, "rebuild:"); for (int v = 0; v < (int)U.size(); v++) { fprintf(stderr, " [k%d", kind[v]); for (int w : U[v]) fprintf(stderr, " w%d", w); fprintf(stderr, "]"); } fprintf(stderr, "\n"); }
      }
      if (fallen >= 0) { if (atomic && !phaseP) m = fallen; else push(fallen); }
    }
  }
  if (phaseP && getenv("PHSTAT")) {
    phW.push_back(R.Ch - phStart);
    vector<long long> v; for (long long x : phW) if (x > 0) v.push_back(x);
    sort(v.begin(), v.end());
    long long sumMin = 0, heavy = 0, lg = max(1, (int)log2(n)); for (long long x : v) { sumMin += min<long long>(x, n); if (x * lg >= n) heavy++; }
    int P = v.size();
    if (pauCap) printf("   PAU sweeps %lld\n", pauSweeps);
    printf("   phase: P %d  W/n %.2f  sum min(u,n)/n %.2f  W/P %.2f  u med %lld p90 %lld max %lld  heavy(u>=n/log n) %lld\n", P, (double)R.Ch / n, (double)sumMin / n,
           P ? (double)R.Ch / P : 0.0, P ? v[P / 2] : 0, P ? v[(9 * P) / 10] : 0, P ? v[P - 1] : 0, heavy);
  }
  if (getenv("CHECKDEAD")) {
    vector<int> ref = mosm(I);
    for (auto& [w, T, h] : decl) if (ref[w] != h && I.tm[ref[w]] == T)
      fprintf(stderr, "dead claim wrong: woman %d type-%c dead with holder %d, final %d\n", w, "XY"[T], h, ref[w]);
  }
  for (int u = 0; u < (int)U.size(); u++) materialize(u);
  heldOut = held;
  return R;
}

vector<int> mosm(const Inst& I) {  // plain O(n^2) GS, FIFO
  int n = I.n;
  vector<int> wc(n), wd(n), ptr(n, 0), held(n, -1);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  deque<int> q;
  for (int m = 0; m < n; m++) q.push_back(m);
  while (!q.empty()) {
    int m = q.front(); q.pop_front();
    while (m >= 0) {
      int w = I.tm[m] == 0 ? wc[ptr[m]++] : wd[ptr[m]++];
      int h = held[w];
      if (h >= 0 && key(w, h) < key(w, m)) continue;
      held[w] = m; m = h;
    }
  }
  return held;
}

Inst load(const string& path) {
  Inst I;
  FILE* f = fopen(path.c_str(), "r");
  if (!f) { fprintf(stderr, "cannot open %s\n", path.c_str()); exit(1); }
  if (fscanf(f, "%d", &I.n) != 1) exit(1);
  for (auto* v : {&I.a, &I.b, &I.c, &I.d, &I.tm, &I.tw}) {
    v->resize(I.n);
    for (int i = 0; i < I.n; i++) if (fscanf(f, "%d", &(*v)[i]) != 1) exit(1);
  }
  fclose(f);
  return I;
}

Inst loadLab(const string& path) {  // lab text format
  ifstream f(path);
  if (!f) { fprintf(stderr, "cannot open %s\n", path.c_str()); exit(1); }
  map<string, vector<string>> F;
  string line;
  while (getline(f, line)) {
    line = line.substr(0, line.find('#'));
    for (char& ch : line) if (strchr("=[],'\"", ch)) ch = ' ';
    stringstream ss(line); string k, t;
    if (!(ss >> k)) continue;
    while (ss >> t) F[k].push_back(t);
  }
  Inst I;
  auto rank = [&](const string& k) { auto& v = F[k]; vector<int> r(v.size()); for (size_t i = 0; i < v.size(); i++) r[stoi(v[i])] = i; return r; };
  auto types = [&](const string& k) { string s; for (auto& t : F[k]) s += t; vector<int> r; for (char ch : s) r.push_back(ch == 'X' ? 0 : 1); return r; };
  I.a = rank("men_x_ranking"); I.b = rank("men_y_ranking"); I.c = rank("women_x_ranking"); I.d = rank("women_y_ranking");
  I.tm = types("men_preferences"); I.tw = types("women_preferences"); I.n = I.a.size();
  return I;
}

// Relative RBGS (env RELATIVE=1; measurement prototype, 2026-10-05, third session).
// Explicit GS state (held, ptr). A module is a set M of women of one type tau with buffer H = holders.
// M is VALID if every woman g in the c-span of M that is not in M is held and rejects every X-man of H
// (X-woman g: a(h(g)) < a(x); Y-woman g: b(h(g)) < b(x)), and the same for the d-span and the Y-men of H.
// An entrant m that passes the same test (precomputed max gap holder keys) enters M as one RB1 step:
// the sub-run ends in the unique stable matching of the sub-market (H + m, M) = serial dictatorship in
// tau-key order; if M overflows, the worst man z leaves after M's span in his own order. Validity
// persists: gap holders only improve, and entries add only tested men. A failed entrant is processed
// as one explicit acceptance and the module is dissolved until the next rebuild (every RELK * n charges).
Res runR(const Inst& I, const string& policy, bool atomic, vector<int>& heldOut, long long& rebuilds) {
  int n = I.n;
  vector<int> wc(n), wd(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  auto posT = [&](int T, int w) { return T == 0 ? I.c[w] : I.d[w]; };
  auto atT = [&](int T, int p) { return T == 0 ? wc[p] : wd[p]; };
  vector<int> held(n, -1), ptr(n, 0);
  vector<char> isHeld(n, 0);
  vector<int> mod(n, -1);
  vector<vector<int>> M;
  vector<array<int, 4>> gm;   // max holder key over gaps: [c-span X-women a, c-span Y-women b, d-span X a, d-span Y b]
  vector<array<int, 2>> hiP;  // last position of the module in c and in d
  vector<char> live;
  vector<char> inS(n, 0);
  // validity of a member set S (one woman type) under the current holders; fills g
  auto valid = [&](const vector<int>& S, array<int, 4>& g) -> bool {
    int lo[2] = {n, n}, hi[2] = {-1, -1};
    int mn[2][2] = {{INT_MAX, INT_MAX}, {INT_MAX, INT_MAX}};  // [man type][a/b]
    bool has[2] = {false, false};
    for (int w : S) {
      inS[w] = 1;
      for (int T : {0, 1}) lo[T] = min(lo[T], posT(T, w)), hi[T] = max(hi[T], posT(T, w));
      int h = held[w];
      if (h >= 0) { int T = I.tm[h]; has[T] = true; mn[T][0] = min(mn[T][0], I.a[h]); mn[T][1] = min(mn[T][1], I.b[h]); }
    }
    g = {INT_MIN, INT_MIN, INT_MIN, INT_MIN};
    for (int T : {0, 1})
      for (int p = lo[T]; p <= hi[T]; p++) {
        int w = atT(T, p);
        if (inS[w]) continue;
        int h = held[w];
        if (h < 0) { g[2 * T] = g[2 * T + 1] = INT_MAX; continue; }
        if (I.tw[w] == 0) g[2 * T] = max(g[2 * T], I.a[h]); else g[2 * T + 1] = max(g[2 * T + 1], I.b[h]);
      }
    gRealWork += (hi[0] - lo[0] + 1) + (hi[1] - lo[1] + 1);
    for (int w : S) inS[w] = 0;
    for (int T : {0, 1}) if (has[T] && !(g[2 * T] < mn[T][0] && g[2 * T + 1] < mn[T][1])) return false;
    return true;
  };
  int LIM = getenv("RELLIM") ? atoi(getenv("RELLIM")) : 2;
  auto build = [&]() {
    fill(mod.begin(), mod.end(), -1); M.clear(); gm.clear(); hiP.clear(); live.clear();
    for (int tau : {0, 1}) {
      vector<int> L;
      for (int p = 0; p < n; p++) if (I.tw[wc[p]] == tau) L.push_back(wc[p]);
      vector<char> asg(n, 0);
      for (int i = 0; i < (int)L.size(); i++) {
        if (asg[L[i]]) continue;
        vector<int> S = {L[i]}; asg[L[i]] = 1;
        array<int, 4> g, gS; valid(S, gS);
        int fails = 0;
        for (int j = i + 1; j < (int)L.size() && fails <= LIM; j++) {
          if (asg[L[j]]) continue;
          S.push_back(L[j]);
          if (valid(S, g)) { asg[L[j]] = 1; gS = g; fails = 0; }
          else { S.pop_back(); fails++; }
        }
        if (S.size() > 1) {
          for (int w : S) mod[w] = M.size();
          int h0 = -1, h1 = -1;
          for (int w : S) h0 = max(h0, I.c[w]), h1 = max(h1, I.d[w]);
          M.push_back(S); gm.push_back(gS); hiP.push_back({h0, h1}); live.push_back(1);
        }
      }
    }
  };
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else if (policy == "pmax") pr[m] = max(I.a[m], I.b[m]);
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  auto push = [&](int m) { if (policy == "lifo" || policy == "fifo") dq.push_back(m); else pq.push({pr[m], m}); };
  auto pop = [&]() -> int {
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() { return (policy == "lifo" || policy == "fifo") ? dq.empty() : pq.empty(); };
  for (int m = 0; m < n; m++) push(m);
  Res R; rebuilds = 0; gRealWork = 0;
  double relk = getenv("RELK") ? atof(getenv("RELK")) : 1.0;
  long long since = 0;
  build();
  long long dissolved = 0;
  while (!empty()) {
    int m = pop();
    if (isHeld[m]) continue;
    while (m >= 0) {
      if (R.Ch * (long long)n > gAbortWork) exit(2);
      if (since >= (long long)(relk * n)) { build(); rebuilds++; since = 0; }
      int T = I.tm[m], p = ptr[m];
      while (p < n) { int w = atT(T, p), h = held[w]; if (h < 0 || key(w, m) < key(w, h)) break; p++; }
      gRealWork += p - ptr[m] + 1;
      if (p >= n) { fprintf(stderr, "man %d unmatched: bug (R)\n", m); exit(1); }
      int ws = atT(T, p), md = mod[ws];
      R.Ch++; R.Au++; since++;
      int fallen = -1;
      bool ok = md >= 0 && live[md] && (T == 0 ? (gm[md][0] < I.a[m] && gm[md][1] < I.b[m]) : (gm[md][2] < I.a[m] && gm[md][3] < I.b[m]));
      if (ok) {
        vector<int>& S = M[md];
        int tau = I.tw[S[0]];
        vector<pair<int, int>> H;
        for (int w : S) if (held[w] >= 0) H.push_back({tau == 0 ? I.a[held[w]] : I.b[held[w]], held[w]});
        H.push_back({tau == 0 ? I.a[m] : I.b[m], m});
        sort(H.begin(), H.end());
        if (H.size() > S.size()) { fallen = H.back().second; H.pop_back(); }
        for (int w : S) held[w] = -1;
        // serial dictatorship: each man takes his first free M-woman in his own order
        vector<int> byC = S, byD = S;
        sort(byC.begin(), byC.end(), [&](int x, int y) { return I.c[x] < I.c[y]; });
        sort(byD.begin(), byD.end(), [&](int x, int y) { return I.d[x] < I.d[y]; });
        size_t ic = 0, id = 0;
        for (auto& [k, x] : H) {
          vector<int>& ord = I.tm[x] == 0 ? byC : byD; size_t& ix = I.tm[x] == 0 ? ic : id;
          while (held[ord[ix]] >= 0) ix++;
          int w = ord[ix]; held[w] = x; isHeld[x] = 1; ptr[x] = posT(I.tm[x], w);
        }
        gRealWork += S.size() * 3;
        if (fallen >= 0) { isHeld[fallen] = 0; ptr[fallen] = hiP[md][I.tm[fallen]] + 1; }
        R.Aex++;
      } else {
        if (md >= 0 && live[md]) { live[md] = 0; dissolved++; }
        int h = held[ws];
        held[ws] = m; isHeld[m] = 1; ptr[m] = p;
        if (h >= 0) { isHeld[h] = 0; ptr[h] = posT(I.tm[h], ws) + 1; fallen = h; }
        R.Aex++;
      }
      m = -1;
      if (fallen >= 0) { if (atomic) m = fallen; else push(fallen); }
    }
  }
  if (getenv("VERBR")) printf("   R %s/%d: Ch/n %.2f rebuilds %lld dissolved %lld modules %d\n", policy.c_str(), (int)atomic, (double)R.Ch / n, rebuilds, dissolved, (int)M.size());
  heldOut = held;
  return R;
}

// min over the 22 orders of Ch in mode QM (checks every result against MOSM)
long long evalCost(const Inst& I) {
  vector<int> ref = mosm(I);
  vector<string> polv = {"lifo", "fifo", "pa", "pb", "pmin", "pmax", "ra", "rb", "rmin", "rmax", "prand"};
  if (getenv("POLS")) { polv.clear(); stringstream ss(getenv("POLS")); string t; while (getline(ss, t, ',')) polv.push_back(t); }
  vector<const char*> pols; for (auto& x : polv) pols.push_back(x.c_str());
  long long best = LLONG_MAX; int nu;
  if (getenv("PAU")) {  // RB14 PAU (needs DEADT-free call; uses runT): one fixed order; cost = Ch + n per capped phase
    vector<int> h; long long rb;
    Res r = runT(I, "phase", 0, h, nu, rb);
    if (h != ref) { fprintf(stderr, "PAU: result differs from MOSM\n"); exit(1); }
    return r.Ch;
  }
  for (const char* p : pols) for (int at = 0; at < 2; at++) {
    vector<int> h;
    Res r;
    if (getenv("BUDGET")) {
      r = runB(I, "QM", p, at, h, nu);
      if (h != ref) { fprintf(stderr, "budget: result differs from MOSM\n"); exit(1); }
      if (getenv("VERBB")) printf("   %s/%s: Ch/n %.2f bulk %lld W/n %.2f\n", p, at ? "at" : "pool", (double)r.Ch / I.n, gBulk, (double)(r.Ch + 2LL * I.n * gBulk) / I.n);
      r.Ch += 2LL * I.n * gBulk;
    } else if (getenv("PHASED")) {
      long long bestH = LLONG_MAX;
      for (int st = 0; st < 2; st++) {
        gStartType = st; r = runPh(I, "QM", p, at, h, nu);
        if (h != ref) { fprintf(stderr, "phased: result differs from MOSM\n"); exit(1); }
        bestH = min(bestH, gHybrid);
        if (getenv("VERBP")) printf("   %s/%s start %c: Ch/n %.2f phases %lld hybrid/n %.2f\n", p, at ? "at" : "pool", "XY"[st], (double)r.Ch / I.n, gPhases, (double)gHybrid / I.n);
      }
      r.Ch = bestH;
    } else if (getenv("RELATIVE")) { long long rb; r = runR(I, p, at, h, rb); }
    else if (getenv("DEADT")) { long long rb; r = runT(I, p, at, h, nu, rb); if (getenv("TBUDGET")) r.Ch += 2LL * I.n * gBulk; }
    else if (getenv("SETTLE")) { long long rb; int sw; r = runS(I, p, at, h, nu, rb, sw); } else r = run(I, "QM", p, at, h, nu);
    if (h != ref) { fprintf(stderr, "result differs from MOSM\n"); exit(1); }
    best = min(best, r.Ch);
  }
  return best;
}

void report(const string& name, const Inst& I) {
  vector<int> ref = mosm(I);
  int n = I.n;
  const char* pols[] = {"lifo", "fifo", "pa", "pb", "pmin", "pmax", "ra", "rb", "rmin", "rmax", "prand"};
  vector<string> modes = {"P", "M", "Q", "QM"};
  if (getenv("MODES")) { modes.clear(); string m = getenv("MODES"); stringstream ss(m); string t; while (getline(ss, t, ',')) modes.push_back(t); }
  for (const string& mode : modes) {
    Res best; best.Au = LLONG_MAX;
    string bestName;
    int nu = 0;
    long long minAex = LLONG_MAX, minStr = LLONG_MAX, minBnd = LLONG_MAX, minCh = LLONG_MAX;
    for (const char* p : pols)
      for (int at = 0; at < 2; at++) {
        vector<int> h;
        Res r;
        if (mode == "QMT") { long long rb; r = runT(I, p, at, h, nu, rb); }
        else if (mode == "QMS") { long long rb; int sw; r = runS(I, p, at, h, nu, rb, sw);
          if (getenv("VERBS")) printf("   QMS %s/%s Ch/n=%.2f rebuilds=%lld settledW=%d\n", p, at ? "at" : "pool", (double)r.Ch / n, rb, sw); }
        else r = run(I, mode, p, at, h, nu);
        if (h != ref) { fprintf(stderr, "%s: result differs from MOSM (%s)\n", name.c_str(), p); exit(1); }
        if (r.Au < best.Au) best = r, bestName = string(p) + (at ? "/at" : "/pool");
        if (getenv("VERB")) printf("   %s %s/%s Au/n=%.2f Ch/n=%.2f Bnd/n=%.2f Aex/n=%.2f\n", mode.c_str(), p, at ? "at" : "pool", (double)r.Au / n, (double)r.Ch / n, (double)r.Bnd / n, (double)r.Aex / n);
        minAex = min(minAex, r.Aex); minCh = min(minCh, r.Ch); minStr = min(minStr, r.Str); minBnd = min(minBnd, r.Bnd);
      }
    printf("%-24s n=%5d %-2s units=%5d | min over orders: Au/n=%7.2f (%-9s) Ch/n=%7.2f Bnd/n=%7.2f Aex/n=%8.2f\n",
           name.c_str(), n, mode.c_str(), nu, (double)best.Au / n, bestName.c_str(), (double)minCh / n, (double)minBnd / n,
           (double)minAex / n);
  }
  fflush(stdout);
}

int main(int argc, char** argv) {
  rng.seed(12345);
  if (argc >= 3 && (string(argv[1]) == "eval" || string(argv[1]) == "evallab")) {
    // cost of run-buffer GS = min over {men, women proposing} x 22 orders of Ch (mode QM)
    Inst I;
    if (string(argv[1]) == "evallab") I = loadLab(argv[2]);
    else if (string(argv[2]) == "gen") { rng.seed(atoi(argv[8])); I = gen(atoi(argv[4]), argv[3], atof(argv[5]), atof(argv[6]), atof(argv[7])); }
    else I = load(argv[2]);
    Inst T = I;
    T.a = I.c; T.b = I.d; T.c = I.a; T.d = I.b; T.tm = I.tw; T.tw = I.tm;
    long long cm = evalCost(I), cw = evalCost(T);
    printf("rbgs_cost %lld men %lld women %lld n %d cost/n %.3f\n", min(cm, cw), cm, cw, I.n, (double)min(cm, cw) / I.n);
    if (gUsize) { printf("unit-size histogram at entry (all runs; bucket = floor(log2 size)):"); for (int b = 0; b < 64; b++) if ((*gUsize)[b]) printf(" [%d..%d]:%lld", 1 << b, (2 << b) - 1, (*gUsize)[b]); printf("\n"); }
    return 0;
  }
  if (argc >= 3 && string(argv[1]) == "dep") {
    // D: m -> mu(w) for every w that m prefers to mu(m) (mu = MOSM). Topological order of D's
    // condensation = sequential GS with no displacement outside SCCs. Reports SCC sizes and the
    // cost of GS / RBGS (QM) when men start in that order (inside an SCC: by key of 4 rules).
    Inst I;
    if (string(argv[2]) == "gen") { rng.seed(atoi(argv[8])); I = gen(atoi(argv[4]), argv[3], atof(argv[5]), atof(argv[6]), atof(argv[7])); }
    else if (string(argv[2]) == "lab") I = loadLab(argv[3]); else I = load(argv[2]);
    if (getenv("TRANS")) { Inst T = I; T.a = I.c; T.b = I.d; T.c = I.a; T.d = I.b; T.tm = I.tw; T.tw = I.tm; I = T; }
    int n = I.n;
    vector<int> held = mosm(I), mu(n);
    for (int w = 0; w < n; w++) mu[held[w]] = w;
    vector<int> wc(n), wd(n);
    for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
    vector<vector<int>> adj(n);
    long long E = 0;
    for (int m = 0; m < n; m++) {
      for (int i = 0; i < n; i++) { int w = I.tm[m] == 0 ? wc[i] : wd[i]; if (w == mu[m]) break; adj[m].push_back(held[w]); E++; }
    }
    // Tarjan (iterative)
    vector<int> idx(n, -1), low(n), comp(n, -1), st; vector<char> on(n, 0); int t = 0, nc = 0;
    for (int s0 = 0; s0 < n; s0++) {
      if (idx[s0] >= 0) continue;
      vector<pair<int, int>> cs = {{s0, 0}};
      idx[s0] = low[s0] = t++; st.push_back(s0); on[s0] = 1;
      while (!cs.empty()) {
        auto& [v, ei] = cs.back();
        if (ei < (int)adj[v].size()) {
          int u = adj[v][ei++];
          if (idx[u] < 0) { idx[u] = low[u] = t++; st.push_back(u); on[u] = 1; cs.push_back({u, 0}); }
          else if (on[u]) low[v] = min(low[v], idx[u]);
        } else {
          if (low[v] == idx[v]) { while (true) { int u = st.back(); st.pop_back(); on[u] = 0; comp[u] = nc; if (u == v) break; } nc++; }
          int vv = v; cs.pop_back();
          if (!cs.empty()) low[cs.back().first] = min(low[cs.back().first], low[vv]);
        }
      }
    }
    vector<int> csz(nc, 0);
    for (int m = 0; m < n; m++) csz[comp[m]]++;
    int big = *max_element(csz.begin(), csz.end()), nontriv = 0, inNon = 0;
    for (int c = 0; c < nc; c++) if (csz[c] > 1) nontriv++, inNon += csz[c];
    // Tarjan numbers SCCs in reverse topological order: a sink SCC (no dependencies) gets 0 first.
    printf("n %d edges/n %.1f SCCs %d nontrivial %d men in nontrivial %d largest %d\n", n, (double)E / n, nc, nontriv, inNon, big);
    {  // trigger candidates: men outside a nontrivial SCC with a D-edge into it
      int Tin = 0;
      for (int m = 0; m < n; m++) { bool t = false; for (int v : adj[m]) if (comp[v] != comp[m] && csz[comp[v]] > 1) t = true; Tin += t; }
      printf("  T_in (men with an edge into another nontrivial SCC) %d\n", Tin);
    }
    {  // plain sequential GS in SCC order (sinks first, atomic cascades); count cross-SCC displacements
      vector<int> ordm(n); iota(ordm.begin(), ordm.end(), 0);
      sort(ordm.begin(), ordm.end(), [&](int x, int y) { return comp[x] != comp[y] ? comp[x] < comp[y] : min(I.a[x], I.b[x]) < min(I.a[y], I.b[y]); });
      vector<int> ptr(n, 0), h(n, -1);
      long long A = 0, cross = 0;
      auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
      for (int m0 : ordm) {
        int m = m0;
        while (m >= 0) {
          int w = I.tm[m] == 0 ? wc[ptr[m]++] : wd[ptr[m]++];
          int g = h[w];
          if (g >= 0 && key(w, g) < key(w, m)) continue;
          A++; h[w] = m;
          if (g >= 0 && comp[g] != comp[m]) cross++;
          m = g;
        }
      }
      printf("  sequential SCC-order GS: A/n %.2f cross-SCC displacements %lld result %s\n", (double)A / n, cross, h == held ? "= MOSM" : "!= MOSM");
    }
    if (getenv("SUBOUT")) {  // write the sub-instance (largest SCC men, their MOSM wives)
      int bc = max_element(csz.begin(), csz.end()) - csz.begin();
      vector<int> M, W;
      for (int m = 0; m < n; m++) if (comp[m] == bc) M.push_back(m), W.push_back(mu[m]);
      int k = M.size();
      auto rk = [&](vector<int> ids, const vector<int>& key) { vector<int> o(k); iota(o.begin(), o.end(), 0);
        sort(o.begin(), o.end(), [&](int x, int y) { return key[ids[x]] < key[ids[y]]; }); vector<int> r(k); for (int i = 0; i < k; i++) r[o[i]] = i; return r; };
      FILE* f = fopen(getenv("SUBOUT"), "w");
      fprintf(f, "%d\n", k);
      for (auto v : {rk(M, I.a), rk(M, I.b), rk(W, I.c), rk(W, I.d)}) { for (int i = 0; i < k; i++) fprintf(f, "%d ", v[i]); fprintf(f, "\n"); }
      for (int i = 0; i < k; i++) fprintf(f, "%d ", I.tm[M[i]]); fprintf(f, "\n");
      for (int i = 0; i < k; i++) fprintf(f, "%d ", I.tw[W[i]]); fprintf(f, "\n");
      fclose(f);
    }
    const char* inner[] = {"a", "b", "min", "max"};
    vector<int> ref = held;
    for (const char* in : inner) {
      gCustom.assign(n, 0);
      for (int m = 0; m < n; m++) {
        long long k = string(in) == "a" ? I.a[m] : string(in) == "b" ? I.b[m] : string(in) == "min" ? min(I.a[m], I.b[m]) : max(I.a[m], I.b[m]);
        gCustom[m] = (long long)comp[m] * (n + 1) + k;
      }
      for (int at = 0; at < 2; at++) {
        vector<int> h; int nu; Res rq = run(I, "QM", "custom", at, h, nu);
        if (h != ref) { fprintf(stderr, "dep: mismatch\n"); return 1; }
        vector<int> h2; int nu2; Res rp = run(I, "singleton", "custom", at, h2, nu2);
        printf("  oracle order (SCC topo, inside by %-3s, %s): GS A/n %.2f  RBGS Ch/n %.2f\n", in, at ? "atomic" : "pool", (double)rp.Aex / n, (double)rq.Ch / n);
      }
    }
    return 0;
  }
  if (argc >= 3 && string(argv[1]) == "one") {
    Inst I;
    if (string(argv[2]) == "gen") { rng.seed(atoi(argv[8])); I = gen(atoi(argv[4]), argv[3], atof(argv[5]), atof(argv[6]), atof(argv[7])); argv += 6; }
    else I = load(argv[2]);
    if (getenv("SAVE")) { FILE* f = fopen(getenv("SAVE"), "w"); fprintf(f, "%d\n", I.n); for (auto* v : {&I.a, &I.b, &I.c, &I.d, &I.tm, &I.tw}) { for (int x : *v) fprintf(f, "%d ", x); fprintf(f, "\n"); } fclose(f); } vector<int> h; int nu; long long rb;
    Res r = runT(I, argv[3], atoi(argv[4]), h, nu, rb);
    vector<int> ref = mosm(I);
    printf("%s\n", h == ref ? "OK" : "MISMATCH");
    for (int w = 0; w < I.n; w++) printf("w%d: got %d ref %d\n", w, h[w], ref[w]);
    return 0;
  }
  if (argc >= 2 && string(argv[1]) == "test") {  // planted modules/runs, all modes and orders vs MOSM
    int T = atoi(argv[2]);
    rng.seed(777);
    for (int t = 0; t < T; t++) {
      int n = 2 + rng() % 14;
      Inst I = gen(n, "uni", 0.5, 0.5, 0);
      // women: random blocks; d-order = blocks permuted, each block internally same or shuffled
      vector<int> cw(n); iota(cw.begin(), cw.end(), 0); shuffle(cw.begin(), cw.end(), rng);
      vector<vector<int>> blocks;
      for (int i = 0; i < n;) { int L = 1 + rng() % 5; vector<int> bl; for (int j = i; j < min(n, i + L); j++) bl.push_back(cw[j]); blocks.push_back(bl); i += L; }
      for (int r = 0; r < n; r++) I.c[cw[r]] = r;
      vector<int> ord(blocks.size()); iota(ord.begin(), ord.end(), 0); shuffle(ord.begin(), ord.end(), rng);
      int r = 0;
      for (int bi : ord) {
        vector<int> bl = blocks[bi];
        int mode = rng() % 3;  // 0 same order, 1 shuffled, 2 reversed
        if (mode == 1) shuffle(bl.begin(), bl.end(), rng); else if (mode == 2) reverse(bl.begin(), bl.end());
        int ty = rng() % 2; bool pure = rng() % 3 != 0;
        for (int w : bl) { I.d[w] = r++; I.tw[w] = pure ? ty : (int)(rng() % 2); }
      }
      if (getenv("DUMP") && t == atoi(getenv("DUMP"))) {
        FILE* f = fopen("/tmp/t_dump.txt", "w"); fprintf(f, "%d\n", I.n);
        for (auto* v : {&I.a, &I.b, &I.c, &I.d, &I.tm, &I.tw}) { for (int x : *v) fprintf(f, "%d ", x); fprintf(f, "\n"); }
        fclose(f);
      }
      report("test" + to_string(t), I);
    }
    return 0;
  }
  if (argc >= 2 && string(argv[1]) == "gen") {
    string kind = argv[2];
    int n = atoi(argv[3]);
    double pxm = atof(argv[4]), pxw = atof(argv[5]), noise = atof(argv[6]);
    int seed = atoi(argv[7]);
    rng.seed(seed);
    Inst I = gen(n, kind, pxm, pxw, noise);
    char buf[200];
    snprintf(buf, sizeof buf, "%s n%d x%.1f/%.1f s%d", kind.c_str(), n, pxm, pxw, seed);
    report(buf, I);
    Inst T = I;
    T.a = I.c; T.b = I.d; T.c = I.a; T.d = I.b; T.tm = I.tw; T.tw = I.tm;
    report(string(buf) + "^T", T);
    return 0;
  }
  for (int i = 1; i < argc; i++) {
    string p = argv[i];
    string nm = p.substr(p.find_last_of('/') + 1);
    Inst I = load(p);
    report(nm, I);
    Inst T = I;  // transpose: women propose
    T.a = I.c; T.b = I.d; T.c = I.a; T.d = I.b; T.tm = I.tw; T.tw = I.tm;
    report(nm + "^T", T);
  }
  return 0;
}
