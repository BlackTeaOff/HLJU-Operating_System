#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

