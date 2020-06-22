#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int
array_grow(void **addr, unsigned nelem, unsigned size)
{
    void *tmp;

    tmp = realloc(*addr, nelem * size);
    *addr = (tmp != NULL) ? tmp : *addr;
    return tmp != NULL;
}

int
monthdist(time_t start, time_t end)
{
    struct tm t;
    int sy, sm;
    int ey, em;

    localtime_r(&start, &t);
    sy = t.tm_year + 1900;
    sm = t.tm_mon + 1;
    
    localtime_r(&end, &t);
    ey = t.tm_year + 1900;
    em = t.tm_mon + 1;

    return (ey - sy) * 12 - sm + em;
}

long
cents(char *num)
{
    long val;
    int cents;
    int i;
    int len;

    len = strlen(num);
    val = 0;
    for (i = 0; i < len && num[i] != '.'; ++i) {
        val = val * 10 + (num[i] - '0');
    }
    val *= 100;
    cents = 0;
    for (++i; i < len; ++i) {
        cents = cents * 10 + (num[i] - '0');
    }
    val += cents;
    return val;
}

char *
monthname(int i)
{
    static char *name[] = {
        "jan", "feb", "mar",
        "apr", "may", "jun",
        "jul", "aug", "sep",
        "oct", "nov", "dec"
    };
    assert(i >= 0 && i <= 11);
    return name[i];
}
