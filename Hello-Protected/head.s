#  head.s contains the 32-bit startup code.
#  Display "Hello" by dirctly writing them in the video frame memory
 
SCRN_SEL	= 0x18

.global startup_32
.text
startup_32:


	mov $SCRN_SEL, %ebx
	mov %bx,%ds
    movb $110,2900   # n
    movb $0x1e,2901
    movb $97,2902    # a
    movb $0x1e,2903
    movb $109,2904   # m
    movb $0x1e,2905
    movb $101,2906   # e
    movb $0x1e,2907
    movb $32,2908    # space
    movb $0x1e,2909
    movb $122,2910   # z
    movb $0x1e,2911
    movb $105,2912   # i
    movb $0x1e,2913
    movb $120,2914   # x
    movb $0x1e,2915
    movb $117,2916   # u
    movb $0x1e,2917
    movb $97,2918    # a
    movb $0x1e,2919
    movb $110,2920   # n
    movb $0x1e,2921
    movb $45,2922    # -
    movb $0x1e,2923
    movb $104,2924   # h
    movb $0x1e,2925
    movb $117,2926   # u
    movb $0x1e,2927
    movb $97,2928    # a
    movb $0x1e,2929
    movb $110,2930   # n
    movb $0x1e,2931
    movb $103,2932   # g
    movb $0x1e,2933

	
	
loop: jmp loop	






