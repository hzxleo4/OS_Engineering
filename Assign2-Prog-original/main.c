#include "type.h"
#include "kernel.h"
#include "page.h"
#include "task.h"

#define	DA_LDT			0x82
#define	DA_386TSS		0x89
#define	DA_386TGate		0x8F
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/

#define	PRIVILEGE_USER	3
#define PRIVILEGE_KERNEL 0
#define KERNEL_BASE     0x10000


void move_to_user_mode();
void systemcall_interrupt();
void timer_interrupt();


static inline u8 in_byte(u16 port) {
    u8 result;
    asm volatile ("inb %w1, %b0" : "=a"(result) : "d"(port));
    return result;
}

static inline void out_byte(u16 port, u8 val) {
    asm volatile ("outb %b0, %w1" : : "a"(val), "d"(port));
}

static void init_8259A(void) {
    out_byte(0x20, 0x11);
    out_byte(0xa0, 0x11);

    out_byte(0x21, 0x20);
    out_byte(0xa1, 0x28);

    out_byte(0x21, 0x4);
    out_byte(0xa1, 0x2);

    out_byte(0x21, 0x1);
    out_byte(0xa1, 0x1);

    out_byte(0x21, ~0x5);
    out_byte(0xa1, 0xff);
}

static void init_8253(void) {
    int freq = 100;
    out_byte(0x43, 0x34);
    out_byte(0x40, (u8)(1193182L/freq));
    out_byte(0x40, (u8)((1193182L/freq) >> 8));
}

// TSS test_tss = {
// 	.backlink = 19,
// 	.esp0 = 4,
// 	.ss0 = 2,
// };

void init_tasks() {
	TASK* task0 = &tasks[0];
	TASK* task1 = &tasks[1];
	current = task0;
	task0->pid = 0;
	task0->pg_dir = (pte_t)&pg_dir_task0 - PAGE_OFFSET;
	task0->sp0 = (u32)&stack0_krn_ptr;
	task0->sp = (u32)&stack0_krn_ptr;
	task0->ip = 0;

	task1->pid = 1;
	task1->pg_dir = (pte_t)&pg_dir_task1 - PAGE_OFFSET;
	// task1->sp0 = (u32)&stack1_krn_ptr;
	// task1->ip = 0;
	task1->sp0 = (u32)&stack1_krn_ptr;
	task1->sp = (u32)&stack1_krn_ptr - sizeof(void *) * 10;
	task1->ip = (u32)&new_task_next_ip;

}

void kernel_main() {

	init_8259A();
    	init_8253();

	// set idt descriptors
	init_idt_desc(0x80, DA_386IGate, systemcall_interrupt, PRIVILEGE_USER);
	init_idt_desc(0x20, DA_386IGate, timer_interrupt, PRIVILEGE_KERNEL);

	// set display descriptor in GDT
	set_gdt_descriptor_base(&gdt[3], 0xb8000 + PAGE_OFFSET);

	
	// set ldt descriptor in GDT (shared by all tasks)
	init_descriptor(&gdt[4], (u32)&ldt0, 3*sizeof(DESCRIPTOR)-1,DA_LDT);

	/*set GDT and page table for task0*/
	init_descriptor(&gdt[5], (u32)&tss0, sizeof(tss0)-1, DA_386TSS);
	
	set_task0_paging();
	set_page_directory(&pg_dir_task0, (void*)pg0_task0, (void*)pg1);

	// set ldt descriptor in GDT all task share the same ldt descriptor
	set_task1_paging();
	set_page_directory(&pg_dir_task1, (void*)pg0_task1, (void*)pg1);

	init_tasks();

	move_to_user_mode();

	while (1) {}
}

