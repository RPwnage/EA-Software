/////////////////////////////////////////////////////////////////////////////
// DemangleGCC.h
//
// Copyright (c) 2008, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EACallstack/internal/Config.h>

#ifndef EACALLSTACK_INTERNAL_DEMANGLEGCC_H

#if EACALLSTACK_GCC_DEMANGLE_ENABLED

namespace EA
{
	namespace Callstack
	{
		namespace Demangle
		{
			size_t UnmangleSymbolGCC(const char* pSymbol, char* buffer, size_t bufferCapacity);
		}
	}
}

#endif // EACALLSTACK_GCC_DEMANGLE_ENABLED

#endif // Header include guard
