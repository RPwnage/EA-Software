#ifndef __ORIGIN_PROFILE_H__
#define __ORIGIN_PROFILE_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup profile Profile
/// \brief This module contains the functions for getting the profile of the default user.
/// 
/// There are sychronous and asynchronous versions of the \ref OriginGetProfile function. For an explanation of
/// why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref ga-intguide-profile in the <em>Integrator's Guide</em>.

/// \ingroup profile
/// \brief Gets the profile of the default player.
///
/// Gets the profile of the user specified by userIndex.
/// \param[in] userIndex The index of the user on the local PC. As only one user is currently supported, this value should be '0'.
/// \param[in] timeout The amount of time to wait for an answer.
/// \param[out] pProfile The profile of the user.
/// \param[out] pHandle A pointer to a resource handle that stores the strings.
/// \return \ref ORIGIN_SUCCESS if the command succeeds;<br>ORIGIN_XXX whenever an error occurs (see \ref error).
OriginErrorT ORIGIN_SDK_API OriginGetProfileSync(uint32_t userIndex, uint32_t timeout, OriginProfileT *pProfile, OriginHandleT *pHandle);

/// \ingroup profile
/// \brief Gets the profile of the default player.
///
/// Gets the profile of the user specified by userIndex.
/// \param[in] userIndex The index of the user on the local PC. As only one user is currently supported, this value should be '0'.
/// \param[in] timeout The amount of time to wait for an answer.
/// \param[in] callback A game-defined callback function.
/// \param[in] pContext A game.
/// \return \ref ORIGIN_SUCCESS if the command succeeds;<br>ORIGIN_XXX whenever an error occurs (see \ref error).
OriginErrorT ORIGIN_SDK_API OriginGetProfile(uint32_t userIndex, uint32_t timeout, OriginResourceCallback callback, void *pContext);


#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_PROFILE_H__