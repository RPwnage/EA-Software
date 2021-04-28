/*! ************************************************************************************************/
/*!
    \file externalaccesstokenutilps5.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_EXTERNAL_ACCESSTOKEN_UTIL_PS5_H
#define COMPONENT_EXTERNAL_ACCESSTOKEN_UTIL_PS5_H

#include "framework/blaze.h"

namespace Blaze
{ 
    namespace PSNServices { class PsnErrorResponse; }
    namespace Arson { class ArsonSlaveImpl; }

    namespace GameManager
    {
        class ExternalAccessTokenUtilPs5
        {
        public:
            ExternalAccessTokenUtilPs5(bool isMockEnabled);
            ~ExternalAccessTokenUtilPs5();

            BlazeRpcError getUserOnlineAuthToken(eastl::string& buf, const UserIdentification& user, const char8_t* serviceName, bool forceRefresh, bool crossGen) const;
        private:
            bool isMockPlatformEnabled() const;
            const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalAccessTokenUtilPs5]").c_str() : mLogPrefix.c_str()); }

        private:
            bool mIsMockEnabled;
            mutable eastl::string mLogPrefix;
#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            friend class Blaze::Arson::ArsonSlaveImpl;
#endif
        };

    }
}

#endif
