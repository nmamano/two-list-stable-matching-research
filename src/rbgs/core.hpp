#pragma once
// core.hpp (shared GS core): GS with cascade anatomy counters (run-buffer implementation, 2026-10-05).
// Notation as in BRIEF.md. A cascade = chain of same-type displacements, run atomically.
// A man displaced by a man of the other type (a "leak") goes to the pool; the pool policy
// decides which cascade starts next.
// Counters: A accepted proposals, S leaks (type switches of a held woman), T transitions
// (an in-flight man displaced from a woman of type t is accepted by a same-world chooser of
// the other type), SH shifts (same type), IH inside hits (first chooser hit by a fresh man).
#include <bits/stdc++.h>
using namespace std;

struct Inst {
  int n;
  vector<int> a, b, c, d, tm, tw;  // ranks; types 0 = X, 1 = Y
};

static mt19937_64 rng;

vector<int> randperm(int n) {
  vector<int> p(n);
  iota(p.begin(), p.end(), 0);
  shuffle(p.begin(), p.end(), rng);
  return p;
}
// rank array q close to rank array p: q = ranks of (p[i] + noise*N(0,1)*n); if rev, use n-1-p[i]
vector<int> noisy(const vector<int>& p, double noise, bool rev) {
  int n = p.size();
  normal_distribution<double> N(0, 1);
  vector<pair<double, int>> v(n);
  for (int i = 0; i < n; i++) v[i] = {(rev ? n - 1 - p[i] : p[i]) + noise * n * N(rng), i};
  sort(v.begin(), v.end());
  vector<int> q(n);
  for (int r = 0; r < n; r++) q[v[r].second] = r;
  return q;
}

Inst gen(int n, string kind, double pxm, double pxw, double noise) {
  Inst I;
  I.n = n;
  I.a = randperm(n);
  I.c = randperm(n);
  if (kind == "uni") {
    I.b = randperm(n);
    I.d = randperm(n);
  } else if (kind == "corr") {
    I.b = noisy(I.a, noise, false);
    I.d = noisy(I.c, noise, false);
  } else if (kind == "anti") {
    I.b = noisy(I.a, noise, true);
    I.d = noisy(I.c, noise, true);
  } else if (kind == "mix") {  // men correlated, women anti
    I.b = noisy(I.a, noise, false);
    I.d = noisy(I.c, noise, true);
  } else {
    fprintf(stderr, "bad kind\n");
    exit(1);
  }
  uniform_real_distribution<double> U(0, 1);
  I.tm.resize(n);
  I.tw.resize(n);
  for (int i = 0; i < n; i++) I.tm[i] = U(rng) < pxm ? 0 : 1;
  for (int i = 0; i < n; i++) I.tw[i] = U(rng) < pxw ? 0 : 1;
  return I;
}

struct Stats {
  long long A = 0, S = 0, T = 0, SH = 0, IH = 0, props = 0, starts = 0, phases = 0;
};

// policy: lifo, fifo, pa (min a first), pb, pmin (min(a,b)), prand, phase
vector<int> gs(const Inst& I, const string& policy, Stats& st) {
  int n = I.n;
  vector<int> wc(n), wd(n);
  for (int w = 0; w < n; w++) wc[I.c[w]] = w, wd[I.d[w]] = w;
  vector<int> ptr(n, 0), held(n, -1), state(n, 2);  // state: 0 L(from X-woman),1 B,2 inside
  auto key = [&](int w, int m) { return I.tw[w] == 0 ? I.a[m] : I.b[m]; };
  auto lst = [&](int m, int i) { return I.tm[m] == 0 ? wc[i] : wd[i]; };
  vector<long long> pr(n);
  for (int m = 0; m < n; m++) {
    if (policy == "pa") pr[m] = I.a[m];
    else if (policy == "pb") pr[m] = I.b[m];
    else if (policy == "pmin") pr[m] = min(I.a[m], I.b[m]);
    else pr[m] = rng() % (1LL << 40);
  }
  deque<int> dq;
  priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> pq;
  deque<int> phq[2];
  int curphase = 0;
  auto push = [&](int m) {
    if (policy == "lifo" || policy == "fifo") dq.push_back(m);
    else if (policy == "phase") phq[I.tm[m]].push_back(m);
    else pq.push({pr[m], m});
  };
  auto pop = [&]() -> int {
    if (policy == "lifo") { int m = dq.back(); dq.pop_back(); return m; }
    if (policy == "fifo") { int m = dq.front(); dq.pop_front(); return m; }
    if (policy == "phase") {
      if (phq[curphase].empty()) { curphase ^= 1; st.phases++; }
      int m = phq[curphase].back(); phq[curphase].pop_back(); return m;
    }
    int m = pq.top().second; pq.pop(); return m;
  };
  auto empty = [&]() {
    if (policy == "lifo" || policy == "fifo") return dq.empty();
    if (policy == "phase") return phq[0].empty() && phq[1].empty();
    return pq.empty();
  };
  if (policy == "phase") { st.phases = 1; curphase = 0; }
  for (int m = 0; m < n; m++) push(m);
  if (policy == "phase" && phq[0].empty()) curphase = 1;
  while (!empty()) {
    int m = pop();
    st.starts++;
    // run cascade atomically
    while (m >= 0) {
      int w = lst(m, ptr[m]++);
      st.props++;
      int h = held[w];
      if (h >= 0 && key(w, h) < key(w, m)) continue;  // rejected
      st.A++;
      held[w] = m;
      if (h < 0) { m = -1; break; }
      if (I.tm[h] != I.tm[m]) {  // leak
        st.S++;
        state[h] = I.tw[w];
        state[m] = 2;
        push(h);
        m = -1;
        break;
      }
      // same-world displacement
      if (state[m] == 2) st.IH++;
      else if (state[m] == I.tw[w]) st.SH++;
      else st.T++;
      state[m] = 2;
      state[h] = I.tw[w];
      m = h;
    }
  }
  return held;  // held[w] = man
}

bool stable(const Inst& I, const vector<int>& held) {
  int n = I.n;
  vector<int> wife(n);
  for (int w = 0; w < n; w++) wife[held[w]] = w;
  for (int m = 0; m < n; m++)
    for (int w = 0; w < n; w++) {
      if (wife[m] == w) continue;
      int rm = I.tm[m] == 0 ? I.c[w] : I.d[w], rc = I.tm[m] == 0 ? I.c[wife[m]] : I.d[wife[m]];
      int km = I.tw[w] == 0 ? I.a[m] : I.b[m], kc = I.tw[w] == 0 ? I.a[held[w]] : I.b[held[w]];
      if (rm < rc && km < kc) return false;
    }
  return true;
}

