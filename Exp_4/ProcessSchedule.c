#include <stdio.h>

typedef struct PCB {
    char name[10];
    int size;
    int arrival_time; // 到达时间
    int burst_time; // 服务时间(需要运行多长时间)
    int finished_time; // 结束运行时间
    int runned_time; // 已运行时间
    struct PCB *next;
} PCB;

// 队列哨兵结构体
typedef struct {
    PCB *head;
    PCB *tail;
    int size;
} Queue;

PCB *running = NULL;
Queue ready; // 不加*也可以, 因为不需要频繁修改, 不需要指针
Queue finished; // 系统自动分配内存

// 初始化队列哨兵节点
// 每个队列都有一个哨兵PCB节点
void init_queue(Queue *q) {
    q->head = (PCB *)malloc(sizeof(PCB));
    q->head->next = NULL;
    q->tail = q->head;
    q->size = 0;
}

// 必须传指针, 否则按值传递, 复制一份结构体
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
    PCB *p = q->head->next;
    q->head->next = p->next;
    p->next = NULL;
    if (q->tail == p) { // 取出的是最后一个节点(尾节点)
        q->tail = q->head; // 更新尾节点
    }
    q->size--;
    return p;
}

// 输入进程数据存到这里
// 执行具体某个算法的时候
// 不修改这个进程数据
// 把它复制到算法里面的数组里
// 这样可以多个算法共用这一个数据
// 不需要重复输入数据
PCB original_procs[10];
int proc_count = 0;

void input_process() {
    printf("请输入进程的数量: ");
    scanf("%d", &proc_count);
    for (int i = 0; i < proc_count; i++) {
        printf("请输入第 %d 个进程的名称: ", i + 1);
        scanf("%s", original_procs[i].name);
        printf("请输入第 %d 个进程的到达时间: ", i + 1);
        scanf("%d", &original_procs[i].arrival_time);
        printf("请输入第 %d 个进程的服务时间: ", i + 1);
        scanf("%d", &original_procs[i].burst_time);
        original_procs[i].runned_time = 0;
        original_procs[i].finished_time = 0;
    }
}

// 执行某个具体的算法之前
// 把origin数组里进程的数据复制进具体算法里面的数组
// 清空所有队列
void reset_scheduling(PCB *work_procs) {
    running = NULL;
    // for (int i = 0; i < ready.size; i++) {
    //     dequeue(&ready);
    // }
    // for (int i = 0; i < finished.size; i++) {
    //     dequeue(&finished);
    // }

    // 直接将tail重置为head, head->next=NULL, size=0即可
    ready.tail = ready.head;
    ready.head->next = NULL;
    ready.size = 0;
    
    finished.tail = finished.tail;
    finished.head->next = NULL;
    finished.size = 0;

    for (int i = 0; i < proc_count; i++) {
        work_procs[i] = original_procs[i];
    }
}