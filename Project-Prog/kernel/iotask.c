#include "type.h"
#include "stdio.h"

void main() {
	while (1) {
		for (int i = 0; i < 1024000; i++) {}  // act as a delay
		//printx("2");
		hd_routine();
	}
}