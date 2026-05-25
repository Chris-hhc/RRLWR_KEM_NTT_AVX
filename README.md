# RRLWR KEM NTT AVX2

This implementation uses a 32-bit NTT arithmetic path with AVX2 assembly for
the polynomial and matrix-vector kernels. The current `avxReal` path uses the
`ring_mul_Awin_*` API: `A` is sampled into the Awin layout, transformed once,
and then multiplied with secret vectors through AVX2 row-dot kernels in
`arith/pointwise.S`.

The benchmark data below is organized from `/home/ubuntu/compare.md`. All cycle
counts are **medians**.

## Test CPU

CPU information was collected with `lscpu` on the benchmark machine.

### Intel Xeon E5-2686 v4 (Broadwell-EP)

| Field | Value |
| --- | --- |
| Architecture | x86_64 |
| CPU model | Intel(R) Xeon(R) CPU E5-2686 v4 @ 2.30GHz |
| Microarchitecture | **Broadwell-EP** (family 6, model 79) |
| CPUs | 2 |
| Cores per socket | 2 |
| Threads per core | 1 |
| Socket(s) | 1 |
| L1d cache | 64 KiB (2 instances) |
| L1i cache | 64 KiB (2 instances) |
| L2 cache | 512 KiB (2 instances) |
| L3 cache | 45 MiB (1 instance) |
| Hypervisor | Xen |
| AVX2 | supported |

## Benchmark Notes

Build the RRLWR real AVX2 path from this directory:

```sh
make clean
make avxReal
./test/test_speed_KEM128_avxReal
./test/test_speed_KEM256_avxReal
./test/test_speed_KEM512_avxReal
```

Build the Kyber AVX2 speed tests from `/home/ubuntu/kyber/avx2`:

```sh
make -B test/test_speed512 test/test_speed1024
./test/test_speed512
./test/test_speed1024
```

The RRLWR speed harness reports the actual `avxReal` Awin kernel path. The
Kyber speed harness reports Kyber kernel names first, with the corresponding
RRLWR operation in parentheses. The mapping is operational, not identical:
Kyber uses 16-bit NTT arithmetic and RRLWR uses 32-bit NTT arithmetic.

### Compile flags

The default `Makefile` uses:

| Flag | Role |
| --- | --- |
| `-O3` | Maximum optimization |
| `-mavx2 -mbmi2 -mpopcnt` | AVX2/BMI2/popcnt code generation |
| `-march=native -mtune=native` | Tune for the local benchmark CPU |
| `-fomit-frame-pointer` | Omit frame pointers |
| `-z noexecstack` | Mark stack non-executable |

## PKE Stage Comparison

### RRLWR-128 vs Kyber512

| Stage | RRLWR-128 | Kyber512 | Faster |
| --- | ---: | ---: | --- |
| PKE keygen | 20,789 | **13,993** | Kyber512 |
| PKE encrypt | 25,530 | **15,070** | Kyber512 |
| PKE decrypt | 7,998 | **1,448** | Kyber512 |

### RRLWR-256 vs Kyber1024

| Stage | RRLWR-256 | Kyber1024 | Faster |
| --- | ---: | ---: | --- |
| PKE keygen | 41,342 | **37,638** | Kyber1024 |
| PKE encrypt | 51,192 | **39,211** | Kyber1024 |
| PKE decrypt | 16,499 | **2,610** | Kyber1024 |

### RRLWR-512 Standalone

| Stage | Median cycles |
| --- | ---: |
| PKE keygen | 101,580 |
| PKE encrypt | 128,917 |
| PKE decrypt | 37,315 |

## KEM Stage Comparison

### RRLWR-128 vs Kyber512

| Stage | RRLWR-128 | Kyber512 | Faster |
| --- | ---: | ---: | --- |
| KEM keygen | 40,971 | **24,420** | Kyber512 |
| KEM encaps | 39,009 | **25,945** | Kyber512 |
| KEM decaps | 44,299 | **27,478** | Kyber512 |

### RRLWR-256 vs Kyber1024

