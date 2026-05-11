#ifndef _KERNEL_H
#define _KERNEL_H
#include "type.h"
#include "proto.h"
/* system descriptor */
typedef struct s_descriptor		/* totally 8 byte */
{
	u16	limit_low;		/* Limit */
	u16	base_low;		/* Base */
	u8	base_mid;		/* Base */
	u8	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	u8	limit_high_attr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	u8	base_high;		/* Base */
}DESCRIPTOR;

/* gate descriptor */
typedef struct s_gate
{
	u16	offset_low;	/* Offset Low */
	u16	selector;	/* Selector */
	u8	dcount;		/* 该字段只在调用门描述符中有效。
				如果在利用调用门调用子程序时引起特权级的转换和堆栈的改变，需要将外层堆栈中的参数复制到内层堆栈。
				该双字计数字段就是用于说明这种情况发生时，要复制的双字参数的数量。 */
	u8	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */
	u16	offset_high;	/* Offset High */
}GATE;

typedef struct s_tss {
	u32	backlink;
	u32	esp0;		/* stack pointer to use during interrupt */
	u32	ss0;		/*   "   segment  "  "    "        "     */
	u32	esp1;
	u32	ss1;
	u32	esp2;
	u32	ss2;
	u32	cr3;
	u32	eip;
	u32	flags;
	u32	eax;
	u32	ecx;
	u32	edx;
	u32	ebx;
	u32	esp;
	u32	ebp;
	u32	esi;
	u32	edi;
	u32	es;
	u32	cs;
	u32	ss;
	u32	ds;
	u32	fs;
	u32	gs;
	u32	ldt;
	u16	trap;
	u16	iobase;	/* I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 */
	/*u8	iomap[2];*/
}TSS;




extern DESCRIPTOR gdt[256];
extern GATE idt[256];
extern u32 init_stack;
extern TSS tss0;
extern DESCRIPTOR ldt0[3];
extern u32 stack0_krn_ptr;
extern u32 stack1_krn_ptr;
extern u32 stack2_krn_ptr;
extern u32 new_task_next_ip;


void init_descriptor(DESCRIPTOR * p_desc, u32 base, u32 limit, u16 attribute);
void init_idt_desc(unsigned char vector, u8 desc_type, int_handler handler, unsigned char privilege);
void set_gdt_descriptor_base(DESCRIPTOR * p_desc, u32 base);
void set_ldt_base(DESCRIPTOR * p_desc, u32 base);

#endif