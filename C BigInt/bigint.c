#include "bigint.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define BIGINT_DEFAULT_INIT_WORD_COUNT 1


#define BIGINT_HALFBASE (1ull << (sizeof(bigint_value_t) * 8 - 2))


bigint_t bigint_dummy = {0};

static const bigint_t bigint_zero = {0, 0, 0, 0};
static const bigint_value_t one___ = 1;
static const bigint_t bigint_one = {&one___, 1, 1, 0};

#define subborrow(borrow, num1, num2, out) _subborrow_u64((borrow), (num1), (num2), (out))
#define addcarry(carry, num1, num2, out)   _addcarry_u64((carry), (num1), (num2), (out))

#define umul(num1, num2, high)          _umul128((num1), (num2), (high))
#define udiv(num1_lo, num1_hi, num2, r) _udiv128((num1_lo), (num1_hi), (num2), (r))

#define msb_zeros(val) __lzcnt64(val)

#ifndef bigint_alloc
# define bigint_alloc(size) malloc((size))
#endif

#ifndef bigint_realloc
# define bigint_realloc(ptr, size) realloc((ptr), (size))
#endif

#ifndef bigint_free
# define bigint_free(ptr) free((ptr))
#endif

#define arr_zero(arr, size) memset(arr, 0, size)

#define arr_reverse(T, arr, size)                \
    do {                                         \
        T tmp;                                   \
        for(size_t i = 0; i < (size) >> 1; i++){ \
            tmp = (arr)[i];                      \
            (arr)[i] = (arr)[(size) - 1 - i];    \
            (arr)[(size) - 1 - i] = tmp;         \
        }                                        \
    } while(0)

// no need for  (size) > 0x80000000 ? 0xFFFFFFFF : (1u << (32u - _lzcnt_u32((size) - 1)))
// because _lzcnt_u32 cant return 0 because size is 31 bit, so atleast 1
//#define calc_capacity(size) (1u << (32u - _lzcnt_u32((size) - 1)))

#define calc_capacity(size) (size)

static inline void bigint_zero_data(bigint_value_t* data, bigint_size_t size) {
    arr_zero(data, size * sizeof(bigint_value_t));
    //arr_zero(num->data, num->capacity * sizeof(bigint_value_t));
}
void bigint_expand(bigint_t* num, bigint_size_t target) {
    if(num->size < target) {
        if(num->capacity < target){
            num->capacity = calc_capacity(target);
            num->data = bigint_realloc(num->data, num->capacity * sizeof(bigint_value_t));
        }
        bigint_zero_data(num->data + num->size, num->capacity - num->size); // always zero added
    }
}

static bigint_size_t bigint_shrink_(bigint_value_t* data, bigint_size_t size) {
    // while most significant digits are 0
    while(size > 0 && !data[size - 1]) {
        size--;
    }
    return size;
}
void bigint_shrink(bigint_t* num) {
    num->size = bigint_shrink_(num->data, num->size);
}

void bigint_clear(bigint_t* num) {
    //bigint_zero_data(num);
    num->size = 0;
    num->negative = false;
}
void bigint_destroy(bigint_t* num) {
    if(num->data)
        bigint_free(num->data);
    num->data = 0;
    num->capacity = 0;
    num->size = 0;
    num->negative = 0;
}

void bigint_init_n(bigint_t* num, bigint_size_t n) {
    num->data = bigint_alloc(n * sizeof(bigint_value_t));
    num->size = 0;
    num->capacity = calc_capacity(n);
    bigint_zero_data(num->data, num->size);
    num->negative = false;
    //arr_zero(num->data, sizeof(bigint_value_t) * n);
}
void bigint_init(bigint_t* num) {
    bigint_init_n(num, BIGINT_DEFAULT_INIT_WORD_COUNT);
}
void bigint_init_from(bigint_t* num, bigint_value_t* data, bigint_size_t size, bigint_size_t capacity){
    num->data = data;
    num->size = size;
    num->capacity = capacity;
    num->negative = false;
}


