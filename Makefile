
example: main.c bigint.c bigint.h
	gcc main.c bigint.c -o example -O2 -std=c99 -Wall -Wno-free-nonheap-object -lm