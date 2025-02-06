#pragma once

#include <stdint.h>
#include <stdbool.h>


#define BIGINT_NONEGATIVE

//dont do this
//#define BIGINT_MANUALMANAGE

#ifndef BIGINT_MANUALMANAGE
	#define BIGINT_AUTOEXPAND
	#define BIGINT_AUTOSHRINK 
#endif

#ifdef BIGINT_NONEGATIVE
	#define BIGINT_MAX_WORD_COUNT UINT32_MAX
#else
	#define BIGINT_MAX_WORD_COUNT INT32_MAX
#endif

//#define BIGINT_WORD_COUNT 32

typedef struct {
	//uint64_t data[BIGINT_WORD_COUNT];
	uint64_t* data;
	uint32_t capacity;
#ifndef BIGINT_NONEGATIVE
	uint32_t size : 31;
	bool negative : 1;
#else
	uint32_t size;
#endif
} bigint_t;


#define BIGINT_FLAG_ADD0X			0x1
#define BIGINT_FLAG_LARGE_LETTERS	0x2


void bigint_init(bigint_t* num);
void bigint_init_0(bigint_t* num);
void bigint_init_n(bigint_t* num, uint32_t n);
void bigint_expand(bigint_t* num, uint32_t target);
void bigint_shrink(bigint_t* num);
void bigint_free(const bigint_t* num);

void bigint_add(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_sub(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_mul(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_div(const bigint_t* num1, const bigint_t* num2, bigint_t* out);

void bigint_copy(const bigint_t* num, bigint_t* out);

void bigint_lshift1(const bigint_t* num, bigint_t* out);
void bigint_rshift1(const bigint_t* num, bigint_t* out);
void bigint_lshift(const bigint_t* num1, uint64_t num2, bigint_t* out);
void bigint_rshift(const bigint_t* num1, uint64_t num2, bigint_t* out);

bool bigint_lesser(const bigint_t* num1, const bigint_t* num2);
bool bigint_greater(const bigint_t* num1, const bigint_t* num2);
bool bigint_eq(const bigint_t* num1, const bigint_t* num2);
int bigint_cmp(const bigint_t* num1, const bigint_t* num2);
bool bigint_is_zero(const bigint_t* num);

void bigint_or(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_and(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_xor(const bigint_t* num1, const bigint_t* num2, bigint_t* out);
void bigint_inv(const bigint_t* num, const bigint_t* out);
void bigint_neg(const bigint_t* num, const bigint_t* out);

void bigint_from_int(int64_t num, bigint_t* out);
//void bigint_from_string(char* str, bigint_t* out);
void bigint_from_xstring(char* str, bigint_t* out);
int64_t bigint_to_int(const bigint_t* num);
int64_t bigint_to_int_greedy(const bigint_t* num);
//int bigint_to_string(bigint_t* num, char* out, int max_size);
int bigint_to_xstring(const bigint_t* num, char* out, int max_size, int flag);

void bigint_print_hex(const bigint_t* num, int flag);