static inline int bigint_abscmp_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2) {
    if(size1 < size2) {
        return -1;
    }else if(size1 == size2) {
        bigint_size_t i = size1;
        while(i--) {
            if(data1[i] != data2[i]) {
                return data1[i] < data2[i] ? -1 : 1;
            }
        }
        return 0;
    }else {
        return 1;
    }
}

static inline void bigint_copy_(bigint_value_t* data, bigint_size_t size, bigint_value_t* data_out) {
    memmove(data_out, data, size * sizeof(bigint_value_t));
}
void bigint_copy(const bigint_t num, bigint_t* out) {
    bigint_expand(out, num.size);
    out->size = num.size;
    bigint_copy_(num.data, num.size, out->data);
}

static bigint_size_t bigint_lshift_(bigint_value_t* data, bigint_size_t size, bigint_value_t num, bigint_value_t* data_out) {
    bigint_size_t offset = (bigint_size_t)num >> 6; // / 64
    bigint_size_t shift_val = num & 63; // % 64
    bigint_size_t inv_shift_val = 64 - shift_val;
    data_out[size + offset] = 0;

    for(bigint_size_t i = 0; i < size; i++) {
        bigint_size_t index = size - i - 1;
        data_out[index + 1 + offset] |= data[index] >> inv_shift_val;
        data_out[index + offset]      = data[index] << shift_val;
    }
    return bigint_shrink_(data_out, size + offset + 1);
}
void bigint_lshift(const bigint_t num1, bigint_value_t num2, bigint_t* out) {
    bigint_size_t offset = (bigint_size_t)num2 >> 6; // / 64
    bigint_expand(out, num1.size + 1 + offset);
    out->size = bigint_lshift_(num1.data, num1.size, num2, out->data);
}

static bigint_size_t bigint_rshift_(bigint_value_t* data, bigint_size_t size, bigint_value_t num, bigint_value_t* data_out) {
    bigint_size_t offset = (bigint_size_t)num >> 6; // / 64
    bigint_size_t shift_val = num & 63;
    bigint_size_t inv_shift_val = 64 - shift_val;
    if(size <= offset) {
        data_out[0] = 0;
        return 1;
    }

    data_out[0] = data[0] >> shift_val;
    for(bigint_size_t i = 1; i < size - offset; i++) {
        data_out[i - 1] |= data[i + offset] << inv_shift_val;
        data_out[i]      = data[i + offset] >> shift_val;
    }

    return bigint_shrink_(data_out, size - offset);
}
void bigint_rshift(const bigint_t num1, bigint_value_t num2, bigint_t* out) {
    bigint_size_t offset = (bigint_size_t)num2 >> 6; // / 64
    bigint_expand(out, num1.size - offset);
    out->size = bigint_rshift_(num1.data, num1.size, num2, out->data);
}



static bigint_size_t bigint_add_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2, bigint_value_t* data_out) { //ignores negatives
    bigint_size_t i;
    uint8_t carry = 0;
    bigint_size_t min_size = min(size1, size2);
    for(i = 0; i < min_size; i++) {
        carry = addcarry(carry, data1[i], data2[i], data_out + i);
    }

    while(i < size1 && carry) {
        carry = ((data_out[i] = data1[i] + 1) == 0);
        i++;
    }
    while(i < size1) {
        data_out[i] = data1[i];
        i++;
    }

    while(i < size2 && carry) {
        carry = ((data_out[i] = data2[i] + 1) == 0);
        i++;
    }
    while(i < size2) {
        data_out[i] = data2[i];
        i++;
    }

    if(carry) {
        data_out[i] = 1;
        i++;
    }

    return i;
}

