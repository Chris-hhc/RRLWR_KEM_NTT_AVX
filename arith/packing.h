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

#ifndef PACKING_H
#define PACKING_H

#include "parameters.h"
#include "ring.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void poly_pack(unsigned char *b, poly *r, int32_t bitlen);
  void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen);
  void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen);
  void poly_unpack_2_avx2(poly *r, const uint8_t *b);
  void poly_unpack_13_avx2(poly *r, const uint8_t *b);
  void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen);
  void ring_unpack_avx2(ring_element *r, const uint8_t *b, int32_t bitlen);
  

#ifdef __cplusplus
}
#endif

#endif