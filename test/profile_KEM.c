#include "pke.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <x86intrin.h>

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

#ifndef NTESTS
#define NTESTS 100000
#endif

#ifndef NWARMUP
#define NWARMUP 1000
#endif

typedef struct {
    const char *name;
    uint64_t cycles;
    uint64_t calls;
} prof_item;

static uint64_t g_prof_overhead = 0;
static volatile unsigned char g_sink = 0;

static inline uint64_t prof_now(void) {
    uint64_t result;

  __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");

  return result;
}

static uint64_t prof_calibrate_overhead(void) {
    uint64_t best = UINT64_MAX;
    for (unsigned int i = 0; i < 10000; i++) {
        uint64_t t0 = prof_now();
        uint64_t t1 = prof_now();
        uint64_t d = t1 - t0;
        if (d < best) {
            best = d;
        }
    }
    return best;
}

static inline void prof_add(prof_item *item, uint64_t delta) {
    if (delta > g_prof_overhead) {
        delta -= g_prof_overhead;
    } else {
        delta = 0;
    }
    item->cycles += delta;
    item->calls++;
}

#define PROF_ACC(ARR, IDX, CODE) do { \
    uint64_t _t0 = prof_now(); \
    CODE; \
    uint64_t _t1 = prof_now(); \
    prof_add(&(ARR)[IDX], _t1 - _t0); \
} while (0)

static void prof_reset(prof_item *items, size_t n) {
    for (size_t i = 0; i < n; i++) {
        items[i].cycles = 0;
        items[i].calls = 0;
    }
}

static void prof_print(const char *title, prof_item *items, size_t n, uint64_t iterations) {
    uint64_t total = 0;

    for (size_t i = 0; i < n; i++) {
        total += items[i].cycles;
    }

    printf("\n=== %s ===\n", title);
    printf("Iterations: %" PRIu64 "\n", iterations);
    printf("Total measured cycles: %" PRIu64 "\n", total);
    printf("Average measured cycles per iteration: %.2f\n", iterations ? (double)total / (double)iterations : 0.0);
    printf("\n%-42s %16s %16s %12s %16s\n", "Stage", "Total cycles", "Calls", "Percent", "Avg cycles");
    printf("%-42s %16s %16s %12s %16s\n", "-----", "------------", "-----", "-------", "----------");

    for (size_t i = 0; i < n; i++) {
        double percent = total ? 100.0 * (double)items[i].cycles / (double)total : 0.0;
        double avg = items[i].calls ? (double)items[i].cycles / (double)items[i].calls : 0.0;

        printf("%-42s %16" PRIu64 " %16" PRIu64 " %11.2f%% %16.2f\n",
               items[i].name,
               items[i].cycles,
               items[i].calls,
               percent,
               avg);
    }
}

enum {
    KG_RING_UNIFORM_A,
    KG_RING_UNIFORM_S,
    KG_RING_PACK_SK,
    KG_RING_MUL32_AS,
    KG_RING_ROUND_B,
    KG_COPY_SEEDA,
    KG_RING_PACK_PK,
    KG_COUNT
};

static prof_item kg_prof[KG_COUNT] = {
    [KG_RING_UNIFORM_A] = {"ring_uniform(a)", 0, 0},
    [KG_RING_UNIFORM_S] = {"ring_uniform(s)", 0, 0},
    [KG_RING_PACK_SK] = {"ring_pack(sk)", 0, 0},
    [KG_RING_MUL32_AS] = {"ring_mul32(A*s)", 0, 0},
    [KG_RING_ROUND_B] = {"ring_round_xtoy(b)", 0, 0},
    [KG_COPY_SEEDA] = {"copy seedA to pk", 0, 0},
    [KG_RING_PACK_PK] = {"ring_pack(pk)", 0, 0}
};

