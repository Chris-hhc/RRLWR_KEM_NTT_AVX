# Kyber768 AVX2 Breakdown

Data was collected from the Kyber768 AVX2 breakdown binary after rebuilding it
from the Kyber AVX2 Makefile:

```sh
make -B test/test_breakdown768
./test/test_breakdown768
```

The tables below use median cycles.  The share column is recomputed from the
median total for each section.

## IND-CPA

### `indcpa_keypair_derand`

Total median: **26,849 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.keypair.hash_g` | 1,243 | 4.63% |
| `indcpa.keypair.gen_a` | 17,618 | 65.62% |
| `indcpa.keypair.sample_noise` | 3,984 | 14.84% |
| `indcpa.keypair.ntt_s_reduce` | 874 | 3.26% |
| `indcpa.keypair.ntt_e` | 739 | 2.75% |
| `indcpa.keypair.matvec_tomont` | 1,307 | 4.87% |
| `indcpa.keypair.add_reduce` | 184 | 0.69% |
| `indcpa.keypair.pack_sk` | 227 | 0.85% |
| `indcpa.keypair.pack_pk` | 228 | 0.85% |

### `indcpa_enc`

Total median: **26,496 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.enc.unpack_pk` | 175 | 0.66% |
| `indcpa.enc.poly_frommsg` | 49 | 0.18% |
| `indcpa.enc.gen_at` | 17,630 | 66.54% |
| `indcpa.enc.sample_noise` | 3,987 | 15.05% |
| `indcpa.enc.ntt_sp` | 718 | 2.71% |
| `indcpa.enc.matvec_b` | 1,199 | 4.53% |
| `indcpa.enc.dot_v` | 396 | 1.49% |
| `indcpa.enc.invntt_b` | 776 | 2.93% |
| `indcpa.enc.invntt_v` | 249 | 0.94% |
| `indcpa.enc.add_reduce` | 280 | 1.06% |
| `indcpa.enc.pack_ct` | 491 | 1.85% |

### `indcpa_dec`

Total median: **2,147 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.dec.unpack_ct` | 212 | 9.87% |
| `indcpa.dec.unpack_sk` | 157 | 7.31% |
| `indcpa.dec.ntt_b` | 718 | 33.44% |
| `indcpa.dec.dot` | 390 | 18.16% |
| `indcpa.dec.invntt` | 249 | 11.60% |
| `indcpa.dec.sub_reduce` | 52 | 2.42% |
| `indcpa.dec.poly_tomsg` | 23 | 1.07% |

## KEM

### `kem_keypair`

Total median: **41,480 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.keypair.hash_g` | 1,249 | 3.01% |
| `indcpa.keypair.gen_a` | 18,393 | 44.34% |
| `indcpa.keypair.sample_noise` | 4,012 | 9.67% |
| `indcpa.keypair.ntt_s_reduce` | 862 | 2.08% |
| `indcpa.keypair.ntt_e` | 724 | 1.75% |
| `indcpa.keypair.matvec_tomont` | 1,310 | 3.16% |
| `indcpa.keypair.add_reduce` | 184 | 0.44% |
| `indcpa.keypair.pack_sk` | 231 | 0.56% |
| `indcpa.keypair.pack_pk` | 224 | 0.54% |
| `kem.keypair.random` | 2,479 | 5.98% |
| `kem.keypair.indcpa` | 27,628 | 66.61% |
| `kem.keypair.copy_pk` | 22 | 0.05% |
| `kem.keypair.hash_pk` | 11,101 | 26.76% |
| `kem.keypair.copy_z` | 0 | 0.00% |

### `kem_encaps`

Total median: **41,250 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.enc.unpack_pk` | 169 | 0.41% |
| `indcpa.enc.poly_frommsg` | 46 | 0.11% |
| `indcpa.enc.gen_at` | 17,747 | 43.02% |
| `indcpa.enc.sample_noise` | 3,990 | 9.67% |
| `indcpa.enc.ntt_sp` | 718 | 1.74% |
| `indcpa.enc.matvec_b` | 1,200 | 2.91% |
| `indcpa.enc.dot_v` | 396 | 0.96% |
| `indcpa.enc.invntt_b` | 779 | 1.89% |
| `indcpa.enc.invntt_v` | 249 | 0.60% |
| `indcpa.enc.add_reduce` | 280 | 0.68% |
| `indcpa.enc.pack_ct` | 497 | 1.20% |
| `kem.enc.random` | 1,945 | 4.72% |
| `kem.enc.copy_m` | 7 | 0.02% |
| `kem.enc.hash_pk` | 11,068 | 26.83% |
| `kem.enc.hash_g` | 1,334 | 3.23% |
| `kem.enc.indcpa` | 26,613 | 64.52% |
| `kem.enc.copy_ss` | 0 | 0.00% |

### `kem_decaps`

Total median: **43,999 cycles**

| Stage | Median cycles | Median share |
|---|---:|---:|
| `indcpa.enc.unpack_pk` | 166 | 0.38% |
| `indcpa.enc.poly_frommsg` | 49 | 0.11% |
| `indcpa.enc.gen_at` | 17,597 | 39.99% |
| `indcpa.enc.sample_noise` | 3,987 | 9.06% |
| `indcpa.enc.ntt_sp` | 715 | 1.63% |
| `indcpa.enc.matvec_b` | 1,195 | 2.72% |
| `indcpa.enc.dot_v` | 396 | 0.90% |
| `indcpa.enc.invntt_b` | 767 | 1.74% |
| `indcpa.enc.invntt_v` | 249 | 0.57% |
| `indcpa.enc.add_reduce` | 279 | 0.63% |
| `indcpa.enc.pack_ct` | 494 | 1.12% |
| `indcpa.dec.unpack_ct` | 215 | 0.49% |
| `indcpa.dec.unpack_sk` | 187 | 0.43% |
| `indcpa.dec.ntt_b` | 724 | 1.65% |
| `indcpa.dec.dot` | 390 | 0.89% |
| `indcpa.dec.invntt` | 261 | 0.59% |
| `indcpa.dec.sub_reduce` | 52 | 0.12% |
| `indcpa.dec.poly_tomsg` | 25 | 0.06% |
| `kem.dec.indcpa` | 2,190 | 4.98% |
| `kem.dec.copy_hpk` | 3 | 0.01% |
| `kem.dec.hash_g` | 1,362 | 3.10% |
| `kem.dec.reencrypt` | 26,444 | 60.10% |
| `kem.dec.verify` | 53 | 0.12% |
| `kem.dec.rkprf` | 13,601 | 30.91% |
| `kem.dec.cmov` | 6 | 0.01% |
