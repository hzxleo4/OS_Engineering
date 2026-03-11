# Makefile for the simple example kernel.
AS86	=as86 -0 -a
LD86	=ld86 -0
AS	=as
LD	=ld
LDFLAGS =-m elf_i386 -Ttext 0xC0010000 -e startup_32 -s -x -M  

all:	Image

Image: boot system mytask mytask1
	dd bs=32 if=boot of=Image skip=1
	objcopy -O binary system head
	cat head >> Image
	objcopy -O binary mytask task0
	dd bs=512 if=task0 of=Image seek=288
	objcopy -O binary mytask1 task1
	dd bs=512 if=task1 of=Image seek=289


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
	rm -f Image System.map core boot head *.o system task0 mytask mytask.map task1 mytask1 mytask1.map
