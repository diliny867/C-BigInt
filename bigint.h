#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define BIGINT_MAX_WORD_COUNT INT32_MAX

// marks when variable must be unique in terms of other inputs/outputs
#define UNIQUE(var) var

// functions dont reuse output for calculations, with it UNIQUE(var) dont matter
// #define BIGINT_NO_UNIQUE

// make capacity of backing buffer always power of 2
// #define BIGINT_POW2_GROW

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
    BI_HEX           = 1 << 0,
    BI_ADD0X         = 1 << 1,
    BI_LARGE_LETTERS = 1 << 2,
    BI_PAD_HEX       = 1 << 3,
    BI_BIF_START     = 1 << 4
} bigint_flag_e;


#define BIGINT_BASE (1llu << 63llu)

#define BIGINT_VALUE_MAX (BIGINT_BASE - 1llu)


#ifndef BIGINT_DEFAULT_INIT_WORD_COUNT
# define BIGINT_DEFAULT_INIT_WORD_COUNT 1
#endif


#ifndef BIGINT_NO_FRACTIONS

typedef struct {
    bigint_t numerator;
    bigint_t denominator;
} bigintf_t;

typedef enum {
    BIF_AS_DECIMAL = BI_BIF_START << 0,
} bigintf_flag_e;

#endif


#ifndef bigint_alloc
# define bigint_alloc(size) malloc((size))
#endif

#ifndef bigint_realloc
# define bigint_realloc(ptr, size) realloc((ptr), (size))
#endif

#ifndef bigint_free
# define bigint_free(ptr) free((ptr))
#endif


void bigint_init(bigint_t* num);
void bigint_init_n(bigint_t* num, bigint_size_t n);
void bigint_init_from(bigint_t* num, bigint_value_t* data, bigint_size_t size, bigint_size_t capacity); // scary
void bigint_init_from0(bigint_t* num, bigint_value_t* data, bigint_size_t size, bigint_size_t capacity, bool negative); // scary

void bigint_init_from_uint(bigint_t* out, bigint_value_t num);
void bigint_init_from_int(bigint_t* out, bigint_ivalue_t num);

void bigint_expand(bigint_t* num, bigint_size_t target);
void bigint_expand_pow2(bigint_t* num, bigint_size_t target);
void bigint_shrink(bigint_t* num);
void bigint_clear(bigint_t* num);
void bigint_destroy(bigint_t* num);

void bigint_copy(const bigint_t num, bigint_t* out);
void bigint_clone(const bigint_t num, bigint_t* out); // should only use with before unused bigints
void bigint_swap(bigint_t* num1, bigint_t* num2);

void bigint_add(const bigint_t num1, const bigint_t num2, bigint_t* out); // a + b
void bigint_sub(const bigint_t num1, const bigint_t num2, bigint_t* out); // a - b

void bigint_inc(const bigint_t num, bigint_t* out); // a + 1
void bigint_dec(const bigint_t num, bigint_t* out); // b - 1

void bigint_mul(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out));                      // a * b
int  bigint_div(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out), bigint_t* UNIQUE(r)); // a / b

int  bigint_mod(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)); // a % b
int  bigint_sqrt(const bigint_t num, bool ceil, bigint_t* UNIQUE(out));           // sqrt(a)
int  bigint_pow(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)); // a ^ b
int  bigint_fact(const bigint_t num, bigint_t* UNIQUE(out));                      // a!
void bigint_log2(const bigint_t num, bigint_t* out);                              // log2(a) and out is always in uint64, maybe output as it and not as bigint
int  bigint_gcd(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)); // gcd(a)

void bigint_lshift(const bigint_t num, bigint_value_t shift, bigint_t* out);  // a << b
void bigint_rshift(const bigint_t num, bigint_value_t shift, bigint_t* out);  // a >> b
void bigint_srshift(const bigint_t num, bigint_value_t shift, bigint_t* out); // a >> b but fills high bits with 1 if number is negative and msb is 1

bool bigint_lesser(const bigint_t num1, const bigint_t num2);  // a < b
bool bigint_greater(const bigint_t num1, const bigint_t num2); // a > b
bool bigint_eq(const bigint_t num1, const bigint_t num2);      // a == b
bool bigint_abseq(const bigint_t num1, const bigint_t num2);   // abs(a) == abs(b)
int  bigint_abscmp(const bigint_t num1, const bigint_t num2);  // cmp(abs(a), abs(b))
int  bigint_cmp(const bigint_t num1, const bigint_t num2);     // cmp(a, b)
#define bigint_is_zero(num) ((num).size == 0)                  // a == 0

