#include "page.h"
#include "type.h"
#include "kernel.h"

pte_t pg_dir_task2[1024] __attribute__((aligned(4096)));
pte_t pg_dir_task3[1024] __attribute__((aligned(4096)));
pte_t pg_dir_task4[1024] __attribute__((aligned(4096)));

pte_t pg0_task2[1024] __attribute__((aligned(4096)));
pte_t pg0_task3[1024] __attribute__((aligned(4096)));
pte_t pg0_task4[1024] __attribute__((aligned(4096)));


void set_task0_paging(){
	
	pte_t init_page_entry = 0x50000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task0[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

	/* ESP starts at 0x60000, so first push touches page 0x5F000. */
	pg0_task0[0x5f] = 0x110000 + 0x07;

}

void set_task1_paging(){
	
	pte_t init_page_entry = 0x60000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task1[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

	pg0_task1[0x6f] = 0x120000 + 0x07;

}

void set_task2_paging(){
	
	pte_t init_page_entry = 0x70000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task2[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

	pg0_task2[0x7f] = 0x130000 + 0x07;

}

void set_task3_paging(){
	
	pte_t init_page_entry = 0x80000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task3[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

	pg0_task3[0x8f] = 0x140000 + 0x07;

}

void set_task4_paging(){
	
	pte_t init_page_entry = 0x90000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task4[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

	pg0_task4[0x9f] = 0x150000 + 0x07;

}


void set_page_directory(pte_t *pg_dir, void* user_entry_value, void* kernel_entry_value) {
	for (int i = 0; i < 1024; i++) {
		pg_dir[i] = 0;
	}
	
	pg_dir[0] = (pte_t)user_entry_value+0x07-PAGE_OFFSET;
	pg_dir[512] = (pte_t)kernel_entry_value+0x03-PAGE_OFFSET;
}

