///////////////////////////////////////////////////////////////////////////////
// ECommerceServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/publishing/ECommerceServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

ECommerceServiceClient::ECommerceServiceClient(NetworkAccessManager *nam)
    : OriginAuthServiceClient(nam) 
{
}

QUrl ECommerceServiceClient::baseUrl() const
{
    return QUrl(Origin::Services::readSetting(Origin::Services::SETTING_EcommerceURL).toString());
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
