#include <stdio.h>

short   global=0;
long foo(int x, int y);
void bar(short z);

int main()
{
    long ret = foo(5000, 100);
    printf("ret=%ld\n", ret);
    return 0;
}

long foo(int x, int y)
{
    bar(x/y);
    return x*y;
}
void bar(short z)
{
    global = z;
}
