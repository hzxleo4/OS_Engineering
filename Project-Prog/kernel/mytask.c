
#include "type.h"
#include "stdio.h"

void main() {
	for (int i = 0; i < 4096000; i++) {}  // act as a delay
	printx("\n\n\n\n\n\n");
	printx("TASK0 Access the FS now...");

	init_fs();


	mkdir("/a");
	mkdir("/a/ab");
	mkdir("/a/ac");
	mkdir("/a/ab/bbb");
	mkdir("/a/ac/ccc");

	int fd = 0;
	fd = open("/a/ab/abc", O_CREAT | O_RDWR);
	//printx("\nAfter open, the fd is ");
	int_printer(fd);

	int ret = 0;
	int write_cnt = 6;
	const char bufw [] = "abcdef"; 
	printx("\nBefore write, the buf is ");
	printx(bufw);
	ret = write(fd, bufw, write_cnt);
	printx("\nAfter write, ret is ");
	int_printer(ret);
	fsync();
	ret = close(fd);

	fd = open("/a/ab/abc", O_RDWR);

	int read_cnt = 3;
	char bufr[6];
	ret = 0;
	ret = read(fd, bufr, read_cnt);
	
	printx(bufr);
	
	ret = 0;
	ret = close(fd);

	print_dir_tree();

	while (1) {
		for (int i = 0; i < 4096000; i++) {}  // act as a delay
		//printx(".");
	}
}
