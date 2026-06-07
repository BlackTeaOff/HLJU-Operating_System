#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define BLOCK_SIZE 1024 // 一个内存块/外存块/页的大小(1K)
#define MEM_SIZE 64 // 64个内存块, 一个内存块在位示图是1bit
#define DISK_SIZE 128 // 128个外存块
#define RESIDENT_SET_SIZE 3 // 每个进程分配3个物理内存块

// 内存位示图(一个块占1bit, char占8bit, MEM_SIZE/8计算出char的个数)
char mem_bitmap[MEM_SIZE / 8];
// 外存位示图
char disk_bitmap[DISK_SIZE / 8];

// 页表
typedef struct {
    int valid_bit; // 存在位: 1表示在内存, 0表示在外存
    int modified_bit; // 修改位: 1表示被修改过, 置换时需要写回
    int mem_block; // 内存块号, 该页对应物理内存的块号
    int disk_block; // 外存块号, 该页对应物理外村的块号
} PTE; // Page Table Entry 页表条目

typedef struct PCB { // 进程控制块
    char name[20]; // 进程名
    
    // 分页内存管理
    int size;
    int block_count;
    PTE *page_table;

    int page_fault_count; // 缺页次数
    int access_count;

    int fifo_queue[RESIDENT_SET_SIZE]; // 局部置换, 3个在内存的页的队列
    int fifo_head;
    int fifo_tail;

    struct PCB *next; // 队列指针
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

/* ================= 实验二：位示图与基础位运算 ================= */
int getbit(char b, int bit_no) {
    char mask = (char)1 << bit_no;
    if (b & mask) {
        return 1;
    } else {
        return 0;
    }
}

void setbit(char *b, int bit_no, int flag) {
    char mask = (char)1 << bit_no;
    if (flag) {
        *b = *b | mask;
    } else {
        mask = ~mask;
        *b = *b & mask;
    }
}

int mylog2(int size) {
    return (int)ceil(log2(size));
}

void init_bitmap() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < MEM_SIZE / 8; i++) {
        mem_bitmap[i] = (char)rand();
    }
    for (int i = 0; i < DISK_SIZE / 8; i++) {
        disk_bitmap[i] = (char)rand();
    }
}

// 在内存/外存中找空位, 标为1并返回块号
int allocate_block(char *bitmap, int max_size) {
    for (int i = 0; i < max_size / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (getbit(bitmap[i], j) == 0) {
                setbit(&bitmap[i], j, 1);
                return i * 8 + j;
            }
        }
    }
    return -1;
}

// 释放内存/外存中指定的块
void free_block(char *bitmap, int block_no) {
    setbit(&bitmap[block_no / 8], block_no % 8, 0);
}

// 统计指定位示图中空闲块数量
int count_free_blocks(char* bitmap, int max_size) {
    int count = 0;
    for (int i = 0; i < max_size / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (getbit(bitmap[i], j) == 0) {
                count++;
            }
        }
    }
    return count;
}


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

    if (size <= 0) {
        return;
    }

    int block_count = (int)ceil((double)size / BLOCK_SIZE);
    // 将前3页(RESIDENT_SET_SIZE)装入物理内存, 若小于3就装入全部页
    int intital_load = (block_count < RESIDENT_SET_SIZE ? block_count : RESIDENT_SET_SIZE);

    // 检查外存容量
    int free_disk = count_free_blocks(disk_bitmap, DISK_SIZE);
    if (free_disk < block_count) {
        printf("外存空间不足! 进程需要 %d 个外存块, 当前仅剩 %d 块.\n", block_count, free_disk);
        return;
    }

    // 检查内存容量
    int free_mem = count_free_blocks(mem_bitmap, MEM_SIZE);
    if (free_mem < intital_load) {
        printf("内存空间不足! 需要装入 %d 块, 当前仅剩 %d 块.\n", intital_load, free_mem);
        return;
    }

    PCB *p = (PCB *)malloc(sizeof(PCB));
    strcpy(p->name, name);
    p->size = size;
    p->block_count = block_count;
    p->page_table = (PTE *)malloc(sizeof(PTE) * block_count); // 页表大小=块数
    p->access_count = 0;
    p->page_fault_count = 0;
    p->fifo_head = 0;
    p->fifo_tail = 0;
    p->next = NULL;

    // 进程所有页写入外存
    for (int i = 0; i < block_count; i++) {
        p->page_table[i].disk_block = allocate_block(disk_bitmap, DISK_SIZE);
        p->page_table[i].valid_bit = 0; // 还未装入内存
        p->page_table[i].modified_bit = 0;
        p->page_table[i].mem_block = -1; // 未装入内存
    }

    for (int i = 0; i < intital_load; i++) {
        p->page_table[i].mem_block = allocate_block(mem_bitmap, MEM_SIZE);
        p->page_table[i].valid_bit = 1;

        // 队列尾指针下标存新的页号
        p->fifo_queue[p->fifo_tail] = i;
        p->fifo_tail = (p->fifo_tail + 1) % RESIDENT_SET_SIZE; // 循环队列, 保证尾指针不超出2 
    }
    
    enqueue(&ready, p);
    printf("进程%s已创建!\n", name);
    // 如果running为空就把这个进程放入running
    dispatch();
}

