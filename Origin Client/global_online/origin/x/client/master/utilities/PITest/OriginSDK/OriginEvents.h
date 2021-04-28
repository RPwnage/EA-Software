#ifndef __ORIGIN_EVENTS_H__
#define __ORIGIN_EVENTS_H__

#include "Origin.h"

#include "OriginTypes.h"
						  
#ifdef __cplusplus
extern "C" 
{
#endif

/// \ingroup sdk
/// \brief Registers a user callback for responding to specific event(s).
///  
/// The callback function will be called for each instance of an event of the specified types that is received.  The
/// registered callbacks are tracked on a per-event basis, not one combinations of events (the bitwise mask is
/// decomposed into individual bits and the each registered independently).
/// \param iEvents A bitwise OR of the ORIGIN_EVENT_* values for the events to be handled by this callback. (See the \ref ORIGIN_EVENT_INVITE and following defines.)
/// \param callback The callback function that will be called from \ref OriginUpdate when an instance of any of the specified events has arrived.  NULL removes the callback.
/// \param pContext The user context pointer for the callback function.  Not used except to pass back to the callback.
OriginErrorT ORIGIN_SDK_API OriginRegisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext);

#ifdef __cplusplus
}
#endif

#endif // __ORIGIN_EVENTS_H__
