#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define BLOCK_SIZE 1024 // 一个内存块/外存块/页的大小(1K)(区别于下面的两个, 这个是大小, 下面是个数)
#define MEM_SIZE 64 // 64个内存块, 一个内存块在位示图是1bit
#define DISK_SIZE 128 // 128个外存块
#define RESIDENT_SET_SIZE 3 // 每个进程分配3个物理内存块
#define MAX_PROCESS 10

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

typedef struct {
    int pid;
    int size;
    int block_count;
    PTE *page_table;

    int page_fault_count; // 缺页次数
    int access_count;

    int fifo_queue[RESIDENT_SET_SIZE]; // 局部置换, 3个在内存的页的队列
    int fifo_head;
    int fifo_tail;
} PCB;

PCB *pcb_pool[MAX_PROCESS];
int next_pid = 1;

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

// 在内存/外存中找空位, 标为1并返回块号
// max_size是位示图中一共有多少块, 决定for循环中查找到多少个char
int allocate_block(char *bitmap, int max_size) {
    for (int i = 0; i < max_size / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (getbit(bitmap[i], j) == 0) {
                setbit(&bitmap[i], j, 1);
                return i * 8 + j;
            }
        }
    }
    // 无空闲块
    return -1;
}

// 释放内存/外存中指定的块
// 知道块号就能算出它在哪个char里, 在该char的第几位
void free_block(char *bitmap, int block_no) {
    setbit(&bitmap[block_no / 8], block_no % 8, 0);
}

void create_process() {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (pcb_pool[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("进程数量已达上限...");
        return;
    }

    int size;
    printf("请输入新进程的大小: ");
    scanf("%d", &size);

    if (size <= 0) {
        return;
    }

    int block_count = (int)ceil((double)size / BLOCK_SIZE);

    PCB *p = (PCB *)malloc(sizeof(PCB));
    p->pid = next_pid++;
    p->size = size;
    p->block_count = block_count;
    p->page_table = (PTE *)malloc(sizeof(PTE) * block_count); // 页表大小=块数
    p->access_count = 0;
    p->page_fault_count = 0;
    p->fifo_head = 0;
    p->fifo_tail = 0;

    // 进程所有页写入外存
    for (int i = 0; i < block_count; i++) {
        p->page_table[i].disk_block = allocate_block(disk_bitmap, DISK_SIZE);
        p->page_table[i].valid_bit = 0; // 还未装入内存
        p->page_table[i].modified_bit = 0;
        p->page_table[i].mem_block = -1; // 未装入内存
    }

    // 将前3页(RESIDENT_SET_SIZE)装入物理内存, 若小于3就装入全部页
    int intital_load = (block_count < RESIDENT_SET_SIZE ? block_count : RESIDENT_SET_SIZE);
    for (int i = 0; i < intital_load; i++) {
        p->page_table[i].mem_block = allocate_block(mem_bitmap, MEM_SIZE);
        p->page_table[i].valid_bit = 1;

        // 队列尾指针下标存新的页号
        p->fifo_queue[p->fifo_tail] = i;
        p->fifo_tail = (p->fifo_tail + 1) % RESIDENT_SET_SIZE; // 循环队列, 保证尾指针不超出2 
    }
    pcb_pool[slot] = p;
    printf("进程(PID: %d) 创建成功! 总页数: %d. 前 %d 页装入内存.\n", p->pid, block_count, intital_load);
}

int find_process_index(int pid) {
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (pcb_pool[i] != NULL && pcb_pool[i]->pid == pid) {
            return i;
        }
    }
    return -1;
}

void print_page_table() {
    int pid;
    printf("请输入要查询页表进程的PID: ");
    scanf("%d", &pid);
    int idx = find_process_index(pid);
    if (idx == -1) {
        printf("找不到PID为 %d 的进程.\n", pid);
        return;
    }
    PCB *p = pcb_pool[idx];
    printf("--------------------------------------------\n");
    printf("页号 | 存在位 | 修改位 | 内存块号 | 外存块号\n");
    for (int i = 0; i < p->block_count; i++) {
        printf(" %2d  |   %d    |   %d    |    %2d    |   %2d  \n", i, p->page_table[i].valid_bit, p->page_table[i].modified_bit, p->page_table[i].mem_block, p->page_table[i].disk_block);
    }
    printf("--------------------------------------------\n");
}

