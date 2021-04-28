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
    printf("Test Program 2 main()!\n");

    printf("Test dll function 'TestDllFunc(20)' returns: %d\n", TestDllFunc(20));

    return 0;
}


