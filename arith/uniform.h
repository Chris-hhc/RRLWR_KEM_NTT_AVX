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

#ifndef UNIFORM_H
#define UNIFORM_H

#include "parameters.h"
#include "fips202x4.h"
#include "fips202.h"
#include "ring.h"
#include "packing.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void ring_uniform(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len);
  void ring_uniform_Awin_base(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len);

#ifdef __cplusplus
}
#endif

#endif
