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
#include "string.h"

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
void pagefault_interrupt();

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
	current = task0;
	
	task0->pid = 0;
	task0->pg_dir = (pte_t)&pg_dir_tasks - PAGE_OFFSET;
	task0->sp0 = (u32)&stack0_krn_ptr;
	task0->sp = (u32)&stack0_krn_ptr;
	task0->ip = 0;
	task0->nr_tty = 0;
	task0->p_flags = 0;
	task0->p_parent = NO_TASK;
	task0->heap_start = HEAP_START;
	task0->heap_brk = HEAP_START;
	task0->heap_limit = HEAP_LIMIT;
	task0->stack_end = STACK_END;
	task0->stack_start = STACK_START;
	//task0->name = "init";
	strcpy(task0->name, "init");

	task1->pid = 1;
	task1->pg_dir = (pte_t)&pg_dir_tasks + 0x1000 - PAGE_OFFSET;
	task1->sp0 = (u32)&stack1_krn_ptr;
	task1->sp = (u32)&stack1_krn_ptr - sizeof(void *) * 5;
	task1->ip = (u32)&new_task_next_ip;
	task1->nr_tty = 0;
	task1->p_flags = 0;
	//task1->p_flags = 0;
	task1->p_parent = NO_TASK;
	strcpy(task1->name, "second");
	task1->heap_start = HEAP_START;
	task1->heap_brk = HEAP_START;
	task1->heap_limit = HEAP_LIMIT;
	task1->stack_end = STACK_END;
	task1->stack_start = STACK_START;
	pcaller = task0;
}

void kernel_main() {
	init_8259A();
  	init_8253();

	// set idt descriptors
	init_idt_desc(0x80, DA_386IGate, systemcall, PRIVILEGE_USER);

	init_idt_desc(0x0E, DA_386IGate, pagefault_interrupt, PRIVILEGE_KRNL);
	init_idt_desc(0x20, DA_386IGate, timer_interrupt, PRIVILEGE_KRNL);
  	init_idt_desc(0x21, DA_386IGate, keyboard_interrupt, PRIVILEGE_KRNL);	
	init_idt_desc(0x2E, DA_386IGate, harddisk_interrupt, PRIVILEGE_KRNL);

	// set display descriptor in GDT
	set_gdt_descriptor_base(&gdt[3], 0xb8000 + PAGE_OFFSET);

	
	// set ldt descriptor in GDT (shared by all tasks)
	init_descriptor(&gdt[4], (u32)&ldt0, 3*sizeof(DESCRIPTOR)-1,DA_LDT);

	// set tss descriptor in GDT (shared by all tasks)
	init_descriptor(&gdt[5], (u32)&tss0, sizeof(tss0)-1, DA_386TSS);
	
	for (int i = 0; i < 2; i++){
		set_task_paging(i);
	}
	init_tasks();
  	
	//pcaller = &tasks[0];
	disp_pos = 0;
	free_phys_page = FREE_PHYS_PAGE;
  	//init_keyboard();
  	init_task_tty();
  	//enable_irq(CLOCK_IRQ);
  	//enable_irq(KEYBOARD_IRQ);
	//sys_init_hd();
	//get_mem_info();

	move_to_user_mode();
	while (1) {}
}

