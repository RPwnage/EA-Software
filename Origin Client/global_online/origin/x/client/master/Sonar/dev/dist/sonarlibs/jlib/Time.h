#pragma once

#include <jlib/types.h>

namespace jlib
{

	class Time
	{
	public:

		static void sleep(int msec);
		static INT64 getTimeAsMSEC(void);
	};

}