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

irq_handler		irq_table[NR_IRQ];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];