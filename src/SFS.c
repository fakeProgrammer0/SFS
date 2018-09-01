#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>
#include <malloc.h>

#include "SFS.h"
#include "MLList.h"
#include "util.h"

// 控制台样式
#define CHANGE_TO_DEFAULT_STYTLE printf("\e[0m");
#define CHANGE_TO_RED_STYTLE printf("\e[1;31m");    // 红色：输出错误
#define CHANGE_TO_GREEN_STYTLE printf("\e[1;32m");  // 绿色：用户名称
#define CHANGE_TO_YELLOW_STYTLE printf("\e[1;33m"); // 黄色：提示输入
#define CHANGE_TO_BLUE_STYTLE printf("\e[1;34m");   // 蓝色：高亮突出某些信息
#define CHANGE_TO_WHITE_STYTLE printf("\e[1;37m");  // 白色高亮
#define CHANGE_TO_CYAN_STYTLE printf("\e[1;36m");   // 蓝绿色：

// 统一用到的各种字符串大小
#define LONG_LONG_STR_LEN 512
#define LONG_STR_LEN 128
#define MID_STR_LEN 64
#define SHORT_STR_LEN 32
#define MINI_STR_LEN 16

// 数据结构
typedef struct UserAccount
{
    char username[MID_STR_LEN]; // 最长64个字符
    char password[MID_STR_LEN];
} Account;

void UserAccount_Assign(void *u, const void *v)
{
    strcpy(((Account *)u)->username, ((Account *)v)->username);
    strcpy(((Account *)u)->password, ((Account *)v)->password);
}

bool UserAccount_Search(const void *element, const void *key)
{
    Account *account = (Account *)element;
    char *username = (char *)key;
    return (account != NULL && key != NULL && strcmp(account->username, username) == 0);
}

typedef struct UserFile
{
    char filename[MID_STR_LEN];
    unsigned int addr;
    unsigned int permission;
    unsigned long fileLength;
    struct tm createdTime;
    struct tm lastModifiedTime;
    struct tm lastAccessTime;
} UFile;

void UFile_Assign(void *dest, const void *src)
{
    UFile *d = (UFile *)dest;
    UFile *s = (UFile *)src;
    strcpy(d->filename, s->filename);
    d->addr = s->addr;
    d->permission = s->permission;
    d->fileLength = s->fileLength;
    d->createdTime = s->createdTime;
    d->lastModifiedTime = s->lastModifiedTime;
    d->lastAccessTime = s->lastAccessTime;
}

bool UFile_Search(const void *element, const void *key)
{
    UFile *file = (UFile *)element;
    char *filename = (char *)key;
    return file != NULL && filename != NULL && strcmp(file->filename, filename) == 0;
}

void Unsigende_Int_Assign(void *u, const void *v)
{
    *((unsigned int *)u) = *((unsigned int *)v);
}

void cstring_Assign(void *u, const void *v)
{
    strcpy((char *)u, (char *)v);
}

// 定义的全局变量等，方便各函数直接使用

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

// 磁盘上记录的目录
static char dataPath[LONG_STR_LEN];
static char dirPath[LONG_STR_LEN]; // 磁盘上存放各种表示[虚拟目录的二进制文件]的[真实目录]路径
static char usrPath[LONG_STR_LEN]; // 磁盘上存放各种表示[用户文件]的[真实目录]路径

// 磁盘上记录的文件
static char accountsFilePath[LONG_STR_LEN];
static char userDirFilePath[LONG_STR_LEN]; // 磁盘上表示[用户虚拟目录]的二进制文件dir的真实路径
static char addrRecorderFilePath[LONG_STR_LEN];

// 对应的链表
static MLList *accounts;
static MLList *addrRecorder;
static MLList *userDir;

// 当前登录的用户
static char *currUsername = NULL;
static bool isSFSRunning = false;

// 声明用到的各个函数

// 提供给外部的API

// void runSFS(); // 移植到FS.h头文件
void initSFS();
void exitSFS();

// 磁盘管理
unsigned int checkoutAFreeDiskAddr();
void freeADiskAddr(unsigned int);

// 操作命令
void handleCommands();
void handleRegister(int, const char *[]);
// void handleLogin(int, const char*[]);
bool handleLogin(int, const char *[]);
void handleLogout(int, const char *[]);
void handleDir(int, const char *[]);
void handleCreate(int, const char *[]);
void handleDelete(int, const char *[]);
void handleChmod(int, const char *[]);
void handleOpen(int, const char *[]);
void handleClose(int, const char *[]);
void handleRead(int, const char *[]);
void handleWrite(int, const char *[]);
void handleHelp(int, const char *[]);
void handleClear(int, const char *[]);
// void handleExit(int, char*[]);
void handleAccount(int, const char *[]);
void handleReadme(int, const char *[]);

// IO 输入输出函数
void printWelcomeMS();
void printCmdHeader();
void printCmdHelp(const char *, int, const char *[], const char *[], const char *);

// void printErrorMs(const char *);

// void acceptALineFromCMD(char *, const char *);
// void acceptAPasword(char *, const char *);
// char getAChoice(const char *);
// void acceptEnter(const char *);

// char **getACMDLine(int *, int);
// void freeACMDLine(int, char *[]);

// 权限转换的函数
void getPermissionStr(char *, unsigned int);  // 0755 -> "rwxr-xr-x"
unsigned int getPermissionCode(const char *); // "rwxr--r--" -> 0744

void runSFS()
{
    isSFSRunning = true;
    initSFS();
    handleCommands();
    exitSFS();
}

void initSFS()
{
    // 初始化一系列的目录路径
    getcwd(dataPath, sizeof(dataPath));
    // printf("Current Dir: %s\n", dataPath);
    strcat(dataPath, "/data/");
    if (access(dataPath, F_OK) != 0)
    {
        mkdir(dataPath, 0755);
    }

    strcpy(dirPath, dataPath);
    strcat(dirPath, "dirFile/");
    if (access(dirPath, F_OK) != 0)
    {
        mkdir(dirPath, 0755);
    }

    strcpy(usrPath, dataPath);
    strcat(usrPath, "usr/");
    if (access(usrPath, F_OK) != 0)
    {
        mkdir(usrPath, 0755);
    }

    // 从文件读取初始化链表等数据结构

    // accounts.dat
    accounts = (MLList *)malloc(sizeof(MLList));
    MLList_init(accounts, sizeof(Account), UserAccount_Assign);
    accounts->search = UserAccount_Search; // 注册search函数，方便查找

    strcpy(accountsFilePath, dirPath);
    strcat(accountsFilePath, "accounts.dat");
    //账户文件存在
    if (access(accountsFilePath, F_OK) == 0)
    {
        accounts->readFile(accounts, accountsFilePath);
    }
    else
    {
        Account rootAccount;
        strcpy(rootAccount.username, "root");
        strcpy(rootAccount.password, "root");

        accounts->addLast(accounts, &rootAccount);
        accounts->writeFile(accounts, accountsFilePath);
    }

    // addrRecorder.dat
    strcpy(addrRecorderFilePath, dirPath);
    strcat(addrRecorderFilePath, "addrRecorder.dat");

    addrRecorder = (MLList *)malloc(sizeof(MLList));
    MLList_init(addrRecorder, sizeof(unsigned int), Unsigende_Int_Assign);
    if (access(addrRecorderFilePath, F_OK) == 0)
    {
        addrRecorder->readFile(addrRecorder, addrRecorderFilePath);
    }

    // userDir等到用户登录时再初始化
}

