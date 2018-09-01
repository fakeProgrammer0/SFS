#include "MLList.h"

#include <stdio.h>
#include <stdlib.h>

void MLList_destroy(MLList *);
void MLList_addLast(MLList *, const void *);
bool MLList_remove(MLList *, int);
void MLList_clear(MLList *);

bool MLList_writeFile(const MLList *, const char *);
bool MLList_readFile(MLList *, const char *);

MLList_Node *MLList_getNode(const MLList *, const void *);

void MLList_init(MLList *self, size_t elementSize, const void (*assign)(void *, const void *))
{
    // 在这里初始化没有用，因为函数里的self只是原来指针的副本而已
    // 所以必须要保证self已经初始化
    // if (self == NULL)
    // {
    //     self = (MLList *)malloc(sizeof(MLList));
    // }

    self->size = 0;
    self->head = self->tail = NULL;

    //注册各个函数指针
    self->destroy = MLList_destroy;
    self->addLast = MLList_addLast;
    self->remove = MLList_remove;
    self->clear = MLList_clear;

    self->readFile = MLList_readFile;
    self->writeFile = MLList_writeFile;

    self->elementSize = elementSize;
    self->assign = assign;

    self->search = NULL;
    self->getNode = MLList_getNode;
}

void MLList_destroy(MLList *self)
{
    MLList_clear(self);
    free(self);
}

void MLList_clear(MLList *self)
{
    MLList_Node *curr = self->head;
    MLList_Node *temp;
    while (curr != NULL)
    {
        free((curr->element));
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    self->head = self->tail = NULL;
    self->size = 0;
}

// void MLList_addLast(MLList *self, const void *item)
void MLList_addLast(MLList *self, const void *item)
{
    MLList_Node *newNode = (MLList_Node *)malloc(sizeof(MLList_Node));
    // newNode->element = malloc(sizeof(self->elementSize)); // 我去，sizeof(self->elementSize)相当于sizeof(size_t) == 8
    newNode->element = malloc(self->elementSize); // 为element申请空间，很重要；不然element只是一个void*而已
    // newNode->element = item; // 浅复制
    self->assign(newNode->element, item); // 深复制
    newNode->next = NULL;

    if (self->head == NULL)
    {
        self->head = self->tail = newNode;
    }
    else
    {
        MLList_Node *temp = self->tail;
        temp->next = newNode;
        self->tail = newNode;
    }
    self->size++;
}

bool MLList_remove(MLList *self, int index)
{
    if (index < 0 || index >= self->size)
    {
        printf("Invalid index: [%d]\n", index);
        return false;
    }

    MLList_Node *removeNode;

    if (index == 0)
    {
        // free(self->head->element); // 释放深复制申请的内存
        removeNode = self->head;
        if (self->head->next == NULL)
        {
            self->head = self->tail = NULL;
        }
        else
        {
            self->head = self->head->next;
        }
    }
    else
    {
        MLList_Node *prev = self->head;
        for (int i = 0; i < index - 1; i++)
        {
            prev = prev->next;
        }

        // free(prev->next->element);
        removeNode = prev->next;

        prev->next = prev->next->next;
        if (index == self->size - 1)
        {
            self->tail = prev;
        }
    }

    free(removeNode->element);
    free(removeNode);
    self->size--;
    return true;
}

bool MLList_writeFile(const MLList *self, const char *filePath)
{
    FILE *fp = fopen(filePath, "wb");
    // TODO: 判断文件打开是否成功

    MLList_Node *curr = self->head;
    while (curr != NULL)
    {
        fwrite(curr->element, self->elementSize, 1, fp);
        curr = curr->next;
    }
    fclose(fp);
    return true;
}

bool MLList_readFile(MLList *self, const char *filePath)
{
    FILE *fp = fopen(filePath, "rb");
    // TODO: 判断文件打开是否成功

    while (1)
    {
        void *element = malloc(self->elementSize);
        fread(element, self->elementSize, 1, fp);
        if (feof(fp))
        {
            free(element);
            break;
        }
        self->addLast(self, element);
        free(element);
    }
    fclose(fp);
    return true;
}

MLList_Node *MLList_getNode(const MLList *self, const void *key)
{
    if (self->search != NULL)
        for (MLList_Node *curr = self->head; curr != NULL; curr = curr->next)
        {
            if (self->search(curr->element, key))
            {
                return curr;
            }
        }
    return NULL;
}