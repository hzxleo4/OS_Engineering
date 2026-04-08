#include "type.h"
#include "stdio.h"

void* malloc(u32 size);
void free(void *ptr);

void main() {
    int *p1 = (int*)malloc(100);
    int *p2 = (int*)malloc(200);

    if (p1) {
        p1[0] = 111;
        p1[1] = 222;
        printx("p1 ok\n");
    }

    if (p2) {
        p2[0] = 333;
        p2[1] = 444;
        printx("p2 ok\n");
    }

    free(p1);
    printx("free p1 ok\n");

    int *p3 = (int*)malloc(80);
    if (p3) {
        p3[0] = 555;
        printx("p3 reuse ok\n");
    }

    while (1) { }
}