! a simple program that prints "hello world"

.globl begtext, begdata, begbss, endtext, enddata, endbss  ! Global id used for ld86 links
.text		! Text segment					
begtext:
.data		! Data segment
begdata:
.bss		! Uninitialized data segment
begbss:
.text		! Text segment


BOOTSEG  = 0x07c0			! Original address of boot-sector
HELLOWORLD = 0x0900				! This is the current segment address 


entry _start				! Inform the linker the program starts executing from here.
_start:
	mov ax,cs 				! The value of the segment register cs -->ax is used
	mov ds,ax 				! to initialize the data segment registers ds and es
	mov es,ax

	mov	ah,#0x03			! Read cursor pos
	xor	bh,bh				! Character display attribute: page 0
	int	0x10 				! The BIOS interrupt call 0x10, function 0x03
	
	mov	cx,#17				! Number of characters
	mov	bx,#0x0006			! Character display attribute: page 0, attribute 7 (normal)
	mov	bp,#msg1			! Point to a string
	mov	ax,#0x1301			! Write string, move cursor
	int	0x10 				! The BIOS interrupt call 0x10, function 0x13, subfunc 01


die: jmp die


msg1:
	.byte 13,10				! carriage return and line feed (13, 10) characters
	.ascii "Hello World"
	.byte 13,10,13,10


.text
endtext:
.data
enddata:
.bss
endbss:
