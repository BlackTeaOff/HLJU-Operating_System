#include <stdio.h>

// 最大进程数和最大资源类别数
#define MAX_PROC 10
#define MAX_RES 20

// 实际进程数量
int n;
// 实际资源类别数
int m;

// Available[j]=k, 代表第j个资源的数量为k
int Available[MAX_RES];
// Max[i, j]=k代表第i个进程需要第j个资源的数量为k
int Max[MAX_PROC][MAX_RES];
// Allocation[i, j]=k代表第i个进程已获得第j个资源的数量为k
int Allocation[MAX_PROC][MAX_RES];
// Need[i, j]=k代表第i个进程还需要第j个资源的数量为k
int Need[MAX_PROC][MAX_RES];
// Need = Max - Allocation

// 输入n, m
// Available, Max, Allocation
// Need可以用Max和Allocation计算出来
void init_banker() {
    printf("请输入进程数量n: ");
    scanf("%d", &n);
    printf("请输入资源类别数量: ");
    scanf("%d", &m);

    for (int i = 0; i < m; i++) {
        printf("请输入第 %d 个资源的数量: ", i + 1);
        scanf("%d", &Available[i]);
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            printf("请输入第 %d 个进程的第 %d 个资源的最大需求: ", i + 1, j + 1);
            scanf("%d", &Max[i][j]);
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            printf("请输入第 %d 个进程的第 %d 个资源的占有数量: ", i + 1, j + 1);
            scanf("%d", &Allocation[i][j]);
            // Available第j个资源减去给第i个进程分配的第j个资源
            Available[j] -= Allocation[i][j];
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            Need[i][j] = Max[i][j] - Allocation[i][j];
        }
    }
}

void print_matrix() {
    printf("Available: [ ");
    for (int i = 0; i < m; i++) {
        printf("%d ", Available[i]);
    }
    printf("]\n");

    printf("Allocation: \n");
    printf("--------\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            printf("%d ", Allocation[i][j]);
        }
        printf("\n");
    }
    printf("--------\n");

    printf("Max: \n");
    printf("--------\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            printf("%d ", Max[i][j]);
        }
        printf("\n");
    }
    printf("--------\n");

    printf("Need: \n");
    printf("--------\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            printf("%d ", Need[i][j]);
        }
        printf("\n");
    }
    printf("--------\n");
}

// 判断系统是否处于安全状态
int is_safe() {
    // 当前的可用资源, 最开始就等于Available
    // 随着资源的释放会增大(完成一个进程释放它的资源)
    int Work[MAX_RES];
    for (int i = 0; i < m; i++) {
        Work[i] = Available[i];
    }
    // 记录某个进程是否能执行
    int Finish[MAX_PROC] = {0};
    // 安全序列
    int safe_sequence[MAX_PROC];
    // 记录安全完成的进程数量
    // 用于判断是否处于安全状态(count==n则处于)
    int count = 0;

    while (1) {
        // 标记是否找到了一个可以运行的进程
        // 如果遍历完所有进程还找不到
        // 代表系统不安全, 或所有进程都完事, 需要看count
        int found = 0;

        // 遍历所有进程(未Finished的)
        // 还有它的Need
        // 该进程的Need全部小于Work(当前可用)则该进程可以运行
        for (int i = 0; i < n; i++) {
            if (Finish[i] == 1) {
                continue;
            }
            // 遍历该进程的每个资源
            for (int j = 0; j < m; j++) {
                if (Need[i][j] <= Work[j]) {
                    // 最后一个资源也小于work
                    // 说明该进程可以执行
                    if (j == m - 1) {
                        found = 1;
                        // 把该进程资源回收到work
                        for (int k = 0; k < m; k++) {
                            Work[k] += Allocation[i][k];
                        }
                        Finish[i] = 1;
                        safe_sequence[count++] = i;
                    }
                // 只要有一个资源大于work, 就break到下一个进程
                } else {
                    break;
                }
            }
        }

        // 遍历一轮发现没有能执行的进程
        // 就退出(有可能不安全, 也有可能所有进程都完事了)
        if (found == 0) {
            break;
        }
    }

    if (count == n) {
        printf("系统处于安全状态.\n");
        printf("安全序列: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", safe_sequence[i]);
        }
        printf("\n");
        return 1;
    } else {
        printf("系统处于不安全状态.\n");
        return 0;
    }
}

void request_resources() {
    int pid;
    printf("请输入发起请求的进程编号: ");
    scanf("%d", &pid);

    int Request[MAX_RES];
    for (int i = 0; i < m; i++) {
        printf("请输入对第 %d 个资源的请求数量: ", i + 1);
        scanf("%d", &Request[i]);

        if (Request[i] > Need[pid][i]) {
            printf("请求资源数超过最大需求.\n");
            return;
        }
        if (Request[i] > Available[i]) {
            printf("系统没有足够的资源.\n");
            return;
        }
    }

    // 分配资源
    for (int i = 0; i < m; i++) {
        Available[i] -= Request[i];
        Allocation[pid][i] += Request[i];
        Need[pid][i] -= Request[i];
    }

    // 执行安全性算法
    if (is_safe()) {
        printf("分配成功.\n");
    } else {
        for (int i = 0; i < m; i++) {
            Available[i] += Request[i];
            Allocation[pid][i] -= Request[i];
            Need[pid][i] += Request[i];
        }
        printf("分配失败.\n");
    }
}

int main() {
    int choice;
    while (1) {
        printf("--------------------\n");
        printf("1. 初始化数据\n");
        printf("2. 执行安全性检查\n");
        printf("3. 进程发起资源请求\n");
        printf("4. 打印当前矩阵\n");
        printf("--------------------\n");
        printf("请输入操作编号: ");
        // 可以通过 < 文件名(banker.txt) 直接把数据初始化
        // 不过stdin会指向那个文件
        if (scanf("%d", &choice) == EOF) {
            // 把输入源且回到键盘
            // CON指的是"Console"
            // freopen(filename, mode, stream)
            // stream是改变某个流(File)的指向
            freopen("CON", "r", stdin);
            continue;
        }

        switch (choice) {
            case 1:
                init_banker();
                break;
            case 2:
                is_safe();
                break;
            case 3:
                request_resources();
                break;
            case 4:
                print_matrix();
                break;
        }
    }
}
