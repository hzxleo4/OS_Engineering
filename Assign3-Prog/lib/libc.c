#include "type.h"
#include "stdio.h"

// 页大小相关定义
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// 内存块头部（16字节）
typedef struct block_header {
    u32 size;                    // 块大小（包含头部）
    struct block_header* next;   // 空闲链表下一块
    u32 used;                    // 是否已分配
    u8 _unused[4];
} block_header_t;

static block_header_t* free_list = NULL;   // 空闲链表头
static void* heap_start = NULL;            // 堆起始地址（页对齐）
static void* heap_brk = NULL;              // 当前堆顶（页对齐）

// 将新扩展的区域作为一个空闲块加入链表
static void add_free_block(void* addr, u32 size) {
    block_header_t* block = (block_header_t*)addr;
    block->size = size;
    block->used = 0;
    block->next = free_list;
    free_list = block;
}

// 扩展堆：至少分配 need 字节，以页为单位扩展，并将新区域加入空闲链表
static int grow_heap(u32 need) {
    if (heap_brk == NULL) {
        // 首次调用，获取当前堆顶并页对齐
        heap_brk = brk(0);
        if (heap_brk == (void*)-1) return -1;
        // 确保堆起始地址页对齐
        heap_brk = (void*)PAGE_ALIGN((u32)heap_brk);
        heap_start = heap_brk;
        // 初始堆为空，直接扩展
    }

    u32 need_aligned = PAGE_ALIGN(need);   // 按页对齐
    void* new_brk = (char*)heap_brk + need_aligned;
    if (brk(new_brk) == (void*)-1) return -1;

    // 将新扩展的区域（从原 heap_brk 到新 heap_brk）作为一个空闲块加入链表
    add_free_block(heap_brk, need_aligned);
    heap_brk = new_brk;
    return 0;
}

// Assume that the requested size in malloc is less than 4KB (one page)
// Thus, we only need to call grou_heap() one time for heap expansion
void* malloc(u32 size) {
    if (size == 0) return NULL;
    u32 total_size = size + sizeof(block_header_t);
    total_size = (total_size + 7) & ~7;   // 8字节对齐

    // 首次调用：确保堆已初始化
    if (heap_brk == NULL) {
        if (grow_heap(PAGE_SIZE) == -1) return NULL;
    }

    //
    // Your Code Here
    //


    // 理论上不会执行到这里（因为新页面至少 PAGE_SIZE 字节，应满足请求）
    return NULL;
}

void free(void *ptr) {
    if (ptr == NULL) return;

    //
    // Your Code Here
    //
 }