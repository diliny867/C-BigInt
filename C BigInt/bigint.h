#pragma once

#include <stdint.h>
#include <stdbool.h>


#define BIGINT_MAX_WORD_COUNT INT32_MAX

typedef uint64_t bigint_value_t;
typedef int64_t  bigint_ivalue_t;

typedef uint32_t  bigint_size_t;

typedef struct {
    bigint_value_t* data;
    bigint_size_t capacity;
    bigint_size_t size : 31;
    bool negative : 1;
} bigint_t;

extern bigint_t bigint_dummy;


#define BIGINT_FLAG_ADD0X           0x1
#define BIGINT_FLAG_LARGE_LETTERS   0x2
#define BIGINT_FLAG_PAD_HEX         0x4


void bigint_init(bigint_t* num);
void bigint_init_n(bigint_t* num, bigint_size_t n);
void bigint_init_from(bigint_t* num, bigint_value_t* data, bigint_size_t size, bigint_size_t capacity); // scary

void bigint_expand(bigint_t* num, bigint_size_t target);
void bigint_shrink(bigint_t* num);
void bigint_clear(bigint_t* num);
void bigint_destroy(bigint_t* num);

void bigint_add(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_sub(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_mul(const bigint_t num1, const bigint_t num2, bigint_t* out);
int  bigint_div(const bigint_t num1, const bigint_t num2, bigint_t* out, bigint_t* r);
int  bigint_mod(const bigint_t num1, const bigint_t num2, bigint_t* out);

void bigint_copy(const bigint_t num, bigint_t* out);

void bigint_lshift1(const bigint_t num, bigint_t* out);
void bigint_rshift1(const bigint_t num, bigint_t* out);
void bigint_lshift(const bigint_t num1, bigint_value_t num2, bigint_t* out);
void bigint_rshift(const bigint_t num1, bigint_value_t num2, bigint_t* out);

bool bigint_lesser(const bigint_t num1, const bigint_t num2);
bool bigint_greater(const bigint_t num1, const bigint_t num2);
bool bigint_eq(const bigint_t num1, const bigint_t num2);
int bigint_abscmp(const bigint_t num1, const bigint_t num2);
int bigint_cmp(const bigint_t num1, const bigint_t num2);
bool bigint_is_zero(const bigint_t num);

void bigint_or(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_and(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_xor(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_inv(const bigint_t num, bigint_t* out);

void bigint_from_uint(uint64_t num, bigint_t* out);
void bigint_from_int(int64_t num, bigint_t* out);

bigint_value_t bigint_to_uint(const bigint_t num);
bigint_ivalue_t bigint_to_int(const bigint_t num);
bigint_value_t bigint_to_uint_greedy(const bigint_t num);
bigint_ivalue_t bigint_to_int_greedy(const bigint_t num);

void bigint_from_string(char* str, bigint_t* out);
void bigint_from_xstring(char* str, bigint_t* out);

int bigint_to_string(const bigint_t num, char* out, int max_size);
int bigint_to_xstring(const bigint_t num, char* out, int max_size, int flag);

int bigint_print(const bigint_t num, int flag);
int bigint_print_hex(const bigint_t num, int flag);
