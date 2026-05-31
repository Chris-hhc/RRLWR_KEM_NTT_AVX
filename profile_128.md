# RRLWR-128 Profiling Notes

This document collects profiling notes for the 128-bit parameter set.  The
first section focuses on PKE key generation and compares the current
RRLWR-128 AVX2 implementation against Kyber512 AVX2.  Other stages can be
added below in the same format.

## 1. PKE Key Generation

### 1.1 Raw Breakdown Data

Current RRLWR-128 `pke_keygen` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `pke_keygen total` | 18,299 | 100.00% |
| `pke.keygen.A_uniform_Awin` | 7,572 | 41.38% |
| `pke.keygen.Awin_base` | 3,938 | 21.52% |
| `pke.keygen.Awin_prepare` | 3,481 | 19.02% |
| `pke.keygen.s_uniform` | 2,169 | 11.85% |
| `pke.keygen.pack_s` | 390 | 2.13% |
| `pke.keygen.A_times_s_round` | 7,600 | 41.53% |
| `pke.keygen.copy_seedA` | 3 | 0.02% |
| `pke.keygen.pack_b` | 377 | 2.06% |

Kyber512 `indcpa_keypair_derand` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa_keypair_derand total` | 14,401 | 100.00% |
| `indcpa.keypair.hash_g` | 1,248 | 8.67% |
| `indcpa.keypair.gen_a` | 6,747 | 46.85% |
| `indcpa.keypair.sample_noise` | 3,920 | 27.22% |
| `indcpa.keypair.ntt_s_reduce` | 565 | 3.92% |
| `indcpa.keypair.ntt_e` | 479 | 3.33% |
| `indcpa.keypair.matvec_tomont` | 586 | 4.07% |
| `indcpa.keypair.add_reduce` | 117 | 0.81% |
| `indcpa.keypair.pack_sk` | 147 | 1.02% |
| `indcpa.keypair.pack_pk` | 148 | 1.03% |

The RRLWR-128 key generation median is about 18,299 cycles, while Kyber512
key generation is about 14,401 cycles.  The observed gap is therefore about
3,898 cycles.

### 1.2 Parameter-Level Comparison

The two implementations are not doing the same amount of arithmetic work.

| Item | RRLWR-128 | Kyber512 |
|---|---:|---:|
| Polynomial degree | `N = 128` | `N = 256` |
| Vector dimension | `K = 5` | `K = 2` |
| Uniform matrix coefficients | `5 * 128 = 640` | `2 * 2 * 256 = 1024` |
| Matrix-vector coefficient products | `5 * 5 * 128 = 3200` | `2 * 2 * 256 = 1024` |
| NTT arithmetic width | 32-bit Montgomery | 16-bit AVX2-oriented arithmetic |
| Keygen output shape | `R_p^5` public-key vector | `R_q^2` public-key vector |
| Matrix representation cost | Awin sampling plus NTT/Awin preparation | sampled in the representation consumed by matvec; no separate A NTT stage |

The important point is the matrix-vector product.  RRLWR-128 performs about
3.125x as many coefficient products as Kyber512 in key generation:

```text
(5 * 5 * 128) / (2 * 2 * 256) = 3200 / 1024 = 3.125
```

This is a parameter and representation cost, not simply an assembly-quality
issue.

### 1.3 Stage-Level Difference

Grouping keygen work by median cycles:

| Group | RRLWR-128 | Kyber512 | Difference |
|---|---:|---:|---:|
| Matrix sampling | 3,938 | 6,747 | -2,809 |
| RRLWR-only A-domain preparation | 3,481 | N/A | N/A |
| Secret or noise sampling | 2,169 | 3,920 | -1,751 |
| Main arithmetic | 7,600 | 1,747 | +5,853 |
| Packing | 767 | 295 | +472 |
| Kyber-only `hash_g` | 0 | 1,248 | -1,248 |

The total median gap is about 3,898 cycles.  The largest positive difference
is the main arithmetic group, while RRLWR matrix sampling and secret sampling
are faster than Kyber512 in this measurement.

#### 1.3.1 Matrix Sampling and A-Domain Preparation

RRLWR separates matrix generation into two parts:

| Stage | Median cycles |
|---|---:|
| `Awin_base` | 3,938 |
| `Awin_prepare` | 3,481 |
| total `A_uniform_Awin` | 7,572 |

Kyber512 `gen_a` costs 6,747 cycles.  This is not directly equivalent to
RRLWR `A_uniform_Awin`.  The closest sampler-only comparison is:

| Stage | Median cycles | Coefficients | Cycles per coefficient |
|---|---:|---:|---:|
| RRLWR-128 `Awin_base` | 3,938 | 640 | 6.15 |
| Kyber512 `gen_a` | 6,747 | 1,024 | 6.59 |

The RRLWR base sampler is not the problem here.  The extra cost is
`Awin_prepare`, which includes NTT conversion of the sampled Awin rows and
precomputation of the `(y + 2) * a_i` twist rows.  Kyber512 samples matrix
entries in the representation consumed by its matvec path and does not pay a
separately measured A-NTT preparation stage.

The `(y + 2)` multiplication is not an optional implementation artifact.  It is
required by the Awin representation so that the later NTT-domain dot kernels
can evaluate the wrapped ring products directly.

#### 1.3.2 Matrix-Vector Dot Kernel

The current RRLWR Awin dot kernel is competitive when normalized per
coefficient product:

| Kernel | Median cycles | Coefficient products | Cycles per product |
|---|---:|---:|---:|
| RRLWR-128 `ring_Awin_dot32(full)` | 1,582 | 3,200 | 0.494 |
| Kyber512 `matvec_tomont` | 586 | 1,024 | 0.572 |

On this metric, the current RRLWR pointwise kernel is already competitive.
The absolute cycle count is higher mostly because the RRLWR-128 keygen matrix
product has 3.125x more coefficient products.

#### 1.3.3 Total Arithmetic Comparison

RRLWR-128 `A_times_s_round` is approximately:

```text
A_times_s_round
  ~= ring_ntt32(s)
   + ring_Awin_dot32(full, ntt-domain)
   + K * poly_invntt32
   + final reduce/round tail
