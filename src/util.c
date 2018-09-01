#include "util.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// #include <unistd.h>

void strToLowerCase(char* str)
{
    size_t len = strlen(str);
    int gap = 'A' - 'a';
    for(int i=0;i<len;i++)
    {
        if(str[i] >= 'A' && str[i] <= 'Z')
            str[i] = str[i] - gap;
    }
}

void strToUpperCase(char* str)
{
    size_t len = strlen(str);
    int gap = 'A' - 'a';
    for(int i=0;i<len;i++)
    {
        if(str[i] >= 'a' && str[i] <= 'z')
            str[i] = str[i] + gap;
    }
}

void clearStr(char* str)
{
    int len = strlen(str);
    for(int i=0;i<len;i++)
    {
        str[i] = '\0';
    }
}

void getCurrentTime(struct tm* t)
{
    time_t temp;
    temp = time(NULL);
    struct tm* t1 = localtime(&temp);
    t->tm_hour = t1->tm_hour;
    t->tm_isdst = t1->tm_isdst;
    t->tm_mday = t1->tm_mday;
    t->tm_min = t1->tm_min;
    t->tm_mon = t1->tm_mon;
    t->tm_sec = t1->tm_sec;
    t->tm_wday = t1->tm_wday;
    t->tm_yday = t1->tm_yday;
    t->tm_year = t1->tm_year;
    // t->tm_gmtoff = t1->tm_gmtoff;
    // t->tm_zone = t1->tm_zone;
}

void getTmStr(char *destStr, const struct tm* t)
{
    const char* WeekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    sprintf(destStr, "%4d-%02d-%02d %3s %02d:%02d:%02d", 1900 + t->tm_year,1 + t->tm_mon, t->tm_mday, WeekDays[t->tm_wday], t->tm_hour, t->tm_min, t->tm_sec);
}