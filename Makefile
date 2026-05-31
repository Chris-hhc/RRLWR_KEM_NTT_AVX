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

.PHONY: all test clean speed KAT breakdown

all: test speed KAT breakdown

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
  test/test_speed_KEM512

KAT: \
  KAT/KAT_KEM128 \
  KAT/KAT_KEM256 \
  KAT/KAT_KEM512

breakdown: \
  test/test_breakdown_KEM128 \
  test/test_breakdown_KEM256 \
  test/test_breakdown_KEM512

test/test_KEM128 test/test_KEM256 test/test_KEM512: test/test_KEM%: $(SOURCES) $(HEADERS) test/test_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* -DMEASURE_HEURISTICS $(CFLAGS) $(SOURCES) test/test_KEM.c -o $@

test/unit_tests_KEM128 test/unit_tests_KEM256 test/unit_tests_KEM512: test/unit_tests_KEM%: $(SOURCES) $(HEADERS) test/unit_tests_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) test/unit_tests_KEM.c -o $@

test/test_speed_KEM128 test/test_speed_KEM256 test/test_speed_KEM512: test/test_speed_KEM%: $(SOURCES) $(HEADERS) test/cpucycles.h test/cpucycles.c test/speed_print.c test/speed_print.h test/test_speed_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed_KEM.c -o $@

KAT/KAT_KEM128 KAT/KAT_KEM256 KAT/KAT_KEM512: KAT/KAT_KEM%: $(SOURCES) $(HEADERS) KAT/KAT_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE=\"KEM_RRLWR$*\" $(CFLAGS) $(SOURCES) KAT/KAT_KEM.c -o $@

test/test_breakdown_KEM128 test/test_breakdown_KEM256 test/test_breakdown_KEM512: test/test_breakdown_KEM%: $(SOURCES) $(HEADERS) pke_test.c pke_test.h kem_test.c kem_test.h test/cpucycles.h test/cpucycles.c test/test_breakdown_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) pke_test.c kem_test.c test/cpucycles.c test/test_breakdown_KEM.c -o $@

clean:
	-$(RM) -f test/test_KEM128 test/test_KEM256 test/test_KEM512
	-$(RM) -f test/unit_tests_KEM128 test/unit_tests_KEM256 test/unit_tests_KEM512
	-$(RM) -f test/test_speed_KEM128 test/test_speed_KEM256 test/test_speed_KEM512
	-$(RM) -f test/test_breakdown_KEM128 test/test_breakdown_KEM256 test/test_breakdown_KEM512
	-$(RM) -f KAT/KAT_KEM128 KAT/KAT_KEM256 KAT/KAT_KEM512
	-$(RM) -Rf output
