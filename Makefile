CC = gcc
CFLAGS += -O3
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -fomit-frame-pointer -mavx2 -mbmi2 -mpopcnt \
  -march=native -mtune=native -z noexecstack

RM = /bin/rm

BASE_FOLDER = .
ARITH_FOLDER = ./arith
UTILS_FOLDER = ./utils
KECCAK4X_FOLDER = $(UTILS_FOLDER)/keccak4x

INCLUDES = -I$(BASE_FOLDER) -I$(UTILS_FOLDER) -I$(KECCAK4X_FOLDER) -I$(ARITH_FOLDER)

SOURCES_RRLWR = $(ARITH_FOLDER)/fprime.c $(ARITH_FOLDER)/poly.c $(ARITH_FOLDER)/poly.S $(ARITH_FOLDER)/ring.c $(ARITH_FOLDER)/packing.c $(ARITH_FOLDER)/packing.S $(ARITH_FOLDER)/uniform.c $(ARITH_FOLDER)/ntt.S $(ARITH_FOLDER)/intt.S $(ARITH_FOLDER)/pointwise.S pke.c kem.c
SOURCES_UTILS = $(UTILS_FOLDER)/drng.c $(UTILS_FOLDER)/fips202.c $(UTILS_FOLDER)/fips202x4.c $(KECCAK4X_FOLDER)/KeccakP-1600-times4-SIMD256.c
SOURCES = $(SOURCES_RRLWR) $(SOURCES_UTILS)

HEADERS_RRLWR = $(ARITH_FOLDER)/fprime.h $(ARITH_FOLDER)/poly.h $(ARITH_FOLDER)/ring.h $(ARITH_FOLDER)/packing.h $(ARITH_FOLDER)/uniform.h $(ARITH_FOLDER)/ntt.h $(ARITH_FOLDER)/ntt_param.h $(ARITH_FOLDER)/intt_param.h $(ARITH_FOLDER)/ntt_avx_macros.inc parameters.h pke.h kem.h
HEADERS_UTILS = $(UTILS_FOLDER)/drng.h $(UTILS_FOLDER)/fips202.h $(UTILS_FOLDER)/fips202x4.h $(KECCAK4X_FOLDER)/KeccakP-1600-times4-SnP.h $(KECCAK4X_FOLDER)/KeccakP-align.h $(KECCAK4X_FOLDER)/KeccakP-brg_endian.h $(KECCAK4X_FOLDER)/KeccakP-SIMD256-config.h
HEADERS = $(HEADERS_RRLWR) $(HEADERS_UTILS)

SOURCES_PROFILE = \
  $(ARITH_FOLDER)/fprime.c \
  $(ARITH_FOLDER)/poly.c \
  $(ARITH_FOLDER)/poly.S \
  $(ARITH_FOLDER)/ring.c \
  $(ARITH_FOLDER)/packing.c \
  $(ARITH_FOLDER)/packing.S \
  $(ARITH_FOLDER)/uniform.c \
  $(ARITH_FOLDER)/ntt.S \
  $(ARITH_FOLDER)/intt.S \
  $(ARITH_FOLDER)/pointwise.S \
  $(SOURCES_UTILS)

.PHONY: all test clean speed KAT ref ref_test ref_speed ref_KAT avxReal refReal

all: test speed KAT

test: \
  test/test_KEM128 \
  test/test_KEM256 \
  test/test_KEM512 \
  test/unit_tests_KEM128 \
  test/unit_tests_KEM256 \
  test/unit_tests_KEM512

speed: \
  test/test_speed_KEM128 \
  test/test_speed_KEM256 \
  test/test_speed_KEM512 \
  test/profile_KEM128 \
  test/profile_KEM256 \
  test/profile_KEM512

KAT: \
  KAT/KAT_KEM128 \
  KAT/KAT_KEM256 \
  KAT/KAT_KEM512

ref: ref_test ref_speed ref_KAT

ref_test: \
  test/test_KEM128_ref \
  test/test_KEM256_ref \
  test/test_KEM512_ref \
  test/unit_tests_KEM128_ref \
  test/unit_tests_KEM256_ref \
  test/unit_tests_KEM512_ref