void exitSFS()
{
    // 各个数据结构写文件到磁盘，释放内存
    // accounts->writeFile(accounts, accountsFilePath);
    accounts->destroy(accounts); // destroy中已经对指针指向的地址空间进行了释放
    accounts = NULL;

    // addrRecorder->writeFile(addrRecorder, addrRecorderFilePath);
    addrRecorder->destroy(addrRecorder);
    addrRecorder = NULL;

    if (currUsername != NULL)
        handleLogout(1, NULL);
}

unsigned int checkoutAFreeDiskAddr()
{
    unsigned int i = 0;
    MLList_Node *curr = addrRecorder->head;
    MLList_Node *prev = NULL;
    while (curr != NULL && *((unsigned int *)(curr->element)) == i)
    {
        prev = curr;
        curr = curr->next;
        i++;
    }

    MLList_Node *newNode = (MLList_Node *)malloc(sizeof(MLList_Node));
    // newNode->element = (void *)(&i); // 浅复制
    newNode->element = malloc(addrRecorder->elementSize);
    addrRecorder->assign(newNode->element, (void *)(&i));

    if (prev != NULL)
        prev->next = newNode;
    else
    {
        addrRecorder->head = newNode;
    }

    if (curr != NULL)
    {
        newNode->next = curr;
    }
    else
    {
        newNode->next = NULL;
        addrRecorder->tail = newNode;
    }

    addrRecorder->size++;
    addrRecorder->writeFile(addrRecorder, addrRecorderFilePath); // 及时保存，防止bug奔溃。。
    return i;
}

void freeADiskAddr(unsigned int addr)
{
    MLList_Node *curr = addrRecorder->head;
    MLList_Node *prev = NULL;
    while (curr != NULL && *((unsigned int *)(curr->element)) < addr)
    {
        prev = curr;
        curr = curr->next;
    }

    if (*((unsigned int *)(curr->element)) == addr)
    {
        MLList_Node *removeNode = curr;
        if (prev == NULL) // 移除的是头节点
        {
            addrRecorder->head = addrRecorder->head->next;
        }
        else
        {
            prev->next = curr->next;
        }

        if (curr == addrRecorder->tail)
        {
            addrRecorder->tail = prev;
        }

        free(removeNode->element);
        free(removeNode);
        addrRecorder->size--;
        addrRecorder->writeFile(addrRecorder, addrRecorderFilePath);
    }
}

void handleCommands()
{
    printWelcomeMS();
    bool loopFlag = true;
    while (loopFlag)
    {
        //处理输入字符串

        printCmdHeader();

        int argc = 0;
        int *argcPtr = &argc;
        char **argv = getACMDLine(argcPtr, MID_STR_LEN);
        // const char **argv = getACMDLine(argcPtr, MID_STR_LEN); //用const的话，最后的内存你怎么释放勒？

        strToLowerCase(argv[0]); // 预处理命令字符串
        bool validCmd = true;

        // 0.任何状态下都能使用的命令
        if (strcmp(argv[0], "help") == 0)
        {
            handleHelp(argc, argv);
        }
        else if (strcmp(argv[0], "readme") == 0)
        {
            handleReadme(argc, argv);
        }
        else if (strcmp(argv[0], "exit") == 0)
        {
            acceptEnter("bye~Press [Enter] to exit...");
            // break;
            loopFlag = false; // 不要直接用break，不然后面的内存释放部分的代码没有执行
        }
        else if (strcmp(argv[0], "register") == 0)
        {
            handleRegister(argc, argv);
        }
        else if (strcmp(argv[0], "login") == 0)
        {
            handleLogin(argc, argv);
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            handleClear(argc, argv);
        }
        // 1.登录状态下才使用的命令
        else if (currUsername != NULL)
        {
            if (strcmp(argv[0], "logout") == 0)
            {
                handleLogout(argc, argv);
            }
            else if (strcmp(argv[0], "dir") == 0)
            {
                handleDir(argc, argv);
            }
            else if (strcmp(argv[0], "create") == 0)
            {
                handleCreate(argc, argv);
            }
            else if (strcmp(argv[0], "delete") == 0)
            {
                handleDelete(argc, argv);
            }
            else if (strcmp(argv[0], "chmod") == 0)
            {
                handleChmod(argc, argv);
            }
            else if (strcmp(argv[0], "open") == 0)
            {
                handleOpen(argc, argv);
            }
            else if (strcmp(argv[0], "close") == 0)
            {
                handleClose(argc, argv);
            }
            else if (strcmp(argv[0], "read") == 0)
            {
                handleRead(argc, argv);
            }
            else if (strcmp(argv[0], "write") == 0)
            {
                handleWrite(argc, argv);
            }
            else if (strcmp(currUsername, "root") == 0) // 管理员账号的指令
            {
                if (strcmp(argv[0], "account") == 0)
                {
                    handleAccount(argc, argv);
                }
                else // 3.error command
                    validCmd = false;
            }
            else // 3.error command
                validCmd = false;
        }
        else // 2.未登录状况下，才能用的命令。暂时还没有这样的指令
        {
            // if (strcmp(argv[0], "") == 0)
            // {
            // }
            // else // 3.error command
            validCmd = false;
        }

        if (!validCmd)
        {
            printErrorMs("Error Command. Type [help] to see what you can do in this file system.");
        }

        freeACMDLine(argc, argv);
        argv = NULL;
    }
}

void printWelcomeMS()
{
    printf("Welcome to \e[1;32mSFS -- a Simple File System\e[0m based on ubuntu 18.\nType \e[1;33m[help]\e[0m to see what you can do in this file system.\n");
}

