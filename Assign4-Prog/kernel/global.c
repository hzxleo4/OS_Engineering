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
	sys_printx, 			// 3
	sys_sendrec, 			// 4
	sys_write_int_routine, 	// 5
	
	sys_hd_open,			// 6
	
	sys_init_fs, 			// 7
	sys_open,				// 8
	sys_close, 				// 9
	sys_read,				// 10
	sys_write,				// 11
	sys_print_task_paging,	// 12
	sys_fork,				// 13
	sys_get_pid, 			// 14
	};		

/* FS related below */
/*****************************************************************************/
/**
 * For dd_map[k],
 * `k' is the device nr.\ dd_map[k].driver_nr is the driver nr.
 *
 * Remeber to modify include/const.h if the order is changed.
 *****************************************************************************/
//struct dev_drv_map dd_map[] = {
	/* driver nr.		major device nr.
	   ----------		---------------- */
//	{INVALID_DRIVER},	/**< 0 : Unused */
//	{INVALID_DRIVER},	/**< 1 : Reserved for floppy driver */
//	{INVALID_DRIVER},	/**< 2 : Reserved for cdrom driver */
//	{TASK_HD},		/**< 3 : Hard disk */
//	{INVALID_DRIVER}	/**< 5 : Reserved for scsi disk driver */
//};

/**
 * 6MB~7MB: buffer for FS
 */
//PUBLIC	u8 *		fsbuf		= (u8*)0x600000;
//PUBLIC	const int	FSBUF_SIZE	= 0x100000;