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

// 队列结构体, 维护一个哨兵头节点(不存数据, 没哨兵头节点需要二重指针修改头节点地址)
typedef struct Queue {
    PCB *head;
    PCB *tail;
    int size;
} Queue;

Queue ready;  // 就绪队列
Queue blocked; // 阻塞队列
PCB *running = NULL; // 运行进程(只有一个, 不需要队列)
MemBlock *mem_head = NULL; // 内存管理链表头指针

void init_queue(Queue *q) {
    q->head = (PCB *)malloc(sizeof(PCB));
    q->head->next = NULL;
    q->tail = q->head;
    q->size = 0;
}

void enqueue(Queue *q, PCB *process) {
    process->next = NULL;
    q->tail->next = process;
    q->tail = process;
    q->size++;
}

PCB* dequeue(Queue *q) {
    if (q->head == q->tail) {
        return NULL;
    }
    PCB *process = q->head->next;
    q->head->next = process->next;

    // 取出的是最后一个接待
    if (process->next == NULL) {
        q->tail = q->head;
    }
}

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

// 传入进程的基址, 在内存链表中找到该进程的内存块
void free_memory(int start_addr) {
    MemBlock *curr = mem_head;
    while (curr != NULL && curr->start_addr != start_addr) {
        curr = curr->next;
    }
    if (curr == NULL) {
        return;
    }
    curr->status = 'H';

    // 看该块后面是否空闲, 若空闲则合并
    if (curr->next != NULL && curr->next->status == 'H') {
        MemBlock *temp = curr->next;
        curr->length += temp->length;
        curr->next = temp->next;
        if (temp->next != NULL) {
            temp->next->prev = curr;
        }
        free(temp);
    }

    // 看该块前面是否空闲, 如空闲则合并
    if (curr->prev != NULL && curr->prev->status == 'H') {
        MemBlock *temp = curr->prev;
        temp->length += curr->length;
        temp->next = curr->next;
        if (curr->next != NULL) {
            curr->next->prev = temp;
        }
        free(curr);
    }
}
