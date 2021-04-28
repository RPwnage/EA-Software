///////////////////////////////////////////////////////////////////////////////
// UpdateServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/log/LogService.h"
#include "services/rest/UpdateServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "version/version.h"

namespace Origin
{
    namespace Services
    {
        UpdateServiceClient::UpdateServiceClient(NetworkAccessManager *nam)
            : OriginServiceClient(nam)
        {

        }

        UpdateQueryResponse* UpdateServiceClient::isUpdateAvailablePriv(QString locale, QString type, QString platform)
        {
            VersionInfo currentOS = PlatformService::currentOSVersion();
            QString location = Origin::Services::readSetting(Origin::Services::SETTING_UpdateURL);
            location = location.arg(EALS_VERSION_P_DELIMITED).arg(locale).arg(type).arg(platform).arg(currentOS.ToStr(Origin::VersionInfo::ABBREVIATED_VERSION));
            ORIGIN_LOG_DEBUG << "Querying update from " << location;
            QNetworkRequest request(location);

            return new UpdateQueryResponse(getNonAuth(request));
        }
    }
}
