#include "bigint.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>


#define BIGINT_DEFAULT_INIT_WORD_COUNT 1

#ifdef BIGINT_AUTOEXPAND
	#define bigint_expand(num, target) bigint_expand_to(num, target)
#else
	#define bigint_expand(num, target) 
#endif

#ifdef BIGINT_AUTOSHRINK
	#define bigint_shrink(num) do{ while(num->size>1 && !num->data[num->size-1]) { num->size--; } }while(0)
#else
	#define bigint_shrink(num) 
#endif


//static inline uint32_t calc_capacity(uint32_t size) {
//	return 1u << (32u - _lzcnt_u32(size - 1));
//}
#define calc_capacity(size) (1u << (32u - _lzcnt_u32((size) - 1)))

inline void bigint_expand_to(bigint_t* num, uint32_t target) {
	if(target > num->size){
		if(num->capacity < target) {
			num->capacity = calc_capacity(target);
			//num->data = realloc(num->data, num->capacity * sizeof(uint64_t));
			void* old = num->data;
			num->data = calloc(num->capacity, sizeof(uint64_t));
			memcpy(num->data, old, num->size * sizeof(uint64_t));
		}
		num->size = target;
	}
}
void bigint_init_n(bigint_t* num, uint32_t n) {
	num->data = calloc(n, sizeof(uint64_t));
	num->size = 1;
	num->capacity = calc_capacity(n);
#ifndef BIGINT_NONEGATIVE
	num->negative = false;
#endif
	//memset(num->data, 0, sizeof(uint64_t) * n);
}
void bigint_init_default(bigint_t* num) {
	bigint_init_n(num, BIGINT_DEFAULT_INIT_WORD_COUNT);
}
void bigint_init_0(bigint_t* num) {
	bigint_init_n(num, 0);
}

static void bigint_add_(bigint_t* num1, bigint_t* num2, bigint_t* out, uint32_t min_size) { //ignores negatives
	uint32_t i;
	uint8_t carry = 0;
	for(i = 0; i < min_size; i++) {
		carry = _addcarry_u64(carry, num1->data[i], num2->data[i], out->data + i);
	}

	while(i < num1->size && carry) {
		carry = ((out->data[i] = num1->data[i] + 1) == 0);
		i++;
	}
	while(i < num1->size) {
		out->data[i] = num1->data[i];
		i++;
	}

	while(i < num2->size && carry) {
		carry = ((out->data[i] = num2->data[i] + 1) == 0);
		i++;
	}
	while(i < num2->size) {
		out->data[i] = num2->data[i];
		i++;
	}

	if(carry) {
		out->data[i] = 1;
	}else {
		out->size -= 1;
	}
}
static void bigint_sub_(bigint_t* num1, bigint_t* num2, bigint_t* out, uint32_t min_size) { //ignores negatives; num1 >= num2
	uint32_t i;
	uint8_t borrow = 0;
	for(i = 0; i < min_size; i++) {
		borrow = _subborrow_u64(borrow, num1->data[i], num2->data[i], out->data + i);
	}

	while(i < num1->size && borrow) {
		borrow = ((out->data[i] = num1->data[i] - 1) == 0xFFFFFFFFFFFFFFFF);
		i++;
	}
	while(i < num1->size) {
		out->data[i] = num1->data[i];
		i++;
	}

	//if(borrow) {
	//	out->data[i] = 0xFFFFFFFFFFFFFFFF;
	//}else {
	//	out->size -= 1;
	//}
}

void bigint_add(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t min_size = min(num1->size, num2->size);
	bigint_expand(out, max(num1->size, num2->size) + 1);
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		if(bigint_lesser(num1, num2)) {
			bigint_sub_(num2, num1, out, min_size);
			out->negative = num2->negative;
		}else {
			bigint_sub_(num1, num2, out, min_size);
			out->negative = num1->negative;
		}
	}else {
		bigint_add_(num1, num2, out, min_size);
		out->negative = num1->negative && num2->negative;
	}
#else
	bigint_add_(num1, num2, out, min_size);
