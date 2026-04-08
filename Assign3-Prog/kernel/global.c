/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "global.h"
#include "proto.h"
#include "tty.h"
#include "console.h"
#include "fs.h"

irq_handler		irq_table[NR_IRQ];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {	
	sys_write_char_routine,	// 0
	sys_tty_routine, 		// 1
	sys_tty_write, 			// 2
	user_printx, 			// 3
	sys_sendrec, 			// 4
	sys_write_int_routine, 	// 5
	
	sys_hd_open,			// 6
	
	sys_init_fs, 			// 7
	sys_open,				// 8
	sys_close, 				// 9
	sys_read,				// 10
	sys_write,				// 11
	sys_print_task_paging,	// 12
	sys_brk					// 13  -- This is for syscall related to brk()
	};		