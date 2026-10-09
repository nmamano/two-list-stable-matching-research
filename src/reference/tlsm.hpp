// tlsm.hpp - 2-list stable matching lab core (owner: lab). Header-only, C++17.
// Notation follows BRIEF.md: ranks a(m), b(m), c(w), d(w); 0 = best.
// Type X man ranks women by c, type Y man by d. Type X woman ranks men by a, type Y woman by b.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace tlsm {
using std::string;
using std::vector;

enum : char { X = 0, Y = 1 };
typedef std::mt19937_64 Rng;

// ---------------------------------------------------------------- instance
struct Instance {
  int n = 0;
  vector<int> a, b, c, d;  // ranks: a[m], b[m], c[w], d[w]
  vector<char> mt, wt;     // types: X = 0, Y = 1
  vector<int> mxl, myl, wxl, wyl;  // lists (position -> id): men_x_ranking, ...

  int man_rank(int m, int w) const { return mt[m] == X ? c[w] : d[w]; }
  int woman_rank(int w, int m) const { return wt[w] == X ? a[m] : b[m]; }
  const vector<int>& man_list(int m) const { return mt[m] == X ? wxl : wyl; }
  const vector<int>& wlist(int t) const { return t == X ? wxl : wyl; }  // women in c- or d-order
  const vector<int>& mlist(int t) const { return t == X ? mxl : myl; }  // men in a- or b-order
  // rank of woman w in the list used by men of type t; rank of man m in list used by women of type t
  int wr(int t, int w) const { return t == X ? c[w] : d[w]; }
  int mr(int t, int m) const { return t == X ? a[m] : b[m]; }

  static vector<int> inv(const vector<int>& p) {
    vector<int> q(p.size());
    for (size_t i = 0; i < p.size(); i++) q[p[i]] = (int)i;
    return q;
  }
  void lists_from_ranks() { mxl = inv(a); myl = inv(b); wxl = inv(c); wyl = inv(d); }
  void ranks_from_lists() { a = inv(mxl); b = inv(myl); c = inv(wxl); d = inv(wyl); }
  void validate() const {
    auto isperm = [&](const vector<int>& p) {
      if ((int)p.size() != n) return false;
      vector<char> s(n, 0);
      for (int x : p) { if (x < 0 || x >= n || s[x]) return false; s[x] = 1; }
      return true;
    };
    if (!isperm(a) || !isperm(b) || !isperm(c) || !isperm(d)) throw std::runtime_error("bad rank permutation");
    if ((int)mt.size() != n || (int)wt.size() != n) throw std::runtime_error("bad type vector");
    for (int i = 0; i < n; i++)
      if ((mt[i] != X && mt[i] != Y) || (wt[i] != X && wt[i] != Y)) throw std::runtime_error("bad type");
  }
};

