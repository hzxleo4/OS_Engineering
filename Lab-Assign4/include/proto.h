/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "const.h"


/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void disable_int();
PUBLIC void enable_int();

/* keyboard.c */
PUBLIC void init_keyboard();
PUBLIC void keyboard_read();

/* main.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* klib.c */
PUBLIC void	delay(int time);
PUBLIC void disp_int(int input);