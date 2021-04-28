///////////////////////////////////////////////////////////////////////////////
// AchievementShareServiceResponses.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ACHIEVEMENTSHARESERVICESRESPONSES_H
#define _ACHIEVEMENTSHARESERVICESRESPONSES_H

#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// Encapsulate responses for actions on other users
        ///
        class ORIGIN_PLUGIN_API AchievementShareServiceResponses : public OriginAuthServiceResponse
        {
        public:
            explicit AchievementShareServiceResponses(AuthNetworkRequest);

            /// \brief returns payload from the response
            QByteArray payload() const;

        protected:
            bool parseSuccessBody(QIODevice *body);
        private:
            QByteArray mPayload;
        };

    }
}

#endif