enum {
    ENC_RING_UNIFORM_A,
    ENC_RING_UNIFORM_SP,
    ENC_RING_MUL32_ASP,
    ENC_RING_ROUND_BP,
    ENC_RING_UNPACK_B,
    ENC_RING_NTT_B,
    ENC_RING_MUL_INVNTT_VP,
    ENC_POLY_ADD_MSG,
    ENC_POLY_COMPRESS,
    ENC_POLY_PACK_CM,
    ENC_RING_PACK_BP,
    ENC_COUNT
};

static prof_item enc_prof[ENC_COUNT] = {
    [ENC_RING_UNIFORM_A] = {"ring_uniform(a)", 0, 0},
    [ENC_RING_UNIFORM_SP] = {"ring_uniform(sp)", 0, 0},
    [ENC_RING_MUL32_ASP] = {"ring_mul32(A*sp)", 0, 0},
    [ENC_RING_ROUND_BP] = {"ring_round_xtoy(bp)", 0, 0},
    [ENC_RING_UNPACK_B] = {"ring_unpack(b)", 0, 0},
    [ENC_RING_NTT_B] = {"ring_ntt32(b)", 0, 0},
    [ENC_RING_MUL_INVNTT_VP] = {"ring_mul_invntt32(vp)", 0, 0},
    [ENC_POLY_ADD_MSG] = {"poly_add_msg(vp,m)", 0, 0},
    [ENC_POLY_COMPRESS] = {"poly_compress(vp[i])", 0, 0},
    [ENC_POLY_PACK_CM] = {"poly_pack(cm[i])", 0, 0},
    [ENC_RING_PACK_BP] = {"ring_pack(bp)", 0, 0}
};

enum {
    DEC_RING_UNPACK_S,
    DEC_RING_UNPACK_BP,
    DEC_POLY_UNPACK_CM,
    DEC_POLY_DECOMPRESS_CM,
    DEC_RING_MUL32_V,
    DEC_POLY_SUBP,
    DEC_POLY_ROUND_XTOY,
    DEC_POLY_PACK_M,
    DEC_COUNT
};

static prof_item dec_prof[DEC_COUNT] = {
    [DEC_RING_UNPACK_S] = {"ring_unpack(s)", 0, 0},
    [DEC_RING_UNPACK_BP] = {"ring_unpack(bp)", 0, 0},
    [DEC_POLY_UNPACK_CM] = {"poly_unpack(cm[i])", 0, 0},
    [DEC_POLY_DECOMPRESS_CM] = {"poly_decompress(cm[i])", 0, 0},
    [DEC_RING_MUL32_V] = {"ring_mul32(bp*s)", 0, 0},
    [DEC_POLY_SUBP] = {"poly_subp(v[i],cm[i])", 0, 0},
    [DEC_POLY_ROUND_XTOY] = {"poly_round_xtoy(v[i])", 0, 0},
    [DEC_POLY_PACK_M] = {"poly_pack(m[i])", 0, 0}
};

static int pke_keygen_prof(unsigned char pk[RRLWR_PKE_PK_LEN],
                    unsigned char sk[RRLWR_PKE_SK_LEN],
                    const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                    const unsigned char seedS[RRLWR_SEED_S_LEN]) {
    ring_element a, s, b;

    PROF_ACC(kg_prof, KG_RING_UNIFORM_A,
        ring_uniform(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN)
    );

    PROF_ACC(kg_prof, KG_RING_UNIFORM_S,
        ring_uniform(&s, RRLWR_PKE_LOG_ETA + 1, seedS, RRLWR_SEED_S_LEN)
    );

    PROF_ACC(kg_prof, KG_RING_PACK_SK,
        ring_pack(sk, &s, RRLWR_PKE_LOG_ETA + 1)
    );

    PROF_ACC(kg_prof, KG_RING_MUL32_AS,
        ring_mul32(b.x, &a, &s, RRLWR_K, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas)
    );

    PROF_ACC(kg_prof, KG_RING_ROUND_B,
        ring_round_xtoy(&b, &b, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP)
    );

    PROF_ACC(kg_prof, KG_COPY_SEEDA,
        for (unsigned int i = 0; i < RRLWR_PKE_SEED_A_LEN; i++) {
            pk[i] = seedA[i];
        }
    );

    PROF_ACC(kg_prof, KG_RING_PACK_PK,
        ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP)
    );

    return 0;
}

