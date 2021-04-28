#include "GeolocationQueryProxy.h"

#include "services/rest/GeoCountryClient.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

QObject* GeolocationQueryProxy::startQuery(QString ipAddress)
{
    QPointer<GeolocationQueryOperationProxy> existingOperation = mOperations.value(ipAddress);

    if (existingOperation)
    {
        return existingOperation;
    }
        
    Services::GeoCountryResponse *geoResp = Services::GeoCountryClient::getGeoAndCommerceCountry(ipAddress);
    GeolocationQueryOperationProxy *newOp = new GeolocationQueryOperationProxy(this, geoResp);

    mOperations[ipAddress] = newOp;
    return newOp;
}

}
}
}