#endif
	bigint_shrink(out);
}
void bigint_sub(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t min_size = min(num1->size, num2->size);
	bigint_expand(out, max(num1->size, num2->size));
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		bigint_add_(num1, num2, out, min_size);
		out->negative = num1->negative;
	}else {
		if(bigint_lesser(num1, num2)) {
			bigint_sub_(num2, num1, out, min_size);
			out->negative = !(num1->negative && num2->negative);
		}else {
			bigint_sub_(num1, num2, out, min_size);
			out->negative = num1->negative && num2->negative;
		}
	}
#else
	if(bigint_greater(num1, num2)) {
		bigint_sub_(num1, num2, out, min_size);
	}else {
		bigint_sub_(num2, num1, out, min_size);
	}
#endif
	bigint_shrink(out);
}
// https://www.codeproject.com/Articles/1276310/Multiple-Precision-Arithmetic-1st-Multiplication-Algorithm
static void bigint_mul_(bigint_t* num1, bigint_t* num2, bigint_t* out) { //ignores negatives
	uint64_t high, low;
	uint32_t k;
	uint8_t carry;
	for(uint32_t j = 0; j < num2->size; j++) {
		for(uint32_t i = 0; i < num1->size; i++) {
			k = i + j;
			low = _umul128(num1->data[i], num2->data[j], &high);
			carry = _addcarry_u64(0, out->data[k], low, out->data + k);
			if(carry || high){
				k++;
				carry = _addcarry_u64(carry, out->data[k], high, out->data + k);
				while(carry) {
					carry = ((out->data[++k] += 1) == 0);
				}
			}
		}
	}
}
void bigint_mul(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t size = num1->size + num2->size;
	bigint_expand(out, size);
	memset(out->data, 0, size * sizeof(uint64_t)); //maybe do something better than clean out number each time
	bigint_mul_(num1, num2, out);
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		out->negative = true;
	}
#endif
	bigint_shrink(out);
}
// https://www.codeproject.com/Articles/1276311/Multiple-Precision-Arithmetic-Division-Algorithm
void bigint_div(bigint_t* num1, bigint_t* num2, bigint_t* out) { //TODO: this
	assert(0);

	int cmp = bigint_cmp(num1, num2);
	if(cmp <= 0) {
		bigint_expand(out, 1);
		out->size = 1;
		out->data[0] = cmp == 0;
	}

	//bigint_expand(out, size);
	//
	//int i = BIGINT_WORD_COUNT - 1;
	//while(i > 0 && num1->data[i] == num2->data[i]) {
	//	i--;
	//}
	//if(num1->data[i] < num2->data[i]) { //num1 is lesser than num2, answer is 0
	//	return;
	//}
	//if(num1->data[i] == num2->data[i]) { // here i can only be 0, num1 and num2 are equal, answer is 1
	//	out->data[0] = 1;
	//	return;
	//}

	bigint_shrink(out);
}

void bigint_copy(bigint_t* num, bigint_t* out) {
	bigint_expand(out, num->size);
	for(uint32_t i = 0; i < num->size; i++) {
		out->data[i] = num->data[i];
	}
}

