#include <stdlib.h>
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
    struct tm *t;
    int sy, sm;
    int ey, em;
    
    t = localtime(&start);
    sy = t->tm_year + 1900;
    sm = t->tm_mon + 1;
    
    t = localtime(&end);
    ey = t->tm_year + 1900;
    em = t->tm_mon + 1;

    return (ey - sy) * 12 - sm + em;
}