void handleHelp(int argc, const char *argv[])
{
    // int numOfHelpCmds = 1;
    // const char *helpCmds[] = {"help"};
    // const char *helpDesc[] = {"display this message"};
    // const char *helpInstr = "A simple specification about the commands offerred by this file system.\nNote: all additional arguments will be ignored.";
    // printCmdHelp(argv[0], numOfHelpCmds, helpCmds, helpDesc, helpInstr);

    CHANGE_TO_WHITE_STYTLE;
    printf("Here's \e[1;32ma simple specification\e[1;37m about the commands offerred by this file system.\n");
    printf("Note: All additional arguments will be ignored when you type \e[1;33m[help]\e[1;36m.\n");

    printf("\nPart 1: Commands\n");

    {
        const char *instr = "\n1.Commands that you can call any time.";
        int numOfCmds = 4;
        const char *cmds[] = {"register", "login", "exit", "clear"};
        const char *desc[] = {
            "register an account",
            "user sign in",
            "exit the system",
            "clear the console"};
        printCmdHelp(NULL, numOfCmds, cmds, desc, instr);
    }
    acceptEnter("\nPress [Enter] to continue...");
    {
        const char *instr = "2.Commands that you can call only when you login. (只有登录时才有效的命令。)";
        int numOfCmds = 7;
        const char *cmds[] = {"logout", "dir", "create", "delete", "chmod", "read", "write"};
        const char *desc[] = {
            "user log out",
            "list all the files",
            "create a file",
            "delete a file",
            "change the permission of a file",
            "read a file",
            "write into a file"};
        printCmdHelp(NULL, numOfCmds, cmds, desc, instr);
    }
    acceptEnter("\nPress [Enter] to continue...");
    {
        const char *instr = "3.Commands only for admin user root.(只有管理员账号才能用的命令))\nNote: username: root; password: root";
        int numOfCmds = 1;
        const char *cmds[] = {"account"};
        const char *desc[] = {
            "handle register accounts"};
        printCmdHelp(NULL, numOfCmds, cmds, desc, instr);
    }
    acceptEnter("\nPress [Enter] to continue...");
    {
        // C语言神奇的字符串拼接方式，可以用来是实现跨行wahhhhh
        const char *instr = "4.Commands \e[1;31mNot offerred\e[1;37m by this file system.\n"
                            "It seems not so insteresting to open a file in the command line and to operate its file pointer.\n"
                            "So this file system offers some APIs (implemented in C language) to operate file input and output.\n"
                            "\e[1;33mFILE* SFS_open(const char* username, const char* password, const char* filename, const char* mode);\n"
                            "int SFS_close(const char* username, const char* password, const char* filename, const char* mode, FILE*);\n"
                            "\e[1;37m(\"在命令行中打开一个文件，并且操作它的文件指针\"？？emmmm~~这听起来不会是一件有趣的事，\n"
                            "所以该文件系统另外提供了一套用C语言封装的API来辅助文件操作。)";
        int numOfCmds = 2;
        const char *cmds[] = {"open", "close"};
        const char *desc[] = {
            "open a file\e[1;37m -- \e[1;31mnot supported\e[1;m in command line mod\e[1;33m", "close a file\e[1;37m -- 命令行模式下\e[1;31m不支持\e[1;m该操作"};
        printCmdHelp(NULL, numOfCmds, cmds, desc, instr);
    }

    acceptEnter("\nPress [Enter] to continue...");

    CHANGE_TO_CYAN_STYTLE;
    printf("Part 2: Console Input and Output Promotion (终端输入输出提示)\n");

    printf("\e[1;37m%-6s\t%-13s\t%-20s\t%-23s\n", "color", "usage", "提示", "example");
    printf("\e[1;32m%-6s\t%-13s\t%-20s\t%-23s\n", "green", "username tag", "用户标签", "$root>");
    printf("\e[1;33m%-6s\t%-13s\t%-20s\t%-23s\n", "yellow", "input line", "用户输入、命令高亮", "username:");
    printf("\e[1;36m%-6s\t%-13s\t%-20s\t%-23s\n", "cyan", "highlight", "高亮突出", "instruction:");
    printf("\e[1;34m%-6s\t%-13s\t%-20s\t%-23s\n", "blue", "title", "标题等", "No  Usage  Description");
    printf("\e[1;31m%-6s\t%-13s\t%-20s\t%-23s\n", "red", "error / warning", "出错警告", "Invalid arguments!");

    acceptEnter("\nPress [Enter] to continue...");

    CHANGE_TO_CYAN_STYTLE;
    printf("Part3 Some Notes.\n\n");
    printf("\e[1;34m#Note1: \e[1;37mWhenever you want to use a command <cmd>, type <cmd --help> first,"
           " such as \"login --help\". (建议每次使用不熟悉的命令，都先码一句help先。)\n");

    printf("\e[1;34m#Note2: \e[1;31m<filename>\e[1;37m means file argument specified by user. For every argument <some arugment>, you have to add double quotas both before and after it "
           "if it has special character such as white space and so on. "
           "For example, the name of the file, Operating System 4th edition, should be written as \e[1;31m\"Operating System 4th edition\"\e[1;37m. "
           "(<filename>意味着用户指定的文件参数。对于含有特殊字符的参数，例如空白符，需要用双引号把参数包括起来，例如\e[1;31m\"操作系统 第4版\"\e[1;37m)\n");

    printf("\e[1;34m#Note3: \e[1;37mEvery time you finish typing, please press [enter]. (每次输入结束，都要按[回车键])\n");

    printf("\e[1;34m#Note4: \e[1;37m关于用户输入\"正确性\"\n");
    printf("——如果我输入错误的指令，那么要怎么样才能得到正确的输出？\n");
    printf("——到底是怎样的思维才能让你对软件有这种想法？？WHAT THE HELL!\n");
    printf("所以该系统所能做的，只是尽量地不让错误的输入使整个系统奔溃而已，很遗憾它无法把错误的输入转化为正确的输出:)\n");

    printf("\nFor more details about the author and the implementation about the file system, it's adivise to type [readme]...\n");
    printf("好了，读了这么长的help文档，你可以开始探索(折腾)这个文件系统了，enjoy yourself ~~\n");

    CHANGE_TO_DEFAULT_STYTLE;
}

