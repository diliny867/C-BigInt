#pragma once

#include <stdint.h>
#include <stdbool.h>


#define BIGINT_MAX_WORD_COUNT INT32_MAX

// marks when variable must be unique in terms of other inputs/outputs
#define UNIQUE(var) var

// functions dont reuse output for calculations, with it BIGINT_UNIQUE_OUT dont matter
// #define BIGINT_NO_UNIQUE

typedef uint64_t bigint_value_t;
typedef int64_t  bigint_ivalue_t;

typedef uint32_t  bigint_size_t;

typedef struct {
    bigint_value_t* data;
    bigint_size_t capacity;
    bigint_size_t size : 31;
    int negative : 1; // if bool, then it doesnt pack in 16 bytes
} bigint_t;


typedef enum {
    BIF_ADD0X         = 0x1,
    BIF_LARGE_LETTERS = 0x2,
    BIF_PAD_HEX       = 0x4,
    BIF_PRINT_COUNT   = 0x8
} bigint_flag_e;


void bigint_init(bigint_t* num);
void bigint_init_n(bigint_t* num, bigint_size_t n);
void bigint_init_from(bigint_t* num, bigint_value_t* data, bigint_size_t size, bigint_size_t capacity); // scary

void bigint_expand(bigint_t* num, bigint_size_t target);
void bigint_shrink(bigint_t* num);
void bigint_clear(bigint_t* num);
void bigint_destroy(bigint_t* num);

void bigint_add(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_sub(const bigint_t num1, const bigint_t num2, bigint_t* out);

void bigint_mul(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out));
int  bigint_div(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out), bigint_t* UNIQUE(r));

int  bigint_mod(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out));
int  bigint_sqrt(const bigint_t num, bigint_t* UNIQUE(out), bool ceil);
void bigint_pow(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out));

void bigint_copy(const bigint_t num, bigint_t* out);

void bigint_lshift(const bigint_t num, bigint_value_t shift, bigint_t* out);
void bigint_rshift(const bigint_t num, bigint_value_t shift, bigint_t* out);

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

bigint_value_t bigint_bit_length(const bigint_t num);
void bigint_setbit(bigint_t* out, bigint_value_t index);

void bigint_from_uint(uint64_t num, bigint_t* out);
void bigint_from_int(int64_t num, bigint_t* out);

bigint_value_t bigint_to_uint(const bigint_t num);
bigint_ivalue_t bigint_to_int(const bigint_t num);
bigint_value_t bigint_to_uint_greedy(const bigint_t num);
bigint_ivalue_t bigint_to_int_greedy(const bigint_t num);

void bigint_from_string(char* str, bigint_t* out);
void bigint_from_xstring(char* str, bigint_t* out);

bigint_value_t bigint_to_string(const bigint_t num, char* out, int max_size);
bigint_value_t bigint_to_xstring(const bigint_t num, char* out, int max_size, int flag);

bigint_value_t bigint_print(const bigint_t num, int flag);
bigint_value_t bigint_print_hex(const bigint_t num, int flag);
