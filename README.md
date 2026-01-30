# C BigInt

## WIP arbitrary precision number library. Containing both big integers and big integer fractions

### Big Integers:
Addition, Substraction, Multiplication, Division, Modulo \
Power, Square Root, Log2, Gcd \
Right and Left bit Shifts, Comparison \
Base 10 and 16 Printing \
To and From Base 10 and 16 Strings \
To and From Integers

### Big Integers Fractions:
Addition, Substraction, Multiplication, Division \
Simplification \
Base 10 Decimal and Fractional Printing \
To and From Integers

## Example:
```c
#include <stdio.h>

#include "bigint.h"

int main(){
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

    return 0;
}
```