void handleRegister(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        char username[MID_STR_LEN] = "";
        char password[MID_STR_LEN] = "";

        bool valid = true;
        if (argc == 1)
        {
            acceptALineFromCMD(username, "username: ");
        }
        else if (argc == 3 && strcmp(argv[1], "-u") == 0)
        {
            strcpy(username, argv[2]);
        }
        else if (argc == 5 && strcmp(argv[1], "-u") == 0 && strcmp(argv[3], "-p") == 0)
        {
            strcpy(username, argv[2]);
            strcpy(password, argv[4]);

            if (isSFSRunning)
                printErrorMs("Not recommended to enter password directly!(不建议直接输入密码)");
        }
        else
            valid = false;

        if (valid)
        {
            if (argc != 5)
            {
                acceptAPasword(password, "password: ");
                char confirmPS[MID_STR_LEN] = {};
                acceptAPasword(confirmPS, "confirm password: ");
                if (strcmp(password, confirmPS) != 0)
                {
                    printErrorMs("Inconsistent password!(两次输入密码不一致)");
                    return;
                }
            }

            //判断账号是否存在
            if (accounts->getNode(accounts, username) != NULL)
            {
                char ms[LONG_STR_LEN] = {};
                sprintf(ms, "user [%s] already exist!", username);

                if (isSFSRunning)
                    printErrorMs(ms);
                return;
            }

            Account newAccount;
            strcpy(newAccount.username, username);
            strcpy(newAccount.password, password);

            accounts->addLast(accounts, &newAccount);
            accounts->writeFile(accounts, accountsFilePath); // 顺便写文件，防止出现bug系统奔溃

            // printf("Register Account [%s] sucessfully!\n", username);
            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 4;
    const char *cmds[] = {"register --help", "register", "register -u <username>", "register -u <username> -p <password>"};
    const char *desc[] = {"display this message", "", "", "Not recommended to enter password directly"};
    const char *instr = "Register an account.\nConstraint: The length of field <username> and <password> shouldn't exceed 64 characters.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);
}

// 这样的代码可读性才高
// void handleLogin(int argc, const char *argv[])
bool handleLogin(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        char username[MID_STR_LEN] = "";
        char password[MID_STR_LEN] = "";

        bool valid = true;
        if (argc == 1)
        {
            acceptALineFromCMD(username, "username: ");
        }
        else if (argc == 2)
        {
            strcpy(username, argv[1]);
        }
        else if (argc == 3 && strcmp(argv[1], "-u") == 0)
        {
            strcpy(username, argv[2]);
        }
        else if (argc == 5 && strcmp(argv[1], "-u") == 0 && strcmp(argv[3], "-p") == 0)
        {
            strcpy(username, argv[2]);
            strcpy(password, argv[4]);

            if (isSFSRunning)
                printErrorMs("Not recommended to enter password directly!(不建议直接输入密码)");
        }
        else
            valid = false;

        if (valid)
        {
            // if (strlen(password) == 0)
            if (argc != 5)
            {
                acceptAPasword(password, "password: ");
            }

            // 先退出登录
            if (currUsername != NULL)
            {
                char choice = getAChoice("Are you sure to logout first?");
                if (choice == 'Y')
                    handleLogout(1, NULL);
                else
                    return false;
            }

            MLList_Node *curr = accounts->getNode(accounts, username);
            if (curr != NULL)
            {
                if (strcmp(password, ((Account *)(curr->element))->password) == 0)
                {
                    // init userDirFilePath
                    strcpy(userDirFilePath, dirPath);
                    currUsername = ((Account *)(curr->element))->username; // 更新当前用户
                    strcat(userDirFilePath, currUsername);
                    strcat(userDirFilePath, ".dir");

                    userDir = (MLList *)malloc(sizeof(MLList));
                    MLList_init(userDir, sizeof(UFile), UFile_Assign);
                    userDir->search = UFile_Search;

                    if (access(userDirFilePath, F_OK) == 0)
                        userDir->readFile(userDir, userDirFilePath);

                    // printf("User [%s] logins successfully!\n", username);
                    return true;
                }
                else
                {
                    if (isSFSRunning)
                        printErrorMs("Invalid password!");
                }
            }
            else
            {
                if (isSFSRunning)
                {
                    char ms[LONG_STR_LEN] = {};
                    sprintf(ms, "user [%s] doesn't exist!", username);
                    printErrorMs(ms);
                }
            }
            return false;
        }
        else
        {
            if (isSFSRunning)
                printErrorMs("Invalid arguments!");
            return false;
        }
    }

    int numOfCmds = 5;
    const char *cmds[] = {"login --help", "login", "login <username>", "login -u <username>", "login -u <username> -p <password>"};
    const char *desc[] = {"display this message", "", "", "", ""};
    const char *instr = "Sign in for an account.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);

    return false;
}

