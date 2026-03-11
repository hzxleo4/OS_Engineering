# Makefile for the simple example kernel.
AS86	=as86 -0 -a
LD86	=ld86 -0
AS	=as
LD	=ld
LDFLAGS =-m elf_i386 -Ttext 0x80010000 -e startup_32 -s -x -M  

all:	Image

Image: boot system mytask mytask1 mytask2 mytask3 mytask4
	dd bs=32 if=boot of=Image skip=1
	objcopy -O binary system head
	cat head >> Image
	objcopy -O binary mytask task0
	dd bs=512 if=task0 of=Image seek=288
	objcopy -O binary mytask1 task1
	dd bs=512 if=task1 of=Image seek=289
	objcopy -O binary mytask2 task2
	dd bs=512 if=task2 of=Image seek=290
	objcopy -O binary mytask3 task3
	dd bs=512 if=task3 of=Image seek=291
	objcopy -O binary mytask4 task4
	dd bs=512 if=task4 of=Image seek=292


disk: Image
	dd bs=8192 if=Image of=/dev/fd0
	sync;sync;sync

mytask.o: mytask.c
	gcc -c -fno-pic -o mytask.o mytask.c  # -fno-pic is used to reduce the size of the object file; remove it is ok

mytask: mytask.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M mytask.o -o mytask > mytask.map

mytask1.o: mytask1.c
	gcc -c -fno-pic -o mytask1.o mytask1.c  # -fno-pic is used to reduce the size of the object file; remove it is ok	

mytask1: mytask1.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M mytask1.o -o mytask1 > mytask1.map

mytask2.o: mytask2.c
	gcc -c -fno-pic -o mytask2.o mytask2.c

mytask2: mytask2.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M mytask2.o -o mytask2 > mytask2.map

mytask3.o: mytask3.c
	gcc -c -fno-pic -o mytask3.o mytask3.c

mytask3: mytask3.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M mytask3.o -o mytask3 > mytask3.map

mytask4.o: mytask4.c
	gcc -c -fno-pic -o mytask4.o mytask4.c

mytask4: mytask4.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M mytask4.o -o mytask4 > mytask4.map


head.o: head.s

main.o: main.c
	gcc -c -fno-pic -o main.o main.c

kernel.o: kernel.c kernel.h type.h
	gcc -c -fno-pic -o kernel.o kernel.c

page.o: page.c page.h
	gcc -c -fno-pic -o page.o page.c

task.o: task.c task.h
	gcc -c -fno-pic -o task.o task.c

system:	head.o main.o kernel.o page.o task.o
	$(LD) $(LDFLAGS) head.o main.o kernel.o page.o task.o  -o system > System.map

boot:	boot.s
	$(AS86) -o boot.o boot.s
	$(LD86) -s -o boot boot.o

clean:
	rm -f Image System.map core boot head *.o system task0 mytask mytask.map task1 mytask1 mytask1.map task2 mytask2 mytask2.map task3 mytask3 mytask3.map task4 mytask4 mytask4.map
