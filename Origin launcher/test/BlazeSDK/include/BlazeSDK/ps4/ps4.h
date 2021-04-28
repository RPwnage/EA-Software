#ifndef ESSENTIALS_PS4_H
#define ESSENTIALS_PS4_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#if defined(EA_PLATFORM_PS4)

#include <np.h>
#include <np/np_npid.h>

typedef SceNpOnlineId PrimarySonyId;

#define PRIMARY_SONY_ACCOUNT_ID_STR "SceNpAccountId"

#endif // EA_PLATFORM_PS4

#endif // ESSENTIALS_PS4_H