| Stage | RRLWR-256 | Kyber1024 | Faster |
| --- | ---: | ---: | --- |
| KEM keygen | 68,712 | **56,310** | Kyber1024 |
| KEM encaps | 72,947 | **57,589** | Kyber1024 |
| KEM decaps | 87,677 | **62,062** | Kyber1024 |

### RRLWR-512 Standalone

| Stage | Median cycles |
| --- | ---: |
| KEM keygen | 142,908 |
| KEM encaps | 163,408 |
| KEM decaps | 203,135 |

## Kernel-Level Comparison

### RRLWR-128 vs Kyber512

| RRLWR operation | RRLWR kernel | RRLWR-128 | Kyber kernel | Kyber512 | Faster |
| --- | --- | ---: | --- | ---: | --- |
| sample | `ring_uniform_Awin + ring_uniform` | **9,624** | `gen_matrix + poly_getnoise_eta1_4x` | 10,265 | RRLWR-128 |
| sample A | `ring_uniform_Awin` | 7,422 | `gen_matrix` | **6,502** | Kyber512 |
| sample secret | `ring_uniform` | **2,150** | `poly_getnoise_eta1_4x` | 3,754 | RRLWR-128 |
| NTT | `poly_ntt32` | 549 | `poly_ntt/ntt_avx` | **230** | Kyber512 |
| inverse NTT | `poly_invntt32` | 553 | `poly_invntt_tomont/invntt_avx` | **243** | Kyber512 |
| base multiplication | `poly_basemul32` | 154 | `poly_basemul_montgomery/basemul_avx` | **111** | Kyber512 |
| Awin prepare base multiplication | `poly_basemul32_avx` | 151 | `poly_basemul_montgomery/basemul_avx` | **111** | Kyber512 |
| accumulate multiply | `poly_basemul_add32` | **181** | `polyvec_basemul_acc_montgomery` | 255 | RRLWR-128 |
| modular add | `poly_add32` | 31 | `poly_add` | **10** | Kyber512 |
| raw add | `poly_add` | **10** | `poly_add` | **10** | Tie |
| modular sub | `poly_sub32` | 31 | `poly_sub` | **10** | Kyber512 |
| vector NTT | `ring_ntt32(s)` | 2,806 | `polyvec_ntt` | **506** | Kyber512 |
| unpack/prepare ell input | `ring_to_Awin_ncoeffs(ell)` | 2,932 | `polyvec_decompress + polyvec_ntt` | **614** | Kyber512 |
| unpack public-key ell input | `ring_unpack_Awin_ncoeffs(ell)` | 3,088 | `polyvec_frombytes` | **123** | Kyber512 |
| full matrix-vector dot | `ring_Awin_dot32(full)` | 4,392 | `polyvec_basemul_acc_montgomery x K` | **522** | Kyber512 |
| ell/scalar dot | `ring_Awin_dot32(ell)` | 942 | `polyvec_basemul_acc_montgomery x 1` | **255** | Kyber512 |
| dot + inverse NTT + round | `ring_Awin_invntt_round_xtoy_32(full)` | 7,597 | `polyvec_basemul_acc_montgomery x K + polyvec_invntt_tomont` | **1,055** | Kyber512 |
| NTT + dot + inverse NTT + round | `ring_Awin_round_xtoy_32(full)` | 10,310 | `polyvec_ntt + dot x K + polyvec_invntt_tomont` | **1,567** | Kyber512 |
| dot + inverse NTT + reduce | `ring_Awin_invntt_reduce_pow2_32(ell)` | 1,567 | `polyvec_basemul_acc_montgomery + poly_invntt_tomont` | **509** | Kyber512 |
| NTT + dot + inverse NTT + reduce | `ring_Awin_reduce_pow2_32(ell)` | 4,285 | `polyvec_ntt + polyvec_basemul_acc_montgomery + poly_invntt_tomont` | **1,027** | Kyber512 |

Kyber-only supporting kernels from the same run:

| Kernel | Median cycles |
| --- | ---: |
| `polyvec_basemul_acc_montgomery x K + poly_tomont x K` | 617 |
| `polyvec_reduce` | 80 |
| `poly_reduce` | 28 |
| `polyvec_compress` | 304 |
| `poly_compress` | 25 |

