
#include "kernel.h"

void main() {
	while (1) {
		for (int i = 0; i < 4096; i++) {}  // act as a delay
		syscall_char_printer('B');  // call our syscall, now it only print a character
	}
	// while (1) {

	// }
}