// Text format (matches the blog input). '#' starts a comment. Characters = [ ] , ' " are ignored,
// so a paste of the blog's Python lists also parses. Keys:
//   n N   (optional)
//   men_x_ranking  i0 i1 ...   (men ordered by X, best first)
//   men_y_ranking ... / women_x_ranking ... / women_y_ranking ...
//   men_preferences XYX...     (or X Y X ..., one char per person)
//   women_preferences ...
inline Instance parse_instance(const string& text) {
  string s;
  bool comment = false;
  for (char ch : text) {
    if (ch == '#') comment = true;
    if (ch == '\n') comment = false;
    if (comment) continue;
    if (ch == '=' || ch == '[' || ch == ']' || ch == ',' || ch == '\'' || ch == '"') ch = ' ';
    s += ch;
  }
  std::istringstream in(s);
  std::map<string, vector<string>> kv;
  string tok, key;
  const vector<string> keys = {"n", "men_x_ranking", "men_y_ranking", "women_x_ranking",
                               "women_y_ranking", "men_preferences", "women_preferences"};
  while (in >> tok) {
    if (std::find(keys.begin(), keys.end(), tok) != keys.end()) { key = tok; kv[key]; continue; }
    if (key.empty()) throw std::runtime_error("value before key: " + tok);
    kv[key].push_back(tok);
  }
  Instance I;
  auto ints = [&](const string& k) {
    vector<int> v;
    for (auto& t : kv[k]) v.push_back(std::stoi(t));
    return v;
  };
  auto types = [&](const string& k) {
    vector<char> v;
    for (auto& t : kv[k])
      for (char ch : t) {
        if (ch == 'X' || ch == 'x') v.push_back(X);
        else if (ch == 'Y' || ch == 'y') v.push_back(Y);
        else throw std::runtime_error("bad type char in " + k);
      }
    return v;
  };
  I.mxl = ints("men_x_ranking"); I.myl = ints("men_y_ranking");
  I.wxl = ints("women_x_ranking"); I.wyl = ints("women_y_ranking");
  I.mt = types("men_preferences"); I.wt = types("women_preferences");
  I.n = (int)I.mxl.size();
  if (kv.count("n") && !kv["n"].empty() && std::stoi(kv["n"][0]) != I.n) throw std::runtime_error("n mismatch");
  I.ranks_from_lists();
  I.validate();
  return I;
}
inline Instance read_instance(const string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open " + path);
  std::stringstream ss; ss << f.rdbuf();
  return parse_instance(ss.str());
}
inline string format_instance(const Instance& I, const string& comment = "") {
  std::ostringstream o;
  if (!comment.empty()) o << "# " << comment << "\n";
  o << "n " << I.n << "\n";
  auto pl = [&](const char* k, const vector<int>& v) {
    o << k;
    for (int x : v) o << ' ' << x;
    o << "\n";
  };
  auto pt = [&](const char* k, const vector<char>& v) {
    o << k << ' ';
    for (char t : v) o << (t == X ? 'X' : 'Y');
    o << "\n";
  };
  pl("men_x_ranking", I.mxl); pl("men_y_ranking", I.myl);
  pl("women_x_ranking", I.wxl); pl("women_y_ranking", I.wyl);
  pt("men_preferences", I.mt); pt("women_preferences", I.wt);
  return o.str();
}
inline void write_instance(const Instance& I, const string& path, const string& comment = "") {
  std::ofstream f(path);
  f << format_instance(I, comment);
}

