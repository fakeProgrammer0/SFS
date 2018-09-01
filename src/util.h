#ifndef UTIL_H
#define UTIL_H

#include <time.h>

void strToLowerCase(char*);
void strToUpperCase(char*);
void clearStr(char*);

void getCurrentTime(struct tm*);
void getTmStr(char *, const struct tm*);

#endif