static bigint_size_t bigint_sub_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2, bigint_value_t* data_out) { //ignores negatives; num1 >= num2
    bigint_size_t i;
    uint8_t borrow = 0;
    bigint_size_t min_size = min(size1, size2);
    for(i = 0; i < min_size; i++) {
        borrow = subborrow(borrow, data1[i], data2[i], data_out + i);
    }

    while(i < size1 && borrow) {
        borrow = ((data_out[i] = data1[i] - 1) == 0xFFFFFFFFFFFFFFFF);
        i++;
    }

    bigint_size_t size_out = i;

    while(i < size1) {
        data_out[i] = data1[i];
        if(data_out[i]) {
            size_out = i + 1;
        }
        i++;
    }

    if(size_out && !data_out[size_out - 1]) {
        size_out -= 1;
    }
    return size_out;
}


void bigint_add(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    if(!out) { // cant do the thing
        return;
    }

    bigint_expand(out, max(num1.size, num2.size) + 1);
    if(num1.negative != num2.negative) {
        if(bigint_lesser(num1, num2)) {
            out->size = bigint_sub_(num2.data, num2.size, num1.data, num1.size, out->data);
            out->negative = num2.negative;
        }else {
            out->size = bigint_sub_(num1.data, num1.size, num2.data, num2.size, out->data);
            out->negative = num1.negative;
        }
    }else {
        out->size = bigint_add_(num1.data, num1.size, num2.data, num2.size, out->data);
        out->negative = num1.negative && num2.negative;
    }
    bigint_shrink(out);
}

void bigint_sub(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    if(!out) { // cant do the thing
        return;
    }

    bigint_expand(out, max(num1.size, num2.size));
    if(num1.negative != num2.negative) {
        out->size = bigint_add_(num1.data, num1.size, num2.data, num2.size, out->data);
        out->negative = num1.negative;
    }else {
        if(bigint_lesser(num1, num2)) {
            out->size = bigint_sub_(num2.data, num2.size, num1.data, num1.size, out->data);
            out->negative = !(num1.negative && num2.negative);
        }else {
            out->size = bigint_sub_(num1.data, num1.size, num2.data, num2.size, out->data);
            out->negative = num1.negative && num2.negative;
        }
    }
    bigint_shrink(out);
}


// https://www.codeproject.com/Articles/1276310/Multiple-Precision-Arithmetic-1st-Multiplication-Algorithm
static bigint_size_t bigint_mul_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2, bigint_value_t* data_out) { //ignores negatives
    arr_zero(data_out, (size1 + size2) * sizeof(bigint_value_t));

    bigint_value_t high, low;
    bigint_size_t k;
    bigint_size_t size = 0;
    uint8_t carry;
    for(bigint_size_t j = 0; j < size2; j++) {
        for(bigint_size_t i = 0; i < size1; i++) {
            k = i + j;
            low = umul(data1[i], data2[j], &high);
            carry = addcarry(0, data_out[k], low, data_out + k);
            if(carry || high){
                k++;
                carry = addcarry(carry, data_out[k], high, data_out + k);
                while(carry) {
                    carry = ((data_out[++k] += 1) == 0);
                }
            }
            size = max(size, k);
        }
    }
    return size + 1;
}

void bigint_mul(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    assert(num1.data != out->data && num2.data != out->data);

    if(!out) { // cant do the thing
        return;
    }

    bigint_expand(out, num1.size + num2.size);

    out->size = bigint_mul_(num1.data, num1.size, num2.data, num2.size, out->data);

    out->negative = num1.negative != num2.negative;

    bigint_shrink(out);
}


