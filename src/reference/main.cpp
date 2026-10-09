#include "tlsm.hpp"
#include "checks.hpp"
using namespace tlsm;
int main(int argc, char** argv) {
  try {
    if (argc < 2) throw std::runtime_error("usage: reference solve INPUT OUTPUT | check INPUT MATCHING | enum INPUT | gen N SEED OUTPUT | selftest");
    string cmd = argv[1];
    if (cmd == "selftest") return selftest(40);
    if (cmd == "gen" && argc == 5) {
      int n = std::stoi(argv[2]);
      if (n < 1) throw std::runtime_error("n must be positive");
      Rng rng(std::stoull(argv[3]));
      write_instance(generate(n, GenParams{}, rng), argv[4], "random instance");
      return 0;
    }
    if (argc < 3) throw std::runtime_error("missing input");
    Instance I = read_instance(argv[2]);
    if (cmd == "solve" && argc == 4) {
      auto p = gs_reference(I);
      std::ofstream out(argv[3]);
      if (!out) throw std::runtime_error("cannot open output");
      for (int w : p) out << w << ' ';
      out << '\n';
      return out ? 0 : 2;
    }
    if (cmd == "check" && argc == 4) {
      std::ifstream in(argv[3]); vector<int> p; int w;
      if (!in) throw std::runtime_error("cannot open matching");
      while (in >> w) p.push_back(w);
      bool ok = in.eof() && stable_fast(I,p);
      std::cout << (ok ? "stable\n" : "invalid or unstable\n");
      return ok ? 0 : 2;
    }
    if (cmd == "enum" && argc == 3) {
      if (I.n > 9) throw std::runtime_error("brute force limited to n <= 9");
      for (auto p : all_stable(I)) {
        for (int w : p) std::cout << w << ' ';
        std::cout << '\n';
      }
      return 0;
    }
    throw std::runtime_error("invalid command or arguments");
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n'; return 2;
  }
}
