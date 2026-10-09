// structured solver: rotation posets of 2-list instances. Hill climbing to maximize an objective.
// Usage: rot n iters seed obj [maxMatchings]
// obj: 0 = #rotations, 1 = poset height (longest chain), 2 = max rotation length,
//      3 = #stable matchings (log), 4 = height with tie-break by #rotations
#define main main_orig
#include "tarski.cpp"
#undef main

typedef vector<pair<int,int>> Rot;

vector<Rot> exposed(const Inst& I, const vector<int>& mu) {
  int n = I.n;
  vector<int> inv(n);
  for (int m = 0; m < n; m++) inv[mu[m]] = m;
  vector<int> nxt(n, -1);
  for (int m = 0; m < n; m++) {
    int cur = mr(I, m, mu[m]);
    int best = -1, bestr = INT_MAX;
    for (int w = 0; w < n; w++) {
      int r = mr(I, m, w);
      if (r > cur && r < bestr && wr(I, w, m) < wr(I, w, inv[w])) { bestr = r; best = w; }
    }
    if (best != -1) nxt[m] = inv[best];
  }
  vector<Rot> out;
  vector<int> state(n, 0);  // 0 unvisited, 1 on path, 2 done
  for (int s = 0; s < n; s++) {
    if (state[s]) continue;
    vector<int> path;
    int m = s;
    while (m != -1 && state[m] == 0) { state[m] = 1; path.push_back(m); m = nxt[m]; }
    if (m != -1 && state[m] == 1) {
      // cycle from m
      Rot r;
      size_t k = find(path.begin(), path.end(), m) - path.begin();
      vector<int> cyc(path.begin() + k, path.end());
      rotate(cyc.begin(), min_element(cyc.begin(), cyc.end()), cyc.end());
      for (int x : cyc) r.push_back({x, mu[x]});
      out.push_back(r);
    }
    for (int x : path) state[x] = 2;
  }
  return out;
}

struct PosetInfo { int nrot = 0, height = 0, maxlen = 0, width = 0; long nmatch = 0; bool overflow = false; vector<Rot> rots; vector<unsigned long long> below; };

PosetInfo analyze(const Inst& I, long maxM) {
  PosetInfo P;
  int n = I.n;
  vector<int> mo = gs_plain(I, true);
  map<vector<int>, unsigned long long> seen;
  map<Rot, int> rid;
  vector<vector<int>> stack{mo};
  seen[mo] = 0;
  vector<unsigned long long> closed;
  while (!stack.empty()) {
    vector<int> mu = stack.back(); stack.pop_back();
    unsigned long long S = seen[mu];
    closed.push_back(S);
    for (auto& r : exposed(I, mu)) {
      if (!rid.count(r)) { int id = rid.size(); if (id >= 64) { P.overflow = true; return P; } rid[r] = id; }
      vector<int> nm = mu;
      for (size_t i = 0; i < r.size(); i++) nm[r[i].first] = r[(i + 1) % r.size()].second;
      if (!seen.count(nm)) {
        seen[nm] = S | (1ULL << rid[r]);
        stack.push_back(nm);
        if ((long)seen.size() > maxM) { P.overflow = true; return P; }
      }
    }
  }
  int k = rid.size();
  P.nrot = k; P.nmatch = seen.size();
  P.rots.resize(k);
  for (auto& [r, i] : rid) { P.rots[i] = r; P.maxlen = max(P.maxlen, (int)r.size()); }
  // below[j] = set of i that must precede j
  P.below.assign(k, 0);
  for (int j = 0; j < k; j++) {
    unsigned long long acc = ~0ULL;
    for (auto S : closed) if (S >> j & 1) acc &= S;
    P.below[j] = acc & ~(1ULL << j);
  }
  // height: longest chain
  vector<int> h(k, 0);
  // topological by popcount of below
  vector<int> ord(k); iota(ord.begin(), ord.end(), 0);
  sort(ord.begin(), ord.end(), [&](int x, int y) { return __builtin_popcountll(P.below[x]) < __builtin_popcountll(P.below[y]); });
  for (int j : ord) { h[j] = 1; for (int i = 0; i < k; i++) if (P.below[j] >> i & 1) h[j] = max(h[j], h[i] + 1); P.height = max(P.height, h[j]); }
  return P;
}

#ifndef NO_ROT_MAIN
int main(int argc, char** argv) {
  int n = atoi(argv[1]); long iters = atol(argv[2]); mt19937_64 rng(atoll(argv[3]));
  int obj = atoi(argv[4]); long maxM = argc > 5 ? atol(argv[5]) : 20000;
  auto val = [&](const PosetInfo& P) -> double {
    if (P.overflow) return -1;
    if (obj == 0) return P.nrot;
    if (obj == 1) return P.height;
    if (obj == 2) return P.maxlen;
    if (obj == 3) return log((double)P.nmatch);
    return P.height + 0.01 * P.nrot;
  };
  Inst I = gen(n, rng, 0);
  PosetInfo B = analyze(I, maxM); double bv = val(B);
  for (long it = 0; it < iters; it++) {
    Inst J = I;
    int k = rng() % 6;
    if (k < 4) { vector<int>& v = k == 0 ? J.a : k == 1 ? J.b : k == 2 ? J.c : J.d; int i = rng() % n, j = rng() % n; swap(v[i], v[j]); }
    else if (k == 4) J.tm[rng() % n] ^= 1; else J.tw[rng() % n] ^= 1;
    PosetInfo P = analyze(J, maxM); double v = val(P);
    if (v >= bv) { bv = v; B = P; I = J; }
  }
  printf("n=%d obj=%d: rotations %d height %d maxlen %d matchings %ld\n", n, obj, B.nrot, B.height, B.maxlen, B.nmatch);
  auto pr = [&](const char* nm, vector<int>& v) { printf("%s", nm); for (int x : v) printf(" %d", x); printf("\n"); };
  pr("a", I.a); pr("b", I.b); pr("c", I.c); pr("d", I.d); pr("tm", I.tm); pr("tw", I.tw);
  for (int i = 0; i < B.nrot; i++) {
    printf("rot %d len %zu below:", i, B.rots[i].size());
    for (int j = 0; j < B.nrot; j++) if (B.below[i] >> j & 1) printf(" %d", j);
    printf(" | pairs:");
    for (auto [m, w] : B.rots[i]) printf(" (m%d%c,w%d%c)", m, "XY"[I.tm[m]], w, "XY"[I.tw[w]]);
    printf("\n");
  }
}
#endif
