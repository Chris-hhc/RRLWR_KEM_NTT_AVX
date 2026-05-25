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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "cpucycles.h"
#include "speed_print.h"
#include "drng.h"
#include "parameters.h"
#include "fprime.h"
#include "poly.h"
#include "ring.h"
#include "pke.h"
#include "kem.h"

#define NUMBER_OF_TESTS 1000
uint64_t t[NUMBER_OF_TESTS];
uint64_t ntt_t[NUMBER_OF_TESTS];
uint64_t ntt_calls[NUMBER_OF_TESTS];
uint64_t intt_t[NUMBER_OF_TESTS];
uint64_t intt_calls[NUMBER_OF_TESTS];

#define RNG_SEED_LENGTH 32
DRNG_ctx drng_algorithm;

extern int32_t rrlwr_pke_zetas[RRLWR_N];

static int cmp_uint64_local(const void *a, const void *b)
{
  uint64_t av = *(const uint64_t *)a;
  uint64_t bv = *(const uint64_t *)b;

  if (av < bv) return -1;
  if (av > bv) return 1;
  return 0;
}

static uint64_t average_local(const uint64_t *x, size_t n)
{
  uint64_t acc = 0;

  for (size_t i = 0; i < n; ++i) {
    acc += x[i];
  }

  return acc / n;
}

static uint64_t median_local(const uint64_t *x, size_t n)
{
  uint64_t tmp[NUMBER_OF_TESTS];

  memcpy(tmp, x, n * sizeof(tmp[0]));
  qsort(tmp, n, sizeof(tmp[0]), cmp_uint64_local);

  if (n & 1) {
    return tmp[n / 2];
  }

  return (tmp[n / 2 - 1] + tmp[n / 2]) / 2;
}

static void print_stage_results(const char *label,
                                const uint64_t *stage_cycles,
                                const uint64_t *ntt_cycles,
                                const uint64_t *ntt_calls_in,
                                const uint64_t *intt_cycles,
                                const uint64_t *intt_calls_in,
                                size_t n)
{
  uint64_t stage_med = median_local(stage_cycles, n);
  uint64_t stage_avg = average_local(stage_cycles, n);

  printf("%s\n", label);
  printf("median: %llu cycles/ticks\n", (unsigned long long)stage_med);
  printf("average: %llu cycles/ticks\n", (unsigned long long)stage_avg);

#ifdef RRLWR_MEASURE_NTT_CYCLES
  uint64_t ntt_med = median_local(ntt_cycles, n);
  uint64_t ntt_avg = average_local(ntt_cycles, n);
  uint64_t ntt_calls_med = median_local(ntt_calls_in, n);
  uint64_t ntt_calls_avg = average_local(ntt_calls_in, n);
  uint64_t intt_med = median_local(intt_cycles, n);
  uint64_t intt_avg = average_local(intt_cycles, n);
  uint64_t intt_calls_med = median_local(intt_calls_in, n);
  uint64_t intt_calls_avg = average_local(intt_calls_in, n);
  double ntt_med_pct = stage_med == 0 ? 0.0 : 100.0 * (double)ntt_med / (double)stage_med;
  double ntt_avg_pct = stage_avg == 0 ? 0.0 : 100.0 * (double)ntt_avg / (double)stage_avg;
  double intt_med_pct = stage_med == 0 ? 0.0 : 100.0 * (double)intt_med / (double)stage_med;
  double intt_avg_pct = stage_avg == 0 ? 0.0 : 100.0 * (double)intt_avg / (double)stage_avg;

  printf("ntt median: %llu cycles/ticks\n", (unsigned long long)ntt_med);
  printf("ntt average: %llu cycles/ticks\n", (unsigned long long)ntt_avg);
  printf("ntt calls median: %llu\n", (unsigned long long)ntt_calls_med);
  printf("ntt calls average: %llu\n", (unsigned long long)ntt_calls_avg);
  printf("ntt/stage median: %.2f%%\n", ntt_med_pct);
  printf("ntt/stage average: %.2f%%\n", ntt_avg_pct);
  printf("intt median: %llu cycles/ticks\n", (unsigned long long)intt_med);
  printf("intt average: %llu cycles/ticks\n", (unsigned long long)intt_avg);
  printf("intt calls median: %llu\n", (unsigned long long)intt_calls_med);
  printf("intt calls average: %llu\n", (unsigned long long)intt_calls_avg);
  printf("intt/stage median: %.2f%%\n", intt_med_pct);
  printf("intt/stage average: %.2f%%\n", intt_avg_pct);
#else
  (void)ntt_cycles;
  (void)ntt_calls_in;
  (void)intt_cycles;
  (void)intt_calls_in;
#endif
  printf("\n");
}

