#include "type.h"
#include "stdio.h"

void main() {
	printx("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	printx("\n Before fork: Parent is running; PID:");
	int_printer(get_pid());

	int ret;
	ret = fork(); //fork


	printx("\n\n After fork: ret value is ");
	int_printer(ret);

	if (ret){
		printx("\n After fork: Parent-spinning; PID:");
		int_printer(get_pid());
		while (1) {}
	} else if (ret ==0) {	// child process
		printx("\n After fork: Child-spinning; PID:");
		int_printer(get_pid());
		while (1) {}
	}else{
		printx("\n fork fail.");
		while (1) {}
	}
}
