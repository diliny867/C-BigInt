#include <stdlib.h>
#include <stdio.h>

#include "bigint.h"

#define PRINTNUMS(num1, num2, num3, num4, num5, num6, FLAG)    \
    do {                                                       \
        print_bigint(num1, FLAG, #num1);                       \
        print_bigint(num2, FLAG, #num2);                       \
        print_bigint(num3, FLAG, #num3);                       \
        print_bigint(num4, FLAG, #num4);                       \
        print_bigint(num5, FLAG, #num5);                       \
        print_bigint(num6, FLAG, #num6);                       \
    } while(0)

#define PRINTNUMS_ALL() do { printf("\n"); PRINTNUMS(num1, num2, num3, num4, num5, num6, BIF_ADD0X | BIF_PRINT_COUNT); }while(0)

void print_bigint(bigint_t num, int flag, char* label) {
    printf("%s: %d %u ", label, num.size, num.capacity);
    bigint_print(num, flag);
    printf(" h: ");
    bigint_print_hex(num, flag);
    printf("\n");
}

int main(int argc, char** argv) {
    bigint_t num1, num2, num3, num4, num5, num6;
    bigint_init(&num1);
    bigint_init(&num2);
    bigint_init(&num3);
    bigint_init(&num4);
    bigint_init(&num5);
    bigint_init(&num6);

    bigint_from_int(0x61287c568192, &num1);
    bigint_from_int(0x12e70219, &num2);

    bigint_from_int(0x123, &num2);
    bigint_from_int(0x11, &num3);

    PRINTNUMS_ALL();

    bigint_sub(num2, num3, &num2);

    PRINTNUMS_ALL();

    bigint_from_int(0x135, &num3);

    PRINTNUMS_ALL();

    bigint_mul(num1, num3, &num4);

    PRINTNUMS_ALL();

    bigint_mul(num1, num4, &num3);

    PRINTNUMS_ALL();


    bigint_from_int(0x11, &num2);
    bigint_div(num3, num2, &num4, &num5);

    PRINTNUMS_ALL();

    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);
    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);

    PRINTNUMS_ALL();

    /*
    bigint_add(num1, num2, &num3);

    bigint_mul(num3, num2, &num4);
    PRINTNUMS_ALL();

    bigint_mul(num4, num4, &num5);
    bigint_mul(num5, num5, &num4);
    PRINTNUMS_ALL();

    bigint_sub(num2, num5, &num6);
    PRINTNUMS_ALL();

    //bigint_sub(&num6, &num6, &num6);
    //PRINTNUMS_ALL();

    bigint_mul(num4, num4, &num1);
    PRINTNUMS_ALL();
    bigint_mul(num1, num1, &num4);
    PRINTNUMS_ALL();
    bigint_mul(num4, num4, &num1);
    PRINTNUMS_ALL();
    bigint_mul(num1, num1, &num4);
    PRINTNUMS_ALL();


    bigint_expand(&num2, 4);
    num2.size = 4;
    for(uint32_t i = 0; i < num2.size; i++) {
        num2.data[i] = 0;
    }
    num2.data[1] = 1;
    num2.data[2] = 1;
    num2.data[num2.size - 1] = 1;
    bigint_from_int(0x1, &num3);
    PRINTNUMS_ALL();

    bigint_sub(num2, num3, &num4);
    PRINTNUMS_ALL();

    bigint_add(num4, num3, &num4);
    PRINTNUMS_ALL();

    bigint_add(num4, num4, &num4);
    PRINTNUMS_ALL();

    bigint_sub(num4, num4, &num4);
    PRINTNUMS_ALL();

    //bigint_mul(&num1, &num1, &num5);
    //PRINTNUMS_ALL();
    //bigint_mul(&num5, &num5, &num1);
    //PRINTNUMS_ALL();
    //bigint_mul(&num1, &num1, &num5);
    //PRINTNUMS_ALL();
    //bigint_mul(&num5, &num5, &num1);
    //PRINTNUMS_ALL();
    //bigint_mul(&num1, &num1, &num5);
    //PRINTNUMS_ALL();
*/
    bigint_destroy(&num1);
    bigint_destroy(&num2);
    bigint_destroy(&num3);
    bigint_destroy(&num4);
    bigint_destroy(&num5);
    bigint_destroy(&num6);

    return 0;
}