// 终止running进程, 回收内存
void terminate_process() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    
    // 回收该进程的内存块和外存块
    for (int i = 0; i < running->block_count; i++) {
        if (running->page_table[i].valid_bit == 1) {
            free_block(mem_bitmap, running->page_table[i].mem_block);
        }
        free_block(disk_bitmap, running->page_table[i].disk_block);
    }
    free(running->page_table);

    printf("已终止并回收进程%s的内存. \n", running->name);
    // 释放PCB内存
    free(running);
    running = NULL;
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


// 对某个进程进行页面置换
int page_replacement(PCB *p) {
    int victim_page = -1;
    // 返回队头的页号
    victim_page = p->fifo_queue[p->fifo_head];
    p->fifo_head = (p->fifo_head + 1) % RESIDENT_SET_SIZE;
    return victim_page;
}

// 地址转换
void translate_address() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }

    int la; // logicalAddress
    printf("请输入进程 %s 的逻辑地址: ", running->name);
    scanf("%d", &la);

    if (la < 0) {
        return;
    }

    if (la >= running->size) {
        printf("逻辑地址 %d 超出了进程大小 %d.\n", la, running->size);
        return;
    }

    char is_write;
    printf("是否为写指令(Y/N): ");
    scanf(" %c", &is_write);
    int write_flag = (is_write == 'Y' || is_write == 'y') ? 1 : 0;

    int shift = mylog2(BLOCK_SIZE);
    int pageno = la >> shift;
    int mask = (0xffffffff) << shift;
    mask = ~mask;
    int offset = la & mask;

    printf("逻辑地址 %d 对应的页号为: %d, 页内偏移地址为: %d\n", la, pageno, offset);

    running->access_count++;
    
    // 查页表该页是不是在内存中
    PTE *pte = &running->page_table[pageno];
    // 命中, 在内存中
    if (pte->valid_bit == 1) {
        printf("%d 号页在内存中, 命中.\n", pageno);
        if (write_flag) {
            // 换出时修改位为1要写回外存
            pte->modified_bit = 1;
        }
        int physical_addr = pte->mem_block * BLOCK_SIZE + offset;
        printf("逻辑地址 %d 对应的物理地址为 %d.\n", la, physical_addr);
    } else { // 不在内存中
        running->page_fault_count++;
        printf("%d 号页不在内存, 外存块号为%d, 需置换...\n", pageno, pte->disk_block);

        // FIFO选淘汰页
        int victim_page = page_replacement(running);
        PTE *victim_pte = &running->page_table[victim_page];
        // 把淘汰页的物理块给该页用
        int mem_block = victim_pte->mem_block;

        printf("利用 FIFO 算法选中内存 %d 号页, 该页内存块号为 %d, 修改位为%d, 外存块号为 %d.\n", victim_page, mem_block, victim_pte->modified_bit, victim_pte->disk_block);

        if (victim_pte->modified_bit == 1) {
            printf("将内存 %d 号块内容写入外存 %d 号块, 成功.\n", mem_block, victim_pte->disk_block);
            victim_pte->modified_bit = 0;
        }

        printf("将外存 %d 号块内容调入内存 %d 号块中, 置换完毕.\n", pte->disk_block, mem_block);

        victim_pte->valid_bit = 0;
        victim_pte->mem_block = -1;

        pte->valid_bit = 1;
        pte->mem_block = mem_block;
        pte->modified_bit = write_flag;
        
        // 将该页入队
        running->fifo_queue[running->fifo_tail] = pageno;
        running->fifo_tail = (running->fifo_tail + 1) % RESIDENT_SET_SIZE;

        int physical_addr = pte->mem_block * BLOCK_SIZE + offset;
        printf("逻辑地址 %d 对应的物理地址为 %d.\n", la, physical_addr);
    }
}