```

Measured median components:

| Component | Median cycles |
|---|---:|
| `ring_ntt32(s)` | 2,773 |
| `ring_Awin_dot32(full, ntt-domain)` | 1,582 |
| post-dot inverse NTT and round | about 3,202 |
| directly measured full `A_times_s_round` stage | 7,600 |

The post-dot row is inferred from standalone dot-plus-INTT measurements.  The
last row is the full keygen stage measurement and should be treated as the
authoritative number for this path.

Kyber512 keygen arithmetic is:

```text
ntt_s_reduce + ntt_e + matvec_tomont + add_reduce
= 565 + 479 + 586 + 117
= 1747 cycles
```

Single-polynomial NTT/INTT microbenchmarks show the cost difference more
directly:

| Transform | RRLWR-128 | Kyber512 | Ratio |
|---|---:|---:|---:|
| forward NTT | 556 | 236 | 2.36x |
| inverse NTT | 559 | 243 | 2.30x |

This gap is not only from using 32-bit arithmetic.  The current RRLWR NTT uses
a larger 32-bit Montgomery prime, so each butterfly has fewer cheap packing
opportunities than Kyber's 16-bit modulo-3329 arithmetic.  It also has less
room for Kyber-style aggressive lazy reduction: intermediate values grow
quickly in 32-bit lanes, so the implementation has to reduce more frequently
to stay within the signed 32-bit range and preserve correctness.

The differences are structural:

1. RRLWR-128 NTTs five secret polynomials, while Kyber512 NTTs two secret
   polynomials and two error polynomials.
2. RRLWR-128 performs `5 * 5 * 128 = 3200` coefficient products, while
   Kyber512 performs `2 * 2 * 256 = 1024`.
3. RRLWR-128 produces five public-key polynomials and therefore pays five
   inverse NTTs plus the final `q -> p` rounding tail.  Kyber512's measured
   keygen arithmetic does not have an equivalent inverse-NTT output conversion.
4. A single RRLWR operation is heavier because it uses 32-bit Montgomery
   arithmetic with a larger NTT prime, while Kyber512 is optimized around
   16-bit arithmetic modulo 3329.

In short, the current dot kernel is not the main source of the keygen gap.
The gap is dominated by Awin preparation, more matrix-vector products, more
output inverse NTTs, and heavier 32-bit modular arithmetic.

## 2. PKE Encryption

### 2.1 Raw Breakdown Data

Current RRLWR-128 `pke_encrypt` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `pke_encrypt total` | 22,734 | 100.00% |
| `pke.enc.A_uniform_Awin` | 7,526 | 33.10% |
| `pke.enc.Awin_base` | 3,938 | 17.32% |
| `pke.enc.Awin_prepare` | 3,453 | 15.19% |
| `pke.enc.sp_uniform` | 2,166 | 9.53% |
| `pke.enc.A_times_sp_round` | 7,637 | 33.59% |
| `pke.enc.unpack_b_Awin` | 3,147 | 13.84% |
| `pke.enc.unpack_b_base` | 301 | 1.32% |
| `pke.enc.unpack_b_prepare` | 2,754 | 12.11% |
| `pke.enc.b_times_sp_reduce` | 991 | 4.36% |
| `pke.enc.add_msg` | 448 | 1.97% |
| `pke.enc.pack_cm` | 114 | 0.50% |
| `pke.enc.pack_bp` | 353 | 1.55% |

Kyber512 `indcpa_enc` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa_enc total` | 15,450 | 100.00% |
| `indcpa.enc.unpack_pk` | 114 | 0.74% |
| `indcpa.enc.poly_frommsg` | 43 | 0.28% |
| `indcpa.enc.gen_at` | 6,686 | 43.28% |
| `indcpa.enc.sample_noise` | 5,505 | 35.63% |
| `indcpa.enc.ntt_sp` | 476 | 3.08% |
| `indcpa.enc.matvec_b` | 512 | 3.31% |
| `indcpa.enc.dot_v` | 255 | 1.65% |
| `indcpa.enc.invntt_b` | 507 | 3.28% |
| `indcpa.enc.invntt_v` | 249 | 1.61% |
| `indcpa.enc.add_reduce` | 206 | 1.33% |
| `indcpa.enc.pack_ct` | 341 | 2.21% |

