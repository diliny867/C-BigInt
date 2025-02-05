#include <stdlib.h>
#include <stdio.h>

#include <immintrin.h>
#include <intrin.h>
#include <intrin0.inl.h>
#include <stdint.h>


int main(int argc, char** argv) {
	//uint32_t a = INT32_MAX - 11;
	//uint32_t b = 10;
	//uint32_t res1;
	//uint32_t res2;
	//res2 = _mulx_u32(a, b, &res1);
	//printf("a:%u b:%u res1:%u res2:%u *:%llu res:%llu\n", a, b, res1, res2, (uint64_t)a*(uint64_t)b, ((uint64_t)res1<<32) + (uint64_t)res2);

	//uint32_t a = 44;
	//uint32_t b = 10;
	//uint32_t c;
	//c = _addcarry_u32(1, a, b, &b);
	//printf("%u %u", a, b);

	//uint32_t a = 1u << 30;
	//uint32_t b = 2;
	//uint32_t res1;
	//uint32_t res2;
	//res1 = _shlx_u32(a, b);
	//printf("%d\n", res1);

	printf("%u", _lzcnt_u32(13));

	return 0;
}