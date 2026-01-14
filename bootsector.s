! a simple booting program that displays some messages

.globl begtext, begdata, begbss, endtext, enddata, endbss  ! Global id used for ld86 links
.text		! Text segment					
begtext:
.data		! Data segment
begdata:
.bss		! Uninitialized data segment
begbss:
.text		! Text segment


BOOTSEG  = 0x07c0			! Original address of boot-sector


entry _start				! Inform the linker the program starts executing from here.
_start:
	jmpi	go,BOOTSEG 		! Jump between segments. BOOTSEG indicates the jump
							! section address, the label go is the offset address.


! Print some messages

go:	
	mov ax,cs 				! The value of the segment register cs -->ax is used
	mov ds,ax 				! to initialize the data segment registers ds and es
	mov es,ax

	mov	ah,#0x03			! Read cursor pos
	xor	bh,bh				! Character display attribute: page 0
	int	0x10 				! The BIOS interrupt call 0x10, function 0x03
	
	mov	cx,#24				! Number of characters
	mov	bx,#0x0007			! Character display attribute: page 0, attribute 7 (normal)
	mov	bp,#msg1			! Point to a string
	mov	ax,#0x1301			! Write string, move cursor
	int	0x10 				! The BIOS interrupt call 0x10, function 0x13, subfunc 01

loop1: 
	jmp loop1				! Infinite loop

msg1:
	.byte 13,10				! carriage return and line feed (13, 10) characters
	.ascii "Loading: Hello ..."
	.byte 13,10,13,10

.org 510					! Indicates statement is stored from address 510 (0x1FE).
boot_flag:
	.word 0xAA55			! Active boot sector flag, used by the BIOS. It must be the last two bytes of the boot sector.

.text
endtext:
.data
enddata:
.bss
endbss:
