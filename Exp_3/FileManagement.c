#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// 兄弟孩子表示法
// 用于把多叉树以二叉树的形式表示
// next指向的是和它在同一层的节点
// child指向的是它下一层的节点

// 文件/目录控制结构体
typedef struct FCB {
    char name[32]; // 文件/目录名
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
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    //printf("%s\n", buffer);
    //printf("%d\n", t2);
}

void string_to_upper(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

// 递归打印当前路径
// 自底向上, 然后自顶向下
void print_path(FCB* node) {
    if (node->parent != NULL) {
        print_path(node->parent);
        printf("%s\\", node->name);
    } else {
        // 已经是根节点
        printf("%s\\", node->name);
    }
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

void cmd_mk(char *name, int size) {
    if (find_child(current_dir, name) != NULL) {
        printf("当前目录下已存在同名文件或目录.\n");
        return;
    }

    FCB *new_file = (FCB *)malloc(sizeof(FCB));
    strcpy(new_file->name, name);
    new_file->size = size;
    new_file->type = 1;
    get_current_time(new_file->datetime);
    new_file->next = NULL;
    new_file->child = NULL;
    new_file->parent = current_dir;

    // 如果当前目录没有子目录或文件, 直接把当前目录的子目录指向这个新文件
    if (current_dir->child == NULL) {
        current_dir->child = new_file;
    } else {
        // 如果有就遍历当前目录的child, 直到最后一个child
        // 把当前FCB放到最后一个FCB后面
        FCB *curr = current_dir->child;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_file;
    }
}

void cmd_cd(char *name) {
    // 返回上一级目录
    if (strcmp(name, "..") == 0) {
        if (current_dir->parent != NULL) {
            current_dir = current_dir->parent;
        }
        return;
    }

    // 寻找子目录
    FCB *target = find_child(current_dir, name);
    // 目录不存在或者不是目录
    if (target == NULL || target->type == 1) {
        printf("系统找不到指定的目录.\n");
    } else {
        current_dir = target;
    }
}

void cmd_rd(char *name) {
    FCB *target = find_child(current_dir, name);
    if (target == NULL || target->type == 1) {
        printf("系统找不到指定的目录.\n");
        return;
    } else if (target->child != NULL) {
        printf("目录不为空.\n");
        return;
    } else {
        // 如果删除的目录是当前目录的第一个子目录
        // 直接让当前目录的child指向target的child
        if (current_dir->child == target) {
            current_dir->child = target->next;
        } else {
            // 不是就要找它的前一个节点
            FCB *curr = current_dir->child;
            while (curr->next != target) {
                curr = curr->next;
            }
            curr->next = target->next;
        }
        free(target);
    }
}

void cmd_del(char *name) {
    FCB *target = find_child(current_dir, name);
    if (target == NULL || target->type == 2) {
        printf("系统找不到指定的文件.\n");
        return;
    }

    // 和上面删除目录的逻辑一样
    if (current_dir->child == target) {
        current_dir->child = target->next;
    } else {
        FCB *curr = current_dir->child;
        while (curr->next != target) {
            curr = curr->next;
        }
        curr->next = target->next;
    }
    free(target);
}

void cmd_dir() {
    FCB *curr = current_dir->child;
    while (curr != NULL) {
        printf("%s", curr->datetime);
        printf(" %10s ", curr->type == 2 ? "<DIR>" : "");
        printf(" %10d ", curr->size);
        printf(" %s ", curr->name);
        printf("\n");
        curr = curr->next;
    }
}

int main() {
    char input[256];
    char cmd[20];
    char arg1[64];
    char arg2[64];

    init();
    
    while (1) {
        print_path(current_dir);
        printf(">");

        // 发生读取错误时, fgets会返回NULL
        // 读取到文件末尾时或输入Ctrl+Z, fgets返回NULL
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // fgets会读取换行符
        // 0和'\0'是一个意思, 0x00
        input[strcspn(input, "\n")] = '\0';
        // printf("%s", input);
        if (strlen(input) == 0) {
            continue;
        }

        // 把input里面的命令以及参数分别格式化到后面的字符串里
        int args = sscanf(input, "%s %s %s", cmd, arg1, arg2);
        string_to_upper(cmd);

        if (strcmp(cmd, "DIR") == 0) {
            cmd_dir();
        } else if (strcmp(cmd, "MD") == 0) {
            if (args != 2) {
                printf("用法: MD <目录名>\n");
            } else {
                cmd_md(arg1);
            }
        } else if (strcmp(cmd, "CD") == 0) {
            if (args > 2) {
                printf("用法: CD <目录名>\n");
            } else if (args == 1) {
                continue;
            } else {
                cmd_cd(arg1);
            }
        } else if (strcmp(cmd, "RD") == 0) {
            if (args != 2) {
                printf("用法: RD <目录名>\n");
            } else {
                cmd_rd(arg1);
            }
        } else if (strcmp(cmd, "MK") == 0) {
            if (args != 3) {
                printf("用法: MK <文件名> <大小>\n");
            } else {
                cmd_mk(arg1, atoi(arg2));
            }
        } else if (strcmp(cmd, "DEL") == 0) {
            if (args != 2) {
                printf("用法: DEL <文件名>\n");
            } else {
                cmd_del(arg1);
            }
        } else if (strcmp(cmd, "CLEAR") == 0) {
            system("cls");
        } else {
            printf("'%s' 不是可执行的指令\n", cmd);
        }
    }

    return 0;
}