# Rotations in the 2-list model

D1 and D2: **P, A+** (proved and independently audited, 2026-10-05).
Full SEP in Section 6 remains partly open.

## 1. Model and results

There are n men and n women. Every man m has ranks a(m), b(m) and every woman w has ranks c(w), d(w)
(0 = best; each of a, b, c, d is a permutation). Every person has a type. We write X, Y for men and
x, y for women. An X-man ranks the women by c, a Y-man by d. An x-woman ranks the men by a, a y-woman
by b. This is the strict, complete (2,2)-list model.

For a stable matching nu, the *label* of a woman is the type of her partner. N_XX(nu) is the number
of pairs (X-man, x-woman) in nu.

*Rotations* . For a stable matching mu and a man m, s(m) is the first woman after
mu(m) in m's list who prefers m to her partner. A rotation exposed in mu is a cycle
rho = (m_1,w_1), ..., (m_r,w_r) with w_i = mu(m_i) and w_{i+1} = s(m_i) (indices mod r). Eliminating
it gives the stable matching mu/rho in which m_i gets w_{i+1}. Every stable matching is reached
from the man-optimal one (MOSM) by eliminating rotations, each rotation is eliminated exactly once on
every path from MOSM to the woman-optimal one (WOSM), and the stable matchings are the downsets of the
rotation poset. dXX(rho) := N_XX(mu/rho) - N_XX(mu); it depends only on rho.

**Theorem 1 (D1).** Every rotation rho has dXX(rho) = +1 or -1. We call it the *sign* of rho.

**Theorem 2 (D2).** The number R of rotations satisfies
  R <= #{x-women whose label in MOSM differs from their label in WOSM},
