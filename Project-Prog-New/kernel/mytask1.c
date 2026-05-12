
#include "type.h"
#include "stdio.h"

void main() {
	for (int i = 0; i < 40960000; i++) {}  // act as a delay
	
	//printx("\n\nTASK1 Access the FS now...");
	//int fd = 0;
	//fd = open("/def", O_CREAT | O_RDWR);
	//printx("\nAfter open, the fd is ");
	//int_printer(fd);

	//int ret = 0;
	//int write_cnt = 6;
	//const char bufw [] = "ghijkl"; 

	//ret = write(fd, bufw, write_cnt);
	//printx("\nAfter write, ret is ");
	//int_printer(ret);
	//fsync();
	//ret = close(fd);

	//fd = open("/def", O_RDWR);

	//int read_cnt = 3;
	//char bufr[6];
	//ret = 0;
	//ret = read(fd, bufr, read_cnt);
	//printx("\nAfter read, ret is ");
	//int_printer(ret);	
	
	//ret = 0;
	//ret = close(fd);

	while (1) {
		for (int i = 0; i < 4096000; i++) {}  // act as a delay
		//printx("+");
	}
}