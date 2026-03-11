#include "page.h"
#include "type.h"
#include "kernel.h"


void set_task0_paging(){
	
	pte_t init_page_entry = 0x50000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task0[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

}

void set_task1_paging(){
	
	pte_t init_page_entry = 0x60000+0x07;
	for (int i = 0; i < 1024; i++) {
		pg0_task1[i] = init_page_entry;
		init_page_entry = init_page_entry + 0x1000;
	}

}


void set_page_directory(pte_t *pg_dir, void* user_entry_value, void* kernel_entry_value) {
	
	pg_dir[0] = (pte_t)user_entry_value+0x07-PAGE_OFFSET;
	pg_dir[768] = (pte_t)kernel_entry_value+0x07-PAGE_OFFSET;
}

