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

#include "pke.h"

int32_t rrlwr_pke_zetas[RRLWR_N] = RRLWR_KEM_ZETAS;
#ifdef PRECOMPUTE_TWIST
int32_t precomputed_twist[RRLWR_N] = {24818892, 34425332, 45709502, 13534722, 49392285, 9851939, 19576705, 39667519, 11536462, 47707762, 16425748, 42818476, 35425675, 23818549, 45698794, 13545430, 44334956, 14909268, -52524232, 4764550, 48273063, 10971161, 20115337, 39128887, 16548189, 42696035, 52319362, 6924862, 12952780, 46291444, 38088316, 21155908, -51278805, 3519123, 49971271, 9272953, 41199173, 18045051, 22956092, 36288132, 49795213, 9449011, 6289318, 52954906, 5090838, -52850520, 19066540, 40177684, 49275624, 9968600, 40036712, 19207512, 7710351, 51533873, 46889112, 12355112, 11722981, 47521243, 6834890, 52409334, 51555458, 7688766, 45492720, 13751504, 24078891, 35165333, 27070326, 32173898, 13216975, 46027249, 10686610, 48557614, 7826385, 51417839, 52154296, 7089928, 33920602, 25323622, 25641518, 33602706, 52970727, 6273497, 42527978, 16716246, 33374512, 25869712, 29216201, 30028023, 26741741, 32502483, -52248025, 4488343, -52811557, 5051875, 31860748, 27383476, 17246060, 41998164, 24560089, 34684135, 10604193, 48640031, 46226096, 13018128, 49833341, 9410883, 9021829, 50222395, 7531552, 51712672, 41463114, 17781110, 6843929, 52400295, 39142128, 20102096, 24599275, 34644949, 20606575, 38637649, 9231004, 50013220, 9750767, 49493457, 6147429, 53096795, 23706125, 35538099};
#endif

void poly_subp(poly *r, const poly *f, const poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    int32_t h2 = (int32_t)1 << (RRLWR_PKE_LOGP-(RRLWR_PKE_LOGT+1));      // p/(2*t)
    h2 -= (int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1));             // p/(2*t) - q/(2*p)
    r->coeffs[i] = (f->coeffs[i] - g->coeffs[i] + h2) & (RRLWR_PKE_P-1); // Reduce mod p
  }
}

void poly_add_msg(poly *r, const unsigned char msg[RRLWR_PKE_MESSAGE_LEN]) {
  int32_t mj;

  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      mj = (msg[i*RRLWR_N/8 + (j >> 3)] >> (j & 0x7)) & 1;                 // Select coefficient j of polynomial i
      r[i].coeffs[j] += (int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1)); // Add q/(2*p)
      r[i].coeffs[j] += -mj & ((int32_t)1 << (RRLWR_PKE_LOGP-1));          // Add p/2 if message bit is 1, otherwise not
      r[i].coeffs[j] &= (RRLWR_PKE_P-1);                                   // Reduce mod p
    }
  }
}

uint8_t ct_cmp(const unsigned char *c1, const unsigned char *c2) {
  uint8_t r = 0;
  for(unsigned int i = 0; i < RRLWR_KEM_CT_LEN; i++) {
    r |= (c1[i] ^ c2[i]); // Only non-zero if c1 != c2
  }

  return -((-(uint32_t)r) >> 31);
}

int pke_keygen(unsigned char pk[RRLWR_PKE_PK_LEN], unsigned char sk[RRLWR_PKE_SK_LEN],
               const unsigned char seedA[RRLWR_PKE_SEED_A_LEN], const unsigned char seedS[RRLWR_SEED_S_LEN]) {

  ring_element s, b;
  ring_element_Awin a;

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN,
                    RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                    RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                    rrlwr_pke_zetas);

  // Generate s with coefficients in [-2, 1] and pack
  ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN);
  ring_pack(sk, &s, RRLWR_PKE_LOG_ETA+1);

  // Compute b = round(p/q*A*s)
  ring_mul_Awin_round_xtoy_32(b.x, &a, &s, RRLWR_K, RRLWR_PKE_PRIME,
                              RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);

  // Pack the public key
  for (unsigned int i = 0; i < RRLWR_PKE_SEED_A_LEN; i++) {
    pk[i] = seedA[i];
  }
  ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP);

  return 0;
}

