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

static block_header_t* free_list = NULL;   // 堆块链表头
static void* heap_start = NULL;            // 堆起始地址（页对齐）
static void* heap_brk = NULL;              // 当前堆顶（页对齐）

// 将新扩展的区域作为一个空闲块加入链表尾部，保持地址顺序
static void add_free_block(void* addr, u32 size) {
    block_header_t* block = (block_header_t*)addr;
    block->size = size;
    block->used = 0;
    block->next = NULL;

    if (free_list == NULL) {
        free_list = block;
        return;
    }

    block_header_t* curr = free_list;
    while (curr->next != NULL) {
        curr = curr->next;
    }

    if (!curr->used && ((char*)curr + curr->size == (char*)block)) {
        curr->size += size;
    } else {
        curr->next = block;
    }
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

    while (1) {
        block_header_t* curr = free_list;

        while (curr != NULL) {
            if (!curr->used && curr->size >= total_size) {
                u32 remain = curr->size - total_size;

                if (remain >= sizeof(block_header_t) + 8) {
                    block_header_t* new_block = (block_header_t*)((char*)curr + total_size);
                    new_block->size = remain;
                    new_block->used = 0;
                    new_block->next = curr->next;

                    curr->size = total_size;
                    curr->next = new_block;
                }

                curr->used = 1;
                return (void*)(curr + 1);
            }
            curr = curr->next;
        }

        if (grow_heap(total_size) == -1) {
            return NULL;
        }
    }
}

void free(void *ptr) {
    if (ptr == NULL) return;

    block_header_t* prev = NULL;
    block_header_t* curr = free_list;

    while (curr != NULL) {
        if ((void*)(curr + 1) == ptr) {
            curr->used = 0;

            if (curr->next != NULL && !curr->next->used &&
                ((char*)curr + curr->size == (char*)curr->next)) {
                curr->size += curr->next->size;
                curr->next = curr->next->next;
            }

            if (prev != NULL && !prev->used &&
                ((char*)prev + prev->size == (char*)curr)) {
                prev->size += curr->size;
                prev->next = curr->next;
            }
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}