// https://www.codeproject.com/Articles/1276311/Multiple-Precision-Arithmetic-Division-Algorithm
static void bigint_div_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2, 
                        bigint_value_t* data_q, bigint_size_t* size_q, bigint_value_t* data_r, bigint_size_t* size_r, bigint_value_t* data_tmp) { // assumes num1 > num2
    bigint_value_t qi, ai_left;
    bigint_value_t b_left = data2[size2 - 1];

    // can use num1 and num2 and bitshift back at the end, but they wont be const anymore
    bigint_value_t* data_a = data_tmp + size1 + 1;
    bigint_value_t* data_b = data_tmp + size1 + 1 + size1 + 1;
    bigint_copy_(data1, size1, data_a);
    bigint_copy_(data2, size2, data_b);

    bigint_size_t size_tmp, size_a, size_b;

    int shift = msb_zeros(b_left); // msb bits
    size_a = bigint_lshift_(data_a, size1, shift, data_a);
    size_b = bigint_lshift_(data_b, size2, shift, data_b);
    size_tmp = size1 + 1;

    b_left = data_b[size_b - 1];

    *size_q = 0;

    bigint_size_t ai_start = size_a - size_b; // from where to take for ai

    if(bigint_abscmp_(data_a + ai_start, size_b, data_b, size_b) < 0) { // need one more for ai
        ai_start--;
    }

    *size_r = size_a - ai_start;
    bigint_copy_(data_a + ai_start, *size_r, data_r); // ai will be in r

    while(1){
        ai_left = data_r[*size_r - 1];
        int cmp = bigint_abscmp_(data_r, *size_r, data_b, size_b);
        if(cmp < 0){
            qi = 0;
        }else { // same or more digits
            if(ai_left == b_left){
                if(*size_r == size_b){ // same digits
                    qi = 1;
                }else { // more digits
                    qi = (bigint_value_t)-1;
                }
            }else if(ai_left > b_left){ // same digits 
                qi = 1; // actually ai_left / b_left, but b_left >= base/2, so always 1
            }else { // more digits
                bigint_value_t _r;
                qi = udiv(data_r[*size_r - 1], data_r[*size_r - 2], b_left, &_r); // a_guess / b_left
            }
        }

        bigint_zero_data(data_tmp, size_tmp);
        if(qi){
            size_tmp = bigint_mul_(data_b, size_b, &qi, 1, data_tmp); // remultiply to check if correct
            size_tmp = bigint_shrink_(data_tmp, size_tmp);

            cmp = bigint_abscmp_(data_tmp, size_tmp, data_r, *size_r);

            int count = 0;
            while(cmp > 0){
                // we overshoot for 1 to 3 iterations
                size_tmp = bigint_sub_(data_tmp, size_tmp, data_b, size_b, data_tmp);
                qi--;
                cmp = bigint_abscmp_(data_tmp, size_tmp, data_r, *size_r);

                assert(count++ < 3);
            }
        }

        data_q[(*size_q)++] = qi; // append in reverse order, to avoid shifting values

        *size_r = bigint_sub_(data_r, *size_r, data_tmp, size_tmp, data_r); // calculate remainder

        if(ai_start == 0){ // if no more in a
            break;
        }

        // shift data one left and get next from a
        bigint_copy_(data_r, (*size_r)++, data_r + 1);
        data_r[0] = data_a[--ai_start];

        *size_r = bigint_shrink_(data_r, *size_r);
    }

    arr_reverse(bigint_value_t, data_q, *size_q); // reverse

    *size_r = bigint_rshift_(data_r, *size_r, shift, data_r); // reverse initial shift
}

int bigint_div(const bigint_t num1, const bigint_t num2, bigint_t* out, bigint_t* r) {
    // these must be different memory
    assert(num1.data != out->data && num2.data != out->data && 
           num1.data != r->data && num2.data != r->data &&
           out->data != r->data);

    if(!out || !r) { // cant do the thing
        return 2;
    }

    bool right_zero = bigint_is_zero(num2);
    if(right_zero) { // division by 0
        return 1;
    }

    int cmp = bigint_cmp(num1, num2);
    if(cmp < 0) { // left is less than right
        out->size = 0;
        out->negative = false;

        bigint_copy(num1, r);

        return 0;
    }else if(cmp == 0) {
        bigint_expand(out, 1);

        out->data[0] = 1;
        out->size = 1;
        out->negative = num1.negative != num2.negative;

        bigint_clear(r);

        return 0;
    }

    bigint_expand(out, num1.size + 1);
    bigint_expand(r, num2.size + 1);

    arr_zero(out->data, out->size * sizeof(bigint_value_t));

    bigint_value_t* tmp = bigint_alloc((num1.size + 1 + num1.size + 1 + num2.size + 1) * sizeof(bigint_value_t));

    bigint_size_t qs, rs;
    bigint_div_(num1.data, num1.size, num2.data, num2.size, out->data, &qs, r->data, &rs, tmp);
    out->size = qs;
    r->size = rs;

    bigint_free(tmp);

    out->negative = num1.negative != num2.negative;

    bigint_shrink(out);
    bigint_shrink(r);

    return 0;
}


