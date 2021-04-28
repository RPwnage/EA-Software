#ifndef __ORIGIN_ENUMERATION_H__
#define __ORIGIN_ENUMERATION_H__

#include "OriginTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup enumerate Enumeration
/// \brief This module contains a typedefs and functions that provide Origin's enumeration capabilities. 
/// 
/// There are sychronous and asynchronous versions of the \ref OriginReadEnumeration function. For an explanation of
/// why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref enumeratorpattern in the <em>Integrator's Guide</em>. 


/// \ingroup enumerate
/// \brief Does the enumerations synchronously.
/// 
/// Perform synchronous enumeration of the data on the passed in handle. This will block execution of the current thread
/// until either the enumeration completes (either successfully or with an error), or the timeout expires. It is still
/// necessary to call \ref OriginDestroyHandle after calling this function to release the handle.
/// \param[in] hHandle The handle from the enumerate function.
/// \param[out] pBufPtr An optional pre-allocated return buffer for enumerated data objects.
/// \param[in] bufSize The length of the return buffer (it should match the size provided by the enumerator function).
/// \param[in] startIndex The element in the enumeration list from which you want to start reading.
/// \param[in] count The number of items you want to read.
/// \param[out] countRead The number of items enumerated.
/// \sa XEnumerate(HANDLE hEnum, PVOID pvBuffer, DWORD cbBuffer, PDWORD pcItemsReturned, PXOVERLAPPED pOverlapped)
OriginErrorT ORIGIN_SDK_API OriginReadEnumerationSync(OriginHandleT hHandle, void *pBufPtr, OriginSizeT bufSize, OriginIndexT startIndex, OriginSizeT count, OriginSizeT *countRead);

/// \ingroup enumerate
/// \brief Does the enumerations asynchronously.
/// 
/// Perform asynchronous enumeration of the data on the passed in handle. This will not block the current thread until
/// the enumeration completes.
/// \param[in] hHandle The handle from the enumerate function.
/// \param[out] pBufPtr An optional pre-allocated return buffer for enumerated data objects.
/// \param[in] bufSize The length of the return buffer in bytes (it should match the size provided by the enumerator function).
/// \param[in] startIndex The element in the enumeration list from which you want to start reading.
/// \param[in] count The number of items you want to read.
/// \param[out] countRead The number of items enumerated.
/// \return \ref ORIGIN_SUCCESS If all data could be read; \ref ORIGIN_PENDING if there is still more data to come.
/// \sa XEnumerate(HANDLE hEnum, PVOID pvBuffer, DWORD cbBuffer, PDWORD pcItemsReturned, PXOVERLAPPED pOverlapped)
OriginErrorT ORIGIN_SDK_API OriginReadEnumeration(OriginHandleT hHandle, void *pBufPtr, OriginSizeT bufSize, OriginIndexT startIndex, OriginSizeT count, OriginSizeT *countRead);

/// \ingroup enumerate
/// \brief The callback signature for the Enumeration Callback functions.
/// 
/// The game defines a callback that matches this signature for receiving asynchronous callbacks from the
/// \ref OriginReadEnumeration. This callback will be called in the context of the thread that is calling the \ref OriginUpdate
/// function.
/// \param *pContext A pointer to a game provided context.
/// \param hHandle The handle associated with this enumeration.
/// \param total The total number of items in this enumeration.
/// \param available The available number of items that can be obtained without waiting.
/// \param err Signals whether an error occurred during the transaction.
typedef OriginErrorT (*OriginEnumerationCallbackFunc)(void *pContext, OriginHandleT hHandle, OriginSizeT total, OriginSizeT available, OriginErrorT err);

/// \ingroup enumerate
/// \brief Polls the status of the enumeration.
///
/// This function polls the status of the enumerator identified by hHandle. If results are available,
/// OriginGetEnumerateStatus will return \ref ORIGIN_SUCCESS, if no data is yet available the OriginGetEnumerateStatus will
/// return \ref ORIGIN_PENDING.
/// \param[in] hHandle The handle of the enumerator.
/// \param[out] total The total number of records in the enumeration.
/// \param[out] available The available records in the enumeration. 
/// \return \ref ORIGIN_SUCCESS When the total number of records is known.<br>\ref ORIGIN_PENDING when the request is still in
/// progress.<br>ORIGIN_XXX for any other error condition (see \ref error).
/// \note The transfer is done when total == available.
OriginErrorT ORIGIN_SDK_API OriginGetEnumerateStatus(OriginHandleT hHandle, OriginSizeT *total, OriginSizeT *available);


#ifdef __cplusplus
}
#endif

#endif
