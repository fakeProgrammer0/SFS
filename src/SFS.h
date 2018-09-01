#ifndef SFS_H
#define SFS_H

#include <stdio.h> // 包含FILE结构体的定义

void runSFS();
FILE* SFS_open(const char* username, const char* password, const char* filename, const char* mode);
int SFS_close(const char* username, const char* password, const char* filename, const char* mode, FILE*);

// 一些IO函数
void printErrorMs(const char *ms);

void acceptALineFromCMD(char *dest, const char *promt);
void acceptAPasword(char *ps, const char *promt);
char getAChoice(const char *promt);
void acceptEnter(const char *promt);

char **getACMDLine(int *argcPtr, int maxArgvSize);
void freeACMDLine(int argc, char *argv[]);

#endif