int bigint_mod(const bigint_t num1, const bigint_t num2, bigint_t* out) { // x - (x / y) * y
    if(!out) { // cant do the thing
        return 2;
    }

    bigint_t tmp;
    bigint_init_n(&tmp, num1.size + 1);

    int ret = bigint_div(num1, num2, &tmp, out);

    bigint_destroy(&tmp);
    
    return ret;
}

bigint_value_t bigint_bit_length(const bigint_t num) {
    if(num.size == 0) {
        return 0;
    }
    return num.size * 64llu + 64 - msb_zeros(num.data[num.size - 1]);
}

void bigint_setbit(bigint_t* out, bigint_value_t index) {
    bigint_size_t size = (bigint_size_t)(index / 64llu + 1llu);
    bigint_expand(out, size);
    out->data[size - 1] |= 1 << (index & 63);
    out->size = size;
}

// https://stackoverflow.com/questions/4407839/how-can-i-find-the-square-root-of-a-java-biginteger
int bigint_sqrt(const bigint_t num, bigint_t* out, bool ceil) {
    if(!out) { // cant do the thing
        return 2;
    }

    int cmp = bigint_cmp(num, bigint_zero);
    if(cmp < 0) {
        return 1;
    }else if(cmp == 0) {
        out->size = 0;
        out->negative = false;
        return 0;
    }

    bigint_expand(out, num.size);

    if(num.size == 1) {
        out->size = 1;
        out->data[0] = sqrt(num.data[0]);
        out->negative = false;
        return 0;
    }

    int ret = 0;
    bigint_t tmp1, tmp2, tmp3;
    bigint_init(&tmp1);
    bigint_init(&tmp2);
    bigint_init(&tmp3);

    bigint_rshift(num, 1, &tmp1);

    while(true) {
        ret = bigint_div(num, tmp1, &tmp2, &tmp3);
        if(ret) { // how
            break;
        }

        if(!bigint_greater(tmp1, tmp2)) {

            if(ceil){
                bigint_mul(tmp1, tmp1, &tmp2);
                if(bigint_lesser(tmp2, num)) {
                    bigint_add(tmp1, bigint_one, &tmp1);
                }
            }

            bigint_copy(tmp1, out);
            break;
        }

        bigint_add(tmp2, tmp1, &tmp1);
        bigint_rshift(tmp1, 1, &tmp1);
    }


    bigint_destroy(&tmp1);
    bigint_destroy(&tmp2);
    bigint_destroy(&tmp3);

    return ret;
}


