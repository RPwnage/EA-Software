#include "ExNativeLibPkg/ExNativeLibPkg.h"
#include <stdio.h>

int main()
{
    printf("Hello world from Native C++ Console Program.\n");

    // Calling Native Lib Functions
    int val = GetInt();
    printf("GetInt() returns: %d\n", val);
    Hello();

    return 0;
}
