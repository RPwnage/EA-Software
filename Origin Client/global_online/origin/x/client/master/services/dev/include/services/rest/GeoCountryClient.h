#ifndef __GEOCOUNTRYCLIENT_H__
#define __GEOCOUNTRYCLIENT_H__

#include "services/rest/OriginServiceClient.h"
#include "services/plugin/PluginAPI.h"
#include "GeoCountryResponse.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API GeoCountryClient : public OriginServiceClient
        {
        public:
            friend class OriginClientFactory<GeoCountryClient>;

            static GeoCountryResponse * getGeoAndCommerceCountry(const QString& ipAddress = QString())
            {
                return OriginClientFactory<GeoCountryClient>::instance()->getGeoAndCommerceCountryPriv(ipAddress);
            }

        private:

            GeoCountryResponse * getGeoAndCommerceCountryPriv(const QString& ipAddress = QString());


            /// 
            /// \brief Creates a new config geo country service client
            ///
            /// \param baseUrl       Base URL for the configuration service API.
            /// \param nam           QNetworkAccessManager instance to send requests through.
            ///
            explicit GeoCountryClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
        };
    }
}

#endif //__GEOCOUNTRYCLIENT_H__