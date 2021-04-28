#ifndef EX_NATIVE_LIB_PKG_H
#define EX_NATIVE_LIB_PKG_H

#if defined(EX_NATIVE_LIB_DLL_BUILD)
#define EX_NATIVE_LIB_API_H extern "C" __declspec (dllimport)
#else
#define EX_NATIVE_LIB_API_H
#endif

EX_NATIVE_LIB_API_H int  GetInt();
EX_NATIVE_LIB_API_H void Hello();


#endif