void handleLogout(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        char username[MID_STR_LEN] = {};
        if (argc == 1)
            valid = true;
        else if (argc == 2)
        {
            strcpy(username, argv[1]);
        }
        else if (argc == 3 && strcmp(argv[1], "-u") == 0) // 需要检查 -u
        {
            strcpy(username, argv[2]);
        }
        else
            valid = false;

        if (valid)
        {
            if (argc == 1 || strcmp(username, currUsername) == 0)
            {
                currUsername = NULL;
                userDir->destroy(userDir);
                userDir = NULL;
                // printf("Logout successfully!\n");
            }
            else
            {
                printErrorMs("Invalid username!\n");
            }
            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 4;
    const char *cmds[] = {"logout --help", "logout", "logout <username>", "logout -u <username>"};
    const char *desc[] = {"display this message", "", "", ""};
    const char *instr = "Log out for an account so you can login another.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);
}

void handleDir(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        if (argc == 1 || argc == 2 && strcmp(argv[1], "-l") == 0)
        {
            int filenameMaxLength = strlen("filename");
            int lengthStrMax = strlen("length");
            int addrMaxLength = strlen("Addr");
            char tempNumFormatStr[MINI_STR_LEN];
            for (MLList_Node *curr = userDir->head; curr != NULL; curr = curr->next)
            {
                UFile *temp = ((UFile *)(curr->element));
                if (strlen(temp->filename) > filenameMaxLength)
                    filenameMaxLength = strlen(temp->filename);

                sprintf(tempNumFormatStr, "%lu", temp->fileLength);
                if (strlen(tempNumFormatStr) > lengthStrMax)
                    lengthStrMax = strlen(tempNumFormatStr);

                sprintf(tempNumFormatStr, "%u", temp->addr);
                if (strlen(tempNumFormatStr) > addrMaxLength)
                    addrMaxLength = strlen(tempNumFormatStr);
            }

            char tempFormatStr[LONG_STR_LEN] = {};
            char formatStr[LONG_STR_LEN] = "%-";
            sprintf(tempFormatStr, "%d", filenameMaxLength);
            strcat(formatStr, tempFormatStr);
            strcat(formatStr, "s  %");
            sprintf(tempFormatStr, "%d", lengthStrMax);
            strcat(formatStr, tempFormatStr);
            strcat(formatStr, "lu  %10s  %-");
            sprintf(tempFormatStr, "%d", addrMaxLength);
            strcat(formatStr, tempFormatStr);
            strcat(formatStr, "u  %-24s  %-24s  %-24s\n");

            // formatStr ~~ "%-32s %8ul %10s %-8u\n"
            // char tmStr[] = "2018-08-22 Wed 11:30:00";

            strcpy(tempFormatStr, formatStr);
            for (int i = 0; i < strlen(tempFormatStr); i++)
            {
                if (tempFormatStr[i] == 'd' || tempFormatStr[i] == 'u')
                    tempFormatStr[i] = 's';
                else if (tempFormatStr[i] == 'l' && tempFormatStr[i + 1] == 'u')
                {
                    tempFormatStr[i++] = 's';
                    int tempLen = strlen(tempFormatStr);
                    for (int j = i; j < tempLen; j++)
                    {
                        tempFormatStr[j] = tempFormatStr[j + 1];
                    }
                }
            }
            // tempFormatStr ~~ "%-32s %8s %10s %-8s\n"

            CHANGE_TO_BLUE_STYTLE;
            printf(tempFormatStr, "filename", "length", "permission", "Addr", "createdTime", "lastModiffiedTime", "lastAccessTime");
            CHANGE_TO_DEFAULT_STYTLE;
            for (MLList_Node *curr = userDir->head; curr != NULL; curr = curr->next)
            {
                UFile *temp = ((UFile *)(curr->element));
                char permissionStr[10] = {};
                getPermissionStr(permissionStr, temp->permission);

                char createTimeStr[SHORT_STR_LEN] = {};
                char lastModifiedTimeStr[SHORT_STR_LEN] = {};
                char lastAccessTimeStr[SHORT_STR_LEN] = {};
                getTmStr(createTimeStr, &(temp->createdTime));
                getTmStr(lastModifiedTimeStr, &(temp->lastModifiedTime));
                getTmStr(lastAccessTimeStr, &(temp->lastAccessTime));

                printf(formatStr, temp->filename, temp->fileLength, permissionStr, temp->addr, createTimeStr, lastModifiedTimeStr, lastAccessTimeStr);
            }
            return;
        }
        else if (argc == 2 && strcmp(argv[1], "-a") == 0)
        {
            // int maxHeadLen = strlen("lastModifiedTime");
            CHANGE_TO_BLUE_STYTLE;
            printf("The total number of files: %lu\n\n", userDir->size);
            CHANGE_TO_DEFAULT_STYTLE;
            for (MLList_Node *curr = userDir->head; curr != NULL; curr = curr->next)
            {
                UFile *temp = ((UFile *)(curr->element));
                char permissionStr[10] = {};
                getPermissionStr(permissionStr, temp->permission);

                char createTimeStr[SHORT_STR_LEN] = {};
                char lastModifiedTimeStr[SHORT_STR_LEN] = {};
                char lastAccessTimeStr[SHORT_STR_LEN] = {};
                getTmStr(createTimeStr, &(temp->createdTime));
                getTmStr(lastModifiedTimeStr, &(temp->lastModifiedTime));
                getTmStr(lastAccessTimeStr, &(temp->lastAccessTime));

                printf("\e[1;34m%17s:\e[0m  %s\n", "filename", temp->filename);
                printf("\e[1;34m%17s:\e[0m  %lu B\n", "length", temp->fileLength);
                printf("\e[1;34m%17s:\e[0m  %s\n", "permission", permissionStr);
                printf("\e[1;34m%17s:\e[0m  File%u\n", "address", temp->addr);
                printf("\e[1;34m%17s:\e[0m  %s\n", "createdTime", createTimeStr);
                printf("\e[1;34m%17s:\e[0m  %s\n", "lastModifiedTime", lastModifiedTimeStr);
                printf("\e[1;34m%17s:\e[0m  %s\n", "lastAccessTime", lastAccessTimeStr);
                printf("\n");
            }

            return;
        }
        else
            valid = false;

        if (!valid)
        {
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 3;
    const char *cmds[] = {"dir --help", "dir -l", "dir -a"};
    const char *desc[] = {"display this message", "show all files in a list view(以列表形式显示文件)", "show the files one by one"};
    const char *instr = "Show all files.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);
}

void handleCreate(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        char filename[MID_STR_LEN];
        bool valid = true;
        if (argc == 1)
        {
            acceptALineFromCMD(filename, "filename: ");
        }
        else if (argc == 2)
        {
            strcpy(filename, argv[1]);
        }
        else
            valid = false;

        if (valid)
        {
            if (userDir->getNode(userDir, filename) != NULL)
            {
                char promt[LONG_STR_LEN] = {};
                sprintf(promt, "File [%s] already exists. \nWant to overwrite it?", filename);
                char choice = getAChoice(promt);
                if (choice == 'N')
                    return;
            }

            // init newFile
            UFile newFile;
            getCurrentTime(&newFile.createdTime);
            newFile.lastModifiedTime = newFile.createdTime;
            newFile.lastAccessTime = newFile.createdTime;

            strcpy(newFile.filename, filename);
            newFile.permission = 0755;
            newFile.fileLength = 0;

            newFile.addr = checkoutAFreeDiskAddr();
            char fileAddr[MINI_STR_LEN];
            sprintf(fileAddr, "File%u", newFile.addr);

            userDir->addLast(userDir, (void *)(&newFile));
            userDir->writeFile(userDir, userDirFilePath);

            char newFilePath[MID_STR_LEN];
            strcpy(newFilePath, usrPath);
            strcat(newFilePath, fileAddr);
            FILE *newFP = fopen(newFilePath, "wb");
            fclose(newFP);

            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 3;
    const char *cmds[] = {"create --help", "create", "create <filename>"};
    const char *desc[] = {"display this message", "", ""};
    const char *instr = "Create a file with the default permission 0755.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);
}

void handleDelete(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        char filename[MID_STR_LEN];
        bool valid = true;
        if (argc == 1)
        {
            acceptALineFromCMD(filename, "filename: ");
        }
        else if (argc == 2)
        {
            strcpy(filename, argv[1]);
        }
        else
            valid = false;

        if (valid)
        {
            int i = 0;
            for (MLList_Node *curr = userDir->head; curr != NULL; curr = curr->next)
            {
                if (strcmp(filename, ((UFile *)(curr->element))->filename) == 0)
                {
                    char choice = getAChoice("Are you sure to delete it?");
                    if (choice == 'N')
                    {
                        return;
                    }
                    else
                    {
                        char fileAddr[MINI_STR_LEN];
                        sprintf(fileAddr, "File%u", ((UFile *)curr->element)->addr);
                        char deleteFilePath[LONG_STR_LEN];
                        strcpy(deleteFilePath, usrPath);
                        strcat(deleteFilePath, fileAddr);

                        remove(deleteFilePath);

                        freeADiskAddr(((UFile *)curr->element)->addr);
                        userDir->remove(userDir, i);
                        userDir->writeFile(userDir, userDirFilePath);

                        return;
                    }
                }
                i++;
            }

            char ms[LONG_STR_LEN] = {};
            sprintf(ms, "File [%s] doesn't exist!", filename);
            printErrorMs(ms);
            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 3;
    const char *cmds[] = {"delete --help", "delete", "delete <filename>"};
    const char *desc[] = {"display this message", "", ""};
    const char *instr = "Delete a file.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instr);
}

void handleChmod(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        char filename[MID_STR_LEN] = {};
        const char *pmPtr = NULL; // 以下代码中没有通过pmPtr改动它指向的原有字符串
        if (argc == 1)
            acceptALineFromCMD(filename, "filename: ");
        else if (argc == 2)
            strcpy(filename, argv[1]);
        else if (argc == 3)
        {
            strcpy(filename, argv[1]);
            pmPtr = argv[2];
        }
        else
            valid = false;

        if (valid)
        {
            unsigned int permission = 01000;
            // if (pmPtr == NULL)
            if (argc != 3)
            {
                char pmStr[10] = {};
                acceptALineFromCMD(pmStr, "permission: ");
                pmPtr = pmStr;
            }

            bool validPerCode = true;
            if (pmPtr[0] >= '0' && pmPtr[0] <= '7')
            {
                for (int i = 0; i < strlen(pmPtr); i++)
                {
                    if (!(pmPtr[i] >= '0' && pmPtr[i] <= '7'))
                    {
                        validPerCode = false;
                        break;
                    }
                }

                validPerCode &= strlen(pmPtr) <= 4;
                if (validPerCode)
                    sscanf(pmPtr, "%o", &permission);
            }
            else if (pmPtr[0] == 'r' || pmPtr[0] == '-')
            {
                if (strlen(pmPtr) == 9)
                    permission = getPermissionCode(pmPtr);
                else
                    validPerCode = false;
            }
            else
                validPerCode = false;

            if (!validPerCode || permission >= 01000)
            {
                printf("Invalid permssion\n");
                return;
            }

            MLList_Node *curr = userDir->getNode(userDir, filename);
            if (curr != NULL)
            {
                UFile *tempFile = (UFile *)(curr->element);
                tempFile->permission = permission;
                getCurrentTime(&(tempFile->lastModifiedTime)); // 更新权限，顺便更新最后改动时间
                userDir->writeFile(userDir, userDirFilePath);
            }
            else
            {
                char ms[LONG_STR_LEN] = {};
                sprintf(ms, "File [%s] doesn't exist!", filename);
                printErrorMs(ms);
            }

            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    // 支持类型
    int numOfCmds = 4;
    const char *cmds[] = {"chmod --help", "chmod", "chmod <filename>", "chomd <filename> <permission>"};
    const char *desc[] = {"display this message", "", "", ""};
    const char *instruction = "Change a file's permission. \nValid permission: eg. any octal number(任意八进制数字) from [0000, 0777]; OR any string like \"rwxr--r--\"";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instruction);
}

void handleOpen(int argc, const char *argv[])
{
    printErrorMs("Not supported in command line mod.");
}

void handleClose(int argc, const char *argv[])
{
    printErrorMs("Not supported in command line mod.");
}

void handleRead(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        char filename[MID_STR_LEN] = {};
        if (argc == 1)
        {
            acceptALineFromCMD(filename, "filename: ");
        }
        else if (argc == 2)
        {
            strcpy(filename, argv[1]);
        }
        else
            valid = false;

        if (valid)
        {
            MLList_Node *curr = userDir->getNode(userDir, filename);
            if (curr != NULL)
            {
                UFile *tempFile = (UFile *)(curr->element);
                char pmStr[10];
                getPermissionStr(pmStr, tempFile->permission);
                if (pmStr[0] == '-')
                {
                    char ms[LONG_STR_LEN] = {};
                    sprintf(ms, "You have no read permission for the file [%s]!", filename);
                    printErrorMs(ms);
                    return;
                }

                char fileAddr[SHORT_STR_LEN] = {};
                sprintf(fileAddr, "File%u", tempFile->addr);

                char tempFilePath[LONG_STR_LEN];
                strcpy(tempFilePath, usrPath);
                strcat(tempFilePath, fileAddr);
                FILE *fp = fopen(tempFilePath, "r");

                if (fp == NULL)
                {
                    printErrorMs("Failed to open the file!");
                    return;
                }

                fseek(fp, 0, SEEK_END);
                long fileLength = ftell(fp);
                if (fileLength > 0)
                {
                    fseek(fp, 0, 0);
                    int lineNo = 1;
                    char ch = '\n';
                    do
                    {
                        if (ch == '\n')
                        {
                            printf("%4d:\t", lineNo++);
                        }
                        ch = fgetc(fp);

                        if (ch != EOF)
                            putchar(ch);
                        else
                            break;
                    } while (1);
                    putchar('\n');
                }

                fclose(fp);

                getCurrentTime(&tempFile->lastAccessTime);
                userDir->writeFile(userDir, userDirFilePath);
            }
            else
            {
                char ms[LONG_STR_LEN] = {};
                sprintf(ms, "File [%s] doesn't exist!", filename);
                printErrorMs(ms);
            }

            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    // 支持类型
    int numOfCmds = 3;
    const char *cmds[] = {"read --help", "read", "read <filename>"};
    const char *desc[] = {"display this message", "", ""};
    const char *instruction = "Read the file in [ASCII plain text mode] and display line numbers.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instruction);
}

void handleWrite(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        char filename[MID_STR_LEN] = {};
        bool appendMod = true;
        if (argc == 1)
        {
            acceptALineFromCMD(filename, "filename: ");
        }
        else if (argc == 2)
        {
            strcpy(filename, argv[1]);
        }
        else if (argc == 3)
        {
            strcpy(filename, argv[2]);
            if (strcmp(argv[1], "-o") == 0)
                appendMod = false;
            else if (strcmp(argv[1], "-a") != 0)
                valid = false;
        }
        else
            valid = false;

        if (valid)
        {
            MLList_Node *curr = userDir->getNode(userDir, filename);
            if (curr != NULL)
            {
                UFile *tempFile = (UFile *)(curr->element);
                char pmStr[10];
                getPermissionStr(pmStr, tempFile->permission);
                if (pmStr[1] == '-')
                {
                    char ms[LONG_STR_LEN] = {};
                    sprintf(ms, "You have no write permission for the file [%s]!", filename);
                    printErrorMs(ms);

                    return;
                }

                char fileAddr[SHORT_STR_LEN] = {};
                sprintf(fileAddr, "File%u", tempFile->addr);

                char tempFilePath[LONG_STR_LEN];
                strcpy(tempFilePath, usrPath);
                strcat(tempFilePath, fileAddr);

                // getCurrentTime(&tempFile->lastAccessTime);

                // w+ 打开可读写文件，若文件存在则文件长度清为零，即该文件内容会消失
                // a 以附加的方式打开只写文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾，即文件原先的内容会被保留。（EOF符保留）

                FILE *fp = NULL;
                if (appendMod)
                    fp = fopen(tempFilePath, "a+");
                // fp = fopen(tempFilePath, "w+");
                else
                    fp = fopen(tempFilePath, "w");

                if (fp == NULL)
                {
                    printErrorMs("Failed to open the file!");
                    return;
                }

                char ch = '\n';
                do
                {
                    ch = getchar();

                    if (ch != EOF)
                        fputc(ch, fp);
                    else
                        break;
                } while (1);
                printf("\n");

                // fclose(fp);

                // 用一种C语言特有的方式获取文件大小
                // 把文件读到内存，然后指针调到文件最后，读取位置指针的偏移
                // fp = fopen(tempFilePath, "r");
                fseek(fp, 0, SEEK_END);
                tempFile->fileLength = ftell(fp);
                fclose(fp);

                getCurrentTime(&tempFile->lastModifiedTime);
                tempFile->lastAccessTime = tempFile->lastModifiedTime;
                userDir->writeFile(userDir, userDirFilePath);
            }
            else
            {
                char ms[LONG_STR_LEN] = {};
                sprintf(ms, "File [%s] doesn't exist.\n", filename);
                printErrorMs(ms);
            }

            return;
        }
        else
        {
            printErrorMs("Invalid arguments!");
        }
    }

    // 支持类型
    int numOfCmds = 5;
    const char *cmds[] = {"write --help", "write", "write <filename>", "write -a <filename>", "write -o <filename>"};
    const char *desc[] = {"display this message", "the same as \"write -a <filename>\"", "the same as \"write -a <filename>\"", "append to the end of the file", "overwrite the file"};
    const char *instruction = "Accept input from the console and write to a file in [ASCII plain text mode].\nWhen you end up input, press [Ctrl+D] twice to finish. (输入结束时，请按[Ctrl+D])";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instruction);
}

void handleClear(int argc, const char *argv[]) // 一个半小时就写了个clear功能，也是服了你了
{
    if (argc == 1)
    {
        // system("clear");
        system("reset"); // 还是这样清屏比较爽快
        return;
    }
    else if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        printErrorMs("Invalid arguments!");
    }
    // 支持类型
    int numOfCmds = 2;
    const char *cmds[] = {"clear --help", "clear"};
    const char *desc[] = {"display this message", ""};
    const char *instruction = "Call the linux terminal command [reset] to clear the console.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instruction);
}

//用于调试
void handleAccount(int argc, const char *argv[])
{
    if (!(argc == 2 && strcmp(argv[1], "--help") == 0))
    {
        bool valid = true;
        if (argc == 2 && (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "-a") == 0))
        {
            int nameMaxLen = strlen("username"), psMaxLen = strlen("password");
            for (MLList_Node *curr = accounts->head; curr != NULL; curr = curr->next)
            {
                Account *account = (Account *)(curr->element);
                if (strlen(account->username) > nameMaxLen)
                    nameMaxLen = strlen(account->username);
                if (strlen(account->password) > psMaxLen)
                    psMaxLen = strlen(account->password);
            }

            char temp[MID_STR_LEN] = {};
            char formatStr[MID_STR_LEN] = "%2";
            // sprintf(temp, "%zu", accounts->size);
            // strcat(formatStr, temp);
            strcat(formatStr, "d  %-");
            sprintf(temp, "%d", nameMaxLen);
            strcat(formatStr, temp);
            strcat(formatStr, "s");

            if (strcmp(argv[1], "-p") == 0)
            {
                strcat(formatStr, "  %-");
                sprintf(temp, "%d", psMaxLen);
                strcat(formatStr, temp);
                strcat(formatStr, "s");
            }
            strcat(formatStr, "\n");

            strcpy(temp, formatStr);
            for (int i = 0; i < strlen(temp); i++)
                if (temp[i] == 'd')
                    temp[i] = 's';

            if (strcmp(argv[1], "-p") == 0)
                printf(temp, "No", "username", "password");
            else
                printf(temp, "No", "username");
            int i = 0;
            for (MLList_Node *curr = accounts->head; curr != NULL; curr = curr->next, i++)
            {
                if (strcmp(argv[1], "-p") == 0)
                    printf(formatStr, i, ((Account *)(curr->element))->username, ((Account *)(curr->element))->password);
                else
                    printf(formatStr, i, ((Account *)(curr->element))->username);
            }
            return;
        }
        else
        {
            valid = false;
            printErrorMs("Invalid arguments!");
        }
    }

    int numOfCmds = 3;
    const char *cmds[] = {"account --help", "account -a", "account -p"};
    const char *desc[] = {"display this message", "show all register accounts", "show all register accounts and their passwords"};
    const char *instruction = "Operations for admin to handle accounts.";
    printCmdHelp(argv[0], numOfCmds, cmds, desc, instruction);
}

void printCmdHeader()
{
    // 设置控制台输出格式
    CHANGE_TO_GREEN_STYTLE;
    printf("$");
    if (currUsername != NULL)
        printf("%s", currUsername);
    printf(">");
    CHANGE_TO_DEFAULT_STYTLE;
}

// 实在是太酷了
void printCmdHelp(const char *cmdType, int numOfCmds, const char *cmds[], const char *desc[], const char *instruction)
{
    int cmdMaxLength = strlen("Command"), descMaxLength = strlen("Description");
    for (int i = 0; i < numOfCmds; i++)
    {
        if (strlen(cmds[i]) > cmdMaxLength)
        {
            cmdMaxLength = strlen(cmds[i]);
        }
        if (strlen(desc[i]) > descMaxLength)
        {
            descMaxLength = strlen(desc[i]);
        }
    }

    char temp[MID_STR_LEN] = {};
    char formatStr[MID_STR_LEN] = "%2d  %-"; // %%转义成%
    sprintf(temp, "%d", cmdMaxLength);
    strcat(formatStr, temp);
    strcat(formatStr, "s  %-");
    sprintf(temp, "%d", descMaxLength);
    strcat(formatStr, temp);
    strcat(formatStr, "s\n");

    CHANGE_TO_WHITE_STYTLE;
    if (cmdType != NULL)
        printf("<%s> Instruction: ", cmdType);
    if (instruction != NULL)
        printf("%s\n", instruction);

    // printf("[%s] Usage:\n", cmdType);

    strcpy(temp, formatStr);
    temp[2] = 's';
    CHANGE_TO_BLUE_STYTLE;
    printf(temp, "No", "Command", "Description");

    // CHANGE_TO_WHITE_STYTLE;
    CHANGE_TO_YELLOW_STYTLE;
    for (int i = 0; i < numOfCmds; i++)
    {
        printf(formatStr, i, cmds[i], desc[i]);
    }
    CHANGE_TO_DEFAULT_STYTLE;
}

void printErrorMs(const char *ms)
{
    CHANGE_TO_RED_STYTLE; // printf("\e[1;31m");
    printf("%s\n", ms);
    CHANGE_TO_DEFAULT_STYTLE;
}

void acceptALineFromCMD(char *dest, const char *promt)
{
    CHANGE_TO_YELLOW_STYTLE;

    if (promt != NULL)
        printf("%s", promt);

    // char *str = (char *)malloc(sizeof(char) * (strlen(dest) + 1)); // 问题在于：没有初始化的dest长度为0！！！！！shit
    char *str = (char *)malloc(sizeof(char) * LONG_LONG_STR_LEN);

    scanf("%*[\n]");
    scanf("%[^\n]s", str);
    setbuf(stdin, NULL); // linux GCC 编译器下真正有用的刷新stdin缓冲区的办法

    CHANGE_TO_DEFAULT_STYTLE;

    // 处理结尾多余的空白
    for (int i = strlen(str) - 1; str[i] == ' '; i--)
    {
        str[i] = '\0';
    }

    // 含有双引号的单个参数，顺便处理
    if (str[0] == '\"' &&
        str[strlen(str) - 1] == '\"' &&
        strchr(str + 1, '\"') == &str[strlen(str) - 1])
        strncpy(dest, str + 1, strlen(str) - 2);
    else
        strcpy(dest, str);
    free(str);
}

// 从终端接收密码，显示*号
// 这个函数的实现摘取了网上的博客
void acceptAPasword(char *ps, const char *promt)
{
    CHANGE_TO_YELLOW_STYTLE;
    if (promt != NULL && strlen(promt) > 0)
        printf("%s", promt);

    struct termios old, new;
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~(ECHO | ICANON);

    char ch;
    int i = 0;
    const int maxLen = MID_STR_LEN;

    while (1)
    {
        tcsetattr(0, TCSANOW, &new); //进入循环将stdin设置为不回显状态
        scanf("%c", &ch);            //在不回显状态下输入密码
        tcsetattr(0, TCSANOW, &old); //每次输入一个密码的字符就恢复正常回显状态
        if (i == maxLen - 1 || ch == '\n')
        {
            printf("\n");
            break;
        } //输入回车符表示密码输入完毕，退出循环；或者超出密码长度退出循环

        else if (ch == 127) // 127退格键
        {
            if (i > 0)
            {
                ps[--i] = '\0';
                printf("\b \b");
            }
        }
        else
        {
            ps[i] = ch;  //将输入的单个字符依次存入数组中
            printf("*"); //在回显状态下输出*
            i++;
        }
    }
    ps[i] = '\0';
    CHANGE_TO_DEFAULT_STYTLE;
}

char getAChoice(const char *promt)
{
    CHANGE_TO_YELLOW_STYTLE;
    if (promt != NULL)
        printf("%s", promt);
    printf("[Y/N]: ");
    char choice = ' ';
    scanf("%*[^YN]");
    scanf("%c", &choice);
    setbuf(stdin, NULL);
    CHANGE_TO_DEFAULT_STYTLE;
    return choice;
}

void acceptEnter(const char *promt)
{
    CHANGE_TO_YELLOW_STYTLE;
    if (promt != NULL)
        printf("%s", promt);
    setbuf(stdin, NULL);
    scanf("%*[^\n]");
    getchar();
    setbuf(stdin, NULL);

    if (promt != NULL)
    {
        printf("\e[A");
        for (int i = 0; i <= strlen(promt); i++)
            printf(" ");
        printf("\n");
    }
    printf("\e[A");
}

char **getACMDLine(int *argcPtr, int maxArgvSize)
{
    *argcPtr = 0;
    MLList *argvList = (MLList *)malloc(sizeof(MLList));
    MLList_init(argvList, sizeof(char) * maxArgvSize, cstring_Assign);

    char str[LONG_LONG_STR_LEN] = {};
    acceptALineFromCMD(str, "");

    char *w = (char *)malloc(sizeof(char) * maxArgvSize);
    int i = 0;

    while (i < strlen(str) && i < maxArgvSize)
    {
        int j = 0;
        clearStr(w);
        while (i < strlen(str) && str[i] == ' ')
            i++;

        if (i < strlen(str) && i < maxArgvSize && str[i] == '\"')
        {
            while (str[++i] != '\"')
            {
                w[j++] = str[i];
            }
            i++;
        }
        else //if (str[i] == ' ')
        {
            while (i < strlen(str) && i < maxArgvSize && str[i] != ' ')
            {
                w[j++] = str[i++];
            }
        }

        w[j] = '\0';
        if (strlen(w) > 0)
            argvList->addLast(argvList, w);
    }

    free(w);
    w = NULL;

    //申请内存
    *argcPtr = argvList->size;
    char **argv = (char **)malloc(sizeof(char *) * (*argcPtr));
    MLList_Node *curr = argvList->head;
    for (int i = 0; i < *argcPtr; i++)
    {
        argv[i] = (char *)malloc(sizeof(char) * maxArgvSize);
        strcpy(argv[i], (char *)(curr->element));
        curr = curr->next;
    }
    argvList->destroy(argvList);
    return argv;
}

void freeACMDLine(int argc, char *argv[])
{
    // 释放内存
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }
    free(argv);
}