ref_speed: \
  test/test_speed_KEM128_ref \
  test/test_speed_KEM256_ref \
  test/test_speed_KEM512_ref \
  test/profile_KEM128_ref \
  test/profile_KEM256_ref \
  test/profile_KEM512_ref

ref_KAT: \
  KAT/KAT_KEM128_ref \
  KAT/KAT_KEM256_ref \
  KAT/KAT_KEM512_ref

avxReal: \
  test/test_KEM128 \
  test/test_KEM256 \
  test/test_KEM512 \
  test/unit_tests_KEM128 \
  test/unit_tests_KEM256 \
  test/unit_tests_KEM512 \
  test/test_speed_KEM128_avxReal \
  test/test_speed_KEM256_avxReal \
  test/test_speed_KEM512_avxReal \
  test/profile_KEM128_avxReal \
  test/profile_KEM256_avxReal \
  test/profile_KEM512_avxReal \
  KAT/KAT_KEM128 \
  KAT/KAT_KEM256 \
  KAT/KAT_KEM512

refReal: \
  test/test_KEM128_refReal \
  test/test_KEM256_refReal \
  test/test_KEM512_refReal \
  test/unit_tests_KEM128_refReal \
  test/unit_tests_KEM256_refReal \
  test/unit_tests_KEM512_refReal \
  test/test_speed_KEM128_refReal \
  test/test_speed_KEM256_refReal \
  test/test_speed_KEM512_refReal \
  test/profile_KEM128_refReal \
  test/profile_KEM256_refReal \
  test/profile_KEM512_refReal \
  KAT/KAT_KEM128_refReal \
  KAT/KAT_KEM256_refReal \
  KAT/KAT_KEM512_refReal

define KEM_RULES
test/test_KEM128$(1): $$(SOURCES) $$(HEADERS) test/test_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=128 $(2) -DMEASURE_HEURISTICS $$(CFLAGS) $$(SOURCES) test/test_KEM.c -o $$@

test/test_KEM256$(1): $$(SOURCES) $$(HEADERS) test/test_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=256 $(2) -DMEASURE_HEURISTICS $$(CFLAGS) $$(SOURCES) test/test_KEM.c -o $$@

test/test_KEM512$(1): $$(SOURCES) $$(HEADERS) test/test_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=512 $(2) -DMEASURE_HEURISTICS $$(CFLAGS) $$(SOURCES) test/test_KEM.c -o $$@

test/unit_tests_KEM128$(1): $$(SOURCES) $$(HEADERS) test/unit_tests_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=128 $(2) $$(CFLAGS) $$(SOURCES) test/unit_tests_KEM.c -o $$@

test/unit_tests_KEM256$(1): $$(SOURCES) $$(HEADERS) test/unit_tests_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=256 $(2) $$(CFLAGS) $$(SOURCES) test/unit_tests_KEM.c -o $$@

test/unit_tests_KEM512$(1): $$(SOURCES) $$(HEADERS) test/unit_tests_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=512 $(2) $$(CFLAGS) $$(SOURCES) test/unit_tests_KEM.c -o $$@

