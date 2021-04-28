/////////////////////////////////////////////////////////////////////////////
// EAMathHelpXenon.inl
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana and Alec Miller
//
// Xenon implementation of fast, specialized scalar math primitives
//
// For useful information regarding PowerPC optimizations, 
// see http://developer.apple.com/hardwaredrivers/ve/algorithms.html.
//
/////////////////////////////////////////////////////////////////////////////


#pragma warning(push, 0)
#include <math.h>
#include <ppcintrinsics.h>
//#include <vectorintrinsics.h>
#pragma warning(pop)


namespace EA
{
namespace StdC
{

	inline int32_t FloorToInt32(float32_t fValue) {
		// Note: This implementation returns +0.0 when the input is -0.0.

		const float kHighBit = 8388608.0f; // 0x1.0p+23 - 1.0*2^32, also see kFToIBiasF32 but that's 3x for negs

		float c = (float)__fsel(fValue, -kHighBit, kHighBit); // fabsf(f)
		float result = (fValue-c) + c;
		result -= (float)__fsel( fValue-result, 0.0f, 1.0f);

		return (int32_t)result;  // This cast will use the fctiwz instruction and be fairly fast.
	}

	inline uint32_t FloorToUint32(float32_t fValue) {
		// Copy-Pasted from the above, since there was an issue with using the 
		// fctiwz instruction on an unsigned value (it would fail the boundary
		// cases of uint32_t due to the differences in casting rules.

		const float kHighBit = 8388608.0f;

		float c = (float)__fsel(fValue, -kHighBit, kHighBit);
		float result = (fValue-c) + c;
		result -= (float)__fsel( fValue-result, 0.0f, 1.0f);

		return (uint32_t)result;  // This cast will use the FCTIDZ instruction instead.
	}

	inline uint32_t RoundToUint32(float32_t fValue) {
		return FloorToUint32(fValue + 0.5f);
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










