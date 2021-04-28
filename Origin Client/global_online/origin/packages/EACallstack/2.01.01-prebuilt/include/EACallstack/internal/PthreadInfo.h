///////////////////////////////////////////////////////////////////////////////
// PthreadInfo.h
//
// Copyright (c) 2012 Electronic Arts Inc.
// Developed and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_INTERNAL_PTHREADINFO_H
#define EACALLSTACK_INTERNAL_PTHREADINFO_H

#include <eathread/internal/config.h>

#if EATHREAD_VERSION_N <= 11702

	#include <EACallstack/internal/Config.h>


	namespace EA
	{
		namespace Callstack
		{

		#if defined(EA_PLATFORM_UNIX)

			// GetPthreadStackInfo
			//
			// With some implementations of pthread, the stack base is returned by pthread as NULL if it's the main thread,
			// or possibly if it's a thread you created but didn't call pthread_attr_setstack manually to provide your
			// own stack. It's impossible for us to tell here whether will be such a NULL return value, so we just do what
			// we can and the user nees to beware that a NULL return value means that the system doesn't provide the
			// given information for the current thread. This function returns false and sets pBase and pLimit to NULL in
			// the case that the thread base and limit weren't returned by the system or were returned as NULL.

			bool GetPthreadStackInfo(void** pBase, void** pLimit);

		#endif


		} // namespace Callstack
	} // namespace EA

#endif

#endif // Header include guard.






