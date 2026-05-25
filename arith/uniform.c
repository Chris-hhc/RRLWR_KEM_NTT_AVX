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

#ifndef RRLWR_UNIFORM_USE_AVX2
#define RRLWR_UNIFORM_USE_AVX2 1
#endif

#include "uniform.h"

#include <stddef.h>
#include <string.h>

#define RRLWR_MAX_SEED_LEN        (128) // Support seed lengths up to 128 bytes, only required to define buffer size
#define RRLWR_MAX_OUTLEN          ((RRLWR_PKE_LOGQ * RRLWR_N / 4 * RRLWR_K) / 8)
#define RRLWR_XOF_BUFLEN          (((RRLWR_MAX_OUTLEN + SHAKE128_RATE - 1) / SHAKE128_RATE) * SHAKE128_RATE)

static void set_seed_nonce(uint8_t *out,
                           const uint8_t *seed, int32_t seed_len,
                           uint8_t nonce)
{
  memcpy(out, seed, (size_t)seed_len);
  out[seed_len] = nonce;
}

#if !RRLWR_UNIFORM_USE_AVX2
static void squeeze_seed_nonce(uint8_t *out, size_t nblocks,
                               const uint8_t *seed, int32_t seed_len,
                               uint8_t nonce)
{
  uint8_t seed_buffer[RRLWR_MAX_SEED_LEN + 1];
  keccak_state state;

  set_seed_nonce(seed_buffer, seed, seed_len, nonce);
  shake128_absorb_once(&state, seed_buffer, (size_t)seed_len + 1);
  shake128_squeezeblocks(out, nblocks, &state);
}
#endif

static void poly_uniform_kx(poly **r, unsigned int npolys,
                            int32_t bitlen, const uint8_t *seed, int32_t seed_len,
                            uint8_t nonce0, uint8_t nonce1,
                            uint8_t nonce2, uint8_t nonce3)
{
  size_t outlen = (size_t)bitlen * (RRLWR_N >> 3);
  size_t nblocks = (((outlen >> 2) * npolys) + SHAKE128_RATE - 1) / SHAKE128_RATE;
  size_t lane_len = nblocks * SHAKE128_RATE;
  uint8_t buf[4 * RRLWR_XOF_BUFLEN];

#if RRLWR_UNIFORM_USE_AVX2
  uint8_t in0[RRLWR_MAX_SEED_LEN + 1], in1[RRLWR_MAX_SEED_LEN + 1];
  uint8_t in2[RRLWR_MAX_SEED_LEN + 1], in3[RRLWR_MAX_SEED_LEN + 1];

  set_seed_nonce(in0, seed, seed_len, nonce0);
  set_seed_nonce(in1, seed, seed_len, nonce1);
  set_seed_nonce(in2, seed, seed_len, nonce2);
  set_seed_nonce(in3, seed, seed_len, nonce3);

  shake128x4(buf + 0 * lane_len,
             buf + 1 * lane_len,
             buf + 2 * lane_len,
             buf + 3 * lane_len,
             lane_len,
             in0, in1, in2, in3,
             (size_t)seed_len + 1);
#else
  squeeze_seed_nonce(buf + 0 * lane_len, nblocks, seed, seed_len, nonce0);
  squeeze_seed_nonce(buf + 1 * lane_len, nblocks, seed, seed_len, nonce1);
  squeeze_seed_nonce(buf + 2 * lane_len, nblocks, seed, seed_len, nonce2);
  squeeze_seed_nonce(buf + 3 * lane_len, nblocks, seed, seed_len, nonce3);
#endif

  for(unsigned int i = 0; i < npolys; i++) {
    poly_unpack(r[i], buf + i * outlen, bitlen);
  }
}

static void ring_uniform_order(poly **r, unsigned int npolys,
                               int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  poly_uniform_kx(r, npolys, bitlen, seed, seed_len, 0, 1, 2, 3);
}

/// @brief Generate a ring element with coefficients pseudo-randomly generated in the uniform distribution [-bitlen/2, bitlen/2-1]
void ring_uniform(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  poly *polys[RRLWR_K];

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    polys[i] = &r->x[i];
  }

  ring_uniform_order(polys, RRLWR_K, bitlen, seed, seed_len);
}

void ring_uniform_Awin_base(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  poly *polys[RRLWR_K];

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    polys[i] = &aw->x[RRLWR_K - 1 - i];
  }

  ring_uniform_order(polys, RRLWR_K, bitlen, seed, seed_len);
}
