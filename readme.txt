SFS —— A Simple File System
代码量：约2200行吧
施工时间：慢慢吞吞地写了10天
作者： greenjiachen@gmail.com, 一枚来自华工软院的理工男
更新日期：2018-08-28

#Part 1. 实现细节
简单来说，这个所谓的“linux之上的二级文件系统”只是简单地调用了几个C语言文件操作API和Linux系统命令，来管理了文件的几个属性，限制用户的读写权限。也就是说，它并不是个真正可以投入实用的文件系统，仅仅是个toy example。

// 数据结构
typedef struct UserAccount
{
    char username[64]; // 定长的文件名：最长64个字符
    char password[64];
} Account;

typedef struct UserFile
{
    char filename[64];
    unsigned int addr;
    unsigned int permission;
    unsigned long fileLength;
    struct tm createdTime;
    struct tm lastModifiedTime;
    struct tm lastAccessTime;
} UFile;

简单的数据结构如上，SFS管理了文件的文件名、文件长度、权限、创建时间、最后修改时间和最后访问时间。

/*
磁盘目录
pwd //当前工作目录
|data
    |dir // 磁盘上存放各种表示[虚拟目录的二进制文件]的[真实目录]
        |accounts.dat // 注册用户信息
        |user1.dir // 用户虚拟目录
        |user2.dir
        |...
    |usr // 磁盘上存放各种表示[用户文件]的[真实目录]
        |addrRecorder.dat // 磁盘使用地址管理
        |File0
        |File1
        |File2
        |...
*/

#Part 2. 提供给外界程序调用的SFS_open和SFS_close函数
FILE* SFS_open(const char* username, const char* password, const char* filename, const char* mode);
int SFS_close(const char* username, const char* password, const char* filename, const char* mode, FILE*);

说明：
以上两个函数分别是对fopen和fwrite函数的封装。所以用SFS_open成功打开文件之后，可以用任意的C语言提供的文件操作函数（fread, fwrite, fgetc, fputc, fprintf, fscanf...）处理文件指针，很方便。
实现细节：
由于SFS只是简单地管理了文件的一些属性，并没有改变文件在磁盘上的存储形式（比如设计INode，把文件分割成磁盘上好几个小块等），所以外界程序想要读写SFS内部的文件的话，只需要把程序指定的文件名映射到磁盘上的真实文件即可，并且在关闭文件时更新该文件的最后访问时间、最后修改时间、长度等即可。
同时，由于SFS限制了登录、读写权限，所以这两个API函数不可避免地要传入用户名和密码的参数。
【注】对这两个函数的调用，不用管SFS是否运行。因为这两个函数的实现并没有和SFS的运行程序有进程间的通信，它们只是利用了SFS提供的一些函数，使得程序在调用这两个函数后，原有的文件在SFS系统中仍保持一致性，这就足够了。

#Part 3. 心得体会
1)陷入“长时间”的代码开发，感觉代码能力提高了不少。
2)对C语言的内存操作真的感到很痛苦，倒不是说因为内存泄露，而是指针越界踩到“别人”的内存，然后debug一整天都找不出什么问题。。
3)链表（数据结构）很强大。因为经常要处理参数个数不确定的命令行、不确定个数的文件等等等，所以就用void指针和函数指针的骚操作（奇技淫巧）手动封装了200行代码的“伪泛型”链表，顺便写了writeFile和readFile的序列化操作，一步到位读写文件，很爽有木有！后来发现这个链表在写代码的过程中给我提供了很大的便利，不用再尴尬地“申请足够空间的数组”了，特别的赞，然后就深深地折服于数据结构的淫威之下。

typedef struct LList_Node
{
    void *element;
    struct LList_Node *next;
}MLList_Node;

typedef struct LList
{
    struct LList_Node *head;
    struct LList_Node *tail;
    size_t size;

    void (*destroy)(struct LList *self);
    void (*addLast)(struct LList *self, const void *element);
    bool (*remove)(struct LList *self, int index);
    void (*clear)(struct LList*self);

    bool (*readFile)(struct LList*self, const char*filePath);
    bool (*writeFile)(const struct LList*self, const char*filePath);

    size_t elementSize;
    void (*assign)(void *u, const void *v); //赋值构造函数，达到深复制的目的

    // 可选的两个方法，方便查找搜索链表，简化代码；虽然这种形式不是链表的规范
    bool (*search)(const void *element, const void *key);
    struct LList_Node* (*getNode)(const struct LList*self, const void*key);
}MLList;

void MLList_init(MLList*, size_t, const void (*)(void *, const void*));

The End...