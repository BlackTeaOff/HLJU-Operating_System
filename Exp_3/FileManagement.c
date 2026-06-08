#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 兄弟孩子表示法
// 用于把多叉树以二叉树的形式表示
// next指向的是和它在同一层的节点
// child指向的是它下一层的节点

// 文件/目录控制结构体
typedef struct FCB {
    char name[8]; // 文件/目录名
    int size; // 文件大小(目录为0)
    int type; // 1为文件, 2为目录
    char datetime[20]; // 日期时间, 格式 yyyymmdd hhmmss
    struct FCB *next; // 兄弟节点(同级文件夹)
    struct FCB *child; // 孩子节点(第一个子文件夹)
    struct FCB *parent; // 指向父节点(父文件夹), 实现CD..
} FCB;

FCB *root; // 指向根目录的指针
FCB *current_dir; // 指向当前目录的指针

// 传入一个字符串的地址
// 这个函数把时间写入这个字符串
void get_current_time(char *buffer) {
    // time函数返回time_t(Unix时间戳)类型, 传入NULL则返回当前时间(自1970以来的)
    // 传入参数也为time_t*类型的, 如果传参则直接把时间写到这个参数里
    time_t t = time(NULL);
    //time_t t2;
    //time(&t2);
    //printf("%d\n", t);

    // tm是一个存储时间数据的结构体(年月日时分秒等等)
    // localtime接受一个time_t参数, 将其转换为本地时间之后, 返回一个struct tm的地址 
    struct tm *tm_info = localtime(&t);
    // strftime(char *str, size_t maxsize, const char *format, const struct tm *t..)
    // str为要存入的字符串
    // maxsize表示存到str数组里的最大字符个数
    // format是格式控制字符串
    // 最后是包含时间信息的struct tm的变量
    strftime(buffer, 20, "%Y%m%d %H:%M:%S", tm_info);
    //printf("%s\n", buffer);
    //printf("%d\n", t2);
}

// 在dir下寻找重名的子文件/子目录
FCB* find_child(FCB *dir, char* name) {
    FCB *curr = dir->child; // 从它的子节点开始, 找它的兄弟节点(同目录下)
    while (curr != NULL) {
        // 找到就返回它的FCB地址
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// 初始化root目录
void init() {
    root = (FCB *)malloc(sizeof(FCB));
    strcpy(root->name, "");
    root->size = 0;
    root->type = 2;
    get_current_time(root->datetime);
    root->next = NULL;
    root->child = NULL;
    root->parent = NULL;

    current_dir = root;
}

// 创建空目录
void cmd_md(char *name) {
    if (find_child(current_dir, name) != NULL) {
        printf("当前目录下已存在同名文件或目录.\n");
        return;
    }

    FCB *new_dir = (FCB *)malloc(sizeof(FCB));
    strcpy(new_dir->name, name);
    new_dir->size = 0;
    new_dir->type = 2;
    get_current_time(new_dir->datetime);
    new_dir->next = NULL;
    new_dir->child = NULL;
    new_dir->parent = current_dir;

    // 如果当前目录没有子目录, 直接把当前目录子目录指向这个新目录
    if (current_dir->child == NULL) {
        current_dir->child = new_dir;
    } else {
        // 有的话就要遍历当前目录的子目录, 直到最后一个子目录
        // 把新目录接到子目录后面
        FCB *curr = current_dir->child;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_dir;
    }
}

int main() {
    init();
    printf("%s\n", root->datetime);
}