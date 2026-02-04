#include "bigint.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <intrin.h>


static const bigint_t bigint_zero = {0, 0, 0, 0};
static const bigint_value_t one___ = 1;
static const bigint_t bigint_one = {&one___, 1, 1, 0};


#define subborrow(borrow, num1, num2, out) _subborrow_u64((borrow), (num1), (num2), (out))
#define addcarry(carry, num1, num2, out)   _addcarry_u64((carry), (num1), (num2), (out))

#define umul(num1, num2, high)          _umul128((num1), (num2), (high))
#define udiv(num1_lo, num1_hi, num2, r) _udiv128((num1_lo), (num1_hi), (num2), (r))

#ifndef _MSC_VER
static unsigned long long _udiv128(unsigned long long lo, unsigned long long hi, unsigned long long divisor, unsigned long long* r) {
    unsigned __int128 num = ((unsigned __int128)hi << (unsigned __int128)64) | ((unsigned __int128)lo);
    unsigned long long q = num / divisor;
    *r = (num - ((unsigned __int128)q * (unsigned __int128)divisor));
    return q;
}
#endif

#define msb_zeros(val) __lzcnt64(val)

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


#ifdef BIGINT_POW2_GROW
// no need for  (size) > 0x80000000 ? 0xFFFFFFFF : (1u << (32u - _lzcnt_u32((size) - 1)))
// because _lzcnt_u32 cant return 0 because size is 31 bit, so atleast 1
# define calc_capacity(size) (1u << (32u - _lzcnt_u32((size) - 1)))
#else
# define calc_capacity(size) (size)
#endif


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

static void bigint_reset_(bigint_t* num) {
    num->size = 0;
    num->capacity = 0;
    num->data = 0;
    num->negative = 0;
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
    out->negative = num.negative;
    bigint_copy_(num.data, num.size, out->data);
}

static bigint_size_t bigint_lshift_(bigint_value_t* data, bigint_size_t size, bigint_value_t shift, bigint_size_t offset, bigint_value_t* data_out) {
    bigint_size_t shift_val = shift & 63; // % 64

    if(shift_val == 0) {
        bigint_copy_(data, size, data_out + offset);
        arr_zero(data_out, offset * sizeof(bigint_value_t));
        return size + offset;
    }

    bigint_value_t inv_shift_val = 64 - shift_val;
    data_out[size + offset] = 0;

    for(bigint_size_t i = 0; i < size; i++) {
        bigint_size_t index = size - i - 1;
        data_out[index + 1 + offset] |= data[index] >> inv_shift_val;
        data_out[index + offset]      = data[index] << shift_val;
    }

    return bigint_shrink_(data_out, size + offset + 1);
}
void bigint_lshift(const bigint_t num, bigint_value_t shift, bigint_t* out) {
    bigint_size_t offset = (bigint_size_t)shift >> 6; // / 64
    bigint_expand(out, num.size + 1 + offset);
    out->size = bigint_lshift_(num.data, num.size, shift, offset, out->data);
}

static bigint_size_t bigint_rshift_(bigint_value_t* data, bigint_size_t size, bigint_value_t shift, bigint_size_t offset, bigint_value_t* data_out) {
    bigint_size_t shift_val = shift & 63;

    if(size <= offset) {
        data_out[0] = 0;
        return 1;
    }

    if(shift_val == 0) {
        bigint_copy_(data + offset, size - offset, data_out);
        return size - offset;
    }

    bigint_size_t inv_shift_val = 64 - shift_val;

    data_out[0] = data[0] >> shift_val;
    for(bigint_size_t i = 1; i < size - offset; i++) {
        data_out[i - 1] |= data[i + offset] << inv_shift_val;
        data_out[i]      = data[i + offset] >> shift_val;
    }

    return bigint_shrink_(data_out, size - offset);
}
void bigint_rshift(const bigint_t num, bigint_value_t shift, bigint_t* out) {
    bigint_size_t offset = (bigint_size_t)shift >> 6; // / 64
    bigint_expand(out, num.size - offset);
    out->size = bigint_rshift_(num.data, num.size, shift, offset, out->data);
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


static bigint_size_t bigint_inc_(bigint_value_t* data, bigint_size_t size, bigint_value_t* data_out) {
    bigint_value_t carry = 1;
    for(bigint_size_t i = 0; i < size && carry; i++) {
        data_out[i] = data[i] + carry;
        carry = data_out[i] == 0; // if zero after + 1
    }

    if(carry) {
        data_out[size] = 1;
        return size + 1;
    }

    return size;
}
static bigint_size_t bigint_dec_(bigint_value_t* data, bigint_size_t size, bigint_value_t* data_out) {
    bigint_value_t carry = 1;
    for(bigint_size_t i = 0; i < size && carry; i++) {
        data_out[i] = data[i] - carry;
        carry = data_out[i] == 0xFFFFFFFFFFFFFFFF; // if zero before - 1
    }

    assert(!carry); // num cant be 0 at size - 1 
    if(data_out[size - 1] == 0) {
        return size - 1;
    }

    return size;
}

void bigint_inc(const bigint_t num, bigint_t* out) {
    if(!out) {
        return;
    }

    if(bigint_is_zero(*out)) {
        bigint_from_uint(1, out);
        return;
    }

    if(num.negative) {
        bigint_expand(out, num.size);
        out->size = bigint_dec_(num.data, num.size, out->data);
        return;
    }

    bigint_expand(out, num.size + 1);
    out->size = bigint_inc_(num.data, num.size, out->data);
}
void bigint_dec(const bigint_t num, bigint_t* out) {
    if(!out) {
        return;
    }

    if(bigint_is_zero(*out)) {
        bigint_from_uint(1, out);
        out->negative = true;
        return;
    }

    if(num.negative) {
        bigint_expand(out, num.size + 1);
        out->size = bigint_inc_(num.data, num.size, out->data);
        return;
    }

    bigint_expand(out, num.size);
    out->size = bigint_dec_(num.data, num.size, out->data);
}


// https://web.archive.org/web/20241114101556/https://www.codeproject.com/Articles/1276310/Multiple-Precision-Arithmetic-1st-Multiplication-Algorithm
static bigint_size_t bigint_mul_(bigint_value_t* data1, bigint_size_t size1, bigint_value_t* data2, bigint_size_t size2, bigint_value_t* data_out) { //ignores negatives
    arr_zero(data_out, (size1 + size2) * sizeof(bigint_value_t));

    bigint_value_t high, low;
    bigint_size_t k;
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
        }
    }
    return size1 + size2; 
}

