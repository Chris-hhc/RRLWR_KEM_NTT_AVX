/* 
 * Copyright 2026 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "poly.h"
#ifndef RRLWR_DISABLE_NTT_AVX
#include "ntt.h"
#endif

#ifdef RRLWR_MEASURE_NTT_CYCLES
static uint64_t poly_ntt32_cycles_acc = 0;
static uint64_t poly_ntt32_calls_acc = 0;
static uint64_t poly_invntt32_cycles_acc = 0;
static uint64_t poly_invntt32_calls_acc = 0;

static inline uint64_t poly_ntt32_cpucycles(void)
{
  uint64_t result;

  __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");

  return result;
}
#else
static uint64_t poly_ntt32_cycles_acc = 0;
static uint64_t poly_ntt32_calls_acc = 0;
static uint64_t poly_invntt32_cycles_acc = 0;
static uint64_t poly_invntt32_calls_acc = 0;
#endif

void poly_ntt32_cycles_reset(void)
{
  poly_ntt32_cycles_acc = 0;
  poly_ntt32_calls_acc = 0;
}

uint64_t poly_ntt32_cycles_total(void)
{
  return poly_ntt32_cycles_acc;
}

uint64_t poly_ntt32_cycles_calls(void)
{
  return poly_ntt32_calls_acc;
}

void poly_invntt32_cycles_reset(void)
{
  poly_invntt32_cycles_acc = 0;
  poly_invntt32_calls_acc = 0;
}

uint64_t poly_invntt32_cycles_total(void)
{
  return poly_invntt32_cycles_acc;
}

uint64_t poly_invntt32_cycles_calls(void)
{
  return poly_invntt32_calls_acc;
}

void poly_ntt32(poly *f, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]) {
#ifdef RRLWR_MEASURE_NTT_CYCLES
  uint64_t poly_ntt32_start = poly_ntt32_cpucycles();
#endif

#ifndef RRLWR_DISABLE_NTT_AVX
  (void)primeinv;
  (void)fp_zetas;
  ntt_avx(f->coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
#else
  unsigned int len, start, j, k;
  int32_t t, fp_zeta;
  int32_t *fc = f->coeffs;

  k = 1;
  for(len = (RRLWR_N >> 1); len >= 1; len >>= 1) {
    for(start = 0; start < RRLWR_N; start = j + len) {
      fp_zeta = fp_zetas[k++];
      for(j = start; j < start + len; j++) {
        t = montgomery_mul32(fp_zeta, fc[j + len], prime, primeinv);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
        fc[j + len] = fc[j] - t;
        fc[j] = fc[j] + t;
      }
    }
  }

  for(j = 0; j < RRLWR_N; j++) {
      fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
  }
#endif

#ifdef RRLWR_MEASURE_NTT_CYCLES
  poly_ntt32_cycles_acc += poly_ntt32_cpucycles() - poly_ntt32_start;
  poly_ntt32_calls_acc++;
#endif
}

void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]) {
#ifdef RRLWR_MEASURE_NTT_CYCLES
  uint64_t poly_invntt32_start = poly_ntt32_cpucycles();
#endif

#ifndef RRLWR_DISABLE_NTT_AVX
  (void)primeinv;
  (void)finalconst;
  (void)fp_zetas;
  intt_avx(f->coeffs, prime, RRLWR_KEM_INTT_FUSED_AVX_PARAMS);
#else
  unsigned int start, len, j, k;
  int32_t t, zeta;
  int32_t *fc = f->coeffs;

  k = RRLWR_N-1;
  for(len = 1; len <= (RRLWR_N >> 1); len <<= 1) {
    for(start = 0; start < RRLWR_N; start = j + len) {
      zeta = fp_zetas[k--];
      for(j = start; j < start + len; j++) {
        t = fc[j];
        fc[j] = t + fc[j + len];
        fc[j + len] = fc[j + len] - t;
        fc[j + len] = montgomery_mul32(zeta, fc[j + len], prime, primeinv);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
      }
    }
  }

  for(j = 0; j < RRLWR_N; j++) {
    fc[j] = montgomery_mul32(fc[j], finalconst, prime, primeinv);
  }
#endif

#ifdef RRLWR_MEASURE_NTT_CYCLES
  poly_invntt32_cycles_acc += poly_ntt32_cpucycles() - poly_invntt32_start;
  poly_invntt32_calls_acc++;
#endif
}
