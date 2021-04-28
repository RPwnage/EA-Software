/////////////////////////////////////////////////////////////////////////////
// timings.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
//
// Created by Rob Parolin
/////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_INTERNAL_TIMINGS_H
#define EATHREAD_INTERNAL_TIMINGS_H

namespace EA
{
	namespace Thread
	{
		
#ifdef EA_PLATFORM_KETTLE 
		// RelativeTimeoutFromAbsoluteTimeout returns a relative timeout in microseconds.
		inline uint32_t RelativeTimeoutFromAbsoluteTimeout(EA::Thread::ThreadTime timeoutAbsolute)
		{
			using namespace EA::Thread;

			EAT_ASSERT((timeoutAbsolute == kTimeoutImmediate) || (timeoutAbsolute > EATHREAD_MIN_ABSOLUTE_TIME)); // Assert that the user didn't make the mistake of treating time as relative instead of absolute.

			uint32_t timeoutRelative = 0;

			if (timeoutAbsolute == kTimeoutNone)
			{
				timeoutRelative = 0xffffffff;
			}
			else if (timeoutAbsolute == kTimeoutImmediate)
			{
				timeoutRelative = 0;
			}
			else
			{
				ThreadTime timeCurrent(GetThreadTime());
				timeoutRelative = (timeoutAbsolute > timeCurrent) ?  EA_THREADTIME_AS_UINT_MICROSECONDS(timeoutAbsolute - timeCurrent) : 0;
			}

			EAT_ASSERT((timeoutRelative == 0xffffffff) || (timeoutRelative < 100000000)); // Assert that the timeout is a sane value and didn't wrap around.
	
			return timeoutRelative;
		}
#endif

	}
}

#endif