void bigint_mul(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)) {
    if(!out) { // cant do the thing
        return;
    }

    if(bigint_is_zero(num1) || bigint_is_zero(num2)){
        bigint_from_uint(0, out);
    }

    if(bigint_eq_uint(num1, 1)){
        bigint_copy(num2, out);
        out->negative = num1.negative != num2.negative;
        return;
    }
    if(bigint_eq_uint(num2, 1)){
        bigint_copy(num1, out);
        out->negative = num1.negative != num2.negative;
        return;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#else
    assert(num1.data != out->data && num2.data != out->data);
#endif


    bigint_expand(out, num1.size + num2.size);

    out->size = bigint_mul_(num1.data, num1.size, num2.data, num2.size, out->data);

    out->negative = num1.negative != num2.negative;

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

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
    assert(shift < 64); // only if b_left is 0, what shouldnt happen

    size_a = bigint_lshift_(data_a, size1, shift, 0, data_a);
    size_b = bigint_lshift_(data_b, size2, shift, 0, data_b);
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
        ai_left = *size_r == 0 ? 0 : data_r[*size_r - 1];

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


            int count = 0;
            while(1){
                cmp = bigint_abscmp_(data_tmp, size_tmp, data_r, *size_r);

                if(cmp <= 0) {
                    break;
                }

                // we overshoot for 1 to 3 iterations
                size_tmp = bigint_sub_(data_tmp, size_tmp, data_b, size_b, data_tmp);
                qi--;

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

    *size_r = bigint_rshift_(data_r, *size_r, shift, 0, data_r); // reverse initial shift
}

int bigint_div(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out), bigint_t* UNIQUE(r)) {
    if(!out || !r) { // cant do the thing
        return 2;
    }

#ifndef BIGINT_NO_UNIQUE
    // these must be different memory
    assert(num1.data != out->data && num2.data != out->data && 
           num1.data != r->data && num2.data != r->data &&
           out->data != r->data);
#endif


    if(bigint_is_zero(num2)) { // division by 0
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

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_t* bigint_old_r__ = r;
    bigint_t bigint_r__ = { 0, 0, 0, 0 };
    r = &bigint_r__;

    bigint_init(out);
    bigint_init(r);
#endif

    bigint_expand(out, num1.size + 1);
    bigint_expand(r, num2.size + 1);

    arr_zero(out->data, out->size * sizeof(bigint_value_t));

    bigint_value_t* tmp = bigint_alloc((num1.size + 1 + num1.size + 1 + num1.size + 1) * sizeof(bigint_value_t));

    bigint_size_t qs, rs;
    bigint_div_(num1.data, num1.size, num2.data, num2.size, out->data, &qs, r->data, &rs, tmp);
    out->size = qs;
    r->size = rs;

    bigint_free(tmp);

    out->negative = num1.negative != num2.negative;
    r->negative = num1.negative;

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;

    bigint_copy(*r, bigint_old_r__);
    bigint_destroy(r);
    r = bigint_old_r__;
#endif

    bigint_shrink(out);
    bigint_shrink(r);

    return 0;
}


int bigint_mod(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)) { // x - (x / y) * y
    if(!out) { // cant do the thing
        return 2;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#else
    assert(num1.data != out->data && num2.data != out->data);
#endif

    bigint_t tmp;
    bigint_init_n(&tmp, num1.size + 1);

    int ret = bigint_div(num1, num2, &tmp, out);

    bigint_destroy(&tmp);

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

    bigint_shrink(out);
    
    return ret;
}

// https://stackoverflow.com/questions/4407839/how-can-i-find-the-square-root-of-a-java-biginteger
int bigint_sqrt(const bigint_t num, bigint_t* UNIQUE(out), bool ceil) {
    if(!out) { // cant do the thing
        return 2;
    }

#ifndef BIGINT_NO_UNIQUE
    assert(num.data != out->data);
#endif

    out->negative = false;

    int cmp = bigint_cmp(num, bigint_zero);
    if(cmp < 0) {
        return 1;
    }else if(cmp == 0) {
        out->size = 0;
        return 0;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#endif

    bigint_expand(out, num.size);

    if(num.size == 1) {
        bigint_from_uint(sqrt(num.data[0]), out);
        return 0;
    }

    int ret = 0;
    bigint_size_t size = num.size + 1;
    bigint_value_t* tmp_alloc = bigint_alloc((size * 3) * sizeof(bigint_value_t));
    bigint_t tmp1, tmp2, tmp3;
    bigint_init_from(&tmp1, tmp_alloc                                , 0, size);
    bigint_init_from(&tmp2, tmp_alloc + tmp1.capacity                , 0, size);
    bigint_init_from(&tmp3, tmp_alloc + tmp1.capacity + tmp2.capacity, 0, size);

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


    bigint_free(tmp_alloc);

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

    bigint_shrink(out);

    return ret;
}

//https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int
void bigint_pow(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)) {
    if(!out) {
        return;
    }

#ifndef BIGINT_NO_UNIQUE
    assert(num1.data != out->data && num2.data != out->data);
#endif


    int cmp = bigint_abscmp(num1, bigint_one);
    if(cmp < 0) { // zero
        bigint_from_uint(0, out);
        return;
    }else if(cmp == 0) { //one
        bigint_from_uint(1, out);
        return;
    }

    cmp = bigint_abscmp(num2, bigint_one);
    if(cmp < 0) { // zero
        bigint_from_uint(1, out);
        return;
    }else if(cmp == 0) { // one
        bigint_copy(num1, out);
        return;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#endif

    bigint_expand(out, num1.size * bigint_to_uint_greedy(num2)); // yeah it grows quickly

    out->data[0] = 1;
    out->size = 1;

    bigint_t tmp, base, exp;
    bigint_init_n(&tmp, out->size);
    bigint_init_n(&base, out->size);
    bigint_init_n(&exp, num1.size);

    bigint_copy(num1, &base);
    bigint_copy(num2, &exp);

    while(1) {

        if(exp.data[0] & 1) {
            bigint_mul(*out, base, &tmp);
            bigint_copy(tmp, out);
        }

        bigint_rshift(exp, 1, &exp);
        if(exp.size == 0) { // zero
            break;
        }

        bigint_mul(base, base, &tmp);
        bigint_copy(tmp, &base);
    }

    bigint_destroy(&tmp);
    bigint_destroy(&base);
    bigint_destroy(&exp);

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

    bigint_shrink(out);
}


// https://proofwiki.org/wiki/Number_of_Digits_in_Factorial
static bigint_size_t factorial_digits(const bigint_t num) {
    if(bigint_abscmp_uint(num, 1) <= 0) { // 0 or 1
        return 1;
    }

    // 1 + (log(2 * pi * n) / 2 + n * log(n / e)), but we overestimate so
    // 1 + (log(8 * n) / 2 + n * log(n / 2))
    // 1 + (log(8 * n) / log(BASE) / 2 + n * log(n / 2) / log(BASE))
    // 1 + (log2(8 * n) / 2 + n * log2(n / 2)) / log2(BASE)
    // 1 + (log2(8 * n) / 2 + n * log2(n / 2)) / 64

    // log2(2^x * n) = log2(n) + x

    // logBASE(x) = log2(x) / log2(BASE) log
    // log2(BASE) = log2(2 ^ 64) = 64

    bigint_value_t log2n = bigint_bit_length(num);
    bigint_value_t n = bigint_to_uint_greedy(num); // shouldnt if we taking larger factorials we have some problems
    bigint_value_t res = ((log2n + 3) << 1) + n * (log2n - 1);

    // dont forget convert to log
    return 1 + ((res - 1) << 6);
}

int bigint_fact(const bigint_t num, bigint_t* UNIQUE(out)) {
    if(!out) {
        return 2;
    }

#ifndef BIGINT_NO_UNIQUE
    assert(num.data != out->data);
#endif

    if(num.negative) {
        return 1;
    }

    if(bigint_is_zero(num) || bigint_eq_uint(num, 1)) {
        bigint_from_uint(1, out);
        return 0;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#endif

    bigint_size_t size = factorial_digits(num);
    bigint_t counter, tmp;
    bigint_init_n(&counter, num.size);
    bigint_init_n(&tmp, size);
    bigint_copy(num, &counter);

    bigint_expand(out, size);

    bigint_from_uint(1, out);

    while(!bigint_is_zero(counter)) {
        bigint_copy(*out, &tmp);
        bigint_mul(tmp, counter, out);

        bigint_dec(counter, &counter);
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

    bigint_shrink(out);

    return 0;
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

void bigint_log2(const bigint_t num, bigint_t* out) {
    if(!out) {
        return;
    }

    out->negative = false;

    bigint_value_t val = bigint_bit_length(num);
    if(val == 0) {
        out->size = 0;
        return;
    }

    bigint_expand(out, 1);
    out->data[0] = val;
    out->size = 1;
}

int bigint_gcd(const bigint_t num1, const bigint_t num2, bigint_t* UNIQUE(out)) {
    if(!out) {
        return 2;
    }

#ifndef BIGINT_NO_UNIQUE
    assert(num1.data != out->data && num2.data != out->data);
#endif

    if(num1.size == 0 || num2.size == 0) {
        return 1;
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_t* bigint_old_out__ = out;
    bigint_t bigint_tmp__ = { 0, 0, 0, 0 };
    out = &bigint_tmp__;

    bigint_init(out);
#endif

    bigint_t dummy, tmp1, tmp2;
    bigint_value_t* tmp_alloc = bigint_alloc((num1.size + 1 + num1.size + 1 + num2.size + 1) * sizeof(bigint_value_t));
    bigint_init_from(&dummy, tmp_alloc,                                   0, num1.size + 1);
    bigint_init_from(&tmp1,  tmp_alloc + dummy.capacity,                  0, num1.size + 1);
    bigint_init_from(&tmp2,  tmp_alloc + dummy.capacity + tmp1.capacity,  0, num2.size + 1);
    bigint_copy(num1, &tmp1);
    bigint_copy(num2, &tmp2);
    bigint_from_uint(1, &dummy);
    tmp1.negative = false;
    tmp2.negative = false;

    int ret = 0;

    while (!bigint_is_zero(tmp2)){
        bigint_copy(tmp2, out);
        ret = bigint_div(tmp1, *out, &dummy, &tmp2);
        if(ret) {
            break;
        }

        bigint_copy(*out, &tmp1);
    }

#ifdef BIGINT_NO_UNIQUE
    bigint_copy(*out, bigint_old_out__);
    bigint_destroy(out);
    out = bigint_old_out__;
#endif

    bigint_free(tmp_alloc);

    out->negative = false;

    bigint_shrink(out);

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
 bool bigint_eq(const bigint_t num1, const bigint_t num2) {
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

bool bigint_eq_uint(const bigint_t num1, bigint_value_t num2){
    if(num2 == 0){
        return bigint_is_zero(num1);
    }
    return num1.size == 1 && num1.data[0] == num2;
}
bool bigint_abscmp_uint(const bigint_t num1, bigint_value_t num2){
    if(num2 == 0){
        return bigint_is_zero(num1);
    }
    if(num1.size > 1){
        return 1;
    }

    if(num1.data[0] == num2){
        return 0;
    }else if(num1.data[0] > num2){
        return 1;
    }

    return -1;
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

void bigint_inv(const bigint_t num, bigint_t* out) {
    bigint_expand(out, num.size);
    for(bigint_size_t i = 0; i < num.size; i++) {
        out->data[i] = ~num.data[i];
    }
}

bool bigint_fits_int(const bigint_t num, bool is_signed) {
    if(num.size > 1) {
        return false;
    }

    if(is_signed) {
        return num.data[0] <= (bigint_value_t)INTMAX_MAX + (bigint_value_t)!!num.negative;
    }

    return true;
}

static inline int hexchar_to_int(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}

void bigint_from_uint(uint64_t num, bigint_t* out) {
    if(!out) {
        return;
    }

    out->negative = false;

    if(num == 0) {
        out->size = 0;
        return;
    }

    bigint_expand(out, 1);

    out->size = 1;
    out->data[0] = num;

    bigint_shrink(out);
}
void bigint_from_int(int64_t num, bigint_t* out) {
    if(!out) {
        return;
    }

    if(num == 0) {
        out->size = 0;
        return;
    }

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

inline bigint_value_t bigint_to_uint(const bigint_t num) {
    if(num.size == 0) {
        return 0;
    }

    return num.data[0];
}
inline bigint_ivalue_t bigint_to_int(const bigint_t num) {
    if(num.size == 0) {
        return 0;
    }

    bigint_value_t res = num.data[0];
    if(num.negative) {
        return res > INT64_MAX ? INT64_MIN : -((bigint_ivalue_t)res);
    }
    return res > INT64_MAX ? INT64_MAX : (bigint_ivalue_t)res;
}

bigint_value_t bigint_to_uint_greedy(bigint_t num) {
    if(num.size > 1) {
        return UINT64_MAX;
    }
    return bigint_to_uint(num);
}
bigint_ivalue_t bigint_to_int_greedy(bigint_t num) {
    if(num.size > 1) {
        if(num.negative) {
            return INT64_MIN;
        }
        return INT64_MAX; 
    }
    return bigint_to_int(num);
}


static bigint_size_t calc_x_len(char* str, int (*proc)(int)) {
    char* start = str;
    while(proc(*str))
        str++;
    return (bigint_size_t)(str - start);
}

void bigint_from_string_dec_(char* str, bigint_t* out) {
    bool neg = false;
    if(*str == '-') {
        neg = true;
        str++;
    }

    bigint_size_t dec_len = calc_x_len(str, isdigit); // allocate hex size just to be sure
    if(dec_len == 0) {
        bigint_from_int(0, out);
        return;
    }
    dec_len = (dec_len - 1u) / 19u + 1u;
    bigint_expand(out, dec_len);

    bigint_value_t ten = 10, num_data = 0;
    bigint_t d, tmp, bignum;
    bigint_init_from(&d, &ten, 1, 1);
    bigint_init_from(&bignum, &num_data, 1, 1);
    bigint_init_n(&tmp, dec_len);

    while(isdigit(*str)) {
        bignum.data[0] = (bigint_value_t)*(str++) - '0';
        bigint_mul(tmp, d, out);
        bigint_add(*out, bignum, &tmp);
    }

    tmp.negative = neg;
    bigint_copy(tmp, out); // now output is in tmp, copy it

    bigint_destroy(&tmp);
}

void bigint_from_string_hex_(char* str, bigint_t* out) {
    bigint_size_t index = 0;
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

    for(bigint_size_t i = 1; i <= hex_len; i++) {
        tmp = (bigint_value_t)hexchar_to_int(str[hex_len - i]) << shift;
        out->data[index] += tmp;
        out->size = index + 1;
        shift = (shift + 4) % 64;
        index += shift == 0;
    }
}

void bigint_from_string(char* str, bigint_t* out, int flag) {
    if(flag & BI_HEX) {
        bigint_from_string_hex_(str, out);
        return;
    }
    bigint_from_string_dec_(str, out);
}


bigint_value_t div_r(bigint_value_t v, bigint_value_t d, bigint_value_t* r) {
    bigint_value_t q = v / d;
    *r = v - q * d;
    return q;
}
bigint_value_t reverse_int_buf(bigint_value_t val, int min_cnt, char* buf, int max_size) {
    bigint_value_t r, size = 0;
    while(max_size--) { // reverse 'print' integer
        val = div_r(val, 10, &r);

        buf[size++] = (char)r + '0';

        if(--min_cnt <= 0 && val <= 0) {
            break;
        }
    }
    return size;
}

static bigint_value_t bigint_to_string_dec_(const bigint_t num, char* out, int max_size, int flag) {
    bigint_value_t written = 0;

    if(!out || max_size <= 0) {
        return 0;
    }

    if(num.size == 0) {
        written += sprintf_s(out, max_size - written, "0");
        return written;
    }

    bigint_value_t val, divisor = 1000000000000000000llu;
    bigint_t d, bignum, tmp1, tmp2;
    bigint_init_from(&d, &divisor, 1, 1);
    bigint_value_t* tmp_alloc = bigint_alloc((num.size + 1 + num.size + 1 + num.size + 1) * sizeof(bigint_value_t));
    bigint_init_from(&bignum, tmp_alloc,                               0, num.size + 1);
    bigint_init_from(&tmp1,   tmp_alloc + 1 + num.size,                0, num.size + 1);
    bigint_init_from(&tmp2,   tmp_alloc + 1 + num.size + 1 + num.size, 0, num.size + 1);
    bigint_copy(num, &tmp1);
    tmp1.negative = false;

    if(num.negative) {
        written += sprintf_s(out, max_size - written, "-");
    }

    int cnt;

    do {
        bigint_copy(tmp1, &tmp2);
        bigint_div(tmp2, d, &tmp1, &bignum);

        cnt = (tmp1.size > 0) * 18; // to print all digits if remainder less than necessary digits count

        val = bignum.size == 0 ? 0 : bignum.data[0];
        written += reverse_int_buf(val, cnt, out + written, max_size - written);
    } while(!bigint_is_zero(tmp1));

    bigint_value_t count = written - !!num.negative;
    arr_reverse(char, out + !!num.negative, count); // reverse only numbers

    //assert(max_size - written > 0);
    out[written] = '\0';

    bigint_free(tmp_alloc);

    return written;
}

static bigint_value_t bigint_to_xstring_(const bigint_t num, char* out, int max_size, int flag) {
    bigint_value_t written = 0;
    int large_letters = flag & BI_LARGE_LETTERS;
    int pad = flag & BI_PAD_HEX;
    char* format;

    if(num.negative) {
        written += sprintf_s(out + written, max_size - written, "-");
    }

    if(flag & BI_ADD0X) {
        written += sprintf_s(out + written, max_size - written, large_letters ? "0X" : "0x");
    }

    format = pad ? (large_letters ? "%016llX" : "%016llx") : (large_letters ? "%llX" : "%llx");
    written += sprintf_s(out + written, max_size - written, format, num.size >= 1 ? num.data[num.size - 1] : 0);

    format = large_letters ? "%016llX" : "%016llx";
    for(bigint_size_t i = 2; i <= num.size; i++) {
        written += sprintf_s(out + written, max_size - written, format, num.data[num.size - i]);
    }

    //assert(max_size - written > 0);
    out[written] = '\0';

    return written;
}

bigint_value_t bigint_to_string(const bigint_t num, char* out, int max_size, int flag) {
    if(flag & BI_HEX) {
        return bigint_to_xstring_(num, out, max_size, flag);
    }
    return bigint_to_string_dec_(num, out, max_size, flag);
}


static bigint_value_t bigint_fprint_hex_(const bigint_t num, FILE* stream, int flag) {
    bigint_value_t written = 0;
    int large_letters = flag & BI_LARGE_LETTERS;
    int pad = flag & BI_PAD_HEX;
    char* format;

    //if(flag & BI_PRINT_COUNT){
    //    int zeros = num.size == 0 ? 0 : 64 - (int)msb_zeros(num.data[num.size - 1]);
    //    bigint_value_t count = num.size == 0 ? 0 : (num.size - 1) * 16llu + (zeros == 0 ? 0 : zeros - 1) / 4 + 1;
    //    written += fprintf(stream, "digits: %llu ", count);
    //}

    if(num.negative) {
        written += fprintf(stream, "-");
    }

    if(flag & BI_ADD0X) {
        written += fprintf(stream, large_letters ? "0X" : "0x");
    }

    if(num.size == 0) {
        written += fprintf(stream, "0");
        return written;
    }

    format = pad ? (large_letters ? "%016llX" : "%016llx") : (large_letters ? "%llX" : "%llx");
    written += fprintf(stream, format, num.size >= 1 ? num.data[num.size - 1] : 0);

    format = large_letters ? "%016llX" : "%016llx";
    for(bigint_size_t i = 2; i <= num.size; i++) {
        written += fprintf(stream, format, num.data[num.size - i]);
    }

    return written;
}

static bigint_value_t bigint_fprint_dec_(const bigint_t num, FILE* stream, int flag){
    bigint_value_t written = 0;

    if(num.size == 0) {
        //if(flag & BI_PRINT_COUNT){
        //    written += fprintf(stream, "digits: 0 ");
        //}
        written += fprintf(stream, "0");
        return written;
    }

    bigint_value_t val, divisor = 1000000000000000000llu;
    bigint_t d, bignum;
    bigint_t tmp1, tmp2;
    bigint_init_from(&d, &divisor, 1, 1);
    bigint_value_t* tmp_alloc = bigint_alloc((num.size + 1 + num.size + 1 + num.size + 1) * sizeof(bigint_value_t));
    bigint_init_from(&bignum, tmp_alloc,                               0, num.size + 1);
    bigint_init_from(&tmp1,   tmp_alloc + 1 + num.size,                0, num.size + 1);
    bigint_init_from(&tmp2,   tmp_alloc + 1 + num.size + 1 + num.size, 0, num.size + 1);
    bigint_copy(num, &tmp1);
    tmp1.negative = false;

    bigint_value_t buf_size = num.size * 20 + 1;
    char* buf = bigint_alloc(buf_size * sizeof(char)); // should be enough
    int cnt;

    do {
        bigint_copy(tmp1, &tmp2);
        bigint_div(tmp2, d, &tmp1, &bignum);

        cnt = (tmp1.size > 0) * 18; // to print all digits if remainder less than necessary digits count

        val = bignum.size == 0 ? 0 : bignum.data[0];
        written += reverse_int_buf(val, cnt, buf + written, 999);

    } while(!bigint_is_zero(tmp1));

    bigint_value_t count = written;
    //if(flag & BI_PRINT_COUNT){
    //    fprintf(stream, "digits: %llu ", count);
    //}

    if(num.negative) {
        written += fprintf(stream, "-");
    }

    for(bigint_value_t i = 1; i <= count; i++) {
        putc(buf[count - i], stream);
    }

    bigint_free(tmp_alloc);

    return written;
}

bigint_value_t bigint_fprint(const bigint_t num, FILE* stream, int flag) {
    if(flag & BI_HEX) {
        return bigint_fprint_hex_(num, stream, flag);
    }
    return bigint_fprint_dec_(num, stream, flag);
}



#ifndef BIGINT_NO_FRACTIONS

#define BIGINT_FRACTION_PRINT_MAX 100

void bigintf_init(bigintf_t* num){
    if(!num){
        return;
    }

    bigint_init(&num->numerator);
    bigint_init(&num->denominator);
}

void bigintf_clear(bigintf_t* num){
    if(!num){
        return;
    }

    bigint_clear(&num->numerator);
    bigint_clear(&num->denominator);
}

void bigintf_destroy(bigintf_t* num){
    if(!num){
        return;
    }

    bigint_destroy(&num->numerator);
    bigint_destroy(&num->denominator);
}

void bigintf_copy(bigintf_t num, bigintf_t* out){
    if(!out){
        return;
    }

    bigint_copy(num.numerator, &out->numerator);
    bigint_copy(num.denominator, &out->denominator);
}

bool bigintf_is_zero(bigintf_t num){
    return num.numerator.size == 0;
}

bool bigintf_abseq(const bigintf_t num1, const bigintf_t num2){
    return bigint_eq(num1.numerator, num2.numerator) && bigint_eq(num1.denominator, num2.denominator);
}

void bigintf_simplify(bigintf_t* num){
    if(!num){
        return;
    }

    if(bigintf_is_zero(*num) || bigint_abscmp_uint(num->numerator, 1) == 0 || bigint_abscmp_uint(num->denominator, 1) == 0){
        return;
    }

    int negative = num->numerator.negative;
    num->numerator.negative = false;

    if(bigint_eq(num->numerator, num->denominator)){
        bigint_from_uint(1, &num->numerator);
        bigint_from_uint(1, &num->denominator);

        num->numerator.negative = negative;

        return;
    }

    bigint_size_t size_max = max(num->numerator.size, num->denominator.size) + 1;
    bigint_size_t size_min = min(num->numerator.size, num->denominator.size) + 1;
    bigint_t gcd, r, tmp;
    bigint_value_t* tmp_alloc = bigint_alloc((size_min + size_max + size_max) * sizeof(bigint_value_t));
    bigint_init_from(&gcd, tmp_alloc,                             0, size_min);
    bigint_init_from(&r,   tmp_alloc + gcd.capacity,              0, size_max);
    bigint_init_from(&tmp, tmp_alloc + gcd.capacity + r.capacity, 0, size_max);

    bigint_gcd(num->numerator, num->denominator, &gcd);

    if(!bigint_eq_uint(gcd, 1)){
        bigint_copy(num->numerator, &tmp);
        bigint_div(tmp, gcd, &num->numerator, &r);

        bigint_copy(num->denominator, &tmp);
        bigint_div(tmp, gcd, &num->denominator, &r);
    }

    num->numerator.negative = negative;

    bigint_free(tmp_alloc);
}

static void bigintf_add_sub_(const bigintf_t num1, const bigintf_t num2, bigintf_t* out, bool sub){
    if(!out){
        return;
    }

    if(bigint_abscmp(num1.denominator, num2.denominator) == 0){
        bigint_copy(num1.denominator, &out->denominator);
        bigint_add(num1.numerator, num2.numerator, &out->numerator);
    }

    bigint_t tmp;
    bigint_init_n(&tmp, num1.numerator.size * num2.numerator.size + 1);

    bigint_mul(num1.denominator, num2.denominator, &out->denominator);

    bigint_mul(num1.numerator, num2.denominator, &out->numerator);
    bigint_mul(num2.numerator, num1.denominator, &tmp);

    if(sub){
        tmp.negative = !tmp.negative;
    }

    bigint_add(out->numerator, tmp, &out->numerator);

    bigint_destroy(&tmp);

    bigintf_simplify(out);
}

void bigintf_add(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out)){
    bigintf_add_sub_(num1, num2, out, 0);
}
void bigintf_sub(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out)){
    bigintf_add_sub_(num1, num2, out, 1);
}

static int bigintf_mul_div_(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out), bool div){
    if(!out){
        return 0;
    }

    if(div){
        if(bigint_eq_uint(num2.numerator, 0)) {
            return 1;
        }

        if(bigintf_abseq(num1, num2)){
            bigint_from_uint(1, &out->numerator);
            bigint_from_uint(1, &out->denominator);
        }

        return 0;
    }

    if(div){
        bigint_mul(num1.numerator, num2.denominator, &out->numerator);
        bigint_mul(num1.denominator, num2.numerator, &out->denominator);
    }else{
        bigint_mul(num1.numerator, num2.numerator, &out->numerator);
        bigint_mul(num1.denominator, num2.denominator, &out->denominator);
    }

    bigintf_simplify(out);

    return 0;
}

void bigintf_mul(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out)){
    if(!out) {
        return;
    }

    bigintf_mul_div_(num1, num2, out, 0);
}
int bigintf_div(const bigintf_t num1, const bigintf_t num2, bigintf_t* UNIQUE(out)){
    if(!out) {
        return 2;
    }

    return bigintf_mul_div_(num1, num2, out, 1);
}


void bigintf_from_bigint(bigint_t num, bigintf_t* out) {
    if(!out) {
        return;
    }

    bigint_copy(num, &out->numerator);
    bigint_from_uint(1, &out->denominator);
}
void bigintf_from_bigints(bigint_t num, bigint_t den, bigintf_t* out) {
    if(!out) {
        return;
    }

    bigint_copy(num, &out->numerator);
    bigint_copy(den, &out->denominator);
}


void bigintf_from_uint(uint64_t num, uint64_t den, bigintf_t* out) {
    if(!out) {
        return;
    }

    bigint_from_uint(num, &out->numerator);
    bigint_from_uint(den, &out->denominator);

    bigintf_simplify(out);
}
void bigintf_from_int(uint64_t num, uint64_t den,bigintf_t* out) {
    if(!out) {
        return;
    }

    bigint_from_int(num, &out->numerator);
    bigint_from_int(den, &out->numerator);

    bigintf_simplify(out);
}

bigint_value_t int_pow10(int n) {
    bigint_value_t res = 1;
    while(n --> 0) {
        res *= 10;
    }
    return res;
}

void bigintf_from_f64(double num, bigintf_t* out) {
    if(!out) {
        return;
    }

    char buf[DOUBLE_BUF_SIZE];
    //char buf_exp10[DOUBLE_BUF_SIZE];

    snprintf(buf, DOUBLE_BUF_SIZE, "%.*f", DOUBLE_DIGITS, num);

    int negative = 0;
    if(buf[0] == '-') {
        negative = 1;
    }

    int whole = calc_x_len(buf + negative, isdigit);

    int decimal = DOUBLE_DIGITS; // count degits after .
    for(; decimal > 0; decimal--){
        if(buf[negative + whole + 1 + decimal - 1] != '0') {
            break;
        }
    }

    memmove(buf + negative + whole, buf + negative + whole + 1, decimal); // copy to eliminate .
    buf[negative + whole + decimal] = '\0'; // add terminator

    bigintf_init(out);
    bigint_from_string(buf, &out->numerator, 0);
    //bigint_from_string(buf_exp10, &out->denominator);
    bigint_from_uint(int_pow10(decimal), &out->denominator);

    out->numerator.negative = negative;

    bigintf_simplify(out);
}


double bigintf_to_f64(const bigintf_t num) {
    char buf[DOUBLE_BUF_SIZE];
    bigint_value_t v =  bigintf_to_string(num, buf, DOUBLE_BUF_SIZE, DOUBLE_DIGITS, BIF_AS_DECIMAL);
    double res = strtod(buf, 0);
    return res;
}

bigint_value_t bigintf_to_string(const bigintf_t num, char* out, bigint_value_t max_size, bigint_value_t fraction_max, int flag) {
    if(!out) {
        return 0;
    }

    bigint_value_t written = 0;

    if(!(flag & BIF_AS_DECIMAL)) {
        written += bigint_to_string(num.numerator, out,  max_size, 0);
        written += snprintf(out + written, max_size - written, " / ");
        written += bigint_to_string(num.denominator, out + written, max_size - written, 0);
        return written;
    }

    bigint_size_t size = max(num.numerator.size + 1, num.denominator.size) + 1;
    bigint_value_t* tmp_alloc = bigint_alloc((size * 3) * sizeof(bigint_value_t));
    bigint_t q, r, nr;
    bigint_init_from(&q,  tmp_alloc                          , 0, size);
    bigint_init_from(&r,  tmp_alloc + q.capacity             , 0, size);
    bigint_init_from(&nr, tmp_alloc + q.capacity + r.capacity, 0, size);

    bigint_copy(num.numerator, &nr);
    nr.negative = false;
    assert(!num.denominator.negative);

    if(num.numerator.negative) {
        written += snprintf(out + written, max_size - written, "-");
    }

    bigint_div(nr, num.denominator, &q, &r);
    written += bigint_to_string(q, out + written, max_size - written, 0);

    if(bigint_is_zero(r)){
        bigint_free(tmp_alloc);

        return written;
    }

    written += snprintf(out + written, max_size - written, ".");

    bigint_copy(r, &nr);

    bigint_value_t denominator = 10;
    bigint_t d;
    bigint_init_from(&d, &denominator, 1, 1);

    if(fraction_max <= 0) {
        fraction_max = BIGINT_FRACTION_PRINT_MAX;
    }

    bigint_value_t written_whole = written;
    while(true){
        bigint_mul(nr, d, &q);
        bigint_copy(q, &nr);

        bigint_div(nr, num.denominator, &q, &r);

        out[written++] = (char)bigint_to_uint(q) + '0';

        if(bigint_is_zero(r) || (written - written_whole >= fraction_max) || (written >= max_size)){
            break;
        }

        bigint_copy(r, &nr);
    }

    out[written] = '\0';

    bigint_free(tmp_alloc);

    return written;
}

bigint_value_t bigintf_fprint(const bigintf_t num, FILE* stream, bigint_value_t fraction_max, int flag){
    bigint_value_t written = 0;

    if(!(flag & BIF_AS_DECIMAL)) {
        written += bigint_fprint(num.numerator, stream, flag);
        written += fprintf(stream, " / ");
        written += bigint_fprint(num.denominator, stream, flag);
        return written;
    }

    bigint_size_t size = max(num.numerator.size + 1, num.denominator.size) + 1;
    bigint_value_t* tmp_alloc = bigint_alloc((size * 3) * sizeof(bigint_value_t));
    bigint_t q, r, nr;
    bigint_init_from(&q,  tmp_alloc                          , 0, size);
    bigint_init_from(&r,  tmp_alloc + q.capacity             , 0, size);
    bigint_init_from(&nr, tmp_alloc + q.capacity + r.capacity, 0, size);

    bigint_copy(num.numerator, &nr);
    nr.negative = false;
    assert(!num.denominator.negative);

    if(num.numerator.negative) {
        written += fprintf(stream, "-");
    }

    bigint_div(nr, num.denominator, &q, &r);
    written += bigint_fprint(q, stream, 0);

    if(bigint_is_zero(r)){
        bigint_free(tmp_alloc);

        return written;
    }

    written += fprintf(stream, ".");

    bigint_copy(r, &nr);

    bigint_value_t denominator = 10;
    bigint_t d;
    bigint_init_from(&d, &denominator, 1, 1);

    if(fraction_max <= 0) {
        fraction_max = BIGINT_FRACTION_PRINT_MAX;
    }

    bigint_value_t written_whole = written;
    while(true){
        bigint_mul(nr, d, &q);
        bigint_copy(q, &nr);

        bigint_div(nr, num.denominator, &q, &r);

        putc(bigint_to_uint(q) + '0', stream);
        written++;

        if(bigint_is_zero(r) || (written - written_whole >= fraction_max)){
            break;
        }

        bigint_copy(r, &nr);
    }

    bigint_free(tmp_alloc);

    return written;
}


#endif