inline bool bigint_lesser(const bigint_t num1, const bigint_t num2) {
    if(num1.negative != num2.negative) {
        return num1.negative;
    }

    bool negative = num1.negative && num2.negative;

    if(num1.size < num2.size) {
        return !negative;
    }
    if(num1.size == num2.size) {
        for(bigint_size_t i = 1; i <= num1.size; i++) {
            if(num1.data[num1.size - i] < num2.data[num1.size - i]) {
                return !negative;
            }else if(num1.data[num1.size - i] > num2.data[num1.size - i]) {
                return negative;
            }
        }
    }
    return negative;
}
inline bool bigint_greater(const bigint_t num1, const bigint_t num2) {
    if(num1.negative != num2.negative) {
        return num2.negative;
    }

    bool negative = num1.negative && num2.negative;

    if(num1.size > num2.size) {
        return !negative;
    }
    if(num1.size == num2.size) {
        for(bigint_size_t i = 1; i <= num1.size; i++) {
            if(num1.data[num1.size - i] > num2.data[num1.size - i]) {
                return !negative;
            }else if(num1.data[num1.size - i] < num2.data[num1.size - i]) {
                return negative;
            }
        }
    }
    return negative;
}
inline bool bigint_eq(const bigint_t num1, const bigint_t num2) {
    if(num1.negative != num2.negative) {
        return false;
    }

    if(num1.size == num2.size) {
        for(bigint_size_t i = 1; i <= num1.size; i++) {
            if(num1.data[num1.size - i] != num2.data[num1.size - i]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

int bigint_abscmp(const bigint_t num1, const bigint_t num2) {
    return bigint_abscmp_(num1.data, num1.size, num2.data, num2.size);
}
inline int bigint_cmp(const bigint_t num1, const bigint_t num2) {
    if(num1.negative != num2.negative) {
        return num1.negative ? -1 : 1;
    }
    int cmp = bigint_abscmp(num1, num2);
    return num1.negative ? -cmp : cmp;
}

inline bool bigint_is_zero(const bigint_t num) {
    return num.size == 0;
    // return num.size == 0 || (num.size == 1 && num.data[0] == 0);
}

#define copy_choose(num1, num2, out, i, max_size)            \
    memmove(out->data + i,                                   \
        (num1.size > num2.size ? num1.data : num2.data) + i, \
        (max_size - i) * sizeof(bigint_value_t))

void bigint_or(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    bigint_size_t min_size = min(num1.size, num2.size);
    bigint_size_t max_size = max(num1.size, num2.size);
    bigint_expand(out, max_size);
    bigint_size_t i = 0;
    for(; i < min_size; i++) {
        out->data[i] = num1.data[i] | num2.data[i];
    }
    copy_choose(num1, num2, out, i, max_size);
}
void bigint_and(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    bigint_size_t min_size = min(num1.size, num2.size);
    bigint_size_t max_size = max(num1.size, num2.size);
    bigint_expand(out, max_size);
    bigint_size_t i = 0;
    for(; i < min_size; i++) {
        out->data[i] = num1.data[i] & num2.data[i];
    }
    copy_choose(num1, num2, out, i, max_size);
}
void bigint_xor(const bigint_t num1, const bigint_t num2, bigint_t* out) {
    bigint_size_t min_size = min(num1.size, num2.size);
    bigint_size_t max_size = max(num1.size, num2.size);
    bigint_expand(out, max_size);
    bigint_size_t i = 0;
    for(; i < min_size; i++) {
        out->data[i] = num1.data[i] ^ num2.data[i];
    }
    copy_choose(num1, num2, out, i, max_size);
}

#undef copy_choose

void bigint_inv(const bigint_t num, bigint_t* out) {
    bigint_expand(out, num.size);
    for(bigint_size_t i = 0; i < num.size; i++) {
        out->data[i] = ~num.data[i];
    }
}

static inline int hexchar_to_int(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f') {
        return c - 'a';
    }
    if(c >= 'A' && c <= 'F') {
        return c - 'A';
    }
    return 0;
}

void bigint_from_uint(uint64_t num, bigint_t* out) {
    bigint_expand(out, 1);
    out->size = 1;

    out->data[0] = num;
    out->negative = false;

    bigint_shrink(out);
}
void bigint_from_int(int64_t num, bigint_t* out) {
    bigint_expand(out, 1);
    out->size = 1;

    if(num < 0) {
        out->data[0] = (uint64_t)(-num);
        out->negative = true;
    }else {
        out->data[0] = num;
        out->negative = false;
    }

    bigint_shrink(out);
}

bigint_value_t bigint_to_uint(const bigint_t num) {
    return num.data[0];
}
bigint_ivalue_t bigint_to_int(const bigint_t num) {
    if(num.negative) {
        return -((bigint_ivalue_t)num.data[0]);
    }
    return (bigint_ivalue_t)num.data[0];
}

bigint_value_t bigint_to_uint_greedy(bigint_t num) {
    if(num.size > 1) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return bigint_to_uint(num);
}
bigint_ivalue_t bigint_to_int_greedy(bigint_t num) {
    if(num.size > 1) {
        if(num.negative) {
            return 0xFFFFFFFFFFFFFFFF;
        }
        // 0x7FFFFFFFFFFFFFFF because that way it is maximal positive integer
        return 0x7FFFFFFFFFFFFFFF; // 0xFFFFFFFFFFFFFFFF
    }
    return bigint_to_int(num);
}


static bigint_size_t calc_x_len(char* str, int (*proc)(int)) {
    char* start = str;
    while(proc(*str))
        str++;
    return (bigint_size_t)(str - start);
}

void bigint_from_string(char* str, bigint_t* out) {
    if(*str == '-') {
        out->negative = true;
        str++;
    }

    bigint_size_t dec_len = calc_x_len(str, isdigit); // allocate hex size just to be sure
    if(dec_len == 0) {
        bigint_from_int(0, out);
        return;
    }
    dec_len = (dec_len - 1u) / 18u + 1u;
    bigint_expand(out, dec_len);

    bigint_value_t ten = 10, num_data = 0;
    bigint_t d, tmp, bignum;
    bigint_init_from(&d, &ten, 1, 1);
    bigint_init_from(&bignum, &num_data, 1, 1);
    bigint_init_n(&tmp, dec_len);
    bigint_size_t tens_count = 0;

    while(isdigit(*str)) {
        bignum.data[0] = (bigint_value_t)*(str++) - '0';
        bigint_mul(tmp, d, out);
        bigint_add(*out, bignum, &tmp);
        tens_count++;
    }

    bigint_copy(tmp, out); // now output is in tmp, copy it

    bigint_destroy(&tmp);
}

void bigint_from_xstring(char* str, bigint_t* out) {
    bigint_size_t i = 0;
    bigint_value_t shift = 0;
    bigint_value_t tmp;

    if(*str == '-') {
        out->negative = true;
        str++;
    }

    if(*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
        str += 2;
    }

    bigint_size_t hex_len = calc_x_len(str, isxdigit);
    if(hex_len == 0) {
        bigint_from_int(0, out);
        return;
    }
    bigint_expand(out, (hex_len - 1u) / 16u + 1u);

    while(isxdigit(*str)) {
        tmp = (bigint_value_t)hexchar_to_int(*(str++)) << shift;
        out->data[i] += tmp;
        shift = (shift + 4) % 64;
        i += shift == 0;
    }
}

bigint_value_t bigint_to_string(const bigint_t num, char* out, int max_size) {
    bigint_value_t written = 0;

    if(num.size == 0) {
        written += sprintf_s(out, max_size, "0");
        return written;
    }

    bigint_value_t ten = 10, num_data = 0, count = 0;
    bigint_t d, bignum, tmp1, tmp2;
    bigint_init_from(&d, &ten, 1, 1);
    bigint_init_from(&bignum, &num_data, 1, 1);
    bigint_init_n(&tmp1, num.size);
    bigint_copy(num, &tmp1);
    bigint_init_n(&tmp2, num.size);

    if(num.negative) {
        written += sprintf_s(out, max_size, "-");
    }

    char* nums_from = out + written;

    do {
        bigint_copy(tmp1, &tmp2);
        bigint_div(tmp2, d, &tmp1, &bignum);

        written += sprintf_s(out + written, max_size - written, "%llu", bignum.data[0]);

        count++;
    } while(!bigint_is_zero(tmp1));

    arr_reverse(char, nums_from, count); // reverse only numbers

    bigint_destroy(&tmp1);
    bigint_destroy(&tmp2);

    return written;
}

bigint_value_t bigint_to_xstring(const bigint_t num, char* out, int max_size, int flag) {
    bigint_value_t written = 0;
    int large_letters = flag & BIF_LARGE_LETTERS;
    int pad = flag & BIF_PAD_HEX;
    char* format;

    if(num.negative) {
        written += sprintf_s(out + written, max_size - written, "-");
    }

    if(flag & BIF_ADD0X) {
        written += sprintf_s(out + written, max_size - written, large_letters ? "0X" : "0x");
    }

    format = pad ? (large_letters ? "%016llX" : "%016llx") : (large_letters ? "%llX" : "%llx");
    written += sprintf_s(out + written, max_size - written, format, num.size >= 1 ? num.data[num.size - 1] : 0);

    format = large_letters ? "%016llX" : "%016llx";
    for(bigint_size_t i = 2; i <= num.size; i++) {
        written += sprintf_s(out + written, max_size - written, format, num.data[num.size - i]);
    }

    return written;
}

bigint_value_t bigint_print_hex(const bigint_t num, int flag) {
    bigint_value_t written = 0;
    int large_letters = flag & BIF_LARGE_LETTERS;
    int pad = flag & BIF_PAD_HEX;
    char* format;

    int zeros = num.size == 0 ? 0 : 64 - (int)msb_zeros(num.data[num.size - 1]);
    bigint_value_t count = num.size == 0 ? 0 : (num.size - 1) * 16llu + (zeros == 0 ? 0 : zeros - 1) / 4 + 1;

    if(flag & BIF_PRINT_COUNT){
        written += printf("digits: %llu ", count);
    }

    if(num.negative) {
        written += printf("-");
    }

    if(flag & BIF_ADD0X) {
        written += printf(large_letters ? "0X" : "0x");
    }

    if(num.size == 0) {
        written += printf("0");
        return written;
    }

    format = pad ? (large_letters ? "%016llX" : "%016llx") : (large_letters ? "%llX" : "%llx");
    written += printf(format, num.size >= 1 ? num.data[num.size - 1] : 0);

    format = large_letters ? "%016llX" : "%016llx";
    for(bigint_size_t i = 2; i <= num.size; i++) {
        written += printf(format, num.data[num.size - i]);
    }

    return written;
}

bigint_value_t bigint_print(const bigint_t num, int flag){
    bigint_value_t written = 0;

    if(num.size == 0) {
        if(flag & BIF_PRINT_COUNT){
            written += printf("digits: 0 ");
        }
        written += printf("0");
        return written;
    }

    bigint_value_t arr[2] = {0}, ten = 10;
    bigint_value_t count = 0;
    bigint_t d, bignum;
    bigint_t tmp1, tmp2;
    bigint_init_from(&d, &ten, 1, 1);
    bigint_init_from(&bignum, arr, 2, 2);
    bigint_init_n(&tmp1, num.size + 1);
    bigint_copy(num, &tmp1);
    bigint_init_n(&tmp2, num.size + 1);

    bigint_value_t buf_size = num.size * 20 + 1;
    char* buf = bigint_alloc(buf_size, sizeof(char)); // should be enough

    if(num.negative) {
        written += sprintf_s(buf, buf_size, "-");
    }
    char* nums_from = buf + written;

    do {
        bigint_copy(tmp1, &tmp2);
        bigint_div(tmp2, d, &tmp1, &bignum);

        written += sprintf_s(buf + written, buf_size - written, "%u", (uint32_t)bignum.data[0]);

        count++;
    } while(!bigint_is_zero(tmp1));

    if(flag & BIF_PRINT_COUNT){
        printf("digits: %llu ", count);
    }

    for(bigint_value_t i = 0; i < count; i++) {
        printf("%c", nums_from[count - i - 1]);
    }

    bigint_destroy(&tmp1);
    bigint_destroy(&tmp2);

    return written;
}