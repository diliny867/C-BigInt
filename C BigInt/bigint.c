#include "bigint.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <intrin.h>
#include <string.h>


typedef uint8_t carry_t;
typedef carry_t borrow_t;


void bigint_init(bigint_t* num) {
	memset(num->data, 0, sizeof(uint64_t) * BIGINT_WORD_COUNT);
}
void bigint_add(const bigint_t* num1, const bigint_t* num2, bigint_t* out) {
	carry_t carry = 0;
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		carry = _addcarry_u64(carry, num1->data[i], num2->data[i], out->data + i);
	}
}
void bigint_sub(const bigint_t* num1, const bigint_t* num2, bigint_t* out) {
	borrow_t borrow = 0;
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		borrow = _subborrow_u64(borrow, num1->data[i], num2->data[i], out->data + i);
	}
}
void bigint_mul(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	bigint_init(out);
	uint64_t high, low;
	int k;
	carry_t carry;
	for(int j = 0; j < BIGINT_WORD_COUNT; j++) {
		for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
			k = i + j;
			low = _umul128(num1->data[i], num1->data[j], &high);
			carry = _addcarry_u64(0, out->data[k], low, out->data + k);
			k++;
			carry = _addcarry_u64(carry, out->data[k], high, out->data + k);
			while(carry) {
				carry = ((out->data[++k] += 1) == 0);
			}
		}
	}
}
void bigint_div(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	assert(0); //TODO: this
	
}

void bigint_copy(bigint_t* num, bigint_t* out) {
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		out->data[i] = num->data[i];
	}
}

void bigint_lshift(bigint_t* num1, uint64_t num2, bigint_t* out) {
	bigint_init(out);
	uint64_t offset = num2 / 64;
	uint32_t shift_val = num2 % 64;
	uint32_t inv_shift_val = 64 - shift_val;
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		if(i + offset < BIGINT_WORD_COUNT) {
			out->data[i + offset] |= num1->data[i] << shift_val;
			if(i + 1 + offset < BIGINT_WORD_COUNT) {
				out->data[i + 1 + offset] |= (num1->data[i] >> inv_shift_val) & ((1 << shift_val) - 1);
			}
		}
	}
}
void bigint_rshift(bigint_t* num1, uint64_t num2, bigint_t* out) {
	bigint_init(out);
	uint64_t offset = num2 / 64;
	uint32_t shift_val = num2 % 64;
	uint32_t inv_shift_val = 64 - shift_val;
	for(int i = BIGINT_WORD_COUNT - 1; i >= 0; i--) {
		if((int64_t)(i - offset) >= 0) {
			out->data[i - offset] |= (num1->data[i] >> shift_val) & ((1 << inv_shift_val) - 1);
			if((int64_t)(i - 1 - offset) >= 0) {
				out->data[i - 1 - offset] |= num1->data[i] << inv_shift_val;
			}
		}
	}
}
void bigint_or(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		out->data[i] = num1->data[i] | num2->data[i];
	}
}
void bigint_and(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		out->data[i] = num1->data[i] & num2->data[i];
	}
}
void bigint_xor(bigint_t* num1, bigint_t* num2, bigint_t* out) {
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		out->data[i] = num1->data[i] ^ num2->data[i];
	}
}
void bigint_inv(bigint_t* num, bigint_t* out) {
	for(int i = 0; i < BIGINT_WORD_COUNT; i++) {
		out->data[i] = ~num->data[i];
	}
}
void bigint_neg(bigint_t* num, bigint_t* out) {
	bigint_inv(num, out);
	carry_t carry;
	int i = 0;
	do {
		carry = (out->data[i++] += 1) == 0;
	} while(carry && i < BIGINT_WORD_COUNT);
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
void bigint_from_int(uint64_t val, bigint_t* out) {
	bigint_init(out);
	out->data[0] = val;
}
//void bigint_from_string(char* str, bigint_t* out) {
//	assert(0);
//	//bigint_init(out);
//
//}
void bigint_from_hexstring(char* str, bigint_t* out) { //TODO: detect 0x
	bigint_init(out);
	int i = 0;
	uint64_t shift = 0;
	uint64_t tmp;
	while(isxdigit(*str)) {
		tmp = (uint64_t)hexchar_to_int(*(str++)) << shift;
		out->data[i] += tmp;
		shift = (shift + 4) % 64;
		i += shift == 0;
	}
}

uint64_t bigint_to_int(bigint_t* num) {
	return num->data[0];
}
//int bigint_to_string(bigint_t* num, char* out, int max_size) {
//	assert(0);
//
//}
int bigint_to_hexstring(bigint_t* num, char* out, int max_size) {
	int written = 0;
	int seen_value = 0;
	for(int i = BIGINT_WORD_COUNT - 1; i >= 0; i--) {
		if(!seen_value && !num->data[i]){
			continue;
		}else {
			seen_value = 1;
		}
		written += sprintf_s(out, max_size, "%llx", num->data[i]);
		out += written;
		max_size -= written;
	}
	if(!seen_value){
		written += sprintf_s(out, max_size, "0");
	}
	return written;
}