// ---------------------------------------------------------------- generators
inline vector<int> rand_perm(int n, Rng& r) {
  vector<int> p(n);
  std::iota(p.begin(), p.end(), 0);
  std::shuffle(p.begin(), p.end(), r);
  return p;
}
// relation of the second list (b or d) to the first list (a or c):
// "indep" = independent random, "same" = equal, "rev" = reversed, "noisyK" = first list + K random swaps,
// "revnoisyK" = reversed + K random swaps.
inline vector<int> second_list(const vector<int>& first, const string& rel, Rng& r) {
  int n = (int)first.size();
  vector<int> s;
  int k = 0;
  if (rel == "indep") return rand_perm(n, r);
  if (rel == "same") return first;
  if (rel == "rev") { s = first; std::reverse(s.begin(), s.end()); return s; }
  if (rel.rfind("noisy", 0) == 0) { s = first; k = std::stoi(rel.substr(5)); }
  else if (rel.rfind("revnoisy", 0) == 0) { s = first; std::reverse(s.begin(), s.end()); k = std::stoi(rel.substr(8)); }
  else throw std::runtime_error("unknown list relation " + rel);
  std::uniform_int_distribution<int> U(0, n - 1);
  for (int i = 0; i < k && n > 1; i++) std::swap(s[U(r)], s[U(r)]);
  return s;
}
// types: if k >= 0, exactly k people of type X at random positions; else each X with prob p.
inline vector<char> rand_types(int n, double p, int k, Rng& r) {
  vector<char> t(n, Y);
  if (k >= 0) {
    auto q = rand_perm(n, r);
    for (int i = 0; i < k && i < n; i++) t[q[i]] = X;
  } else {
    std::bernoulli_distribution B(p);
    for (int i = 0; i < n; i++) t[i] = B(r) ? X : Y;
  }
  return t;
}
struct GenParams {
  string kind = "uniform";        // uniform | blocks | cycles
  string men_rel = "indep";       // relation of men's Y list to men's X list
  string women_rel = "indep";
  double px = 0.5, pw = 0.5;      // prob a man / woman is type X
  int kx = -1, kw = -1;           // exact counts of X men / X women (override px/pw if >= 0)
  int blocks = 4;                 // number of blocks for kind=blocks
};
inline Instance generate(int n, const GenParams& P, Rng& r) {
  Instance I;
  I.n = n;
  if (P.kind == "uniform") {
    I.mxl = rand_perm(n, r); I.myl = second_list(I.mxl, P.men_rel, r);
    I.wxl = rand_perm(n, r); I.wyl = second_list(I.wxl, P.women_rel, r);
    I.mt = rand_types(n, P.px, P.kx, r); I.wt = rand_types(n, P.pw, P.kw, r);
  } else if (P.kind == "blocks") {
    // ids are split into K contiguous blocks; every list ranks block 0 first, then block 1, ...;
    // inside a block the order is random (independent per list). Types random.
    int K = std::max(1, std::min(P.blocks, n));
    auto blk = [&]() {
      vector<int> L;
      for (int k = 0; k < K; k++) {
        int lo = (int)((long long)n * k / K), hi = (int)((long long)n * (k + 1) / K);
        vector<int> part(hi - lo);
        std::iota(part.begin(), part.end(), lo);
        std::shuffle(part.begin(), part.end(), r);
        L.insert(L.end(), part.begin(), part.end());
      }
      return L;
    };
    I.mxl = blk(); I.myl = blk(); I.wxl = blk(); I.wyl = blk();
    I.mt = rand_types(n, P.px, P.kx, r); I.wt = rand_types(n, P.pw, P.kw, r);
  } else if (P.kind == "cycles") {
    // F8 family: n/2 blocks of 2 men + 2 women, each block a 2x2 cycle (2 stable matchings per block).
    // Block i: men 2i (X), 2i+1 (Y); women 2i (X), 2i+1 (Y). Lists order the blocks 0,1,2,...
    // c: w2i < w2i+1, d: w2i+1 < w2i, a: m2i+1 < m2i, b: m2i < m2i+1. If n is odd, the last person is alone.
    I.mt.assign(n, X); I.wt.assign(n, X);
    for (int i = 0; 2 * i + 1 < n; i++) {
      I.mt[2 * i] = X; I.mt[2 * i + 1] = Y; I.wt[2 * i] = X; I.wt[2 * i + 1] = Y;
      I.wxl.push_back(2 * i); I.wxl.push_back(2 * i + 1);
      I.wyl.push_back(2 * i + 1); I.wyl.push_back(2 * i);
      I.mxl.push_back(2 * i + 1); I.mxl.push_back(2 * i);
      I.myl.push_back(2 * i); I.myl.push_back(2 * i + 1);
    }
    if (n % 2) { I.wxl.push_back(n - 1); I.wyl.push_back(n - 1); I.mxl.push_back(n - 1); I.myl.push_back(n - 1); }
  } else if (P.kind == "trap" || P.kind == "trapI" || P.kind == "trapII") {
    // Found by adversarial search on min A over all orders (n=6), then generalized. n = 2k (+1 dummy if odd).
    // X-men x_1..x_k = ids 0..k-1; Y-men y_1..y_k = ids k..2k-1.
    // Y-women q_1..q_{k-1} = ids 0..k-2; X-women p_1..p_k = ids k-1..2k-2; X-woman z = id 2k-1.
    // c (X-men's list): q.., p.., z.   d (Y-men's list): p.., q.., z.
    // a (X-women's list): x.., y..     b (Y-women's list): y.., x..
    int k = n / 2;
    I.mt.assign(n, X); I.wt.assign(n, X);
    for (int i = 0; i < k; i++) I.mt[k + i] = Y;
    for (int i = 0; i + 1 < k; i++) I.wt[i] = Y;
    for (int i = 0; i < 2 * k; i++) { I.wxl.push_back(i); I.mxl.push_back(i); }
    for (int i = k - 1; i <= 2 * k - 2; i++) I.wyl.push_back(i);
    for (int i = 0; i <= k - 2; i++) I.wyl.push_back(i);
    if (k >= 1) I.wyl.push_back(2 * k - 1);
    for (int i = k; i < 2 * k; i++) I.myl.push_back(i);
    for (int i = 0; i < k; i++) I.myl.push_back(i);
    // trapI: Y-women's list (b) interleaves types: y_1 x_1 y_2 x_2 ...; trapII: also a = x_1 y_1 x_2 y_2 ...
    if (P.kind != "trap") {
      I.myl.clear();
      for (int i = 0; i < k; i++) { I.myl.push_back(k + i); I.myl.push_back(i); }
    }
    if (P.kind == "trapII") {
      I.mxl.clear();
      for (int i = 0; i < k; i++) { I.mxl.push_back(i); I.mxl.push_back(k + i); }
    }
    if (n % 2) { I.wxl.push_back(n - 1); I.wyl.push_back(n - 1); I.mxl.push_back(n - 1); I.myl.push_back(n - 1); }
  } else {
    throw std::runtime_error("unknown kind " + P.kind);
  }
  I.ranks_from_lists();
  I.validate();
  return I;
}