The RRLWR-128 encryption median is about 22,734 cycles, while Kyber512
encryption is about 15,450 cycles.  The observed gap is therefore about
7,284 cycles.

### 2.2 Short Interpretation

The `A_uniform_Awin` versus Kyber `gen_at` difference is the same pattern as
key generation: RRLWR has base sampling plus Awin preparation, while Kyber's
generated matrix is already in the representation consumed by its matvec path.

The encryption-only preparation cost is `unpack_b_Awin`.  The base unpack is
only 301 cycles, but `unpack_b_prepare` costs 2,754 cycles because the rounded
public-key vector must be converted back into the NTT/Awin multiplication
representation.  For RRLWR-128, `ell = 1`, so this step is essentially the NTT
conversion of the packed vector; for larger `ell`, the same Awin rule also
requires precomputing `(y + 2) * b` twist rows.  Kyber's `unpack_pk` is only
114 cycles because the public key is serialized in the representation consumed
by the NTT-domain dot path.

The multiplication stages follow the same matrix-vector structure discussed in
key generation.  `A_times_sp_round` is the full `K = 5` path.  By contrast,
`b_times_sp_reduce` only needs the `[K - ell, K - 1]` output range; for
RRLWR-128, `ell = 1`, so it computes one output polynomial instead of five and
is much smaller at 991 cycles.

## 3. PKE Decryption

### 3.1 Raw Breakdown Data

Current RRLWR-128 `pke_decrypt` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `pke_decrypt total` | 7,830 | 100.00% |
| `pke.dec.unpack_s` | 286 | 3.65% |
| `pke.dec.unpack_bp` | 295 | 3.77% |
| `pke.dec.unpack_cm` | 71 | 0.91% |
| `pke.dec.bp_to_Awin` | 2,969 | 37.92% |
| `pke.dec.bp_to_Awin_base` | 104 | 1.33% |
| `pke.dec.bp_to_Awin_prepare` | 2,757 | 35.21% |
| `pke.dec.bp_times_s_reduce` | 3,788 | 48.38% |
| `pke.dec.sub_round_pack` | 92 | 1.17% |

Kyber512 `indcpa_dec` breakdown:

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa_dec total` | 1,687 | 100.00% |
| `indcpa.dec.unpack_ct` | 175 | 10.37% |
| `indcpa.dec.unpack_sk` | 120 | 7.11% |
| `indcpa.dec.ntt_b` | 476 | 28.22% |
| `indcpa.dec.dot` | 255 | 15.12% |
| `indcpa.dec.invntt` | 249 | 14.76% |
| `indcpa.dec.sub_reduce` | 53 | 3.14% |
| `indcpa.dec.poly_tomsg` | 22 | 1.30% |

The RRLWR-128 decryption median is about 7,830 cycles, while Kyber512
decryption is about 1,687 cycles.  The observed gap is therefore about
6,143 cycles.

### 3.2 Short Interpretation

The source path confirms that the decryption gap is mostly explicit NTT work.
After unpacking, RRLWR converts `bp` to Awin form:

```text
ring_to_Awin_ncoeffs(bp_aw, bp, ell)
  -> ring_Awin_ntt_prepare_ncoeffs(bp_aw, ell)
  -> poly_ntt32 on all K bp polynomials
```

For RRLWR-128, `ell = 1`, so the twist preparation returns immediately after
the NTT step.  Thus `bp_to_Awin_prepare` is essentially five forward NTTs of
the ciphertext `bp` vector.  For the higher parameter sets where `ell > 1`,
this same preparation also has to compute the mandatory `(y + 2) * bp` twist
products required by the Awin representation.

The next stage also includes an NTT that is easy to miss:

```text
ring_mul_Awin_reduce_pow2_32(v, bp_aw, s, ell)
  -> ring_ntt32(s)
  -> ring_mul_Awin_invntt_reduce_pow2_32(...)
```

This means `bp_times_s_reduce` is not just the ell dot plus inverse NTT.  It
also includes five forward NTTs of the secret key `s`, because the secret key
is unpacked from compact coefficient-domain form during decryption.

This explains the difference between encryption and decryption:

| Stage | Median cycles | NTT of secret included? |
|---|---:|---|
| encryption `b_times_sp_reduce` | 991 | no, `sp` was already NTT-transformed |
| decryption `bp_times_s_reduce` | 3,788 | yes, includes `ring_ntt32(s)` |

The difference is about 2,797 cycles, which is almost exactly the cost of one
`ring_ntt32` on a five-polynomial RRLWR-128 vector.  Kyber512 avoids this
because its secret key is serialized in the NTT representation; decryption only
needs `ntt_b`, then `dot`, then `invntt`.

So the main decryption bottleneck is not the ell dot kernel.  The expensive
part is the two forward-NTT preparations: `bp_to_Awin_prepare` for ciphertext
`bp`, and the implicit `ring_ntt32(s)` inside `bp_times_s_reduce`.
