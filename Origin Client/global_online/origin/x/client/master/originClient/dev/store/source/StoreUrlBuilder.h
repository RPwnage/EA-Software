///////////////////////////////////////////////////////////////////////////////
// StoreUrlBuilder.h
//
// Copyright (c) 2012-2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef STOREURLBUILDER_H
#define STOREURLBUILDER_H

#include <QString>
#include <QUrl>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API StoreUrlBuilder
{
public:
	/// \brief Gets our path to product pages
	QUrl productUrl(QString productId, QString masterTitleId="") const;
    QUrl masterTitleUrl(QString masterTitleId) const;
	QUrl homeUrl() const;
    QUrl freeGamesUrl() const;
    QUrl onTheHouseStoreUrl(const QString& trackingParam) const;
    QUrl emptyShelfUrl() const;
    QUrl subscriptionUrl() const;
    QUrl subscriptionVaultUrl() const;
    QUrl entitleFreeGamesUrl(QString productId) const;
    QString storeEnvironment() const;
};

}
}

#endif
