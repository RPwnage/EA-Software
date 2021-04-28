///////////////////////////////////////////////////////////////////////////////
// UpdateServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef UPDATESERVICERESPONSE_H
#define UPDATESERVICERESPONSE_H

#include "OriginServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        enum UpdateRule
        {
            UpdateRule_Optional,
            UpdateRule_Required,
            UpdateRule_Mandatory
        };

        struct AvailableUpdate
        {
            QString version;
            QString buildType;
            QString downloadURL;
            UpdateRule updateRule;
            QString eulaVersion;
            QString eulaURL;

            // Can be NULL for patches without an activation date
            QDateTime activationDate;

            bool operator==(const AvailableUpdate& au) const
            {
                return version.compare(au.version) == 0
                    && buildType.compare(au.buildType) == 0
                    && downloadURL.compare(au.downloadURL) == 0
                    && eulaVersion.compare(au.eulaVersion) == 0
                    && eulaURL.compare(au.eulaURL) == 0
                    && updateRule == au.updateRule;
            }
        };

		class ORIGIN_PLUGIN_API UpdateQueryResponse : public OriginServiceResponse
		{
		public:
			explicit UpdateQueryResponse(QNetworkReply*);

            AvailableUpdate getUpdate() const { return mUpdate; }

		protected:
			bool parseSuccessBody(QIODevice *body);

            AvailableUpdate mUpdate;

		};
    }
}

#endif