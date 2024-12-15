#pragma once

#include <stdint.h>

#define BIGINT_WORD_COUNT 32

typedef struct {
	uint64_t data[BIGINT_WORD_COUNT];
} bigint_t;


void bigint_init(bigint_t* num);
void bigint_add(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_sub(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_mul(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_div(bigint_t* num1, bigint_t* num2, bigint_t* out);

void bigint_copy(bigint_t* num, bigint_t* out);

void bigint_lshift(bigint_t* num1, uint64_t num2, bigint_t* out);
void bigint_rshift(bigint_t* num1, uint64_t num2, bigint_t* out);
void bigint_or(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_and(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_xor(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_inv(bigint_t* num, bigint_t* out);
void bigint_neg(bigint_t* num, bigint_t* out);

void bigint_from_int(uint64_t val, bigint_t* out);
//void bigint_from_string(char* str, bigint_t* out);
void bigint_from_hexstring(char* str, bigint_t* out);
uint64_t bigint_to_int(bigint_t* num);
//int bigint_to_string(bigint_t* num, char* out, int max_size);
int bigint_to_hexstring(bigint_t* num, char* out, int max_size);