test/test_speed_KEM128$(1): $$(SOURCES) $$(HEADERS) test/cpucycles.h test/cpucycles.c test/speed_print.c test/speed_print.h test/test_speed_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=128 $(2) $(3) $$(CFLAGS) $$(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed_KEM.c -o $$@

test/test_speed_KEM256$(1): $$(SOURCES) $$(HEADERS) test/cpucycles.h test/cpucycles.c test/speed_print.c test/speed_print.h test/test_speed_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=256 $(2) $(3) $$(CFLAGS) $$(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed_KEM.c -o $$@

test/test_speed_KEM512$(1): $$(SOURCES) $$(HEADERS) test/cpucycles.h test/cpucycles.c test/speed_print.c test/speed_print.h test/test_speed_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=512 $(2) $(3) $$(CFLAGS) $$(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed_KEM.c -o $$@

KAT/KAT_KEM128$(1): $$(SOURCES) $$(HEADERS) KAT/KAT_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=128 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE=\"KEM_RRLWR128\" $(2) $$(CFLAGS) $$(SOURCES) KAT/KAT_KEM.c -o $$@

KAT/KAT_KEM256$(1): $$(SOURCES) $$(HEADERS) KAT/KAT_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=256 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE=\"KEM_RRLWR256\" $(2) $$(CFLAGS) $$(SOURCES) KAT/KAT_KEM.c -o $$@

KAT/KAT_KEM512$(1): $$(SOURCES) $$(HEADERS) KAT/KAT_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=512 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE=\"KEM_RRLWR512\" $(2) $$(CFLAGS) $$(SOURCES) KAT/KAT_KEM.c -o $$@

test/profile_KEM128$(1): $$(SOURCES_PROFILE) $$(HEADERS) test/profile_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=128 $(2) $$(CFLAGS) $$(SOURCES_PROFILE) test/profile_KEM.c -o $$@

test/profile_KEM256$(1): $$(SOURCES_PROFILE) $$(HEADERS) test/profile_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=256 $(2) $$(CFLAGS) $$(SOURCES_PROFILE) test/profile_KEM.c -o $$@

test/profile_KEM512$(1): $$(SOURCES_PROFILE) $$(HEADERS) test/profile_KEM.c
	$$(CC) $$(INCLUDES) -DRRLWR_SECURITY_LEVEL=512 $(2) $$(CFLAGS) $$(SOURCES_PROFILE) test/profile_KEM.c -o $$@
endef

$(eval $(call KEM_RULES,,,-DRRLWR_MEASURE_NTT_CYCLES))
$(eval $(call KEM_RULES,_ref,-DRRLWR_DISABLE_NTT_AVX,-DRRLWR_MEASURE_NTT_CYCLES))
$(eval $(call KEM_RULES,_avxReal,,))
$(eval $(call KEM_RULES,_refReal,-DRRLWR_DISABLE_NTT_AVX,))

clean:
	-$(RM) -f test/test_KEM128 test/test_KEM256 test/test_KEM512
	-$(RM) -f test/test_KEM128_ref test/test_KEM256_ref test/test_KEM512_ref
	-$(RM) -f test/test_KEM128_refReal test/test_KEM256_refReal test/test_KEM512_refReal
	-$(RM) -f test/unit_tests_KEM128 test/unit_tests_KEM256 test/unit_tests_KEM512
	-$(RM) -f test/unit_tests_KEM128_ref test/unit_tests_KEM256_ref test/unit_tests_KEM512_ref
	-$(RM) -f test/unit_tests_KEM128_refReal test/unit_tests_KEM256_refReal test/unit_tests_KEM512_refReal
	-$(RM) -f test/test_speed_KEM128 test/test_speed_KEM256 test/test_speed_KEM512
	-$(RM) -f test/test_speed_KEM128_ref test/test_speed_KEM256_ref test/test_speed_KEM512_ref
	-$(RM) -f test/test_speed_KEM128_avxReal test/test_speed_KEM256_avxReal test/test_speed_KEM512_avxReal
	-$(RM) -f test/test_speed_KEM128_refReal test/test_speed_KEM256_refReal test/test_speed_KEM512_refReal
	-$(RM) -f test/profile_KEM128 test/profile_KEM256 test/profile_KEM512
	-$(RM) -f test/profile_KEM128_ref test/profile_KEM256_ref test/profile_KEM512_ref
	-$(RM) -f test/profile_KEM128_avxReal test/profile_KEM256_avxReal test/profile_KEM512_avxReal
	-$(RM) -f test/profile_KEM128_refReal test/profile_KEM256_refReal test/profile_KEM512_refReal
	-$(RM) -f KAT/KAT_KEM128 KAT/KAT_KEM256 KAT/KAT_KEM512
	-$(RM) -f KAT/KAT_KEM128_ref KAT/KAT_KEM256_ref KAT/KAT_KEM512_ref
	-$(RM) -f KAT/KAT_KEM128_refReal KAT/KAT_KEM256_refReal KAT/KAT_KEM512_refReal
	-$(RM) -Rf output