// Swap the roles of men and women (women-proposing GS on I = men-proposing GS on transpose(I)).
inline Instance transpose(const Instance& I) {
  Instance J;
  J.n = I.n;
  J.mxl = I.wxl; J.myl = I.wyl; J.wxl = I.mxl; J.wyl = I.myl;
  J.mt = I.wt; J.wt = I.mt;
  J.ranks_from_lists();
  return J;
}
// Block union: people of I1 keep ids 0..n1-1, people of I2 get ids n1..; every list ranks all I1
// people first (in I1's order), then all I2 people (in I2's order).
inline Instance block_union(const Instance& I1, const Instance& I2) {
  Instance J;
  J.n = I1.n + I2.n;
  auto cat = [&](const vector<int>& u, const vector<int>& v) {
    vector<int> r = u;
    for (int x : v) r.push_back(x + I1.n);
    return r;
  };
  J.mxl = cat(I1.mxl, I2.mxl); J.myl = cat(I1.myl, I2.myl);
  J.wxl = cat(I1.wxl, I2.wxl); J.wyl = cat(I1.wyl, I2.wyl);
  J.mt = I1.mt; J.mt.insert(J.mt.end(), I2.mt.begin(), I2.mt.end());
  J.wt = I1.wt; J.wt.insert(J.wt.end(), I2.wt.begin(), I2.wt.end());
  J.ranks_from_lists();
  J.validate();
  return J;
}

// ---------------------------------------------------------------- matchings
// A matching is a vector wife (man -> woman, -1 = free). husband is the inverse.
inline vector<int> inverse_matching(const vector<int>& wife, int n) {
  vector<int> h(n, -1);
  for (int m = 0; m < n; m++) if (wife[m] >= 0) h[wife[m]] = m;
  return h;
}
inline bool is_perfect(const vector<int>& wife, int n) {
  if ((int)wife.size() != n) return false;
  vector<char> s(n, 0);
  for (int w : wife) { if (w < 0 || w >= n || s[w]) return false; s[w] = 1; }
  return true;
}