bool bigint_eq_uint(const bigint_t num1, bigint_value_t num2);     // a == uint(b)
int  bigint_abscmp_uint(const bigint_t num1, bigint_value_t num2); // cmp(abs(a) == uint(b))
int  bigint_cmp_int(const bigint_t num1, bigint_ivalue_t num2);    // cmp(a, int(b))

void bigint_or(const bigint_t num1, const bigint_t num2, bigint_t* out);  // a | b
void bigint_and(const bigint_t num1, const bigint_t num2, bigint_t* out); // a & b
void bigint_xor(const bigint_t num1, const bigint_t num2, bigint_t* out); // a ^ b
void bigint_inv(const bigint_t num, bigint_t* out);                       // ~a

bool bigint_fits_int(const bigint_t num, bool is_signed);

bigint_value_t bigint_bit_length(const bigint_t num); // is also log2 of num
void bigint_setbit(bigint_value_t index, bigint_t* out);
void bigint_unsetbit(bigint_value_t index, bigint_t* out);
void bigint_togglebit(bigint_value_t index, bigint_t* out);

void bigint_from_uint(bigint_value_t num, bigint_t* out);
void bigint_from_int(bigint_ivalue_t num, bigint_t* out);

bigint_value_t bigint_to_uint(const bigint_t num);
bigint_ivalue_t bigint_to_int(const bigint_t num);
bigint_value_t bigint_to_uint_greedy(const bigint_t num);
bigint_ivalue_t bigint_to_int_greedy(const bigint_t num);

void bigint_from_string(char* str, bigint_t* out, int flag);

bigint_value_t bigint_to_string(const bigint_t num, char* out, bigint_value_t max_size, int flag);

bigint_value_t bigint_fprint(const bigint_t num, FILE* stream, int flag);
#define bigint_print(num, flag) bigint_fprint(num, stdout, flag)



#ifndef BIGINT_NO_FRACTIONS

void bigintf_init(bigintf_t* num);
void bigintf_clear(bigintf_t* num);
void bigintf_destroy(bigintf_t* num);

void bigintf_copy(const bigintf_t num, bigintf_t* out);
void bigintf_clone(const bigintf_t num, bigintf_t* out); // should only use with before unused bigints
void bigintf_swap(bigintf_t* num1, bigintf_t* num2);

void bigintf_simplify(bigintf_t* num);

#define bigintf_is_zero(num) ((num).numerator.size == 0)

int bigintf_abscmp(const bigintf_t num1, const bigintf_t num2);
int bigintf_cmp(const bigintf_t num1, const bigintf_t num2);
bool bigintf_abseq(const bigintf_t num1, const bigintf_t num2);

void bigintf_add(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));
void bigintf_sub(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));

void bigintf_mul(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));
int  bigintf_div(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));

int  bigintf_floor(const bigintf_t num, bigintf_t* UNIQUE(out));
int  bigintf_ceil(const bigintf_t num, bigintf_t* UNIQUE(out));
int  bigintf_round(const bigintf_t num, bigintf_t* UNIQUE(out));

void bigintf_from_bigint(const bigint_t num, bigintf_t* out);
void bigintf_from_bigints(const bigint_t num, bigint_t den, bigintf_t* out);

void bigintf_from_uint(const bigint_value_t num, const bigint_value_t den, bigintf_t* out);
void bigintf_from_int(const bigint_ivalue_t num, const bigint_ivalue_t den, bigintf_t* out);
void bigintf_from_f64(const double num, bigintf_t* out);

void bigintf_from_string(char* str1, char* str2, bigint_t* out1, bigint_t* out2, int flag);

double bigintf_to_f64(const bigintf_t num);
bigint_value_t bigintf_to_string(const bigintf_t num, char* out, bigint_value_t max_size, bigint_value_t fraction_max, int flag);

bigint_value_t bigintf_fprint(const bigintf_t num, FILE* stream, bigint_value_t fraction_max, int flag);
#define bigintf_print(num, fraction_max, flag) bigintf_fprint(num, stdout, fraction_max, flag)

#endif