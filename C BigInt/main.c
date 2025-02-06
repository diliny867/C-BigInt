#include <stdlib.h>
#include <stdio.h>

#include <immintrin.h>
#include <intrin.h>
#include <intrin0.inl.h>
#include <stdint.h>

#include "bigint.h"

#define PRINTNUMS(num1, num2, num3, num4, num5)				\
	do {													\
		bigint_print_xstring(&num1, BIGINT_FLAG_ADD0X);		\
		printf("\n");										\
		bigint_print_xstring(&num2, BIGINT_FLAG_ADD0X);		\
		printf("\n");										\
		bigint_print_xstring(&num3, BIGINT_FLAG_ADD0X);		\
		printf("\n");										\
		bigint_print_xstring(&num4, BIGINT_FLAG_ADD0X);		\
		printf("\n");										\
		bigint_print_xstring(&num5, BIGINT_FLAG_ADD0X);		\
		printf("\n");										\
	} while(0)

#define PRINTNUMS_ALL() do { printf("\n"); PRINTNUMS(num1, num2, num3, num4, num5); }while(0)


int main(int argc, char** argv) {
	//uint32_t a = INT32_MAX - 11;
	//uint32_t b = 10;
	//uint32_t res1;
	//uint32_t res2;
	//res2 = _mulx_u32(a, b, &res1);
	//printf("a:%u b:%u res1:%u res2:%u *:%llu res:%llu\n", a, b, res1, res2, (uint64_t)a*(uint64_t)b, ((uint64_t)res1<<32) + (uint64_t)res2);

	//uint32_t a = 44;
	//uint32_t b = 10;
	//uint32_t c;
	//c = _addcarry_u32(1, a, b, &b);
	//printf("%u %u", a, b);

	//uint32_t a = 1u << 30;
	//uint32_t b = 2;
	//uint32_t res1;
	//uint32_t res2;
	//res1 = _shlx_u32(a, b);
	//printf("%d\n", res1);

	//printf("%u", _lzcnt_u32(13));

	bigint_t num1, num2, num3, num4, num5;
	bigint_init_default(&num1);
	bigint_init_default(&num2);
	bigint_init_default(&num3);
	bigint_init_default(&num4);
	bigint_init_default(&num5);

	bigint_from_int(0x61287568192, &num1);
	bigint_from_int(0x1270214, &num2);

	PRINTNUMS_ALL();

	bigint_add(&num1, &num2, &num3);

	PRINTNUMS_ALL();

	bigint_mul(&num3, &num2, &num4);

	PRINTNUMS_ALL();

	bigint_mul(&num4, &num3, &num5);

	PRINTNUMS_ALL();

	return 0;
}