// Reference men-proposing GS, O(n^2), explicit ranks, LIFO free stack.
inline vector<int> gs_reference(const Instance& I, long long* proposals = nullptr) {
  int n = I.n;
  vector<int> next(n, 0), husband(n, -1), wife(n, -1), st;
  for (int m = n - 1; m >= 0; m--) st.push_back(m);
  long long P = 0;
  while (!st.empty()) {
    int m = st.back(); st.pop_back();
    int w = I.man_list(m)[next[m]++];
    P++;
    int h = husband[w];
    if (h < 0) { husband[w] = m; wife[m] = w; }
    else if (I.woman_rank(w, m) < I.woman_rank(w, h)) {
      husband[w] = m; wife[m] = w; wife[h] = -1; st.push_back(h);
    } else st.push_back(m);
  }
  if (proposals) *proposals = P;
  return wife;
}

// Brute stability check, O(n^2). Returns true iff perfect and no blocking pair.
inline bool stable_brute(const Instance& I, const vector<int>& wife) {
  int n = I.n;
  if (!is_perfect(wife, n)) return false;
  auto h = inverse_matching(wife, n);
  for (int m = 0; m < n; m++)
    for (int w = 0; w < n; w++)
      if (I.man_rank(m, w) < I.man_rank(m, wife[m]) && I.woman_rank(w, m) < I.woman_rank(w, h[w])) return false;
  return true;
}

// Fast stability check, O(n) (F1). For each block (tm, tw), sweep the tm-list of women from worst to best.
// Blocking pair (m in M_tm, w in W_tw): wr(tm,w) < wr(tm,wife[m]) and mr(tw,m) < mr(tw,husband[w]).
// If bp != nullptr and unstable, *bp = a blocking pair (m, w).
inline bool stable_fast(const Instance& I, const vector<int>& wife, std::pair<int, int>* bp = nullptr) {
  int n = I.n;
  if (!is_perfect(wife, n)) return false;
  auto h = inverse_matching(wife, n);
  for (int tm = 0; tm < 2; tm++)
    for (int tw = 0; tw < 2; tw++) {
      const vector<int>& L = I.wlist(tm);
      int best = n, bestm = -1;  // min mr(tw, m) over tm-men whose wife is worse (in tm-list) than position p
      for (int p = n - 1; p >= 0; p--) {
        int w = L[p];
        if (I.wt[w] == tw && best < I.mr(tw, h[w])) { if (bp) *bp = {bestm, w}; return false; }
        int m = h[w];
        if (I.mt[m] == tm && I.mr(tw, m) < best) { best = I.mr(tw, m); bestm = m; }
      }
    }
  return true;
}

// All stable matchings by backtracking (men in id order). Pairwise check of assigned pairs is exact,
// because a blocking pair (m, w') involves exactly the two pairs (m, w) and (m', w').
inline vector<vector<int>> all_stable(const Instance& I, size_t limit = (size_t)-1) {
  int n = I.n;
  vector<vector<int>> out;
  vector<int> wife(n, -1);
  vector<char> used(n, 0);
  std::function<void(int)> rec = [&](int m) {
    if (out.size() >= limit) return;
    if (m == n) { out.push_back(wife); return; }
    for (int w = 0; w < n; w++) {
      if (used[w]) continue;
      bool ok = true;
      for (int m2 = 0; m2 < m && ok; m2++) {
        int w2 = wife[m2];
        if (I.man_rank(m, w2) < I.man_rank(m, w) && I.woman_rank(w2, m) < I.woman_rank(w2, m2)) ok = false;
        else if (I.man_rank(m2, w) < I.man_rank(m2, w2) && I.woman_rank(w, m2) < I.woman_rank(w, m)) ok = false;
      }
      if (!ok) continue;
      used[w] = 1; wife[m] = w;
      rec(m + 1);
      used[w] = 0; wife[m] = -1;
    }
  };
  rec(0);
  return out;
}