int main() {

  poly f, g, h;
  ring_element r, a, s;
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];
  unsigned char seedSp[RRLWR_SEED_S_LEN];
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN];
  unsigned char mp[RRLWR_PKE_MESSAGE_LEN];
  unsigned char kem_sk[RRLWR_KEM_SK_LEN];
  unsigned char kem_pk[RRLWR_KEM_PK_LEN];
  unsigned char kem_ct[RRLWR_KEM_CT_LEN];
  unsigned char ss[RRLWR_KEM_SS_LEN];
  unsigned long long length;

  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);
  uint64_t overhead = cpucycles_overhead();

  GENERATE_RANDOM_BYTES(seedA, RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS, RRLWR_SEED_S_LEN, &drng_algorithm);
  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    ring_uniform(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);
    ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN);
    t[i] = cpucycles();
  }
  print_results("sample: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    poly_ntt32(&f, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, rrlwr_pke_zetas);
    t[i] = cpucycles();
  }
  print_results("poly_ntt32: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    poly_invntt32(&f, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, rrlwr_pke_zetas);
    t[i] = cpucycles();
  }
  print_results("poly_invntt32: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    poly_basemul32(&h, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
    t[i] = cpucycles();
  }
  print_results("poly_basemul32: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    poly_add32(&h, &f, &g, RRLWR_PKE_PRIME);
    t[i] = cpucycles();
  }
  print_results("poly_add32: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    for(unsigned int j=0;j<RRLWR_N;j++) {
      h.coeffs[j] = f.coeffs[j] + g.coeffs[j];
    }
    t[i] = cpucycles();
  }
  print_results("poly_add: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    poly_sub32(&h, &f, &g, RRLWR_PKE_PRIME);
    t[i] = cpucycles();
  }
  print_results("poly_sub32: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    ring_mul32(r.x, &a, &s, RRLWR_K, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("ring_mul32 (full): ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    ring_mul32(r.x, &a, &s, 1, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("ring_mul32 (1 coefficient): ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    pke_keygen(pk, sk, seedA, seedS);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("pke_keygen: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    pke_encrypt(ct, pk, m, seedSp);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("pke_encrypt: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    pke_decrypt(mp, ct, sk);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("pke_decrypt: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    kem_keygen(kem_pk, &length, kem_sk, &length);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("kem_keygen: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    kem_enc(kem_pk, RRLWR_KEM_PK_LEN, ss, &length, kem_ct, &length);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("kem_encaps: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    uint64_t ntt_start = poly_ntt32_cycles_total();
    uint64_t calls_start = poly_ntt32_cycles_calls();
    uint64_t intt_start = poly_invntt32_cycles_total();
    uint64_t intt_calls_start = poly_invntt32_cycles_calls();
    kem_dec(kem_sk, RRLWR_KEM_SK_LEN, kem_ct, RRLWR_KEM_CT_LEN, ss, &length);
    t[i] = cpucycles() - start - overhead;
    ntt_t[i] = poly_ntt32_cycles_total() - ntt_start;
    ntt_calls[i] = poly_ntt32_cycles_calls() - calls_start;
    intt_t[i] = poly_invntt32_cycles_total() - intt_start;
    intt_calls[i] = poly_invntt32_cycles_calls() - intt_calls_start;
  }
  print_stage_results("kem_decaps: ", t, ntt_t, ntt_calls, intt_t, intt_calls, NUMBER_OF_TESTS);
}