void getPermissionStr(char *dest, unsigned int permission)
{
    char token[] = {'r', 'w', 'x'};
    char result[10] = {}; // 留一个位置给'\0'
    for (int i = 0; i < 9; i++)
        result[i] = '-';
    for (unsigned int i = 0, term = permission; term > 0; i++)
    {
        unsigned int m = term % 2;
        term /= 2;
        if (m == 1)
        {
            result[8 - i] = token[2 - i % 3];
        }
    }

    strcpy(dest, result);
}

unsigned int getPermissionCode(const char *permissionStr)
{
    bool validCode = strlen(permissionStr) == 9;
    char token[3] = {'r', 'w', 'x'};
    unsigned int proNum = 0;
    for (int i = 0; i < 9; i++)
    {
        proNum *= 2;
        if (permissionStr[i] == token[i % 3])
        {
            proNum++;
        }
        else if (permissionStr[i] != '-')
        {
            validCode = false;
            break;
        }
    }

    if (validCode)
        return proNum;
    else
    {
        // printf("Invalid permission string!");
        return 01000;
    }
}

// 实现形式1：虽然没有和当前运行的文件系统打交道，但是调用这两个函数，可以保证文件的一致性，这就足够了
FILE *SFS_open(const char *username, const char *password, const char *filename, const char *mode)
{
    // 为了不影响到当前界面（正在体验终端）的用户，不能直接调用login函数，不然会logout切换用户的
    // 忽略上一句话

    initSFS();
    char *argv[5];
    argv[0] = "login";
    argv[1] = "-u";
    argv[2] = username;
    argv[3] = "-p";
    argv[4] = password;

    FILE *fp = NULL;
    if (handleLogin(5, argv))
    {
        MLList_Node *tempNode = userDir->getNode(userDir, filename);
        if (tempNode != NULL)
        {
            UFile *tempFile = (UFile *)(tempNode->element);

            // 检查读写访问权限
            char pmStr[10] = {};
            getPermissionStr(pmStr, tempFile->permission);
            if (
                !(
                    mode[strlen(mode) - 1] == '+' && !(pmStr[0] == 'r' && pmStr[1] == 'w') ||
                    mode[0] == 'r' && pmStr[0] != 'r' ||
                    (mode[0] == 'w' || mode[0] == 'a') && pmStr[0] != 'w'))
            {
                char tempFilePath[LONG_STR_LEN] = {};
                strcpy(tempFilePath, usrPath);

                char fileAddr[MINI_STR_LEN] = {};
                sprintf(fileAddr, "File%u", tempFile->addr);
                strcat(tempFilePath, fileAddr);

                fp = fopen(tempFilePath, mode);
            }
        }
    }

    exitSFS();
    return fp;
}

