#ifndef	_PAGE_H_
#define	_PAGE_H_
#include "type.h"

// page table entry
typedef unsigned int pte_t;

extern pte_t pg_dir[1024];
extern pte_t pg0[1024];
extern pte_t pg1[1024];

extern pte_t pg_dir_task0;
extern pte_t pg_dir_task1;

extern pte_t pg0_task1[1024];

extern pte_t pg0_task0[1024];

// task0 and task1 share the pg1

#define PAGE_OFFSET 0xC0000000

void set_task1_paging();
void set_task0_paging();
void set_page_directory(pte_t *pg_dir, void* user_entry_value, void* kernel_entry_value);


#endif