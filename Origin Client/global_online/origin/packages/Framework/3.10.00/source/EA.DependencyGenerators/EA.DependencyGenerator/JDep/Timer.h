/*

	Timer.h

*/

#ifndef TIMER_H_
#define TIMER_H_

inline void TimerStart(__int64 &TimerHandle)
{
	__asm {
		mov	ebx, dword ptr [TimerHandle]
		rdtsc

		mov [ebx], eax
		mov [ebx+4], edx
	}
}

inline void TimerStop(__int64 &TimerHandle)
{
__int64 End;

	__asm {
		rdtsc
		lea ebx, dword ptr End

		mov [ebx], eax
		mov [ebx+4], edx
	}
	TimerHandle = End - TimerHandle;
}

#endif
