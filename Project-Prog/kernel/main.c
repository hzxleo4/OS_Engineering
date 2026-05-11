#include "type.h"
#include "kernel.h"
#include "page.h"
#include "task.h"
#include "keyboard.h"
#include "const.h"
#include "proto.h"
#include "tty.h"
#include "console.h"
#include "global.h"

#define	DA_LDT			0x82
#define	DA_386TSS		0x89
#define	DA_386TGate		0x8F
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/

#define	PRIVILEGE_USER	3
#define PRIVILEGE_KRNL 0
#define KERNEL_BASE     0x10000

/* 中断向量 */
#define	INT_VECTOR_IRQ0			0x20
#define	INT_VECTOR_IRQ8			0x28

void move_to_user_mode();
void systemcall();
void timer_interrupt();
void keyboard_interrupt();
void harddisk_interrupt();
//void get_mem_info();

static void init_8259A(void) {
    out_byte(0x20, 0x11);
    out_byte(0xa0, 0x11);

    out_byte(0x21, 0x20);
    out_byte(0xa1, 0x28);

    out_byte(0x21, 0x4);
    out_byte(0xa1, 0x2);

    out_byte(0x21, 0x1);
    out_byte(0xa1, 0x1);

    out_byte(0x21, 0xff);
    out_byte(0xa1, 0xff);

}


/*======================================================================*
                           put_irq_handler
 *======================================================================*/

static void init_8253(void) {
    int freq = 100;
    out_byte(0x43, 0x34);
    out_byte(0x40, (u8)(1193182L/freq));
    out_byte(0x40, (u8)((1193182L/freq) >> 8));
}


void init_tasks() {
	TASK* task0 = &tasks[0];
	TASK* task1 = &tasks[1];
	TASK* task2 = &tasks[2];
	current = task0;
	
	task0->pid = 0;
	task0->pg_dir = (pte_t)&pg_dir_tasks - PAGE_OFFSET;
	task0->sp0 = (u32)&stack0_krn_ptr;
	task0->sp = (u32)&stack0_krn_ptr;
	task0->ip = 0;
	task0->nr_tty = 0;
	task0->p_flags = READY;

	task1->pid = 1;
	task1->pg_dir = (pte_t)&pg_dir_tasks + 0x1000 - PAGE_OFFSET;
	task1->sp0 = (u32)&stack1_krn_ptr;
	task1->sp = (u32)&stack1_krn_ptr - sizeof(void *) * 5;
	task1->ip = (u32)&new_task_next_ip;
	task1->nr_tty = 0;
	task1->p_flags = READY;

	task2->pid = 2;
	task2->pg_dir = (pte_t)&pg_dir_tasks + 0x2000 - PAGE_OFFSET;
	task2->sp0 = (u32)&stack2_krn_ptr;
	task2->sp = (u32)&stack2_krn_ptr - sizeof(void *) * 5;
	task2->ip = (u32)&new_task_next_ip;
	task2->nr_tty = 0;
	task2->p_flags = READY;
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
//PUBLIC void panic(const char *fmt, ...)
//{
//	int i;
//	char buf[256];
//
//	/* 4 is the size of fmt in the stack */
//	va_list arg = (va_list)((char*)&fmt + 4);
//
//	i = vsprintf(buf, fmt, arg);
//
//	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);
//
//	/* should never arrive here */
//	__asm__ __volatile__("ud2");
//}

void kernel_main() {

	init_8259A();
  	init_8253();

	// set idt descriptors
	init_idt_desc(0x80, DA_386IGate, systemcall, PRIVILEGE_USER);
  	//init_idt_desc(0x81, DA_386IGate, systemcall_interrupt_1, PRIVILEGE_USER);
  	//init_idt_desc(0x82, DA_386IGate, systemcall_interrupt_2, PRIVILEGE_USER);
  	//init_idt_desc(0x83, DA_386IGate, systemcall_interrupt_3, PRIVILEGE_USER);
  	//init_idt_desc(0x84, DA_386IGate, systemcall_interrupt_4, PRIVILEGE_USER);
  	//init_idt_desc(0x85, DA_386IGate, systemcall_interrupt_5, PRIVILEGE_USER);
  	//init_idt_desc(0x86, DA_386IGate, systemcall_interrupt_6, PRIVILEGE_USER);
  	//init_idt_desc(0x87, DA_386IGate, systemcall_interrupt_7, PRIVILEGE_USER);

	init_idt_desc(0x20, DA_386IGate, timer_interrupt, PRIVILEGE_KRNL);
  	init_idt_desc(0x21, DA_386IGate, keyboard_interrupt, PRIVILEGE_KRNL);
	
	init_idt_desc(0x2E, DA_386IGate, harddisk_interrupt, PRIVILEGE_KRNL);

	// set display descriptor in GDT
	set_gdt_descriptor_base(&gdt[3], 0xb8000 + PAGE_OFFSET);

	
	// set ldt descriptor in GDT (shared by all tasks)
	init_descriptor(&gdt[4], (u32)&ldt0, 3*sizeof(DESCRIPTOR)-1,DA_LDT);

	// set tss descriptor in GDT (shared by all tasks)
	init_descriptor(&gdt[5], (u32)&tss0, sizeof(tss0)-1, DA_386TSS);
	
	//set_task0_paging();
	//set_page_directory(&pg_dir_task0, (void*)pg0_task0, (void*)pg1);

	//init_task_tty();
	//set_task1_paging();
	//set_page_directory(&pg_dir_task1, (void*)pg0_task1, (void*)pg1);
	for (int i = 0; i < 3; i++){
		set_task_paging(i);
	}

	//sys_printx("Set up paging for pid [");
	//sys_write_int_routine(1);
	//sys_printx("]\n");


	init_tasks();
  	disp_pos = 0;
  	//init_keyboard();
  	init_task_tty();
  	enable_irq(CLOCK_IRQ);
  	enable_irq(KEYBOARD_IRQ);
	sys_init_hd();
	//get_mem_info();
	//sys_init_fs();

	move_to_user_mode();
	while (1) {}
}


