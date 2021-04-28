#include "GeoCountryClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        GeoCountryResponse * GeoCountryClient::getGeoAndCommerceCountryPriv(const QString& ipAddress)
        {
            QUrl serviceUrl(baseUrl().toString() + "/ipToCountryCurrency");
            QNetworkRequest request(serviceUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, contentTypeHeader);

            if (ipAddress.length() > 0)
            {
                request.setRawHeader("Origin-ClientIp", ipAddress.toLatin1());
            }

            return new GeoCountryResponse(getNonAuth(request));
        }

        GeoCountryClient::GeoCountryClient(const QUrl &baseUrl, NetworkAccessManager *nam) : 
            OriginServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                setBaseUrl(Origin::Services::readSetting(Origin::Services::SETTING_EcommerceURL).toString());
            }
        }
    }
}