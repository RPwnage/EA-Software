///////////////////////////////////////////////////////////////////////////////
// ECommerceServiceClient.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ECOMMERCESERVICECLIENT_H_INCLUDED_
#define _ECOMMERCESERVICECLIENT_H_INCLUDED_

#include "services/rest/OriginAuthServiceClient.h"
#include "services/rest/OriginServiceResponse.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

class ECommerceServiceClient : public OriginAuthServiceClient
{
protected:
    /// \brief The ECommerceServiceClient constructor.
    /// \param nam QNetworkAccessManager instance to send requests through
    explicit ECommerceServiceClient(NetworkAccessManager *nam = NULL);

    ///
    /// \brief Returns the base URL for this service
    ///
    virtual QUrl baseUrl() const;
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif
