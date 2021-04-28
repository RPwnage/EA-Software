///////////////////////////////////////////////////////////////////////////////
/// \file       OOALicenseManager.h
///
/// \author     James Fairweather
/// \date       2015
/// \copyright  Copyright (c) 2015 Electronic Arts. All rights reserved.

#ifndef __OOALICENSE_MANAGER_H__
#define __OOALICENSE_MANAGER_H__

namespace Origin
{
    namespace Engine
    {
        namespace Subscription
        {
            void RefreshOOALicenses(bool bIsUserOnline, bool hasOfflineTimeRemaining);
        }
    }
}

#endif //__OOALICENSE_MANAGER_H__
