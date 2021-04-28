/////////////////////////////////////////////////////////////////////////////
// EAMathHelpPS3.inl
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana and Karl Hillesland
//
// PS3 implementation of fast, specialized scalar math primitives.
// FloorToInt32 is based on the Xenon implementation. 
//
// For useful information regarding PowerPC optimizations, 
// see http://developer.apple.com/hardwaredrivers/ve/algorithms.html.
//
/////////////////////////////////////////////////////////////////////////////


#include <math.h>

namespace EA
{
namespace StdC
{
	inline float fsels(double test, double a, double b)
	{
		#if defined(__SNC__)
			return (float)__builtin_fsel(test, a, b);
		#else
			float result;
			__asm__ ("fsel %0,%1,%2,%3" : "=f" (result) : "f" (test), "f" (a), "f" (b));
			return result;
		#endif
	}

	inline int32_t FloorToInt32(float32_t fValue) {
		// Note: This implementation returns +0.0 when the input is -0.0.

		const float kHighBit = 8388608.0f; // 0x1.0p+23 - 1.0*2^32, also see kFToIBiasF32 but that's 3x for negs

		float c = fsels(fValue, -kHighBit, kHighBit); // fabsf(f)
		float result = (fValue-c) + c;
		result -= fsels( fValue-result, 0.0f, 1.0f);

		return (int32_t)result;  // This cast will use the fctiwz instruction and be fairly fast.
	}

	inline uint32_t RoundToUint32(float32_t fValue) {
		return (uint32_t)FloorToInt32(fValue + 0.5f);
	}

	inline int32_t RoundToInt32(float32_t fValue) {
		return FloorToInt32(fValue + 0.5f);
	}

	inline int32_t CeilToInt32(float32_t fValue) {
		return (int32_t)ceilf(fValue);
	}

	inline int32_t TruncateToInt32(float32_t fValue) {
		return (int32_t)fValue; // TODO: optimize
	}

	inline uint8_t UnitFloatToUint8(float fValue) {
		// To do: Verify that the compiler uses fmadd to do the multiply-add below.
		return (uint8_t)FloorToInt32((fValue * 255.0f) + 0.5f);
	}

	inline uint8_t ClampUnitFloatToUint8(float fValue) {
		// To do: Optimize this with fsel instruction usage.
		if (fValue < 0.0f)
			fValue = 0.0f;
		else if (fValue > 1.0f)
			fValue = 1.0f;

		// To do: Verify that the compiler uses fmadd to do the multiply-add below.
		return (uint8_t)FloorToInt32((fValue * 255.0f) + 0.5f);
	}

	// This function is deprecated, as it's not very useful any more.
	inline int32_t FastRoundToInt23(float32_t fValue) {
		return FloorToInt32(fValue + 0.5f);
	}

} // namespace StdC
} // namespace EA

