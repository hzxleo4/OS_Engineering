AS86	=as86 -0 -a
LD86	=ld86 -0

all:	Image
Image:	bootsector.s helloworld.s
	$(AS86) -o boot.o bootsector.s
	$(LD86) -s -o boot boot.o
	dd bs=1 if=boot of=Image skip=32
	$(AS86) -o helloworld.o helloworld.s
	$(LD86) -s -o helloworld helloworld.o
	dd bs=1 if=helloworld of=HelloWorldImage skip=32
	cat HelloWorldImage >> Image

clean: 
	rm boot.o
	rm boot
	rm helloworld.o
	rm helloworld
	rm Image
	rm HelloWorldImage

