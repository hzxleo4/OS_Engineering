#include "type.h"
#include "stdio.h"

void* malloc(u32 size);

void main() {
	int *p1 = malloc(123);
	if(p1){
		p1[120] = 123;
	}

	volatile unsigned int *invalid_addr2 = (volatile unsigned int *)0x4100;
	*invalid_addr2 = 456;

	while (1) {
		for (int i = 0; i < 4096000; i++) {}  // act as a delay
	}
}
