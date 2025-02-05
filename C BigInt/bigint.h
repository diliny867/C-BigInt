#pragma once

#include <stdint.h>
#include <stdbool.h>


#define BIGINT_NEGATIVE
#define BIGINT_AUTOEXPAND

//#define BIGINT_WORD_COUNT 32

typedef struct {
	//uint64_t data[BIGINT_WORD_COUNT];
	uint64_t* data;
	uint32_t size;
	uint32_t capacity;
#ifdef BIGINT_NEGATIVE
	bool negative;
#endif
} bigint_t;


void bigint_init_default(bigint_t* num);
void bigint_init_0(bigint_t* num);
void bigint_init_n(bigint_t* num, uint32_t n);
void bigint_expand_to(bigint_t* num, uint32_t target);
void bigint_add(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_sub(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_mul(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_div(bigint_t* num1, bigint_t* num2, bigint_t* out);

void bigint_copy(bigint_t* num, bigint_t* out);

void bigint_lshift1(bigint_t* num, bigint_t* out);
void bigint_rshift1(bigint_t* num, bigint_t* out);
void bigint_lshift(bigint_t* num1, uint64_t num2, bigint_t* out);
void bigint_rshift(bigint_t* num1, uint64_t num2, bigint_t* out);

bool bigint_lesser(bigint_t* num1, bigint_t* num2);
bool bigint_greater(bigint_t* num1, bigint_t* num2);
bool bigint_eq(bigint_t* num1, bigint_t* num2);
int bigint_cmp(bigint_t* num1, bigint_t* num2);

void bigint_or(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_and(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_xor(bigint_t* num1, bigint_t* num2, bigint_t* out);
void bigint_inv(bigint_t* num, bigint_t* out);

void bigint_from_int(int64_t val, bigint_t* out);
//void bigint_from_string(char* str, bigint_t* out);
void bigint_from_hexstring(char* str, bigint_t* out);
uint64_t bigint_to_int(bigint_t* num);
uint64_t bigint_to_int_greedy(bigint_t* num);
//int bigint_to_string(bigint_t* num, char* out, int max_size);
int bigint_to_hexstring(bigint_t* num, char* out, int max_size);