// 对某个进程进行页面置换
// 从队列里选出最先进入队列的, 返回页号
int page_replacement(int idx) {
    int victim_page = -1;

    // 返回队头的页号
    victim_page = pcb_pool[idx]->fifo_queue[pcb_pool[idx]->fifo_head];
    pcb_pool[idx]->fifo_head = (pcb_pool[idx]->fifo_head + 1) % RESIDENT_SET_SIZE;

    return victim_page;
}

void translate_address() {
    int pid;
    printf("请输入要进行地址转换的进程PID: ");
    scanf("%d", &pid);
    int idx = find_process_index(pid);
    if (idx == -1) {
        printf("找不到PID为 %d 的进程.\n", pid);
        return;
    }

    PCB *p = pcb_pool[idx];
    int la; // logicalAddress
    printf("请输入逻辑地址: ");
    scanf("%d", &la);

    if (la < 0) {
        return;
    }

    if (la >= p->size) {
        printf("逻辑地址 %d 超出了进程大小 %d.\n", la, p->size);
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

    p->access_count++;
    
    // 查页表该页是不是在内存中
    PTE *pte = &p->page_table[pageno];
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
        p->page_fault_count++;
        printf("%d 号页不在内存, 外存块号为%d, 需置换...\n", pageno, pte->disk_block);

        // FIFO选淘汰页
        int victim_page = page_replacement(idx);
        PTE *victim_pte = &p->page_table[victim_page];
        // 把淘汰页的物理块给该页用
        int mem_block = victim_pte->mem_block;

        printf("利用 FIFO 算法选中内存 %d 号页, 该页内存块号为 %d, 修改位为%d, 外存块号为 %d.\n", victim_page, mem_block, victim_pte->disk_block);

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
        p->fifo_queue[p->fifo_tail] = pageno;
        p->fifo_tail = (p->fifo_tail + 1) % RESIDENT_SET_SIZE;

        int physical_addr = pte->mem_block * BLOCK_SIZE + offset;
        printf("逻辑地址 %d 对应的物理地址为 %d.\n", la, physical_addr);
    }
}

void print_statistics() {
    int pid;
    printf("请输入要查询置换统计的进程PID: ");
    scanf("%d", &pid);
    int idx = find_process_index(pid);
    if (idx == -1) {
        printf("找不到PID为 %d 的进程.\n", pid);
        return;
    }

    printf("总访问次数: %d\n", pcb_pool[idx]->access_count);
    printf("缺页次数: %d\n", pcb_pool[idx]->page_fault_count);
    printf("缺页率: %0.2f%%\n", (float)pcb_pool[idx]->page_fault_count / pcb_pool[idx]->access_count * 100);
}

void destory_process() {
    int pid;
    printf("请输入要撤销的进程PID: ");
    scanf("%d", &pid);
    int idx = find_process_index(pid);
    if (idx == -1) {
        printf("找不到PID为 %d 的进程.\n", pid);
        return;
    }

    PCB *p = pcb_pool[idx];
    for (int i = 0; i < p->block_count; i++) {
        if (p->page_table[i].valid_bit == 1) {
            free_block(mem_bitmap, p->page_table[i].mem_block);
        }
        free_block(disk_bitmap, p->page_table[i].disk_block);
    }
    free(p->page_table);
    free(p);
    pcb_pool[idx] = NULL;
    printf("进程PID: %d 已撤销.\n", pid);
}

int main() {
    int choice;
    init_bitmap();

    while (1) {
        printf("---------------分页式存储管理----------------\n");
        printf("1. 查看位示图\n");
        printf("2. 创建进程\n");
        printf("3. 地址转换\n");
        printf("4. 撤销进程\n");
        printf("5. 查看页表\n");
        printf("6. 查看置换统计\n");
        printf("0. 退出\n");
        printf("---------------------------------------------\n");
        printf("请输入要执行的命令: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                print_bitmap();
                break;
            case 2:
                create_process();
                break;
            case 3:
                translate_address();
                break;
            case 4:
                destory_process();
                break;
            case 5:
                print_page_table();
                break;
            case 6:
                print_statistics();
                break;
            case 0:
                exit(0);
        }
    }
    return 0;
}

// int main() {
//     init_map();
//     print_bitmap();
//     create_process();
//     print_bitmap();
//     print_page_table();
// }