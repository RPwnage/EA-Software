#ifndef __ORIGIN_PLATFORM_OVERLAY_SUPPORT__
#define __ORIGIN_PLATFORM_OVERLAY_SUPPORT__

#ifdef ENABLE_PLATFORM_OVERLAY_SUPPORT


#include "OriginTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// \defgroup steam Steam
	/// \brief This module contains the functions for supporting messaging from overlays other than the Origin IGO.
	/// 
	

	/// \ingroup steam
	/// \brief Tell Origin that the first party overlay is up or down.
	///
	OriginErrorT ORIGIN_SDK_API OriginOverlayStateChanged(bool isUp, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);

	/// \ingroup steam
	/// \brief Indicate whether a purchase action is accepted or rejected.
	///
	OriginErrorT ORIGIN_SDK_API OriginCompletePurchaseTransaction(uint32_t appId, uint64_t orderId, bool authorized, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);

	/// \ingroup steam
	/// \brief Indicate whether a purchase action is accepted or rejected.
	///
	OriginErrorT ORIGIN_SDK_API OriginRefreshEntitlements(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);

	/// \ingroup steam
	/// \brief Set the DLC installation state for the game.
	OriginErrorT ORIGIN_SDK_API OriginSetPlatformDLC(DLC* dlc, uint32_t count, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);

	/// \ingroup steam
    /// \brief Set the DLC installation state for the game.
    OriginErrorT ORIGIN_SDK_API OriginSetPlatformDLCSync(DLC* dlc, uint32_t count, uint32_t timeout);


	/// \ingroup steam
	/// \brief Determine the Commerce Currency for this session.
	OriginErrorT ORIGIN_SDK_API OriginDetermineCommerceCurrencySync(uint32_t timeout);

	OriginErrorT ORIGIN_SDK_API OriginSendSteamAchievementErrorTelemetry(bool validStat, bool getStat, bool setStat, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);

    OriginErrorT ORIGIN_SDK_API OriginSetSteamLocaleSync(const OriginCharT *language, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif

#endif // __ORIGIN_PLATFORM_OVERLAY_SUPPORT__