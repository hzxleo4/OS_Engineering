#include "page.h"
#include "type.h"
#include "kernel.h"


//void set_task0_paging(){
	
//	pte_t init_page_entry = 0x50000+0x07;
//	for (int i = 0; i < 1024; i++) {
//		pg0_task0[i] = init_page_entry;
//		init_page_entry = init_page_entry + 0x1000;
//	}

//}

//void set_task1_paging(){
	
//	pte_t init_page_entry = 0x60000+0x07;
//	for (int i = 0; i < 1024; i++) {
//		pg0_task1[i] = init_page_entry;
//		init_page_entry = init_page_entry + 0x1000;
//	}

//}

void set_task_paging(int pid){
	//unsigned int task_base = 0x400000;
	//unsigned int task_size = 0x400000;
	unsigned int task_base = 0x50000;
	unsigned int task_size = 0x10000;
	unsigned int mask = 0x07;
	unsigned int index = pid * 1024;

	//pte_t init_page_entry = task_base + task_size * pid + mask;
	pte_t init_page_entry = task_base + mask;
	for (int i = 0; i < pid; i++) {
		init_page_entry = init_page_entry + task_size;
	}
	
	//only init one page, not 16 pages
	for (int i = 0; i < 16; i++) {
		pg_tasks[index + i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}


	//pg_dir_tasks[index] = (pte_t) pg_tasks + index * 4 + mask - PAGE_OFFSET;
	pte_t tmp = (pte_t) pg_tasks + mask - PAGE_OFFSET;
	for (int i = 0; i < pid; i++) {
		tmp = tmp + 0x1000;
	}
	pg_dir_tasks[index] = tmp;
	
	pg_dir_tasks[index + 768] = (pte_t) pg1 + mask - PAGE_OFFSET;
}

void sys_print_task_paging(int pid) {
	unsigned int task_base = 0x50000;
	unsigned int task_size = 0x10000;
	unsigned int mask = 0x07;
	unsigned int index = pid * 1024;

	sys_printx("print page mappings for pid [");
	sys_write_int_routine(pid);
	sys_printx("]\n");

	
	for (int i = 0; i < 16; i++) {
		sys_printx("page[");
		sys_write_int_routine(i);
		sys_printx("] is [");
		sys_write_int_routine(pg_tasks[index + i]);
		sys_printx("]\n");
	}

	sys_printx("page table entry [");
	sys_write_int_routine(0);
	sys_printx("] is [");
	sys_write_int_routine(pg_dir_tasks[index]);
	sys_printx("]\n");

	sys_printx("page table entry[");
	sys_write_int_routine(768);
	sys_printx("] is [");
	sys_write_int_routine(pg_dir_tasks[index + 768]);
	sys_printx("]\n");
}


//void set_page_directory(pte_t *pg_dir, void* user_entry_value, void* kernel_entry_value) {
	
//	pg_dir[0] = (pte_t)user_entry_value+0x07-PAGE_OFFSET;
//	pg_dir[768] = (pte_t)kernel_entry_value+0x07-PAGE_OFFSET;
//}