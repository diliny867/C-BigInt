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
    BI_ADD0X         = 1 << 0,
    BI_LARGE_LETTERS = 1 << 1,
    BI_PAD_HEX       = 1 << 2,
    BI_PRINT_COUNT   = 1 << 3,
    BI_BIF_START     = 1 << 4
} bigint_flag_e;


#ifndef BIGINT_NO_FRACTIONS

typedef struct {
    bigint_t numerator;
    bigint_t denominator;
} bigintf_t;

typedef enum {
    BIF_AS_DECIMAL = BI_BIF_START,
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
void bigint_log2(const bigint_t num, bigint_t* out); // out is always in uint64, maybe output as it and not as bigint
int bigint_gcd(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out));

void bigint_copy(const bigint_t num, bigint_t* out);

void bigint_lshift(const bigint_t num, bigint_value_t shift, bigint_t* out);
void bigint_rshift(const bigint_t num, bigint_value_t shift, bigint_t* out);

bool bigint_lesser(const bigint_t num1, const bigint_t num2);
bool bigint_greater(const bigint_t num1, const bigint_t num2);
bool bigint_eq(const bigint_t num1, const bigint_t num2);
int bigint_abscmp(const bigint_t num1, const bigint_t num2);
int bigint_cmp(const bigint_t num1, const bigint_t num2);
//bool bigint_is_zero(const bigint_t num);
#define bigint_is_zero(num) (num.size == 0)

bool bigint_eq_uint(const bigint_t num1, bigint_value_t num2);
bool bigint_abscmp_uint(const bigint_t num1, bigint_value_t num2);

void bigint_or(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_and(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_xor(const bigint_t num1, const bigint_t num2, bigint_t* out);
void bigint_inv(const bigint_t num, bigint_t* out);

bool bigint_fits_int(const bigint_t num, bool is_signed);

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

bigint_value_t bigint_to_string(const bigint_t num, char* out, bigint_value_t max_size);
bigint_value_t bigint_to_xstring(const bigint_t num, char* out, bigint_value_t max_size, int flag);

bigint_value_t bigint_print(const bigint_t num, int flag);
bigint_value_t bigint_print_hex(const bigint_t num, int flag);



#ifndef BIGINT_NO_FRACTIONS

// for from double conversion
#include <float.h>

#define DOUBLE_DIGITS DBL_DIG
#define DOUBLE_BUF_SIZE (308 + DOUBLE_DIGITS + 3)


void bigintf_init(bigintf_t* num);
void bigintf_clear(bigintf_t* num);
void bigintf_destroy(bigintf_t* num);

void bigintf_copy(bigintf_t* num, bigintf_t* out);

void bigintf_simplify(bigintf_t* num);

bool bigintf_is_zero(bigintf_t* num);

int bigintf_abscmp(const bigintf_t num1, const bigintf_t num2);
int bigintf_cmp(const bigintf_t num1, const bigintf_t num2);
bool bigintf_abseq(const bigintf_t num1, const bigintf_t num2);

void bigintf_add(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));
void bigintf_sub(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));

void bigintf_mul(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));
int  bigintf_div(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out));

void bigintf_from_bigint(bigint_t num, bigintf_t* out);
void bigintf_from_bigints(bigint_t num, bigint_t den, bigintf_t* out);

void bigintf_from_uint(uint64_t num, uint64_t den, bigintf_t* out);
void bigintf_from_int(uint64_t num, uint64_t den,bigintf_t* out);
void bigintf_from_f64(double num,bigintf_t* out);

double bigintf_to_f64(const bigintf_t num);
bigint_value_t bigintf_to_string(const bigintf_t num, char* out, bigint_value_t max_size, bigint_value_t fraction_max, int flag);

bigint_value_t bigintf_print(const bigintf_t num, bigint_value_t fraction_max, int flag);

#endif