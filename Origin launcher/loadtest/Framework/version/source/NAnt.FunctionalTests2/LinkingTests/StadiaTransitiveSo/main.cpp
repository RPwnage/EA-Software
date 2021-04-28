#include <stdio.h>
#include <unistd.h>

#include "midlib.h"
#include "midso.h"

int main()
{
    fputs("Start\n", stdout);
    fflush(stdout);
    int ret = midso() + midlib();
    fputs("Done\n", stdout);
    fflush(stdout);
    sleep(5);
    return ret;
}