### RRLWR-256 vs Kyber1024

| RRLWR operation | RRLWR kernel | RRLWR-256 | Kyber kernel | Kyber1024 | Faster |
| --- | --- | ---: | --- | ---: | --- |
| sample | `ring_uniform_Awin + ring_uniform` | **14,530** | `gen_matrix + poly_getnoise_eta1_4x` | 27,932 | RRLWR-256 |
| sample A | `ring_uniform_Awin` | **12,144** | `gen_matrix` | 26,009 | RRLWR-256 |
| sample secret | `ring_uniform` | 2,374 | `poly_getnoise_eta1_4x` | **1,899** | Kyber1024 |
| NTT | `poly_ntt32` | 534 | `poly_ntt/ntt_avx` | **230** | Kyber1024 |
| inverse NTT | `poly_invntt32` | 552 | `poly_invntt_tomont/invntt_avx` | **246** | Kyber1024 |
| base multiplication | `poly_basemul32` | 148 | `poly_basemul_montgomery/basemul_avx` | **108** | Kyber1024 |
| Awin prepare base multiplication | `poly_basemul32_avx` | 148 | `poly_basemul_montgomery/basemul_avx` | **108** | Kyber1024 |
| accumulate multiply | `poly_basemul_add32` | **181** | `polyvec_basemul_acc_montgomery` | 528 | RRLWR-256 |
| modular add | `poly_add32` | 28 | `poly_add` | **13** | Kyber1024 |
| raw add | `poly_add` | **10** | `poly_add` | 13 | RRLWR-256 |
| modular sub | `poly_sub32` | 28 | `poly_sub` | **13** | Kyber1024 |
| vector NTT | `ring_ntt32(s)` | 4,861 | `polyvec_ntt` | **1,022** | Kyber1024 |
| unpack/prepare ell input | `ring_to_Awin_ncoeffs(ell)` | 5,161 | `polyvec_decompress + polyvec_ntt` | **1,417** | Kyber1024 |
| unpack public-key ell input | `ring_unpack_Awin_ncoeffs(ell)` | 5,542 | `polyvec_frombytes` | **326** | Kyber1024 |
| full matrix-vector dot | `ring_Awin_dot32(full)` | 13,932 | `polyvec_basemul_acc_montgomery x K` | **2,128** | Kyber1024 |
| ell/scalar dot | `ring_Awin_dot32(ell)` | 3,211 | `polyvec_basemul_acc_montgomery x 1` | **531** | Kyber1024 |
| dot + inverse NTT + round | `ring_Awin_invntt_round_xtoy_32(full)` | 19,394 | `polyvec_basemul_acc_montgomery x K + polyvec_invntt_tomont` | **3,162** | Kyber1024 |
| NTT + dot + inverse NTT + round | `ring_Awin_round_xtoy_32(full)` | 24,270 | `polyvec_ntt + dot x K + polyvec_invntt_tomont` | **4,205** | Kyber1024 |
| dot + inverse NTT + reduce | `ring_Awin_invntt_reduce_pow2_32(ell)` | 4,435 | `polyvec_basemul_acc_montgomery + poly_invntt_tomont` | **782** | Kyber1024 |
| NTT + dot + inverse NTT + reduce | `ring_Awin_reduce_pow2_32(ell)` | 9,298 | `polyvec_ntt + polyvec_basemul_acc_montgomery + poly_invntt_tomont` | **1,819** | Kyber1024 |

Kyber-only supporting kernels from the same run:

| Kernel | Median cycles |
| --- | ---: |
| `polyvec_basemul_acc_montgomery x K + poly_tomont x K` | 2,288 |
| `polyvec_reduce` | 178 |
| `poly_reduce` | 28 |
| `polyvec_compress` | 697 |
| `poly_compress` | 62 |

### RRLWR-512 Standalone Kernels

