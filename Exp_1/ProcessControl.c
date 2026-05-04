#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_MEMORY 1024

typedef struct MemBlock {
    char status; // H-空闲段, P-进程占用段
    int start_addr; // 该内存段的起始地址
    int length;
    struct MemBlock *prev;
    struct MemBlock *next;
} MemBlock;

typedef struct PCB { // 进程控制块
    char name[20];
    int mem_start; // 基址
    int mem_length;
    struct PCB *next;
} PCB;

PCB *ready = NULL; // 就绪队列
PCB *blocked = NULL; // 阻塞队列
PCB *running = NULL; // 运行进程
MemBlock *mem_head = NULL; // 内存管理链表头指针

// 初始化内存
void init_memory() {
    mem_head = (MemBlock *)malloc(sizeof(MemBlock));
    mem_head->status = 'H';
    mem_head->start_addr = 0;
    mem_head->length = TOTAL_MEMORY;
    mem_head->prev = NULL;
    mem_head->next = NULL;
}

// 为进程分配内存, 传入所需内存大小, 返回分配的内存首地址(失败返回-1)
int allocate_memory(int size) {
    MemBlock *curr = mem_head; // 从头找
    while (curr != NULL) {
        // 找到空闲且大小足够的内存块
        if (curr->status == 'H' && curr->length >= size) {
            // 如果该内存块大小大于所需内存, 需要分割(把这个内存块分为两部分, 需要插入一块新内存块)
            if (curr->length > size) {
                MemBlock *new_block = (MemBlock *)malloc(sizeof(MemBlock));
                new_block->status = 'H';
                new_block->start_addr = curr->start_addr + size; // 新块的基址是当前块的基址加上前面需要的空间
                new_block->length = curr->length - size; // 新块的大小等于原块大小减所需大小
                
                // 把新块插入到链表中
                new_block->prev = curr;
                new_block->next = curr->next;

                if (curr->next != NULL) {
                    curr->next->prev = new_block;
                }
                curr->next = new_block;
            }
            // 设置分配的内存块的状态和大小(curr->length=size会跳过上面的分割, 直接到这里, 设置状态就可以)
            curr->status = 'P';
            curr->length = size;
            return curr->start_addr;
        }
        curr = curr->next;
    }
    return -1; // 没找到大小合适的内存块
}
