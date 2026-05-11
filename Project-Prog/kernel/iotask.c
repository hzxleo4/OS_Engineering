#include "type.h"
#include "stdio.h"

void main() {
	while (1) {
		for (int i = 0; i < 1000; i++) {}  // keep a tiny delay but service requests quickly
		//printx("2");
		hd_routine();
	}
}