// ---------------------------------------------------------------- fast GS (F2)
// Segment tree over the positions of one women-list (c-order or d-order). Leaf value: threshold
// S(w) = woman_rank(w, current husband), or n if w is free. Stores max S over X-women and over Y-women.
// first(p, ra, rb): first position q >= p whose woman accepts a man with ranks (a, b) = (ra, rb),
// i.e. (w is X and S(w) > ra) or (w is Y and S(w) > rb). Returns n if none. O(log n).
struct ThreshTree {
  int n = 0, sz = 1;
  vector<int> mx, my;
  void init(int n_, const vector<int>& order, const vector<char>& wt, const vector<int>& S) {
    n = n_; sz = 1;
    while (sz < n) sz <<= 1;
    mx.assign(2 * sz, -1); my.assign(2 * sz, -1);
    for (int p = 0; p < n; p++) {
      int w = order[p];
      (wt[w] == X ? mx : my)[sz + p] = S[w];
    }
    for (int i = sz - 1; i >= 1; i--) { mx[i] = std::max(mx[2 * i], mx[2 * i + 1]); my[i] = std::max(my[2 * i], my[2 * i + 1]); }
  }
  void set(int p, char type, int v) {
    int i = sz + p;
    (type == X ? mx : my)[i] = v;
    for (i >>= 1; i >= 1; i >>= 1) { mx[i] = std::max(mx[2 * i], mx[2 * i + 1]); my[i] = std::max(my[2 * i], my[2 * i + 1]); }
  }
  int first(int p, int ra, int rb) const {
    if (p >= n) return n;
    int r = rec(1, 0, sz, p, ra, rb);
    return r < 0 ? n : r;
  }
  int rec(int node, int lo, int hi, int p, int ra, int rb) const {
    if (hi <= p || (mx[node] <= ra && my[node] <= rb)) return -1;
    if (hi - lo == 1) return lo;
    int mid = (lo + hi) / 2;
    int r = rec(2 * node, lo, mid, p, ra, rb);
    if (r >= 0) return r;
    return rec(2 * node + 1, mid, hi, p, ra, rb);
  }
};

struct GSStats {
  long long accepted = 0;       // A: accepted proposals
  long long rejected = 0;       // rejections that plain GS would do (skipped in bulk here)
  long long displacements = 0;  // accepted proposals to a woman who held a man (= A - n at the end)
  long long switches = 0;       // displacements where the held man changes type X<->Y
  long long max_chain = 0;      // longest displacement chain (causal depth, see README)
  long long rounds = 0;         // bulk parallel rounds (only for gs_rounds)
  long long steps = 0;          // free-man selections
};

// Rules for choosing the next free man.
//  fifo, lifo, random, pa (best a first), pa_rev (worst a first), pb, pb_rev, pmin (min(a,b) first),
//  pmax (max(a,b) first), xfirst (free X-men before Y-men, FIFO inside), yfirst,
//  mw (McVitie-Wilson: introduce men in id order, a displaced man proposes at once = LIFO stack
//  with man 0 on top), mw_a (introduce in a-order), mw_b (in b-order),
//  nodisp (greedy: a free man whose next acceptance is at a free woman, if any; else FIFO. Costly.)
inline const vector<string>& all_rules() {
  static const vector<string> R = {"fifo", "lifo", "random", "pa", "pa_rev", "pb", "pb_rev", "pmin",
                                   "pmax", "xfirst", "yfirst", "mw", "mw_a", "mw_b", "nodisp"};
  return R;
}

class FastGS {
 public:
  const Instance& I;
  int n;
  vector<int> wife, husband, ptr, S, depth;
  ThreshTree T[2];
  GSStats st;
  // Optional callback after every acceptance: (man, woman, displaced man or -1).
  std::function<void(int, int, int)> on_accept;

