#include "GeolocationQueryOperationProxy.h"

#include "services/rest/GeoCountryResponse.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace
{
    // Convert GeoCountryResponse to the QVariant form that JS expects
    QVariantMap productDataToQVariant(const Origin::Services::GeoCountryResponse *resp)
    {
        QVariantMap ret;

        ret["geoCountry"] = resp->geoCountry();
        ret["commerceCountry"] = resp->commerceCountry();
        ret["commerceCurrency"] = resp->commerceCurrency();

        return ret;
    }
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    GeolocationQueryOperationProxy::GeolocationQueryOperationProxy(QObject *parent, Services::GeoCountryResponse *resp) :
        QObject(parent),
        mGeoCountryResponse(resp)
    {
        ORIGIN_VERIFY_CONNECT(mGeoCountryResponse, SIGNAL(error(Origin::Services::restError)), this, SLOT(queryFailed(Origin::Services::restError)));
        ORIGIN_VERIFY_CONNECT(mGeoCountryResponse, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(queryFailed(Origin::Services::HttpStatusCode)));
        ORIGIN_VERIFY_CONNECT(mGeoCountryResponse, SIGNAL(success()), this, SLOT(querySucceeded()));

        ORIGIN_VERIFY_CONNECT(mGeoCountryResponse, SIGNAL(finished()), mGeoCountryResponse, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(succeeded(QVariant)), this, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(failed()), this, SLOT(deleteLater()));
    }

    void GeolocationQueryOperationProxy::querySucceeded()
    {
        emit succeeded(productDataToQVariant(mGeoCountryResponse));
    }

    void GeolocationQueryOperationProxy::queryFailed(Origin::Services::HttpStatusCode httpCode)
    {
        ORIGIN_LOG_ERROR << "Geolocation query failed.  Http code: " << httpCode;
        emit failed();
    }
    
    void GeolocationQueryOperationProxy::queryFailed(Origin::Services::restError restError)
    {
        ORIGIN_LOG_ERROR << "Geolocation query failed.  Rest error code: " << restError;
        emit failed();
    }
}
}
}
