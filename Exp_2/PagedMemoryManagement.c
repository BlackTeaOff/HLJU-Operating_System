#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define BLOCK_SIZE 1024 // 块大小1K, 页大小和块大小相同, 也是1K
#define MEM_SIZE 64 // 块个数64个
#define MAX_PROCESS 10 // 进程数组中最大进程数量

// 位示图
// 一个char占1字节(8bit), 每一bit代表某一个块是否被占用
// 一共有MEM_SIZE个块, 需要MEM_SIZE个bit, 需要MEM_SIZE / 8个byte(1byte = 8bit), MEM_SIZE / 8个char
char bitmap[MEM_SIZE / 8];

typedef struct {
    int pid;
    int size; // 进程大小(Byte)
    int block_count; // 占用的块(页)数(size / 1024向上取整, 一块(页)是1K(1024Bytes))
    int *page_table; // 页表指针(指向页表数组)
} PCB;

// 进程数组, 存多个进程的指针
PCB *pcb_pool[MAX_PROCESS];
// 从1开始, pid0通常不分配给普通用户进程
int next_pid = 1;

// 得到char(8位)中的第bit_no位的bit是1还是0(该块的占用情况) 
// 使用(char)1, 00000001, 将其左移bit_no位, 放入mask(遮罩)
// 会使1和要检查的bit对齐(遮住了), 这两个char(8位)进行与操作
// 因为mask中除了1之外, 其他都是0(0和什么进行与操作都是0)
// 所以b和mask与的结果就看b中第bit_no位是1还是0
// 第bit_no位如果是1, 那么1和1与的结果就是1, 结果就是mask本身(不是0)
// 第bit_no位如果是0, 1与0还是0, 与的结果就是0
// bit_no是从右向左看的
int getbit(char b, int bit_no) {
    char mask = (char)1 << bit_no;
    if (b & mask) {
        return 1;
    } else {
        return 0;
    }
}

// 将char中第bit_no位(从右向左数)的bit置为0/1(flag), *b是因为要修改b本身
// 同样使用(char)1, 00000001, 将其左移bit_no位, 放入mask(它记录了bit_no的位置, 方便对char执行操作)
// flag为1代表置1, 将b和mask进行按位或操作, mask为0的部分b保持不变, mask为1的位置会置b对应位置为1
// flag为0代表置0, 先将mask取反, 这样mask的bit_no位就为0了
// 将b和mask进行按位与操作, 原先是0的与完还是0, 是1的与完还是1, 第bit_no位因为mask为0, 所以该位必为0
void setbit(char *b, int bit_no, int flag) {
    char mask = (char)1 << bit_no;
    if (flag) {
        *b = *b | mask;
    } else {
        mask = ~mask;
        *b = *b & mask;
    }
}

// 逻辑地址包含两部分
// 页号和偏移量
// 偏移量能表示的大小也是块(页)的大小(因为偏移量可以是页内的任意位置)
// 所以知道块(页)的大小, 就可以推出来偏移量在逻辑地址里面占了哪几个bit
// 假如偏移量占0-11bit, 那块的大小就是2的12次方 = 4K
// 12可以用log以2为底, 4096的对数算出来
// ceil - 向上取整
// 实验讲义里用的是换底公式, 算的也是log以2为底的size
int mylog2(int size) {
    return (int)ceil(log2(size));
}

// 初始化位示图
void init_bitmap() {
    srand((unsigned)time(NULL));
    // 遍历bitmap里的每一个char, 他们存了8个bit的信息(8个块的存储情况)
    for (int i = 0; i < MEM_SIZE / 8; i++) {
        // 可能会发生截断, 因为char是8bit(-128~127), 而rand生成的int是32bit
        bitmap[i] = (char)rand();
    }
}

void print_bitmap() {
    printf("-------------------------\n");
    for (int i = 0; i < MEM_SIZE / 8; i++) {
        printf("第 %d 字节\t", i);
        for (int j = 0; j < 8; j++) {
            printf("%d", getbit(bitmap[i], j));
        }
        printf("\n");
    }
    printf("-------------------------\n");
}

void create_process() {
    int slot = -1;
    // 在进程数组里找空位
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (pcb_pool[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        printf("进程数量已达上限...!");
        return;
    }

    int size;
    printf("请输入新进程的大小: ");
    scanf("%d", &size);

    if (size <= 0) {
        return;
    }

    // 根据进程大小, 计算出该进程需要分多少页(块)
    // 用进程大小除以块的大小(也是页的大小), 向上取整
    int block_count = (int)ceil((double)size / BLOCK_SIZE);

    // 计算是否有足够的空闲块
    int free_blocks = 0;
    // 一共有MEM_SIZE个块, MEM_SIZE个bit, 除以8得到byte(char的个数)
    for (int i = 0; i < MEM_SIZE / 8; i++) {
        // 遍历每个char, 用getbit得到对应位的bit
        for (int j = 0; j < 8; j++) {
            if (getbit(bitmap[i], j) == 0) {
                free_blocks++;
            }
        }
    }

    if (free_blocks < block_count) {
        printf("内存不足...需要 %d 块, 剩余 %d 块...!", block_count, free_blocks);
        return;
    }

    PCB *p = (PCB *)malloc(sizeof(PCB));
    p->pid = next_pid++;
    p->size = size;
    p->block_count = block_count;
    // 页表是一维数组, 通过index页号对应里面的块号, 一共有多少块, 页表的大小就是多少
    p->page_table = (int *)malloc(sizeof(int) * block_count);

    // 去内存块给每个页分配内存
    int allocated = 0; // 当前正在分配第几页(从0开始)
    // 遍历内存, 找空块
    for (int i = 0; i < MEM_SIZE / 8 && allocated < block_count; i++) {
        // 正在分配的页号大于一共分配的页号就退出
        for (int j = 0; j < 8 && allocated < block_count; j++) {
            if (getbit(bitmap[i], j) == 0) {
                // 找到空闲内存块, 把它的位置写入页表
                // i是第i个char, 每个char占8位, 当前char的起始位置就是i * 8
                // 在该char中的第j位有空闲, 所以该bit的位置就是i * 8 + j
                p->page_table[allocated] = i * 8 + j;
                setbit(&bitmap[i], j, 1);
                allocated++;
                printf("\033[0m\033[1;31m%s\033[0m", "1");
            } else {
                printf("1");
            }
        }
        printf("\n");
    }

    pcb_pool[slot] = p;
    printf("进程(PID: %d) 创建成功! 占用 %d 个内存块\n", p->pid, block_count);
}

void print_all_processes() {
    printf("---------------进程列表---------------\n");
    int count = 0;
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (pcb_pool[i] != NULL) {
            count++;
            printf("PID: %d | 大小: %d 字节 | 占用页数: %d\n", pcb_pool[i]->pid, pcb_pool[i]->size, pcb_pool[i]->block_count);
        }
    }
    if (count == 0) {
        printf("当前无进程.\n");
    }

    printf("---------------ENDD---------------\n");
}

int main() {
    init_bitmap();
    print_bitmap();
    create_process();
    print_bitmap();
    print_all_processes();
    //printf("\033[0m\033[1;31m%s\033[0m", "test");
    // for (int i = 0; i < MEM_SIZE / 8; i++) {
    //     printf("%d\n", bitmap[i]);
    //     for (int j = 7; j >= 0; j--) {
    //         printf("%d", getbit(bitmap[i], j));
    //     }
    //     printf("\n\n");
    // }
}