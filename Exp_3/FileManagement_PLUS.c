#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 1024 // 一个块1024字节(Byte) - 1K
#define TOTAL_BLOCK 128 // 共128块, 虚拟磁盘总大小1x128=128K
// FAT表中, 用4位十六进制数(一个十六进制数4bit)表示一个块的占用情况(如下)
// 一个块需要16bit(2byte), FAT的大小就是总块数乘2(byte)
#define FAT_SIZE (TOTAL_BLOCK * 2)

#define EMPTY_BLOCK 0x0000 // 磁盘块为空块的FAT表项
#define LAST_BLOCK 0xFFFF // 磁盘块为文件或目录的最后一块的FAT表项
// 如果不是最后一块就填入下一块的块号

typedef struct {
    char name[8];
    int size;
    int first_block; // 首块号
    char datetime[15];
    char type;
} FCB; // 一个文件/目录的结构体

// 虚拟硬盘总大小就是FAT表大小(块数×2byte)+数据区大小(块数×块大小)(byte)
#define DISK_SIZE (FAT_SIZE + BLOCK_SIZE * TOTAL_BLOCK)

// 一个char 1byte, 用char数组来表示整块磁盘空间
// 用unsigned char, 最高位不需要是符号位(0~255), 符号位影响位运算(符号位扩展等等)
unsigned char disk[DISK_SIZE];

// short占两byte, 正好对应fat表表示一块的大小
// 移动指针按2byte移动, fat[0]读取的就是前2byte(0, 1)的数据(第一块)
// fat[1]读取的就是第1*2(2, 3)byte的数据(第二块的数据)
unsigned short *fat = (unsigned short *)disk;

// 当前所在路径(文件夹/目录)所在的块号
// 根目录的块号是0
int current_block;

// 路径名栈
// 记录当前目录各级目录的名字
char path_stack[64][9];
// 深度初始化为1, 根目录
int path_depth = 1;

void init_disk() {
    // 只读打开二进制文件
    FILE *virtual_disk = fopen("virtual_disk.vhd", "rb");

    // 文件存在, 直接写入数组中
    if (virtual_disk != NULL) {
        printf("文件存在!\n");
        printf("开始读取磁盘文件...\n");
        fread(disk, sizeof(unsigned char) * DISK_SIZE, 1, virtual_disk);
        printf("读取完成!\n");
        fclose(virtual_disk);
    } else {
        printf("文件不存在!\n");
        printf("新建磁盘文件...\n");
        FILE *new_virtual_disk = fopen("virtual_disk.vhd", "wb");
        // 文件不存在, 新建一个virtual_disk
        // 先将disk清零(初始有垃圾值)
        for (int i = 0; i < DISK_SIZE; i++) {
            // int截断到unsigned char
            disk[i] = (unsigned char)0;
        }

        // 根目录(0号块)设置为已占用(就占一块也是最后一块, 所以是FFFF)
        fat[0] = LAST_BLOCK;

        // 写入文件
        fwrite(disk, sizeof(unsigned char) * DISK_SIZE, 1, new_virtual_disk);

        printf("新建磁盘文件成功!\n");
        fclose(new_virtual_disk);
    }
}

// 打印当前路径
void print_path() {
    for (int i = 0; i < path_depth; i++) {
        printf("%s", path_stack[i]);
        printf("/");
    }
}

int main() {
    init_disk();
    char input[64];
    char cmd[20];
    char arg1[64];
    char arg2[64];

    while (1) {
        print_path();
        printf(">");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            continue;
        }
        int args = sscanf(input, "%s %s %s", cmd, arg1, arg2);
    }
    return 0;
}

/**
 * 没时间写了(懒), 烂尾
 * 对于看到这的你嘛, 我只能说一句
 * I'm S~o~r~r~y.....
 *              _____                   _______                   _____                    _____                _____          
 *             /\    \                 /::\    \                 /\    \                  /\    \              |\    \         
 *            /::\    \               /::::\    \               /::\    \                /::\    \             |:\____\        
 *           /::::\    \             /::::::\    \             /::::\    \              /::::\    \            |::|   |        
 *          /::::::\    \           /::::::::\    \           /::::::\    \            /::::::\    \           |::|   |        
 *         /:::/\:::\    \         /:::/~~\:::\    \         /:::/\:::\    \          /:::/\:::\    \          |::|   |        
 *        /:::/__\:::\    \       /:::/    \:::\    \       /:::/__\:::\    \        /:::/__\:::\    \         |::|   |        
 *        \:::\   \:::\    \     /:::/    / \:::\    \     /::::\   \:::\    \      /::::\   \:::\    \        |::|   |        
 *      ___\:::\   \:::\    \   /:::/____/   \:::\____\   /::::::\   \:::\    \    /::::::\   \:::\    \       |::|___|______  
 *     /\   \:::\   \:::\    \ |:::|    |     |:::|    | /:::/\:::\   \:::\____\  /:::/\:::\   \:::\____\      /::::::::\    \ 
 *    /::\   \:::\   \:::\____\|:::|____|     |:::|    |/:::/  \:::\   \:::|    |/:::/  \:::\   \:::|    |    /::::::::::\____\
 *    \:::\   \:::\   \::/    / \:::\    \   /:::/    / \::/   |::::\  /:::|____|\::/   |::::\  /:::|____|   /:::/~~~~/~~      
 *     \:::\   \:::\   \/____/   \:::\    \ /:::/    /   \/____|:::::\/:::/    /  \/____|:::::\/:::/    /   /:::/    /         
 *      \:::\   \:::\    \        \:::\    /:::/    /          |:::::::::/    /         |:::::::::/    /   /:::/    /          
 *       \:::\   \:::\____\        \:::\__/:::/    /           |::|\::::/    /          |::|\::::/    /   /:::/    /           
 *        \:::\  /:::/    /         \::::::::/    /            |::| \::/____/           |::| \::/____/    \::/    /            
 *         \:::\/:::/    /           \::::::/    /             |::|  ~|                 |::|  ~|           \/____/             
 *          \::::::/    /             \::::/    /              |::|   |                 |::|   |                               
 *           \::::/    /               \::/____/               \::|   |                 \::|   |                               
 *            \::/    /                 ~~                      \:|   |                  \:|   |                               
 *             \/____/                                           \|___|                   \|___|                               
 * 
 * 有兴趣去听一下 Chester Young 的 Sorry!                                                                                                                           
 */