| Kernel | Median cycles |
| --- | ---: |
| `sample` | 27,002 |
| `sample_Awin(A)` | 23,963 |
| `sample_secret(s)` | 2,822 |
| `poly_ntt32` | 549 |
| `poly_invntt32` | 559 |
| `poly_basemul32` | 154 |
| `poly_basemul32_avx(Awin prepare)` | 151 |
| `poly_basemul_add32(accumulate)` | 175 |
| `poly_add32` | 28 |
| `poly_add` | 10 |
| `poly_sub32` | 28 |
| `ring_ntt32(s)` | 9,431 |
| `ring_to_Awin_ncoeffs(ell)` | 10,460 |
| `ring_unpack_Awin_ncoeffs(ell)` | 10,988 |
| `ring_Awin_dot32(full, ntt-domain)` | 50,788 |
| `ring_Awin_dot32(ell, ntt-domain)` | 11,930 |
| `ring_Awin_invntt_round_xtoy_32(full)` | 61,760 |
| `ring_Awin_round_xtoy_32(full)` | 71,318 |
| `ring_Awin_invntt_reduce_pow2_32(ell)` | 14,481 |
| `ring_Awin_reduce_pow2_32(ell)` | 23,948 |

## High-Level Takeaways

On this Broadwell-EP machine, Kyber's 16-bit NTT kernels are substantially
faster than the current RRLWR 32-bit NTT kernels. RRLWR sampling is competitive
or faster for the larger `A` sampling cases, and `poly_basemul_add32` is faster
than Kyber's closest accumulation benchmark, but the full matrix-vector path is
dominated by RRLWR's 32-bit NTT, inverse NTT, and dot kernels.

For the current NTT AVX2 implementation, the largest remaining cycle sinks are:

| Area | Evidence from tables |
| --- | --- |
| Full Awin dot | `ring_Awin_dot32(full)` is 4,392 / 13,932 / 50,788 cycles for RRLWR-128/256/512 |
| NTT-domain conversion | `ring_ntt32(s)` scales from 2,806 to 9,431 cycles |
| Fused output path | `ring_Awin_round_xtoy_32(full)` reaches 71,318 cycles at RRLWR-512 |
| Decryption ell path | `ring_Awin_reduce_pow2_32(ell)` reaches 23,948 cycles at RRLWR-512 |

## Appendix: Kernel Operation Notes

This appendix gives a short description of the small kernels used in the
breakdown tables. The descriptions focus on what each benchmarked operation
does in the current speed harness.

### RRLWR Kernels

| Kernel | Operation |
| --- | --- |
| `sample` | Generates the public matrix input through `ring_uniform_Awin` and the secret vector through `ring_uniform`. This approximates the keygen/encrypt sampling front end. |
| `sample_Awin(A)` | Samples `A` directly into the Awin layout, runs the NTT on each base row, and prepares the `(y+2) * a_i` rows used by the Awin matrix-vector dot product. |
| `sample_secret(s)` | Samples the small secret vector `s` with coefficients in the eta range. The output is still in coefficient domain. |
| `poly_ntt32` | Runs one 32-bit forward NTT on a single polynomial. |
| `poly_invntt32` | Runs one 32-bit inverse NTT on a single polynomial and applies the final NTT constant. |
| `poly_basemul32` | Multiplies two NTT-domain polynomials coefficient-wise with 32-bit Montgomery multiplication. |
| `poly_basemul32_avx(Awin prepare)` | AVX2 base multiplication used while preparing Awin twist rows. It computes `(y+2) * a_i` in the NTT domain. |
| `poly_basemul_add32(accumulate)` | Multiplies two NTT-domain polynomials and accumulates the product into an existing output polynomial. This is the scalar building block behind row-dot accumulation. |
| `poly_add32` | Adds two polynomials with reduction modulo the NTT prime. |
| `poly_add` | Adds two polynomials without modular reduction. |
| `poly_sub32` | Subtracts two polynomials with reduction modulo the NTT prime. |
| `ring_ntt32(s)` | Applies `poly_ntt32` to every polynomial in a secret vector. This converts `s` to NTT domain before Awin dot products. |
| `ring_to_Awin_ncoeffs(ell)` | Converts a coefficient-domain ring element to Awin layout for `ell` output coefficients, including NTT and twist-row preparation. Used by decrypt-style paths. |
| `ring_unpack_Awin_ncoeffs(ell)` | Unpacks a packed public-key/ciphertext ring element into Awin layout for `ell` output coefficients, including NTT and twist-row preparation. Used by encrypt-style public-key multiplication. |
| `ring_Awin_dot32(full, ntt-domain)` | Computes the full Awin matrix-vector dot product when both `Awin` and the secret vector are already in NTT domain. This isolates the AVX2 row-dot kernels in `pointwise.S`. |
| `ring_Awin_dot32(ell, ntt-domain)` | Same as the full dot product, but only computes the `ell` output coefficients needed for the second multiplication path. |
| `ring_Awin_invntt_round_xtoy_32(full)` | Runs Awin NTT-domain dot product, inverse NTT, and fused reduce/round from `q` to `p`. The secret vector is already in NTT domain. |
| `ring_Awin_round_xtoy_32(full)` | Full keygen/encrypt-style path: NTT the secret vector, run Awin dot product, inverse NTT, and fused reduce/round from `q` to `p`. |
| `ring_Awin_invntt_reduce_pow2_32(ell)` | Runs Awin NTT-domain dot product, inverse NTT, and reduce modulo a power of two for `ell` outputs. The secret vector is already in NTT domain. |
| `ring_Awin_reduce_pow2_32(ell)` | Full decrypt/encrypt second-product style path: NTT the vector, run Awin dot product, inverse NTT, and reduce modulo a power of two for `ell` outputs. |

