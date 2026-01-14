#  head.s contains the 32-bit startup code.
#  Display "Hello" by dirctly writing them in the video frame memory
 
SCRN_SEL	= 0x18

.global startup_32
.text
startup_32:


	mov $SCRN_SEL, %ebx
	mov %bx,%ds
	movb $72,2900
	movb $0x1e,2901
	movb $69,2902
	movb $0x1e,2903
	movb $76,2904
	movb $0x1e,2905
	movb $76,2906
	movb $0x1e,2907
	movb $79,2908
	movb $0x1e,2909
	
	
loop: jmp loop	






