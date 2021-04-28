#include "stdafx.h"
#include "OriginSDKimpl.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

OriginErrorT OriginSDK::ReadEnumerationSync(OriginHandleT hHandle, void *pBufPtr, size_t bufSize, size_t startIndex, size_t count, size_t * countRead)
{
	if(!IsHandleValid(hHandle))
		return REPORTERROR(ORIGIN_ERROR_INVALID_HANDLE);

	IEnumerator *pEnumerator = (IEnumerator *)hHandle;

	  // Wait until the data is available
	if(pEnumerator->Wait())
	{
		// Have the SDK fill in the user buffer.
		return REPORTERROR(pEnumerator->ConvertData(pBufPtr, bufSize, startIndex, count, countRead));
	}
	else
	{
		return REPORTERROR(pEnumerator->GetErrorCode());
	}
}

OriginErrorT OriginSDK::ReadEnumeration(OriginHandleT hHandle, void *pBufPtr, size_t bufSize, size_t startIndex, size_t count, size_t *countRead)
{
	if(!IsHandleValid(hHandle))
		return REPORTERROR(ORIGIN_ERROR_INVALID_HANDLE);

	IEnumerator *pEnumerator = (IEnumerator *)hHandle;

	if(pEnumerator->IsReady())
	{
		return ReadEnumerationSync(hHandle, pBufPtr, bufSize, startIndex, count, countRead);
	}
	else
	{
		return REPORTERROR(ORIGIN_PENDING);
	}
}

OriginErrorT OriginSDK::GetEnumerateStatus(OriginHandleT hHandle, size_t *total, size_t *available)
{
	if(!IsHandleValid(hHandle))
		return REPORTERROR(ORIGIN_ERROR_INVALID_HANDLE);

	IEnumerator *pEnumerator = (IEnumerator *)hHandle;

	OriginErrorT err = pEnumerator->GetErrorCode();

	if(err == ORIGIN_SUCCESS)
	{
		if(total != NULL) *total = pEnumerator->GetTotalCount();
		if(available != NULL) *available = pEnumerator->GetAvailableCount();
	}
	else
	{
		if(total != NULL) *total = 0;
		if(available != NULL) *available = 0;
	}
	return REPORTERROR(err);
}



};