#include <stdio.h>

#if (defined(__APPLE__) && defined(__MACH__))

int TestDllFunc(int inval);

#elif defined(__linux__)

int TestDllFunc(int inval);

#elif defined(_WIN32)

extern "C" __declspec (dllimport) int TestDllFunc(int inval);

#else

#error This platform is not suppoted yet!

#endif


int main()
{
    printf("hello world!\n");

    printf("Test dll function 'TestDllFunc(10)' returns: %d\n", TestDllFunc(10));

    return 0;
}


