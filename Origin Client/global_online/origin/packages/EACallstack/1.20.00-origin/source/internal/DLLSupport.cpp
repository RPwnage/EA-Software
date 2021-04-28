///////////////////////////////////////////////////////////////////////////////
// DLLSupport.cpp
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

#include <EACallstack/internal/Config.h>


#if EACALLSTACK_DLL

#include <EAStdC/EASprintf.h>


// We provide implementations of some required functions in a DLL build.
// The functions below are present in the UTFFoundation package, but we 
// don't want to be dependent on that package.

int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

#if (EASTDC_VERSION_N >= 10600)
    int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }
#endif


#endif



