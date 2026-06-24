#include <stdio.h>
#include <stdlib.h>

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
    
    finished.tail = finished.head;
    finished.head->next = NULL;
    finished.size = 0;

    for (int i = 0; i < proc_count; i++) {
        work_procs[i] = original_procs[i];
    }
}

// 打印当前时间运行的进程和就绪队列
// 在算法内被调用, 不是手动调用
// 时间单位为秒
void print_queue_status(int current_time) {
    printf("[Time %d] 运行中: %s (进度: %d/%d) | 就绪队列: ", current_time, running == NULL ? "无" : running->name, running == NULL ? 0 : running->runned_time, running == NULL ? 0 : running->burst_time);
    PCB *p = ready.head->next;
    if (p == NULL) {
        printf("空\n");
        return;
    }
    while (p != NULL) {
        printf("%s ", p->name);
        p = p->next;
    }
    printf("\n");
}

// 根据进程的数据
// 打印出进程的周转时间, 带权周转时间
// 平均周转时间, 平均带权周转时间 
// %f-float, %lf-double, &ld-long
void print_scheduling_results(PCB *work_procs) {
    if (proc_count == 0) {
        return;
    }
    int sumOfturnarountTime = 0;
    double sumOfweightedTurnaroundTime = 0;

    for (int i = 0; i < proc_count; i++) {
        int turnaroundTime = work_procs[i].finished_time - work_procs[i].arrival_time;
        double weightedTurnaroundTime = (double)turnaroundTime / work_procs[i].burst_time;

        printf("[%d] %s | ", i + 1, work_procs[i].name);
        printf("周转时间: %d, ", turnaroundTime);
        printf("带权周转时间: %lf\n", weightedTurnaroundTime);
        sumOfturnarountTime += turnaroundTime;
        sumOfweightedTurnaroundTime += weightedTurnaroundTime;
    }
    printf("平均周转时间: %lf\n", (double)sumOfturnarountTime / proc_count);
    printf("平均带权周转时间: %lf\n", sumOfweightedTurnaroundTime / proc_count);
}

// First_come_First_serve
// 先来先服务
void fcfs() {
    // 算法内的进程数组, 互相独立, 不影响其他算法运行
    PCB procs[10];
    // 把origin数组进程复制到procs, 清空running和各队列
    reset_scheduling(procs);
    // 模拟算法的全局时间
    int time = 0;
    // 完成的进程数量, 如果有没完成的进程就一直循环
    int completed = 0;

    // 每while循环一次代表一秒
    while (completed < proc_count) {
        // 首先看arrival_time如果等于现在的time
        // 就放入就绪队列
        // 放入就绪队列的顺序就是先来先服务的顺序
        // 后续直接出队就行
        for (int i = 0; i < proc_count; i++) {
            if (procs[i].arrival_time == time) {
                enqueue(&ready, &procs[i]);
            }
        }

        // 如果running==NULL而且就绪队列有进程
        // 就出队这个进程到running
        if (running == NULL && ready.size > 0) {
            running = dequeue(&ready);
        }

        print_queue_status(time);

        // running != NULL, 就模拟这个进程的运行
        if (running != NULL) {
            running->runned_time++;
            if (running->runned_time == running->burst_time) {
                // 走完这一秒才算完成
                running->finished_time = time + 1;
                enqueue(&finished, running);
                completed++;
                running = NULL;
            }
        }
        time++;
    }
    print_scheduling_results(procs);
}

// shortest job first
// 最短作业优先
void sjf() {
    PCB procs[10];
    reset_scheduling(procs);

    int time = 0;
    int completed = 0;
    while (completed < proc_count) {
        // arrival_time=time, 放入就绪队列
        for (int i = 0; i < proc_count; i++) {
            if (procs[i].arrival_time == time) {
                enqueue(&ready, &procs[i]);
            }
        }

        // 如果running为空而且就绪队列有进程
        // 就找服务时间最短的进程给running
        if (running == NULL && ready.size > 0) {
            // 找服务时间最短的
            PCB *min_prev = ready.head;
            PCB *curr = ready.head->next;
            PCB *prev = ready.head;
            while (curr != NULL) {
                if (curr->burst_time < min_prev->next->burst_time) {
                    min_prev = prev;
                }
                curr = curr->next;
                prev = prev->next;
            }
            PCB *min = min_prev->next;
            min_prev->next = min->next;
            min->next = NULL;
            // 如果最短服务时间的进程在末尾
            // 需要重置尾指针
            if (ready.tail == min) {
                ready.tail = min_prev;
            }
            ready.size--;

            running = min;
        }

        print_queue_status(time);

        if (running != NULL) {
            running->runned_time++;
            if (running->runned_time == running->burst_time) {
                running->finished_time = time + 1;
                enqueue(&finished, running);
                completed++;
                running = NULL;
            }
        }
        time++;
    }
    print_scheduling_results(procs);
}

// round-robin
// 时间片轮转算法
void rr() {
    PCB procs[10];
    reset_scheduling(procs);
    // 时间片的大小
    int slice;
    printf("请输入时间片的大小: ");
    scanf("%d", &slice);
    // 当前一轮走了多长时间
    int current_slice = 0;

    int time = 0;
    int completed = 0;
    while (completed < proc_count) {
        for (int i = 0; i < proc_count; i++) {
            if (procs[i].arrival_time == time) {
                enqueue(&ready, &procs[i]);
            }
        }

        if (running == NULL && ready.size > 0) {
            running = dequeue(&ready);
            // 换新进程需要重置current_slice
            current_slice = 0;
        }

        print_queue_status(time);

        if (running != NULL) {
            running->runned_time++;
            current_slice++;
            if (running->runned_time == running->burst_time) {
                running->finished_time = time + 1;
                enqueue(&finished, running);
                completed++;
                running = NULL;
            } else if (current_slice == slice) {
                enqueue(&ready, running);
                running = NULL;
            }
        }
        time++;
    }
    print_scheduling_results(procs);
}

int main() {
    init_queue(&ready);
    init_queue(&finished);

    int choice;
    while (1) {
        printf("--------------------\n");
        printf("1. 输入进程信息\n");
        printf("2. 运行 FCFS 算法\n");
        printf("3. 运行 SJF 算法\n");
        printf("4. 运行 RR 算法\n");
        printf("--------------------\n");
        printf("请输入操作编号: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                input_process();
                break;
            case 2:
                fcfs();
                break;
            case 3:
                sjf();
                break;
            case 4:
                rr();
                break;
        }
    }
}