int pke_encrypt(unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char pk[RRLWR_PKE_PK_LEN],
                const unsigned char m[RRLWR_PKE_MESSAGE_LEN], const unsigned char seedSp[RRLWR_SEED_S_LEN]) {

  ring_element sp, bp;
  ring_element_Awin a, b;
  poly vp[RRLWR_PKE_ELL];
  const unsigned char *seedA = &pk[0];

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN,
                    RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                    RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                    rrlwr_pke_zetas);

  // Generate s_prime with coefficients in [-2, 1]
  ring_uniform(&sp, RRLWR_PKE_LOG_ETA+1, seedSp, RRLWR_SEED_S_LEN);

  // Compute b_prime = round(p/q*A*s_prime)
  ring_mul_Awin_round_xtoy_32(bp.x, &a, &sp, RRLWR_K, RRLWR_PKE_PRIME,
                              RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);

  // Compute v_prime = b*s_prime
  ring_unpack_Awin_ncoeffs(&b, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP,
                           RRLWR_PKE_ELL, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                           RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                           rrlwr_pke_zetas);
  ring_mul_Awin_invntt_reduce_pow2_32(vp, &b, &sp, RRLWR_PKE_ELL,
                                      RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                      RRLWR_NTTINV_FINALCONST,
                                      RRLWR_PKE_LOGP, rrlwr_pke_zetas); // Do not repeat NTT(s)

  // Convert message to polynomial representation and compute (v_prime + q/(2*p) + p/2*m) mod p
  poly_add_msg(vp, m);

  // Round from R_p to R_t with result in [-t/2+1, t/2] and pack into ciphertext buffer
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_compress(&vp[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT); // Multiply by t/p and floor
    poly_pack(ct + i*RRLWR_PKE_PACKED_POLYT_LEN, &vp[i], RRLWR_PKE_LOGT);
  }

  // Return ct = (cm = vp, b) and sk = s
  ring_pack(ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, &bp, RRLWR_PKE_LOGP);

  return 0;
}

int pke_decrypt(unsigned char m[RRLWR_PKE_MESSAGE_LEN], 
                const unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char sk[RRLWR_PKE_SK_LEN]) {
  ring_element bp, s; 
  ring_element_Awin bp_aw;
  poly v[RRLWR_PKE_ELL];
  poly cm[RRLWR_PKE_ELL];

  // Unpack s, b and cm and decompress cm
  ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA+1);
  ring_unpack(&bp, ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP);
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_unpack(&cm[i], ct + i*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGT);
    poly_decompress(&cm[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT); // Multiply by p/t
  }

  // Compute v = b_prime*s
  ring_to_Awin_ncoeffs(&bp_aw, &bp, RRLWR_PKE_ELL,
                       RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                       RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                       rrlwr_pke_zetas);
  ring_mul_Awin_reduce_pow2_32(v, &bp_aw, &s, RRLWR_PKE_ELL,
                               RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                               RRLWR_NTTINV_FINALCONST, RRLWR_PKE_LOGP,
                               rrlwr_pke_zetas);

  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_subp(&v[i], &v[i], &cm[i]);                     // Compute (v - (p/t)*cm) mod p
    poly_round_xtoy(&v[i], &v[i], RRLWR_PKE_LOGP, 1);    // Round to a single-bit message polynomial m'
    poly_pack(m + i*RRLWR_PKE_PACKED_POLY1_LEN, &v[i], 1); // Convert the message polynomial to a bit string
  }

  return 0;
}