  explicit FastGS(const Instance& inst) : I(inst), n(inst.n) {
    wife.assign(n, -1); husband.assign(n, -1); ptr.assign(n, 0); S.assign(n, n); depth.assign(n, 0);
    T[X].init(n, I.wxl, I.wt, S); T[Y].init(n, I.wyl, I.wt, S);
  }
  // Next woman (position in his list) who accepts free man m now; skips rejections.
  int target_pos(int m) const { return T[(int)I.mt[m]].first(ptr[m], I.a[m], I.b[m]); }
  int target(int m) const {
    int q = target_pos(m);
    return q < n ? I.man_list(m)[q] : -1;
  }
  void set_thresh(int w, int v) {
    S[w] = v;
    T[X].set(I.c[w], I.wt[w], v);
    T[Y].set(I.d[w], I.wt[w], v);
  }
  // Free man m makes his next accepted proposal. Returns the displaced man or -1.
  int step(int m) {
    int q = target_pos(m);
    if (q >= n) throw std::runtime_error("free man without target (bug)");
    st.rejected += q - ptr[m];
    return accept(m, q);
  }
  int accept(int m, int q) {
    int w = I.man_list(m)[q];
    int old = husband[w];
    st.accepted++;
    husband[w] = m; wife[m] = w; ptr[m] = q;
    set_thresh(w, I.woman_rank(w, m));
    if (old >= 0) {
      st.displacements++;
      if (I.mt[old] != I.mt[m]) st.switches++;
      wife[old] = -1; ptr[old]++;
      depth[old] = depth[m] + 1;
      st.max_chain = std::max<long long>(st.max_chain, depth[old]);
    }
    if (on_accept) on_accept(m, w, old);
    return old;
  }
  // Run with a rule; returns wife.
  vector<int> run(const string& rule, uint64_t seed = 1);
  // Bulk parallel rounds (item 4).
  vector<int> run_rounds();
};

inline vector<int> FastGS::run(const string& rule, uint64_t seed) {
  Rng rng(seed);
  if (rule == "fifo" || rule == "xfirst" || rule == "yfirst") {
    std::deque<int> q[2];
    for (int m = 0; m < n; m++) q[rule == "fifo" ? 0 : (int)I.mt[m]].push_back(m);
    int first = rule == "yfirst" ? 1 : 0;
    while (!q[0].empty() || !q[1].empty()) {
      int k = !q[first].empty() ? first : 1 - first;
      int m = q[k].front(); q[k].pop_front();
      st.steps++;
      int old = step(m);
      if (old >= 0) q[rule == "fifo" ? 0 : (int)I.mt[old]].push_back(old);
    }
  } else if (rule == "lifo" || rule == "mw" || rule == "mw_a" || rule == "mw_b") {
    vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    if (rule == "mw_a") order = I.mxl;
    if (rule == "mw_b") order = I.myl;
    vector<int> stk;
    if (rule == "lifo") stk = order;  // man n-1 on top
    else stk.assign(order.rbegin(), order.rend());  // first introduced man on top
    while (!stk.empty()) {
      int m = stk.back(); stk.pop_back();
      st.steps++;
      int old = step(m);
      if (old >= 0) stk.push_back(old);
    }
  } else if (rule == "random") {
    vector<int> fr(n);
    std::iota(fr.begin(), fr.end(), 0);
    while (!fr.empty()) {
      size_t i = std::uniform_int_distribution<size_t>(0, fr.size() - 1)(rng);
      int m = fr[i]; fr[i] = fr.back(); fr.pop_back();
      st.steps++;
      int old = step(m);
      if (old >= 0) fr.push_back(old);
    }
  } else if (rule == "nodisp") {
    std::deque<int> q;
    for (int m = 0; m < n; m++) q.push_back(m);
    while (!q.empty()) {
      size_t pick = 0;
      for (size_t i = 0; i < q.size(); i++) {
        int w = target(q[i]);
        if (husband[w] < 0) { pick = i; break; }
      }
      int m = q[pick]; q.erase(q.begin() + pick);
      st.steps++;
      int old = step(m);
      if (old >= 0) q.push_back(old);
    }
  } else {
    std::function<long long(int)> key;
    if (rule == "pa") key = [&](int m) { return (long long)I.a[m]; };
    else if (rule == "pa_rev") key = [&](int m) { return -(long long)I.a[m]; };
    else if (rule == "pb") key = [&](int m) { return (long long)I.b[m]; };
    else if (rule == "pb_rev") key = [&](int m) { return -(long long)I.b[m]; };
    else if (rule == "pmin") key = [&](int m) { return (long long)std::min(I.a[m], I.b[m]) * n + m; };
    else if (rule == "pmax") key = [&](int m) { return (long long)std::max(I.a[m], I.b[m]) * n + m; };
    else throw std::runtime_error("unknown rule " + rule);
    typedef std::pair<long long, int> P;
    std::priority_queue<P, vector<P>, std::greater<P>> pq;
    for (int m = 0; m < n; m++) pq.push({key(m), m});
    while (!pq.empty()) {
      int m = pq.top().second; pq.pop();
      st.steps++;
      int old = step(m);
      if (old >= 0) pq.push({key(old), old});
    }
  }
  return wife;
}

