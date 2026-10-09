// Keyword/order-format instance loader and symmetric transforms (swap X/Y, swap men/women).
#pragma once
Inst loadLab(const char* fn) {
  ifstream in(fn); string line; Inst I; map<string, vector<int>> L; string mp, wp;
  while (getline(in, line)) {
    for (char& ch : line) if (ch == '=' || ch == '[' || ch == ']' || ch == ',' || ch == '\'' || ch == '"') ch = ' ';
    size_t h = line.find('#'); if (h != string::npos) line = line.substr(0, h);
    istringstream ss(line); string key; if (!(ss >> key)) continue;
    if (key == "men_preferences" || key == "women_preferences") { string t, all; while (ss >> t) all += t; (key[0] == 'm' ? mp : wp) = all; continue; }
    int v; vector<int> vals; while (ss >> v) vals.push_back(v); L[key] = vals;
  }
  int n = mp.size(); I.n = n;
  auto rk = [&](const vector<int>& ord) { vector<int> r(n); for (int i = 0; i < n; i++) r[ord[i]] = i; return r; };
  I.a = rk(L["men_x_ranking"]); I.b = rk(L["men_y_ranking"]); I.c = rk(L["women_x_ranking"]); I.d = rk(L["women_y_ranking"]);
  I.tm.resize(n); I.tw.resize(n);
  for (int i = 0; i < n; i++) { I.tm[i] = mp[i] == 'Y'; I.tw[i] = wp[i] == 'Y'; }
  return I;
}
Inst swapXY(const Inst& I) { Inst J = I; swap(J.a, J.b); swap(J.c, J.d); for (auto& t : J.tm) t ^= 1; for (auto& t : J.tw) t ^= 1; return J; }
Inst dualInst(const Inst& I) { Inst J = I; J.a = I.c; J.b = I.d; J.c = I.a; J.d = I.b; J.tm = I.tw; J.tw = I.tm; return J; }