// int SFS_close(FILE *fp)
int SFS_close(const char *username, const char *password, const char *filename, const char *mode, FILE *fp)
{
    initSFS();
    char *argv[5];
    argv[0] = "login";
    argv[1] = "-u";
    argv[2] = username;
    argv[3] = "-p";
    argv[4] = password;

    int closeState = EOF;
    if (handleLogin(5, argv))
    {
        MLList_Node *tempNode = userDir->getNode(userDir, filename);
        if (tempNode != NULL)
        {
            UFile *tempFile = (UFile *)(tempNode->element);
            // int openMode = fp->_mode;

            fseek(fp, 0, SEEK_END);
            tempFile->fileLength = ftell(fp);
            closeState = fclose(fp);
            if (closeState == 0) // 能够正常关闭文件指针
            {
                getCurrentTime(&tempFile->lastAccessTime);

                if (mode[strlen(mode) - 1] == '+' || mode[0] == 'w' || mode[0] == 'a')
                {
                    tempFile->lastModifiedTime = tempFile->lastAccessTime;
                }

                userDir->writeFile(userDir, userDirFilePath);
            }
        }
    }

    exitSFS();
    return closeState;
}

void handleReadme(int argc, const char*argv[])
{
    FILE* readmeFP = fopen("readme.txt", "r");
    if(readmeFP==NULL)
    {
        printErrorMs("Readme.txt file is lost.");
        return;
    }

    char ch = ' ';
    while(1)
    {
        ch = fgetc(readmeFP);
        if(ch != EOF)
            putchar(ch);
        else break;
    }
    putchar('\n');
    fclose(readmeFP);
}