inline vector<int> FastGS::run_rounds() {
  vector<int> fr(n);
  std::iota(fr.begin(), fr.end(), 0);
  vector<int> best(n, -1), bestq(n, -1), tq(n, -1);
  while (!fr.empty()) {
    st.rounds++;
    vector<int> touched;
    for (int m : fr) {  // all queries use the thresholds of the round start
      int q = target_pos(m);
      if (q >= n) throw std::runtime_error("free man without target (bug)");
      st.rejected += q - ptr[m];
      tq[m] = q;
      int w = I.man_list(m)[q];
      if (best[w] < 0) { touched.push_back(w); best[w] = m; }
      else if (I.woman_rank(w, m) < I.woman_rank(w, best[w])) best[w] = m;
    }
    vector<int> nf;
    for (int m : fr) {
      int w = I.man_list(m)[tq[m]];
      if (best[w] != m) { st.rejected++; ptr[m] = tq[m] + 1; nf.push_back(m); }
    }
    for (int w : touched) {
      int m = best[w];
      best[w] = -1;
      int old = accept(m, tq[m]);
      if (old >= 0) nf.push_back(old);
    }
    fr.swap(nf);
  }
  return wife;
}

// MOSM by fast GS with the given rule.
inline vector<int> mosm_fast(const Instance& I, const string& rule = "fifo", GSStats* s = nullptr, uint64_t seed = 1) {
  FastGS g(I);
  auto r = g.run(rule, seed);
  if (s) *s = g.st;
  return r;
}

// ---------------------------------------------------------------- structural helpers (F4, F5)
// F4 greedy for men of type t on the women set Wset (flag per woman): women of Wset in the t-list
// order (c for X, d for Y); each takes her favorite still-free man among `men` (flag per man).
// Returns wife restricted to these men (-1 elsewhere). O(n^2).
inline vector<int> greedy_side(const Instance& I, int t, const vector<char>& Wset, const vector<char>& men) {
  int n = I.n;
  vector<int> wife(n, -1);
  vector<char> taken(n, 0);
  for (int p = 0; p < n; p++) {
    int w = I.wlist(t)[p];
    if (!Wset[w]) continue;
    int bm = -1;
    for (int m = 0; m < n; m++)
      if (men[m] && !taken[m] && (bm < 0 || I.woman_rank(w, m) < I.woman_rank(w, bm))) bm = m;
    if (bm < 0) return vector<int>();  // not enough men
    taken[bm] = 1; wife[bm] = w;
  }
  return wife;
}
// Women's-side version: men of Mset in the t-list of men (a for X, b for Y); each takes his favorite
// still-free woman among `women`. Returns husband restricted (-1 elsewhere).
inline vector<int> greedy_side_w(const Instance& I, int t, const vector<char>& Mset, const vector<char>& women) {
  int n = I.n;
  vector<int> husband(n, -1);
  vector<char> taken(n, 0);
  for (int p = 0; p < n; p++) {
    int m = I.mlist(t)[p];
    if (!Mset[m]) continue;
    int bw = -1;
    for (int w = 0; w < n; w++)
      if (women[w] && !taken[w] && (bw < 0 || I.man_rank(m, w) < I.man_rank(m, bw))) bw = w;
    if (bw < 0) return vector<int>();
    taken[bw] = 1; husband[bw] = m;
  }
  return husband;
}

}  // namespace tlsm