void bigint_lshift1(bigint_t* num, bigint_t* out) {
	uint32_t size = num->size + (num->data[num->size - 1] >= 0x8000000000000000);
	bigint_expand(out, size);
	num->data[size - 1] <<= 1;
	//for(int i = size - 2; i >= 0; i--) {
	for(uint32_t i = 2; i <= num->size; i++) {
		num->data[num->size - i + 1] |= (num->data[num->size - i] & 0x8000000000000000) > 0;
		num->data[num->size - i] <<= 1;
	}
}
void bigint_rshift1(bigint_t* num, bigint_t* out) {
	bigint_expand(out, num->size);
	num->data[0] >>= 1;
	for(uint32_t i = 1; i < num->size; i++) {
		num->data[i - 1] &= (num->data[i] << 63) | 0x7fffffffffffffff;
		num->data[i] >>= 1;
	}
	num->data[num->size - 1] &= 0x7fffffffffffffff;
}
void bigint_lshift(bigint_t* num1, uint64_t num2, bigint_t* out) {
	uint64_t offset = num2 / 64;
	uint32_t shift_val = num2 % 64;
	uint32_t inv_shift_val = 64 - shift_val;
	uint32_t size = num1->size + num2;
	bigint_expand(out, size);
	for(uint32_t i = 0; i < size; i++) {
		if(i + offset < size) {
			out->data[i + offset] |= num1->data[i] << shift_val;
			if(i + 1 + offset < size) {
				out->data[i + 1 + offset] |= (num1->data[i] >> inv_shift_val) & ((1 << shift_val) - 1);
			}
		}
	}
}
void bigint_rshift(bigint_t* num1, uint64_t num2, bigint_t* out) {
	uint64_t offset = num2 / 64;
	uint32_t shift_val = num2 % 64;
	uint32_t inv_shift_val = 64 - shift_val;
	uint32_t i;
	bigint_expand(out, num1->size);
	for(uint32_t index = 1; index <= num1->size; index++) {
		i = num1->size - index;
		if((int64_t)(i - offset) >= 0) {
			out->data[i - offset] |= (num1->data[i] >> shift_val) & ((1 << inv_shift_val) - 1);
			if((int64_t)(i - 1 - offset) >= 0) {
				out->data[i - 1 - offset] |= num1->data[i] << inv_shift_val;
			}
		}
	}
}

inline bool bigint_lesser(bigint_t* num1, bigint_t* num2) {
//#ifdef BIGINT_NEGATIVE
//	if(num1->negative != num2->negative) {
//		return num1->negative;
//	}
//#endif
//	if(num1->size < num2->size) {
//#ifdef BIGINT_NEGATIVE
//		return !(num1->negative && num2->negative);
//#else
//		return true;
//#endif
//	}else if(num1->size == num2->size) {
//		for(uint32_t i = 1; i <= num1->size; i++) {
//			if(num1->data[num1->size - i] < num2->data[num1->size - i]) {
//#ifdef BIGINT_NEGATIVE
//				return !(num1->negative && num2->negative);
//#else
//				return true;
//#endif
//			}
//		}
//	}
//#ifdef BIGINT_NEGATIVE
//	return num1->negative && num2->negative;
//#else
//	return false;
//#endif
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		return num1->negative;
	}
	if(num1->size < num2->size) {
		return !(num1->negative && num2->negative);
	}else if(num1->size == num2->size) {
		for(uint32_t i = 1; i <= num1->size; i++) {
			if(num1->data[num1->size - i] < num2->data[num1->size - i]) {
				return !(num1->negative && num2->negative);
			}
		}
	}
	return num1->negative && num2->negative;
#else
	if(num1->size < num2->size) {
		return true;
	}else if(num1->size == num2->size) {
		for(uint32_t i = 1; i <= num1->size; i++) {
			if(num1->data[num1->size - i] < num2->data[num1->size - i]) {
				return true;
			}
		}
	}
	return false;
#endif
}
inline bool bigint_greater(bigint_t* num1, bigint_t* num2) {
//#ifdef BIGINT_NEGATIVE
//	if(num1->negative != num2->negative) {
//		return num2->negative;
//	}
//#endif
//	if(num1->size > num2->size) {
//#ifdef BIGINT_NEGATIVE
//		return !(num1->negative && num2->negative);
//#else
//		return true;
//#endif
//	}else if(num1->size == num2->size) {
//		for(uint32_t i = 1; i <= num1->size; i++) {
//			if(num1->data[num1->size - i] > num2->data[num1->size - i]) {
//#ifdef BIGINT_NEGATIVE
//				return !(num1->negative && num2->negative);
//#else
//				return true;
//#endif
//			}
//		}
//	}
//#ifdef BIGINT_NEGATIVE
//	return num1->negative && num2->negative;
//#else
//	return false;
//#endif
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		return num2->negative;
	}
	if(num1->size > num2->size) {
		return !(num1->negative && num2->negative);
	}else if(num1->size == num2->size) {
		for(uint32_t i = 1; i <= num1->size; i++) {
			if(num1->data[num1->size - i] > num2->data[num1->size - i]) {
				return !(num1->negative && num2->negative);
			}
		}
	}
	return num1->negative && num2->negative;
