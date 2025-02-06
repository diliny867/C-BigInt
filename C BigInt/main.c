#include <stdlib.h>
#include <stdio.h>

#include "bigint.h"

#define PRINTNUMS(num1, num2, num3, num4, num5, num6)		\
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
		bigint_print_hex(&num6, BIGINT_FLAG_ADD0X);			\
		printf("\n");										\
	} while(0)

#define PRINTNUMS_ALL() do { printf("\n"); PRINTNUMS(num1, num2, num3, num4, num5, num6); }while(0)


int main(int argc, char** argv) {
	bigint_t num1, num2, num3, num4, num5, num6;
	bigint_init(&num1);
	bigint_init(&num2);
	bigint_init(&num3);
	bigint_init(&num4);
	bigint_init(&num5);
	bigint_init(&num6);

	bigint_from_int(0x61287c568192, &num1);
	bigint_from_int(0x12e70219, &num2);
	PRINTNUMS_ALL();

	bigint_add(&num1, &num2, &num3);
	PRINTNUMS_ALL();

	bigint_mul(&num3, &num2, &num4);
	PRINTNUMS_ALL();

	bigint_mul(&num4, &num4, &num5);
	bigint_mul(&num5, &num5, &num4);
	PRINTNUMS_ALL();

	bigint_sub(&num2, &num5, &num6);
	PRINTNUMS_ALL();

	//bigint_sub(&num6, &num6, &num6);
	//PRINTNUMS_ALL();

	bigint_mul(&num4, &num4, &num1);
	PRINTNUMS_ALL();
	bigint_mul(&num1, &num1, &num4);
	PRINTNUMS_ALL();
	bigint_mul(&num4, &num4, &num1);
	PRINTNUMS_ALL();
	bigint_mul(&num1, &num1, &num4);
	PRINTNUMS_ALL();


	bigint_expand(&num2, 4);
	num2.size = 4;
	for(uint32_t i = 0; i < num2.size; i++) {
		num2.data[i] = 0;
	}
	num2.data[1] = 1;
	num2.data[2] = 1;
	num2.data[num2.size - 1] = 1;
	bigint_from_int(0x1, &num3);
	PRINTNUMS_ALL();

	bigint_sub(&num2, &num3, &num4);
	PRINTNUMS_ALL();

	bigint_add(&num4, &num3, &num4);
	PRINTNUMS_ALL();

	bigint_add(&num4, &num4, &num4);
	PRINTNUMS_ALL();

	bigint_sub(&num4, &num4, &num4);
	PRINTNUMS_ALL();

	//bigint_mul(&num1, &num1, &num5);
	//PRINTNUMS_ALL();
	//bigint_mul(&num5, &num5, &num1);
	//PRINTNUMS_ALL();
	//bigint_mul(&num1, &num1, &num5);
	//PRINTNUMS_ALL();
	//bigint_mul(&num5, &num5, &num1);
	//PRINTNUMS_ALL();
	//bigint_mul(&num1, &num1, &num5);
	//PRINTNUMS_ALL();

	bigint_free(&num1);
	bigint_free(&num2);
	bigint_free(&num3);
	bigint_free(&num4);
	bigint_free(&num5);
	bigint_free(&num6);

	return 0;
}