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

    // 取出的是最后一个节点, 重置尾指针指向哨兵头节点
    if (process->next == NULL) {
        q->tail = q->head;
    }

    process->next = NULL;
    q->size--;
    return process;
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

// 调度, 出队就绪队列第一个进程, 放入running
void dispatch() {
    if (running == NULL && ready.size > 0) {
        running = dequeue(&ready);
    }
}

void create_process() {
    char name[20];
    int size;
    printf("请输入新进程名称: ");
    scanf("%s", name);
    printf("请输入进程需要申请的内存大小: ");
    scanf("%d", &size);

    int start_addr = allocate_memory(size);
    if (start_addr == -1) {
        printf("内存不足, 无法创建进程!\n");
        return;
    }

    PCB *p = (PCB *)malloc(sizeof(PCB));
    strcpy(p->name, name);
    p->mem_start = start_addr;
    p->mem_length = size;
    p->next = NULL;

    enqueue(&ready, p);
    printf("进程%s已创建!\n", name);
    // 如果running为空就把这个进程放入running
    dispatch();
}

// 把当前running放入就绪队列, 从就绪队列出队一个进程放入running
void time_slice_out() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    printf("进程%s时间片到, 放入就绪队列. \n", running->name);
    enqueue(&ready, running);
    running = NULL;
    dispatch();
}

// 把当前running进程放入阻塞队列, 从就绪队列里出队一个进程放入running
void block_process() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    printf("进程%s被阻塞, 放入阻塞队列. \n", running->name);
    enqueue(&blocked, running);
    running = NULL;
    dispatch();
}

// 从阻塞队列出队一个进程, 放入就绪队列
void wakeup_process() {
    if (blocked.size == 0) {
        printf("当前阻塞队列为空, 无可唤醒进程. \n");
        return;
    }
    PCB *p = dequeue(&blocked);
    enqueue(&ready, p);
    printf("唤醒进程%s, 放入就绪队列. \n", p->name);
    // 如果ready里没有进程, 就从ready里出队一个进程放入running
    dispatch();
}

// 终止running进程, 回收内存
void terminate_process() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    free_memory(running->mem_start);
    printf("已终止并回收进程%s的内存. \n", running->name);
    // 释放PCB内存
    free(running);
    running = NULL;
    dispatch();
}

void print_status() {
    printf("------------------------------\n");
    printf("[运行中]: ");
    if (running != NULL) {
        printf("%s (内存: %d~%d)", running->name, running->mem_start, running->mem_start + running->mem_length - 1);
    } else {
        printf("无运行进程");
    }
    printf("\n");

    printf("[就绪队列](共%d个): ", ready.size);
    PCB *temp = ready.head->next;
    if (!temp) {
        printf("无就绪进程");
    }
    while (temp) {
        printf("%s", temp->name);
        if (temp->next) {
            printf(" -> ");
        }
        temp = temp->next;
    }
    printf("\n");

    printf("[阻塞队列](共%d个): ", blocked.size);
    temp = blocked.head->next;
    if (!temp) {
        printf("无阻塞进程");
    }
    while (temp) {
        printf("%s", temp->name);
        if (temp->next) {
            printf(" -> ");
        }
        temp = temp->next;
    }
    printf("\n");

    int mem_using = 0;

    printf("[内存分布链表]:");
    MemBlock *m = mem_head;
    while (m) {
        printf("[%c | %d | %d]", m->status, m->start_addr, m->length);
        if (m->next) {
            printf(" <-> ");
        }
        if (m->status == 'P') {
            mem_using += m->length;
        }
        m = m->next;
    }
    printf("\n系统总内存: %d", TOTAL_MEMORY);
    printf("\n可用内存: %d", TOTAL_MEMORY - mem_using);
    printf("\n已用内存: %d", mem_using);
    printf("\n------------------------------");
}

int main() {
    init_memory();
    init_queue(&ready);
    init_queue(&blocked);

    int choice;
    while (1) {
        print_status();
        printf("\n1. 创建新进程\n");
        printf("2. 执行进程时间片到\n");
        printf("3. 阻塞执行进程\n");
        printf("4. 唤醒第一个阻塞进程\n");
        printf("5. 终止执行进程\n");
        printf("0. 退出\n");
        printf("请输入操作编号: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                create_process();
                break;
            case 2:
                time_slice_out();
                break;
            case 3:
                block_process();
                break;
            case 4:
                wakeup_process();
                break;
            case 5:
                terminate_process();
                break;
            case 0:
                exit(0);
            default:
                printf("输入的编号无效! \n");
        }
    }
    return 0;
}