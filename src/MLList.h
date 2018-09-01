#ifndef MLLIST_H
#define MLLIST_H

#include <stdbool.h>
#include <stdlib.h>

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

    // void (*init)(struct LList*); //省略
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

#endif