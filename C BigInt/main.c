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

#define PRINTNUMS_ALL() do { printf("\n"); PRINTNUMS(num1, num2, num3, num4, num5, num6, BI_ADD0X | BI_PRINT_COUNT); }while(0)

void print_bigint(bigint_t num, int flag, char* label) {
    printf("%s: %3d %3u  dec: ", label, num.size, num.capacity);
    bigint_print(num, flag);
    printf("  hex: ");
    bigint_print_hex(num, flag);
    printf("\n");
}

void test_bigints() {
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

    bigint_from_int(-0x135, &num3);

    PRINTNUMS_ALL();

    bigint_mul(num1, num3, &num4);

    PRINTNUMS_ALL();

    bigint_mul(num1, num4, &num3);

    PRINTNUMS_ALL();


    bigint_from_int(0x11, &num2);
    bigint_div(num3, num2, &num4, &num5);

    PRINTNUMS_ALL();

    bigint_sqrt(num3, &num5, 0);
    bigint_sqrt(num4, &num6, 0);

    PRINTNUMS_ALL();

    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);
    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);
    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);
    bigint_mul(num3, num3, &num4);
    bigint_mul(num4, num4, &num3);

    bigint_sqrt(num4, &num5, 0);

    PRINTNUMS_ALL();

    bigint_from_string("-207428133789596931991798422905", &num5);
    bigint_from_xstring("-0x29e3c87e076f96884328a1d79", &num4);

    char buf1[256], buf2[256];
    bigint_to_string(num5, buf1, 256);
    bigint_to_xstring(num4, buf2, 256, BI_ADD0X);

    PRINTNUMS_ALL();
    printf("num5  s: %s\n", buf1);
    printf("num4 xs: %s\n", buf2);


    bigint_from_int(27, &num1);
    bigint_from_int(360, &num2);

    bigint_gcd(num1, num2, &num6);

    //bigint_pow(num4, num2, &num5);

    PRINTNUMS_ALL();


    bigint_destroy(&num1);
    bigint_destroy(&num2);
    bigint_destroy(&num3);
    bigint_destroy(&num4);
    bigint_destroy(&num5);
    bigint_destroy(&num6);
}

void example(){
    bigint_t num1, num2, num3;
    // alternatively can be zero initialized (but you still will need to bigint_destroy them)
    bigint_init(&num1); 
    bigint_init(&num2);
    bigint_init(&num3);

    // set values
    bigint_from_int(0x61287c568192, &num1);
    bigint_from_string("7298570923875238950239857290387523759237905820395870293572398572390857023895", &num2);

    // num3 = num1 * num2
    bigint_mul(num1, num2, &num3);

    // num3 = num3 - num2
    bigint_sub(num3, num2, &num3);

    printf("Integers:\n\n");

    printf("num1: ");
    bigint_print(num1, 0);
    printf("\nnum2: ");
    bigint_print(num2, 0);
    printf("\nnum3: ");
    bigint_print(num3, 0);

    bigint_destroy(&num1); 
    bigint_destroy(&num2);
    bigint_destroy(&num3);

    printf("\n\nFractions:\n\n");

    bigintf_t numf1, numf2, numf3;
    bigintf_init(&numf1);
    bigintf_init(&numf2);
    bigintf_init(&numf3);

    bigintf_from_uint(179865796402395673, 231677809364, &numf1);
    bigintf_from_uint(5292035895, 62397852395, &numf2);

    printf("numf1: ");
    bigintf_print(numf1, 0, 0);
    printf("\nnumf2: ");
    bigintf_print(numf2, 0, 0);

    bigintf_mul(numf1, numf2, &numf3);
    bigintf_mul(numf1, numf3, &numf2);
    bigintf_mul(numf2, numf3, &numf1);
    bigintf_mul(numf1, numf2, &numf3);

    printf("\nnumf2: ");
    bigintf_print(numf1, 0, 0);

    printf("\nnumf2: ");
    bigintf_print(numf2, 0, 0);

    printf("\nnumf3: ");
    bigintf_print(numf3, 0, 0);
    printf("\nnumf3 as decimal (with 200 max digits after dot): ");
    bigintf_print(numf3, 200, BIF_AS_DECIMAL);

    bigintf_destroy(&numf1); 
    bigintf_destroy(&numf2);
    bigintf_destroy(&numf3);
}


void test_bigint_fractions() {
    bigintf_t num1,num2, num3;
    bigintf_init(&num1);
    bigintf_init(&num2);
    bigintf_init(&num3);

    bigintf_from_uint(179865, 67, &num1);
    bigintf_from_uint(52, 6, &num2);

    bigintf_print(num1, 0, 0);
    printf("\n");
    bigintf_print(num2, 0, 0);
    printf("\n");

    bigintf_mul(num1, num2, &num3);

    bigintf_print(num3, 0, 0);
    printf("\n");
    bigintf_print(num3, 200, BIF_AS_DECIMAL);
    printf("\n");
    printf("\n");

    bigintf_from_f64(123425789346578.235, &num3);

    printf("printed:   ");
    bigintf_print(num3, 200, BIF_AS_DECIMAL);
    printf("\n");

    char buf[1000];
    bigintf_to_string(num3, buf, 1000, 200, BIF_AS_DECIMAL);
    printf("to string: %s\n", buf);

    double f = bigintf_to_f64(num3);
    printf("to double: %f", f);

    ///bigint_from_uint(1, &num3.denominator);

    //bigintf_mul(num3, num3, &num2);
    //bigintf_mul(num2, num2, &num3);

    //bigintf_mul(num3, num3, &num2);
    //bigintf_mul(num2, num2, &num3);

    //bigintf_mul(num3, num3, &num2);
    //bigintf_mul(num2, num2, &num3);

    //bigintf_print(num3, 0, 0);
    //printf("\n");
    //bigintf_print(num3, 200, BIF_AS_DECIMAL);
    //printf("\n");
}

int main(int argc, char** argv) {

    //test_bigints();

    test_bigint_fractions();

    //example();


    return 0;
}