#else
	if(num1->size > num2->size) {
		return true;
	}else if(num1->size == num2->size) {
		for(uint32_t i = 1; i <= num1->size; i++) {
			if(num1->data[num1->size - i] > num2->data[num1->size - i]) {
				return true;
			}
		}
	}
	return false;
#endif
}
inline bool bigint_eq(bigint_t* num1, bigint_t* num2) {
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		return false;
	}
#endif
	if(num1->size == num2->size) {
		for(uint32_t i = 1; i <= num1->size; i++) {
			if(num1->data[num1->size - i] != num2->data[num1->size - i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}
inline int bigint_cmp(bigint_t* num1, bigint_t* num2) {
//#ifdef BIGINT_NEGATIVE
//	if(num1->negative != num2->negative) {
//		return num1->negative ? -1 : 1;
//	}
//#endif
//	if(num1->size < num2->size) {
//#ifdef BIGINT_NEGATIVE
//		return num1->negative ? 1 : -1;
//#else
//		return -1;
//#endif
//	}else if(num1->size == num2->size) {
//		int64_t cmp;
//		for(uint32_t i = 1; i <= num1->size; i++) {
//			cmp = num1->data[num1->size - i] - num2->data[num1->size - i];
//			if(cmp != 0) {
//#ifdef BIGINT_NEGATIVE
//				return num1->negative ? (cmp < 0 ? 1 : -1) : (cmp < 0 ? -1 : 1);
//#else
//				return cmp < 0 ? -1 : 1;
//#endif
//			}
//		}
//		return 0;
//	}else {
//#ifdef BIGINT_NEGATIVE
//		return num1->negative ? -1 : 1;
//#else
//		return 1;
//#endif
//	}
#ifndef BIGINT_NONEGATIVE
	if(num1->negative != num2->negative) {
		return num1->negative ? -1 : 1;
	}
	if(num1->size < num2->size) {
		return num1->negative ? 1 : -1;
	}else if(num1->size == num2->size) {
		int64_t cmp;
		for(uint32_t i = 1; i <= num1->size; i++) {
			cmp = num1->data[num1->size - i] - num2->data[num1->size - i];
			if(cmp != 0) {
				return num1->negative ? (cmp < 0 ? 1 : -1) : (cmp < 0 ? -1 : 1);
			}
		}
		return 0;
	}else {
		return num1->negative ? -1 : 1;
	}
#else
	if(num1->size < num2->size) {
		return -1;
	}else if(num1->size == num2->size) {
		int64_t cmp;
		for(uint32_t i = 1; i <= num1->size; i++) {
			cmp = num1->data[num1->size - i] - num2->data[num1->size - i];
			if(cmp != 0) {
				return cmp < 0 ? -1 : 1;
			}
		}
		return 0;
	}else {
		return 1;
	}
#endif

}

#define copy_choose(num1, num2, out, i, max_size) \
	do {										  \
		if(num1->size > num2->size) {			  \
			for(; i < max_size; i++) {			  \
				out->data[i] = num1->data[i];	  \
			}									  \
		}else {									  \
			for(; i < max_size; i++) {			  \
				out->data[i] = num2->data[i];	  \
			}									  \
		}										  \
	} while(0)

void bigint_or(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t min_size = min(num1->size, num2->size);
	uint32_t max_size = max(num1->size, num2->size);
	bigint_expand(out, max_size);
	uint32_t i = 0;
	for(; i < min_size; i++) {
		out->data[i] = num1->data[i] | num2->data[i];
	}
	copy_choose(num1, num2, out, i, max_size);
}
void bigint_and(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t min_size = min(num1->size, num2->size);
	uint32_t max_size = max(num1->size, num2->size);
	bigint_expand(out, max_size);
	uint32_t i = 0;
	for(; i < min_size; i++) {
		out->data[i] = num1->data[i] & num2->data[i];
	}
	copy_choose(num1, num2, out, i, max_size);
}
void bigint_xor(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	uint32_t min_size = min(num1->size, num2->size);
	uint32_t max_size = max(num1->size, num2->size);
	bigint_expand(out, max_size);
	uint32_t i = 0;
	for(; i < min_size; i++) {
		out->data[i] = num1->data[i] ^ num2->data[i];
	}
	copy_choose(num1, num2, out, i, max_size);
}
void bigint_inv(bigint_t* num, bigint_t* out) {
	bigint_expand(out, num->size);
	for(uint32_t i = 0; i < num->size; i++) {
		out->data[i] = ~num->data[i];
	}
}
void bigint_neg(bigint_t* num, bigint_t* out) {
	bigint_expand(out, num->size);
	for(uint32_t i = 0; i < num->size; i++) {
		out->data[i] = ~num->data[i] + 1;
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
void bigint_from_int(int64_t num, bigint_t* out) {
	bigint_expand_to(out, 1);
#ifndef BIGINT_NONEGATIVE
	if(num < 0) {
		out->data[0] = -num;
		out->negative = true;
	}else {
		out->data[0] = num;
	}
#else
	out->data[0] = num;
#endif
	out->size = 1;
}
//void bigint_from_string(char* str, bigint_t* out) {
//	assert(0);
//	//bigint_init(out);
//
//}
void bigint_from_xstring(char* str, bigint_t* out) {
	bigint_init_default(out);
	uint32_t i = 0;
	uint64_t shift = 0;
	uint64_t tmp;
#ifndef BIGINT_NONEGATIVE
	if(*str == '-') {
		out->negative = true;
		str++;
	}
#endif
	if(*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X')) {
		str += 2;
	}
	while(isxdigit(*str)) {
		tmp = (uint64_t)hexchar_to_int(*(str++)) << shift;
		out->data[i] += tmp;
		shift = (shift + 4) % 64;
		i += shift == 0;
	}
}

int64_t bigint_to_int(bigint_t* num) {
#ifndef BIGINT_NONEGATIVE
	if(num->negative) {
		return -(int64_t)num->data[0];
	}
#endif
	return num->data[0];
}
int64_t bigint_to_int_greedy(bigint_t* num) {
	if(num->size > 1) {
		return 0xFFFFFFFFFFFFFFFF; //0x7FFFFFFFFFFFFFFF
	}
	return bigint_to_int(num);
}

//int bigint_to_string(bigint_t* num, char* out, int max_size) {
//	assert(0);
//
//}
int bigint_to_xstring(bigint_t* num, char* out, int max_size, int flag) {
	uint32_t tmp, written = 0;
#ifndef BIGINT_NONEGATIVE
	if(num->negative) {
		tmp = sprintf_s(out, max_size, "-");
		written += tmp;
		out += tmp;
		max_size -= tmp;
	}
#endif
	if(flag & BIGINT_FLAG_ADD0X) {
		tmp = sprintf_s(out, max_size, flag & BIGINT_FLAG_LARGE_LETTERS ? "0X" : "0x");
		written += tmp;
		out += tmp;
		max_size -= tmp;
	}
	if(num->size >= 1) {
		tmp = sprintf_s(out, max_size, flag & BIGINT_FLAG_LARGE_LETTERS ? "%llX" : "%llx", num->data[num->size - 1]);
		written += tmp;
		out += tmp;
		max_size -= tmp;
	}
	for(uint32_t i = 2; i <= num->size; i++) {
		tmp = sprintf_s(out, max_size, flag & BIGINT_FLAG_LARGE_LETTERS ? "%016llX" : "%016llx", num->data[num->size - i]);
		written += tmp;
		out += tmp;
		max_size -= tmp;
	}
	return written;
}

void bigint_print_hex(bigint_t* num, int flag) {
#ifndef BIGINT_NONEGATIVE
	if(num->negative) {
		printf("-");
	}
#endif
	if(flag & BIGINT_FLAG_ADD0X) {
		printf(flag & BIGINT_FLAG_LARGE_LETTERS ? "0X" : "0x");
	}
	if(num->size >= 1) {
		printf(flag & BIGINT_FLAG_LARGE_LETTERS ? "%llX" : "%llx", num->data[num->size - 1]);
	}
	for(uint32_t i = 2; i <= num->size; i++) {
		printf(flag & BIGINT_FLAG_LARGE_LETTERS ? "%016llX" : "%016llx", num->data[num->size - i]);
	}
}

