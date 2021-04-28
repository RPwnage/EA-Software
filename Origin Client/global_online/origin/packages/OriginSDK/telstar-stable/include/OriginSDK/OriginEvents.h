#ifndef __ORIGIN_EVENTS_H__
#define __ORIGIN_EVENTS_H__

#include "Origin.h"

#include "OriginTypes.h"
						  
#ifdef __cplusplus
extern "C" 
{
#endif

/// \ingroup sdk
/// \brief Registers a user callback for responding to specific event.
/// The callback will be called for each event from origin, that matches the event Id. You can register multiple events for each event Id.  
/// \param iEvents The id of the event ORIGIN_EVENT_* values for the events to be handled by this callback. (See the \ref ORIGIN_EVENT_INVITE and following defines.)
/// The special event id ORIGIN_EVENT_FALLBACK allows for configuring a fall back callback for all events that will be called only when no other event callbacks are registered.
/// \param callback The callback function that will be called from \ref OriginUpdate when an instance of any of the specified events has arrived.  
/// \param pContext The user context pointer for the callback function.  Not used except to pass back to the callback.
/// \note OR'ing eventIds together is no longer supported.
OriginErrorT ORIGIN_SDK_API OriginRegisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext);

/// \ingroup sdk
/// \brief Unregisters a user callback for responding to specific event.
/// The callback function will be called for each instance of an event of the specified types that is received.  The
/// registered callbacks are tracked on a per-event basis, not one combinations of events (the bitwise mask is
/// decomposed into individual bits and the each registered independently).
/// \param iEvents The id of the event ORIGIN_EVENT_* values for the events to be handled by this callback. (See the \ref ORIGIN_EVENT_INVITE and following defines.)
/// The special event id ORIGIN_EVENT_FALLBACK allows for configuring a fallback for all events that will be called only when no other event callback is registered.
/// \param callback The callback function that will be called from \ref OriginUpdate when an instance of any of the specified events has arrived.  
/// \param pContext The user context pointer for the callback function.  Not used except to pass back to the callback.
OriginErrorT ORIGIN_SDK_API OriginUnregisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext);

#ifdef __cplusplus
}
#endif

#endif // __ORIGIN_EVENTS_H__