and the same bound holds with y-women, X-men (label = type of partner) or Y-men. In particular
  R <= min(#X-men, #Y-men, #x-women, #y-women) <= n/2.

**Corollary 3.** A 2-list instance has at most 2^R <= 2^min(#X-men,#Y-men,#x-women,#y-women) <=
2^(n/2) stable matchings (they are downsets of a poset with R elements).

**Tightness.** Take n/2 disjoint blocks of two men (one X, one Y) and two women (one x, one y), all
lists ranking block 0 first, then block 1, and so on. Inside a block: the X-man prefers the x-woman,
the x-woman the Y-man, the Y-man the y-woman, the y-woman the X-man. Each block is one rotation, so
R = n/2 and there are 2^(n/2) stable matchings. 

## 2. Basic facts

- (F1) Stability: in a stable nu, a woman w prefers nu(w) to every man who prefers w to his partner.
- (F2) Rotation: if m_k is moved by rho, every woman strictly between w_k and w_{k+1} in m_k's list
  prefers her mu-partner to m_k. Men strictly worsen and women strictly improve. So consecutive
  X-men in rho move to women with larger c, consecutive Y-men to women with larger d.
- (D) Two women who rank two men in opposite orders have different types.
- (D0) Every rotation contains X-men and Y-men, and x-women and y-women (an all-X cycle would need
  c to increase around a cycle; an all-x cycle would need a to decrease around a cycle).
- (C4) In a stable nu, if v, w have the same type, v is X-labeled, c(w) < c(v) and d(w) > d(v), then
  w is X-labeled. Proof: else w's partner l is a Y-man. v's partner p (X-man) has c(nu(p)) = c(v) >
  c(w), so w prefers l to p (F1). l has d(nu(l)) = d(w) > d(v), so v prefers p to l (F1).
  Contradiction with (D).
- Symmetries (all keep the set of stable matchings and the rotations): sigma swaps x <-> y and a <-> b
  (flips every sign); mu swaps X <-> Y, x <-> y, a <-> b, c <-> d (keeps every sign, since
  dN_YY = dN_XX); T swaps men and women (reverses the lattice, flips every sign).

## 3. Proof of Theorem 1, and the shape of a rotation

**Runs and boundaries.** Fix rho exposed in mu, mu' = mu/rho. An X-run is a maximal cyclic block of
consecutive X-men m_i, ..., m_j of rho. Its women w_i, ..., w_{j+1} have increasing c. Its first
woman A := w_i is X-labeled in mu and Y-labeled in mu' (an *A-boundary*); its last woman B := w_{j+1}
is Y-labeled in mu and X-labeled in mu' (a *B-boundary*). Y-runs are the same with Y-men; a Y-run
goes from a B to an A with d increasing. Women inside a run keep their label. With k X-runs, the
cycle reads A_0 B_0 A_1 B_1 ... A_{k-1} B_{k-1} with X-runs R_i = (A_i, B_i) and Y-runs
S_i = (B_i, A_{i+1}). Since only boundary women change label,
  dXX(rho) = #(x-type B's) - #(x-type the run-buffer implementation).                                      (1)

**Flight lemma.** The flight of an X-man f of rho is (c(mu(f)), c(mu'(f))]; its starter is mu(f), its
lander mu'(f). Let f != g have intersecting flights, c(mu(f)) < c(mu(g)).
- (T1) mu(f), mu(g) have different types. mu(g) lies strictly inside f's flight, so it prefers g to f
  (F2); mu(f) sees g (c(mu(g)) > c(mu(f))), so it prefers f to g (F1). Use (D).
- (T2) If f lands first, then mu'(f) has the type of mu(f). mu'(f) prefers f to g in mu' (F1, g lands
  later), while mu(g) prefers g to f; so type(mu'(f)) != type(mu(g)) != type(mu(f)). If g starts first
  and f lands first, the same works with T1 for the swapped pair. The same holds for Y-men and d.

**Run lemma (K2, K3).** Let R, R' be X-runs with c(A_R) < c(A_R') < c(B_R). Then:
nested (c(B_R') < c(B_R)) implies type(A_R') = type(B_R'); crossing (c(B_R) < c(B_R')) implies
type(B_R) != type(A_R'). Proof: a woman v != A_R' of R' with c(v) <= min(c(B_R), c(B_R')) lies
strictly inside a flight of R and her own flight lands first, so by T2 she has the type of the
previous woman of R'; induct from A_R'. The flight of R containing c(A_R') meets the first flight of
R', so by T1 its starter has the other type; the R-women after it up to the same bound keep that type
by T2. Take v = B_R' (nested) or the R-woman B_R (crossing). K3 is the mirror for Y-runs in d.

**Chain lemma (K1).** Boundary women of the same type are comparable in the product order on (c, d).
- (P1) A and B: if c(A) < c(B) and d(A) > d(B), C4 in mu' (B X-labeled) makes A X-labeled; if c(B) <
  c(A) and d(B) > d(A), C4 in mu (A X-labeled) makes B X-labeled. Both false.
- (P2) Two the run-buffer implementation with c(A1) < c(A2), d(A1) > d(A2): let p_k = mu(A_k) (X-men), l_1 = mu'(A1) (Y-man).
  A1 sees p_2, so p_1 > p_2 in their common list. A1 improves: l_1 > p_1. If d(mu(l_1)) > d(A2), A2
  sees l_1 in mu, so p_2 > l_1; else A2 is strictly inside l_1's move, so p_2 > l_1 by F2. Cycle.
- (P3) Two B's with c(B1) < c(B2), d(B1) > d(B2): let l_k = mu(B_k) (Y-men), p_2 = mu'(B2). B2 sees
  l_1, so l_2 > l_1; B2 improves, p_2 > l_2; B1 sees p_2 in mu or is strictly inside p_2's move, so
  l_1 > p_2. Cycle.

**Configurations.** A *configuration* is a cyclic sequence A_0 B_0 ... A_{k-1} B_{k-1} (k >= 1) of points
with distinct c's, distinct d's and types x/y, satisfying K1-K3 above and
K4: c(A_i) < c(B_i) and d(B_i) < d(A_{i+1}). The boundary women of a rotation form a configuration.
A configuration is GOOD if: (i) sorted by c the roles read A B A B ...; (ii) sorted by d they read
B A B A ...; (iii) E_X: in c-order each B has the type of the A right after it; (iv) E_Y: in d-order
each A has the type of the B right after it; (v), (vi): the first and last points have different
types, in c-order and in d-order. By (i) each X-run is a c-adjacent pair; by (ii) each Y-run is a
d-adjacent pair.

**Theorem 4.** Every configuration is GOOD. (Also checked exhaustively for k <= 4.)

*Proof.* Induction on k; the mirror (c, A, X-runs) <-> (d, B, Y-runs) keeps the hypotheses and GOOD.
- k = 1: if type(A_0) = type(B_0), K1 and c(A_0) < c(B_0) give d(A_0) < d(B_0), against K4.
- No same-type run, k >= 2, is impossible: types alternate around the cycle, so all the run-buffer implementation have one
  type and all B's the other. K2/K3 forbid nested runs, K1 makes the the run-buffer implementation and the B's chains. For
  non-nested intervals the order by left ends equals the order by right ends, so rank(A_i) = rank(B_i)
  (X-runs, in c) and rank(B_i) = rank(A_{i+1}) (Y-runs, in d). So A_i = A_{i+1} and k = 1.
- A same-type Y-run exists (else use the mirror). (a) Choose a same-type Y-run S_i, of type tau, with
  the fewest tau-points strictly between B_i and A_{i+1} on the tau-chain. There are none: such a B_j
  starts a Y-run inside S_i (K3: crossing contradicts type(A_{i+1}) = tau; nested gives a smaller
  same-type run), and such an A_j ends one (K3 crossing contradicts type(A_j) = tau; nested is
  smaller). So B_i, A_{i+1} are consecutive on the tau-chain and c(B_i) < c(A_{i+1}).
  (b) Contract: delete B_i, A_{i+1} and merge R_i, S_i, R_{i+1} into the X-run (A_i, B_{i+1}). K1, K3,
  K4 are inherited. K2 for the merged run (A_i, B_{i+1}) against another X-run R_j = (u, v), with
  a < b < a' < b' the c-values of A_i, B_i, A_{i+1}, B_{i+1}, J = (b, a') (no tau-point has c in J):
  * u < a < b' < v (merged run nested in R_j): R_i, R_{i+1} are nested in R_j, so by K2
    type(A_i) = type(B_i) = tau = type(A_{i+1}) = type(B_{i+1}).
  * u < a < v < b' (crossing; need type(B_j) != type(A_i)): if v < b, K2(R_j, R_i) crossing; if v in J,
    R_i is nested (type(A_i) = tau) and type(B_j) != tau (gap); if v > a', R_i is nested and
    K2(R_j, R_{i+1}) crossing gives type(B_j) != type(A_{i+1}) = tau.
  * a < u < b' < v (crossing the other way; need type(B_{i+1}) != type(A_j)): if u < b, K2(R_i, R_j)
    crossing gives type(A_j) != tau and R_{i+1} is nested in R_j (type(B_{i+1}) = tau); if u in J,
    the gap gives type(A_j) != tau and R_{i+1} is nested; if u > a', K2(R_{i+1}, R_j) crossing.
  * a < u < v < b' (R_j nested in the merged run; need type(A_j) = type(B_j)): if u, v lie in the same
    one of (a, b), J, (a', b'), use K2 or the gap; otherwise type(A_j) != tau (K2(R_i, R_j) crossing,
    or gap) and type(B_j) != tau (gap, or K2(R_j, R_{i+1}) crossing), so they are equal.
  The contraction is a configuration with k - 1 runs, so it is GOOD.
  (c) c-order: B_i, A_{i+1} sit inside the merged run, which is c-adjacent; inserting them keeps (i),
  adds a junction of type tau (iii), and keeps the ends (v).
  (d) d-order: let P, N be the tau-chain neighbours of B_i, A_{i+1}. In a GOOD configuration,
  consecutive chain points (B, then A) are c-adjacent, (A, then B) are d-adjacent; a first chain point
  that is an A is c-first, one that is a B is d-first; a last chain point that is a B is c-last, one
  that is an A is d-last. If P is a B, it lies inside the merged run in c or is c-last: impossible.
  If N is an A and P is absent, N is c-first although c(A_i) < c(N): impossible. If both are absent the
  contraction has one type, against (v). So P is an A or absent, N is a B or absent, and B_i, A_{i+1}
  go into a d-adjacent tau-junction (or an end next to a tau-point), which keeps (ii), (iv), (vi). QED

*Proof of Theorem 1.* By Theorem 4, sorted by c the boundary types read t_0, t_1 t_1, ..., t_{k-1}
t_{k-1}, t_k with t_0 != t_k, and the X-runs are (t_0, t_1), (t_1, t_2), ... So by (1)
dXX = [t_k = x] - [t_0 = x] = +-1. QED

So: sign -1 iff the c-first boundary A_0 is x-type iff the c-last boundary B_c is y-type, and by the
mirror in d-order (dN_YY = dXX telescopes over Y-runs) iff the d-first boundary B_d is y-type iff the
d-last boundary A_d is x-type. The *c-hull* is H_c(rho) = [c(A_0), c(B_c)], the *d-hull*
H_d(rho) = [d(B_d), d(A_d)]; every X-run woman lies in H_c, every Y-run woman in H_d.

**Prefix lemma.** If sign(rho) = -1, every woman of rho with c below c(A_0) is x-type (and
by sigma, y-type for sign +1). So the sign is -1 iff the c-best woman of the support is x-type.

## 4. Proof of Theorem 2

**Column data.** For a stable nu and a column t, Q(t) = {X-men g : c(nu(g)) >= t};
kappa_a(t) = min a over Q(t), kappa_b(t) = min b over Q(t) (min of empty = infinity); both are
nondecreasing in t.
- (O1) If w is Y-labeled with partner l and t >= c(w): x-type w has a(l) < kappa_a(t), y-type w has
  b(l) < kappa_b(t). (Every g in Q(c(w)) has c(nu(g)) > c(w), so w prefers l to g by F1.)
- (O2) If w is X-labeled and t <= c(w): nu(w) is in Q(t), so kappa_a(t) <= a(nu(w)) and
  kappa_b(t) <= b(nu(w)); if w is x-type then a(nu(w)) = kappa_a(c(w)), if y-type
  b(nu(w)) = kappa_b(c(w)).

**Witnesses.** A (-1)-witness of nu is a pair (A, B): A x-type and Y-labeled, B y-type and
X-labeled, c(A) < c(B), d(B) < d(A). A (+1)-witness: the same with A y-type and B x-type. Its span is
[c(A), c(B)]; W^s(nu) is the union of the spans of the s-witnesses. sigma maps s-witnesses to
(-s)-witnesses; the mirror maps an s-witness (A, B) to the s-witness (B, A) with span [d(B), d(A)].

**Lemma 5.** W^{-1}(nu) and W^{+1}(nu) are disjoint.
*Proof.* Let t be in the spans of a -1-witness (A, B) and a +1-witness (A', B'); l = nu(A),
l' = nu(A') (Y-men), p = nu(B), p' = nu(B') (X-men). O1, O2: a(l) < kappa_a(t) <= a(p') and
b(l') < kappa_b(t) <= b(p). So B' prefers l to p', hence d(A) < d(B') (F1 for l); and B prefers l' to
p, hence d(A') < d(B). With the witness inequalities: d(A) < d(B') < d(A') < d(B) < d(A). QED

**Lemma 6 (covering).** In a configuration with c-first type t_0 and c-last type t_k, every two
c-consecutive points U, V have an A-point alpha of type t_0 and a B-point beta of type t_k with
c(alpha) <= c(U) < c(V) <= c(beta) and d(beta) < d(alpha).
*Proof.* Induction along the contraction of Theorem 4. k = 1: alpha = A_0, beta = B_0 (K4).
Y-contraction: the new pairs lie inside the merged run (A_i, B_{i+1}), a c-adjacent pair of the
contraction, whose (alpha, beta) covers them; t_0, t_k do not change. X-contraction of a same-type
X-run R_i = (A_i, B_i) of type tau (the mirror case of the contraction; the Y-run S* = (B_{i-1}, A_{i+1})
replaces S_{i-1}, R_i, S_i): if R_i is neither c-first nor c-last, its c-neighbours P, N are
consecutive in the contraction and cover it. If A_i is c-first, then tau = t_0 and the next point N
has type tau (E_X); B_{i-1} is not of type tau (else K1 and K4 put it before A_i in c), so it has
type t_k, lies after N in c, and d(B_{i-1}) < d(A_i) (K4): take alpha = A_i, beta = B_{i-1}. If B_i is
c-last, symmetrically take alpha = A_{i+1} (type t_0, c(A_{i+1}) <= c(P)) and beta = B_i. QED

**Corollary 7.** If rho has sign s, then H_c(rho) is contained in W^s(mu/rho). (In mu/rho every
A-point is Y-labeled and every B-point X-labeled, so each (alpha, beta) of Lemma 6 is an s-witness.)

**Lemma 8.** If sign(rho) = -1, every X-run (A, B) of rho with A y-type and B x-type has
d(A) < d(B). (Else (A, B) is a +1-witness of mu/rho inside H_c(rho), against Corollary 7 and Lemma 5.)

**Lemma 9.** If sign(rho) = -1 and A is an x-type A-boundary, the first y-type boundary y* after A
in c-order exists and d(y*) < d(A).
*Proof.* y* exists since B_c is y-type. Let z_f be the first element of the maximal c-block of x-type
boundaries containing A. Either z_f = A_0, or z_f is the B of an X-run (A', z_f) with A' y-type (an A
other than A_0 is preceded by a B of its type; a B is preceded by its run's A). If z_f = A_0, y* is
the c-first, hence (K1) the d-first, y-type boundary; B_d is y-type, so y* = B_d and d(y*) < d(A).
Otherwise d(A') < d(z_f) (Lemma 8); A' is not d-last (A_d is x-type), so its d-successor is a y-type B
(GOOD (ii), (iv)), which is A''s successor on the y-chain, i.e. y*. Then d(z_f) > d(y*), and K1 on
the x-type points z_f, A (c(z_f) < c(A)) gives d(A) > d(z_f) > d(y*). QED

**Lemma 10.** If tau has sign -1 and is exposed in nu, and A is an x-type A-boundary of tau, then
c(A) is not in W^{+1}(nu).
*Proof.* t = c(A), nu' = nu/tau. p = nu(A) has a(p) = kappa_a(t) (O2); l = nu'(A) is the last Y-man of
the Y-run ending at A, a(l) < a(p); u = nu(l), and the B-boundary beta starting that Y-run has
d(beta) <= d(u) and is the run-buffer implementation d-predecessor. For y* of Lemma 9: b(nu(y*)) >= kappa_b(t) (if y* is an A,
by O2; if y* is a B, the last X-man p* of its X-run is in Q(t) and y* improves to p*). Also
d(y*) <= d(beta) <= d(u). Let (A', B') be a +1-witness with c(A') <= t <= c(B'). By O2, B' prefers l
to her partner, so d(u) < d(B') (F1). By O1, y* prefers nu(A') to her partner, so d(A') < d(y*).
Hence d(u) < d(B') < d(A') < d(y*) <= d(u). QED (By sigma: for sign +1 and a y-type A-boundary,
c(A) is not in W^{-1}(nu).)

**Lemma 11 (monotonicity).** If tau is exposed in nu, then W^s(nu) is contained in W^s(nu/tau).
*Proof.* By sigma take s = -1. Let (A*, B*) be a -1-witness of nu; only boundaries of tau change
label (A* changes iff it is a B-boundary of tau; B* iff it is an A-boundary).
- sign(tau) = +1: B* cannot be an A-boundary (Lemma 10^sigma: c(B*) would be outside W^{-1}(nu)); A*
  cannot be a B-boundary (the same in the mirrored instance, where (B*, A*) is a -1-witness and A* a
  y-type A-boundary). So the witness survives.
- sign(tau) = -1: H_c(tau) is in W^{-1}(nu/tau) (Corollary 7). If only A* changes, c(A*) is in H_c(tau);
  the part of the span beyond max H_c(tau) is covered by the -1-witness (A_d(tau), B*). If only B*
  changes, the part before min H_c(tau) is covered by (A*, B_d(tau)). If both change, the span is
  inside H_c(tau). (Those two pairs are witnesses: d(B*) < d(A*) < d(A_d) and
  d(B_d) < d(B*) < d(A*).) QED

**Theorem 12 (HULL).** Rotations whose c-hulls meet, or whose d-hulls meet, have the same sign.
*Proof.* By Corollary 7 and Lemma 11, H_c(rho) is contained in W^{sign rho}(WOSM) for every rotation
rho. Apply Lemma 5 at WOSM. For d-hulls use the mirror. QED

**Corollary 13 (weak SEP).** All rotations that move a given person have the same sign. (If an
X-man is moved into w by rho and out of w by rho', c(w) is in both c-hulls; Y-men use d; a woman
moved by consecutive rotations shares her intermediate partner with both.)

*Proof of Theorem 2.* For an x-woman w and a rotation rho let e_rho(w) = +1 if w is a B-boundary of
rho, -1 if an A-boundary, 0 otherwise. Then dXX(rho) = sum_w e_rho(w), and sign(rho) dXX(rho) = 1.
So R = sum_w sum_rho sign(rho) e_rho(w). For fixed w all rotations with e_rho(w) != 0 move w, so they
have one sign s_w (Corollary 13), and the inner sum telescopes along a maximal chain to
s_w ([w is X-labeled in WOSM] - [w is X-labeled in MOSM]), which is in {-1, 0, 1} and 0 if the labels
agree. The same argument works for y-women (dN_XY = -dXX), X-men (N_XX) and Y-men (N_YY). QED

## 5. Scope

Theorem 2 bounds the number of rotations. It does not classify the shapes of
rotation posets and gives no subquadratic algorithm. A nonplanar Hasse diagram
with a subdivision of K3,3 is known (D-PL: P, A+). The size bound is tight by the
two-person block construction in Section 1.

## 6. Open: full SEP

Full SEP says that comparable rotations have the same sign. This remains partly
open and is not needed for D2. The weak separation theorem in Section 4 is
proved and audited (P, A+). SEP for covers between two length-2 rotations is
also proved and audited (SEP-2: P, A+). Exact finite checks for length pairs
(2,3), (3,2), (3,3), (2,4), and (4,2) are T, A+ (2026-10-05), not the general theorem.

## 7. Verification

The independent check in `verify/rotation_count.py` tests the witness, hull,
persistence, shared-person and endpoint-count statements. The original audit
(2026-10-05) covered 12,092 instances and 19,969 rotations. This repository
runs a smaller reproducible subset with `make test`. Finite tests do not prove the theorem.
