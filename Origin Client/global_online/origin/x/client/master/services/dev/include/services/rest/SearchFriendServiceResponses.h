///////////////////////////////////////////////////////////////////////////////
// SearchFriendServiceResponses.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SEARCHFRIENDSERVICERESPONSES_H
#define _SEARCHFRIENDSERVICERESPONSES_H

#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {

        struct SearchInfo
        {
            QString mAvatarLink;
            QString mEaid;
            QString mFriendUserId;
            QString mFullName;
        };

        ///
        /// Encapsulate responses for actions on other users
        ///
        class ORIGIN_PLUGIN_API SearchFriendServiceResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
            
        public:
            explicit SearchFriendServiceResponse(AuthNetworkRequest reply);

            const QList<SearchInfo> list() const;

            int totalCount() const;

        protected:
            bool parseSuccessBody(QIODevice *body);

            QList<SearchInfo> mList;

            int mTotalCount;

        };

    }
}


#endif
