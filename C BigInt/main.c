#include <stdlib.h>
#include <stdio.h>

#include "bigint.h"

#define PRINTNUMS(num1, num2, num3, num4, num5)				\
	do {													\
		bigint_print_hex(&num1, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
		bigint_print_hex(&num2, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
		bigint_print_hex(&num3, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
		bigint_print_hex(&num4, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
		bigint_print_hex(&num5, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
	} while(0)

#define PRINTNUMS_ALL() do { printf("\n"); PRINTNUMS(num1, num2, num3, num4, num5); }while(0)


int main(int argc, char** argv) {
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

	bigint_mul(&num4, &num4, &num5);

	PRINTNUMS_ALL();

	return 0;
}