static int pke_encrypt_prof(unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char pk[RRLWR_PKE_PK_LEN],
                     const unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char seedSp[RRLWR_SEED_S_LEN]) {
    ring_element a, sp, b, bp;
    poly vp[RRLWR_PKE_ELL];
    const unsigned char *seedA = &pk[0];

    PROF_ACC(enc_prof, ENC_RING_UNIFORM_A,
        ring_uniform(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN)
    );

    PROF_ACC(enc_prof, ENC_RING_UNIFORM_SP,
        ring_uniform(&sp, RRLWR_PKE_LOG_ETA + 1, seedSp, RRLWR_SEED_S_LEN)
    );

    PROF_ACC(enc_prof, ENC_RING_MUL32_ASP,
        ring_mul32(bp.x, &a, &sp, RRLWR_K, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas)
    );

    PROF_ACC(enc_prof, ENC_RING_ROUND_BP,
        ring_round_xtoy(&bp, &bp, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP)
    );

    PROF_ACC(enc_prof, ENC_RING_UNPACK_B,
        ring_unpack(&b, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP)
    );

    PROF_ACC(enc_prof, ENC_RING_NTT_B,
        ring_ntt32(&b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, rrlwr_pke_zetas)
    );

    PROF_ACC(enc_prof, ENC_RING_MUL_INVNTT_VP,
        ring_mul_invntt32(vp, &b, &sp, RRLWR_PKE_ELL, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas)
    );

    PROF_ACC(enc_prof, ENC_POLY_ADD_MSG,
        poly_add_msg(vp, m)
    );

    for (unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
        PROF_ACC(enc_prof, ENC_POLY_COMPRESS,
            poly_compress(&vp[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT)
        );

        PROF_ACC(enc_prof, ENC_POLY_PACK_CM,
            poly_pack(ct + i * RRLWR_PKE_PACKED_POLYT_LEN, &vp[i], RRLWR_PKE_LOGT)
        );
    }

    PROF_ACC(enc_prof, ENC_RING_PACK_BP,
        ring_pack(ct + RRLWR_PKE_ELL * RRLWR_PKE_PACKED_POLYT_LEN, &bp, RRLWR_PKE_LOGP)
    );

    return 0;
}

static int pke_decrypt_prof(unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char sk[RRLWR_PKE_SK_LEN]) {
    ring_element bp, s;
    poly v[RRLWR_PKE_ELL];
    poly cm[RRLWR_PKE_ELL];

    PROF_ACC(dec_prof, DEC_RING_UNPACK_S,
        ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA + 1)
    );

    PROF_ACC(dec_prof, DEC_RING_UNPACK_BP,
        ring_unpack(&bp, ct + RRLWR_PKE_ELL * RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP)
    );

    for (unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
        PROF_ACC(dec_prof, DEC_POLY_UNPACK_CM,
            poly_unpack(&cm[i], ct + i * RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGT)
        );

        PROF_ACC(dec_prof, DEC_POLY_DECOMPRESS_CM,
            poly_decompress(&cm[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT)
        );
    }

    PROF_ACC(dec_prof, DEC_RING_MUL32_V,
        ring_mul32(v, &bp, &s, RRLWR_PKE_ELL, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas)
    );

    for (unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
        PROF_ACC(dec_prof, DEC_POLY_SUBP,
            poly_subp(&v[i], &v[i], &cm[i])
        );

        PROF_ACC(dec_prof, DEC_POLY_ROUND_XTOY,
            poly_round_xtoy(&v[i], &v[i], RRLWR_PKE_LOGP, 1)
        );

        PROF_ACC(dec_prof, DEC_POLY_PACK_M,
            poly_pack(m + i * RRLWR_PKE_PACKED_POLY1_LEN, &v[i], 1)
        );
    }

    return 0;
}

static void init_bytes(unsigned char *x, size_t n, unsigned int domain) {
    for (size_t i = 0; i < n; i++) {
        x[i] = (unsigned char)((i * 131u + domain * 17u + 7u) & 0xffu);
    }
}

int main(int argc, char **argv) {
    uint64_t iterations = NTESTS;

    if (argc >= 2) {
        char *end = NULL;
        unsigned long long v = strtoull(argv[1], &end, 10);
        if (end != argv[1] && v > 0) {
            iterations = (uint64_t)v;
        }
    }

    unsigned char pk[RRLWR_PKE_PK_LEN];
    unsigned char sk[RRLWR_PKE_SK_LEN];
    unsigned char ct[RRLWR_PKE_CT_LEN];
    unsigned char m[RRLWR_PKE_MESSAGE_LEN];
    unsigned char mout[RRLWR_PKE_MESSAGE_LEN];
    unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
    unsigned char seedS[RRLWR_SEED_S_LEN];
    unsigned char seedSp[RRLWR_SEED_S_LEN];

    init_bytes(pk, sizeof(pk), 1);
    init_bytes(sk, sizeof(sk), 2);
    init_bytes(ct, sizeof(ct), 3);
    init_bytes(m, sizeof(m), 4);
    init_bytes(mout, sizeof(mout), 5);
    init_bytes(seedA, sizeof(seedA), 6);
    init_bytes(seedS, sizeof(seedS), 7);
    init_bytes(seedSp, sizeof(seedSp), 8);

    g_prof_overhead = prof_calibrate_overhead();

    for (uint64_t i = 0; i < NWARMUP; i++) {
        pke_keygen_prof(pk, sk, seedA, seedS);
        pke_encrypt_prof(ct, pk, m, seedSp);
        pke_decrypt_prof(mout, ct, sk);
        g_sink ^= pk[0] ^ sk[0] ^ ct[0] ^ mout[0];
    }

    prof_reset(kg_prof, KG_COUNT);
    prof_reset(enc_prof, ENC_COUNT);
    prof_reset(dec_prof, DEC_COUNT);

    for (uint64_t i = 0; i < iterations; i++) {
        pke_keygen_prof(pk, sk, seedA, seedS);
        g_sink ^= pk[i % RRLWR_PKE_PK_LEN] ^ sk[i % RRLWR_PKE_SK_LEN];
    }

    prof_print("pke_keygen_prof breakdown", kg_prof, KG_COUNT, iterations);

    pke_keygen_prof(pk, sk, seedA, seedS);

    prof_reset(enc_prof, ENC_COUNT);

    for (uint64_t i = 0; i < iterations; i++) {
        pke_encrypt_prof(ct, pk, m, seedSp);
        g_sink ^= ct[i % RRLWR_PKE_CT_LEN];
    }

    prof_print("pke_encrypt_prof breakdown", enc_prof, ENC_COUNT, iterations);

    pke_encrypt_prof(ct, pk, m, seedSp);

    prof_reset(dec_prof, DEC_COUNT);

    for (uint64_t i = 0; i < iterations; i++) {
        pke_decrypt_prof(mout, ct, sk);
        g_sink ^= mout[i % RRLWR_PKE_MESSAGE_LEN];
    }

    prof_print("pke_decrypt_prof breakdown", dec_prof, DEC_COUNT, iterations);

    printf("\nMeasurement overhead: %" PRIu64 " cycles\n", g_prof_overhead);
    printf("Sink: %u\n", (unsigned int)g_sink);

    return 0;
}