void print_status() {
    printf("\n------------------------------\n");
    printf("[运行中]: ");
    if (running != NULL) {
        printf("%s", running->name);
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
    
    // 打印现在的位示图状态
    int free_mem = count_free_blocks(mem_bitmap, MEM_SIZE);
    int free_disk = count_free_blocks(disk_bitmap, DISK_SIZE);
    printf("\n[位示图状态]:");
    printf("\n可用内存块: %d / %d", free_mem, MEM_SIZE);
    printf("\n可用外存块: %d / %d", free_disk, DISK_SIZE);
    printf("\n------------------------------\n");
}

void print_bitmap() {
    printf("---------内存位示图---------\n");
    for (int i = 0; i < MEM_SIZE / 8; i++) {
        printf("第 %d 字节: ", i);
        for (int j = 0; j < 8; j++) {
            printf("%d ", getbit(mem_bitmap[i], j));
        }
        printf("\n");
    }
    printf("----------------------------\n");
    printf("---------外存位示图---------\n");
    for (int i = 0; i < DISK_SIZE / 8; i++) {
        printf("第 %2d 字节: ", i);
        for (int j = 0; j < 8; j++) {
            printf("%d ", getbit(disk_bitmap[i], j));
        }
        printf("\n");
    }
    printf("----------------------------\n");
}

void print_page_table() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    printf("-------------进程 %s 的页表-------------\n", running->name);
    printf("页号 | 存在位 | 修改位 | 内存块号 | 外存块号\n");
    for (int i = 0; i < running->block_count; i++) {
        printf(" %2d  |   %d    |   %d    |    %2d    |   %2d  \n", 
            i, running->page_table[i].valid_bit, running->page_table[i].modified_bit, 
            running->page_table[i].mem_block, running->page_table[i].disk_block);
    }
    printf("--------------------------------------------\n");
}

void print_statistics() {
    if (running == NULL) {
        printf("当前没有正在运行的进程. \n");
        return;
    }
    if (running->access_count == 0) {
        printf("进程 %s 尚未进行任何地址访问.\n", running->name);
        return;
    }
    printf("进程 %s 的置换统计:\n", running->name);
    printf("总访问次数: %d\n", running->access_count);
    printf("缺页次数: %d\n", running->page_fault_count);
    printf("缺页率: %0.2f%%\n", (float)running->page_fault_count / running->access_count * 100);
}

int main() {
    init_bitmap();
    init_queue(&ready);
    init_queue(&blocked);

    int choice;
    while (1) {
        print_status();
        printf("1. 创建新进程\n");
        printf("2. 执行进程时间片到\n");
        printf("3. 阻塞执行进程\n");
        printf("4. 唤醒第一个阻塞进程\n");
        printf("5. 终止执行进程\n");
        printf("6. 地址转换\n");
        printf("7. 查看位示图\n");
        printf("8. 查看运行进程页表\n");
        printf("9. 查看运行进程置换统计\n");
        printf("0. 退出\n");
        printf("请输入操作编号: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: create_process(); break;
            case 2: time_slice_out(); break;
            case 3: block_process(); break;
            case 4: wakeup_process(); break;
            case 5: terminate_process(); break;
            case 6: translate_address(); break;
            case 7: print_bitmap(); break;
            case 8: print_page_table(); break;
            case 9: print_statistics(); break;
            case 0: exit(0);
            default: printf("输入的编号无效! \n");
        }
    }
    return 0;
}