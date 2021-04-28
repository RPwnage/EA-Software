
#include <stdio.h>

#if defined(EX_NATIVE_LIB_DLL_BUILD)
#define EX_NATIVE_LIB_API extern "C" __declspec (dllexport)
#else
#define EX_NATIVE_LIB_API 
#endif


EX_NATIVE_LIB_API int  GetInt();
EX_NATIVE_LIB_API void Hello();

EX_NATIVE_LIB_API int GetInt()
{
    return 123;
}

EX_NATIVE_LIB_API void Hello()
{
    printf("Hello there from ExNativeLibPkg.\n\n");
}