### Kyber Kernels

| Kernel | Operation |
| --- | --- |
| `gen_matrix + poly_getnoise_eta1_4x` | Generates Kyber's public matrix with SHAKE128 rejection sampling and samples secret/noise polynomials with the 4-way eta sampler. This corresponds to RRLWR `sample`. |
| `gen_matrix` | Generates the public matrix `A` or `A^T`; output polynomials are unpacked into Kyber's NTT-friendly order. This corresponds roughly to RRLWR `sample_Awin(A)`. |
| `poly_getnoise_eta1_4x` | Four-way secret/noise sampler using SHAKE256 and CBD conversion. This corresponds to RRLWR `sample_secret(s)`. |
| `poly_ntt/ntt_avx` | Runs one Kyber 16-bit forward NTT. |
| `poly_invntt_tomont/invntt_avx` | Runs one Kyber 16-bit inverse NTT and converts to Montgomery form. |
| `poly_basemul_montgomery/basemul_avx` | Multiplies two Kyber NTT-domain polynomials with the AVX2 basemul kernel. |
| `polyvec_basemul_acc_montgomery` | Computes one vector dot product in NTT domain by repeated basemul and add. This is the closest Kyber equivalent to one RRLWR Awin row-dot output. |
| `polyvec_ntt` | Applies Kyber `poly_ntt` to every polynomial in a vector. This corresponds to RRLWR `ring_ntt32(s)`. |
| `polyvec_decompress + polyvec_ntt` | Decompresses a ciphertext vector and transforms it to NTT domain. This corresponds to a decrypt-side Awin input preparation path. |
| `polyvec_frombytes` | Loads an already serialized NTT-domain public-key/secret-key vector. This is much cheaper than RRLWR Awin unpack+NTT because Kyber serializes key polynomials in NTT representation. |
| `polyvec_basemul_acc_montgomery x K` | Computes all `K` output rows of a matrix-vector product by calling `polyvec_basemul_acc_montgomery` once per row. |
| `polyvec_basemul_acc_montgomery x 1` | Computes a single vector dot product, corresponding to an `ell`/scalar output path. |
| `poly_tomont x K` | Converts each keygen matrix-vector output into Montgomery form after the dot product. Kyber keygen includes this postscale step. |
| `polyvec_invntt_tomont` | Applies inverse NTT to every polynomial in a vector after the matrix-vector dot product. |
| `poly_invntt_tomont` | Applies inverse NTT to a single polynomial after one vector dot product. |
| `polyvec_reduce` | Barrett-reduces every coefficient in a Kyber polyvec after additions. |
| `poly_reduce` | Barrett-reduces one Kyber polynomial. |
| `polyvec_compress` | Compresses and serializes a Kyber ciphertext vector. |
| `poly_compress` | Compresses and serializes the Kyber scalar ciphertext polynomial. |
