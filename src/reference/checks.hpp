// checks.hpp - self tests and structural checks F3, F4, F5, F7 (owner: lab).
#pragma once
#include "tlsm.hpp"

namespace tlsm {

// A random generator configuration, to cover many families in tests.
inline GenParams random_params(Rng& r) {
  static const vector<string> kinds = {"uniform", "uniform", "uniform", "blocks", "cycles"};
  static const vector<string> rels = {"indep", "same", "rev", "noisy2", "revnoisy2"};
  static const vector<double> ps = {0.5, 0.0, 1.0, 0.9, 0.1, 0.5};
  GenParams P;
  P.kind = kinds[r() % kinds.size()];
  P.men_rel = rels[r() % rels.size()];
  P.women_rel = rels[r() % rels.size()];
  P.px = ps[r() % ps.size()];
  P.pw = ps[r() % ps.size()];
  P.blocks = 1 + r() % 4;
  return P;
}

// Independent brute force: all permutations, stable_brute. n <= 7.
inline size_t count_stable_perm(const Instance& I) {
  vector<int> p(I.n);
  std::iota(p.begin(), p.end(), 0);
  size_t c = 0;
  do { if (stable_brute(I, p)) c++; } while (std::next_permutation(p.begin(), p.end()));
  return c;
}

inline int selftest(int trials) {
  Rng r(12345);
  long long fails = 0, tests = 0;
  auto fail = [&](const string& what, const Instance& I) {
    fails++;
    if (fails <= 5) {
      std::cerr << "FAIL: " << what << "\n" << format_instance(I) << "\n";
      write_instance(I, "selftest_fail_" + std::to_string(fails) + ".txt", what);
    }
  };
  // 1) tiny n: enumeration vs permutation brute force; MOSM in list and man-optimal; checkers agree.
  for (int t = 0; t < trials; t++) {
    int n = 1 + r() % 7;
    Instance I = generate(n, random_params(r), r);
    auto all = all_stable(I);
    tests++;
    if (all.size() != count_stable_perm(I)) fail("enum count != permutation brute count", I);
    auto ref = gs_reference(I);
    if (std::find(all.begin(), all.end(), ref) == all.end()) fail("MOSM not in enumeration", I);
    for (auto& m : all)
      for (int x = 0; x < n; x++)
        if (I.man_rank(x, ref[x]) > I.man_rank(x, m[x])) fail("MOSM not man-optimal", I);
    // checkers on all permutations
    vector<int> p(n);
    std::iota(p.begin(), p.end(), 0);
    do {
      if (stable_brute(I, p) != stable_fast(I, p)) { fail("stable_brute != stable_fast", I); break; }
    } while (std::next_permutation(p.begin(), p.end()));
  }
  // 2) n up to 9 enumeration vs checker; larger n: all rules and rounds equal reference; checkers agree.
  for (int t = 0; t < trials; t++) {
    int n = 8 + r() % 2;
    Instance I = generate(n, random_params(r), r);
    auto all = all_stable(I);
    for (auto& m : all) if (!stable_brute(I, m)) fail("enumerated matching unstable", I);
    tests++;
  }
  for (int t = 0; t < trials; t++) {
    int n = 1 + (int)(r() % (t % 10 == 0 ? 3000 : 200));
    Instance I = generate(n, random_params(r), r);
    // round trip through text format
    Instance J = parse_instance(format_instance(I));
    if (J.a != I.a || J.b != I.b || J.c != I.c || J.d != I.d || J.mt != I.mt || J.wt != I.wt) fail("format round trip", I);
    auto ref = gs_reference(I);
    tests++;
    if (!stable_brute(I, ref) || !stable_fast(I, ref)) fail("reference GS unstable", I);
    for (auto& rule : all_rules()) {
      if (rule == "nodisp" && n > 500) continue;
      GSStats s;
      auto m = mosm_fast(I, rule, &s, r());
      if (m != ref) fail("fast GS " + rule + " != reference", I);
      if (s.displacements != s.accepted - n) fail("displacements != A - n", I);
    }
    FastGS g(I);
    if (g.run_rounds() != ref) fail("rounds != reference", I);
    // perturbed matchings: checkers agree
    if (n >= 2 && n <= 300) {
      for (int k = 0; k < 5; k++) {
        auto m = ref;
        int i = r() % n, j = r() % n;
        std::swap(m[i], m[j]);
        if (stable_brute(I, m) != stable_fast(I, m)) fail("checkers disagree on perturbed", I);
      }
    }
  }
  printf("selftest: %lld instance groups, %lld failures\n", tests, fails);
  return fails ? 1 : 0;
}

// ---------------------------------------------------------------- structure checks
struct StructCounts {
  long long inst = 0, matchings = 0, f3 = 0, f4 = 0, f4w = 0, f5steps = 0, f5 = 0, f7cases = 0, f7 = 0, f7a = 0;
};

// F3: inside each block (tm, tw), pairs keep order: wr(tm,w1) < wr(tm,w2) iff mr(tw,m1) < mr(tw,m2).
inline bool check_f3(const Instance& I, const vector<int>& wife) {
  for (int m1 = 0; m1 < I.n; m1++)
    for (int m2 = 0; m2 < I.n; m2++) {
      if (m1 == m2 || I.mt[m1] != I.mt[m2]) continue;
      int w1 = wife[m1], w2 = wife[m2];
      if (I.wt[w1] != I.wt[w2]) continue;
      int tm = I.mt[m1], tw = I.wt[w1];
      if ((I.wr(tm, w1) < I.wr(tm, w2)) != (I.mr(tw, m1) < I.mr(tw, m2))) return false;
    }
  return true;
}
// F4 both sides.
inline bool check_f4(const Instance& I, const vector<int>& wife) {
  int n = I.n;
  auto h = inverse_matching(wife, n);
  for (int t = 0; t < 2; t++) {
    vector<char> Wset(n, 0), men(n, 0);
    for (int m = 0; m < n; m++) if (I.mt[m] == t) { men[m] = 1; Wset[wife[m]] = 1; }
    auto g = greedy_side(I, t, Wset, men);
    for (int m = 0; m < n; m++) if (I.mt[m] == t && g[m] != wife[m]) return false;
  }
  return true;
}
inline bool check_f4w(const Instance& I, const vector<int>& wife) {
  int n = I.n;
  auto h = inverse_matching(wife, n);
  for (int t = 0; t < 2; t++) {
    vector<char> Mset(n, 0), women(n, 0);
    for (int w = 0; w < n; w++) if (I.wt[w] == t) { women[w] = 1; Mset[h[w]] = 1; }
    auto g = greedy_side_w(I, t, Mset, women);
    for (int w = 0; w < n; w++) if (I.wt[w] == t && g[w] != h[w]) return false;
  }
  return true;
}
// F5: GS state (husband) is the F4 greedy on H_t for each man type t.
inline bool check_f5_state(const Instance& I, const vector<int>& husband) {
  int n = I.n;
  for (int t = 0; t < 2; t++) {
    vector<char> Wset(n, 0), men(n, 0);
    for (int w = 0; w < n; w++) if (husband[w] >= 0 && I.mt[husband[w]] == t) { Wset[w] = 1; men[husband[w]] = 1; }
    auto g = greedy_side(I, t, Wset, men);
    for (int w = 0; w < n; w++)
      if (Wset[w] && g[husband[w]] != w) return false;
  }
  return true;
}

inline int structcheck(int trials, int nmax) {
  Rng r(777);
  StructCounts C;
  long long fails = 0;
  auto fail = [&](const string& what, const Instance& I) {
    fails++;
    if (fails <= 5) {
      std::cerr << "FAIL " << what << "\n" << format_instance(I) << "\n";
      write_instance(I, "struct_fail_" + std::to_string(fails) + ".txt", what);
    }
  };
  for (int t = 0; t < trials; t++) {
    int n = 1 + r() % nmax;
    Instance I = generate(n, random_params(r), r);
    C.inst++;
    auto all = all_stable(I);
    for (auto& m : all) {
      C.matchings++;
      if (!check_f3(I, m)) { C.f3++; fail("F3", I); }
      if (!check_f4(I, m)) { C.f4++; fail("F4 (men side)", I); }
      if (!check_f4w(I, m)) { C.f4w++; fail("F4 (women side)", I); }
    }
    // F5 under several rules, after every acceptance
    for (const string rule : {"random", "fifo", "lifo", "xfirst", "pmin"}) {
      FastGS g(I);
      bool bad = false;
      g.on_accept = [&](int, int, int) {
        C.f5steps++;
        if (!bad && !check_f5_state(I, g.husband)) bad = true;
      };
      g.run(rule, r());
      if (bad) { C.f5++; fail("F5 rule " + rule, I); }
    }
    // F7
    int ma = I.mxl[0], mb = I.myl[0], wc = I.wxl[0], wd = I.wyl[0];
    auto mtop = [&](int m) { return I.mt[m] == X ? wc : wd; };
    auto wtop = [&](int w) { return I.wt[w] == X ? ma : mb; };
    bool soul = false;
    for (int m : {ma, mb})
      for (int w : {wc, wd})
        if (mtop(m) == w && wtop(w) == m) soul = true;
    if (soul) {  // F7a (proved easy part): a mutual-top pair among them is matched in every stable matching
      for (int m : {ma, mb})
        for (int w : {wc, wd})
          if (mtop(m) == w && wtop(w) == m)
            for (auto& mm : all)
              if (mm[m] != w) { C.f7a++; fail("F7a soulmates not matched", I); }
    } else {
      C.f7cases++;
      bool ok = ma != mb && wc != wd;
      for (auto& m : all) {
        int x = m[ma], y = m[mb];
        if (!((x == wc && y == wd) || (x == wd && y == wc))) ok = false;
      }
      if (!ok) {  // F7 is retracted (BRIEF.md): expected to fail; save a few counterexamples
        C.f7++;
        if (C.f7 <= 3) write_instance(I, "instances/f7_counterexample_" + std::to_string(C.f7) + ".txt", "F7 (retracted) fails here");
      }
    }
  }
  printf("struct: %lld instances (n<=%d), %lld stable matchings, %lld GS steps checked\n", C.inst, nmax, C.matchings, C.f5steps);
  printf("  F3 failures %lld | F4 men-side %lld | F4 women-side %lld | F5 %lld | F7a (soulmates) %lld\n",
         C.f3, C.f4, C.f4w, C.f5, C.f7a);
  printf("  F7 (retracted): no-soulmate cases %lld, failures %lld (expected > 0)\n", C.f7cases, C.f7);
  return fails ? 1 : 0;
}

}  // namespace tlsm
