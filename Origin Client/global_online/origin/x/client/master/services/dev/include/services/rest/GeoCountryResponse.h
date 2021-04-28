#ifndef __GEOCOUNTRYRESPONSE_H__
#define __GEOCOUNTRYRESPONSE_H__

#include "services/rest/OriginServiceResponse.h"
#include "services/plugin/PluginAPI.h"
#include "ecommerce2.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API GeoCountryResponse : public OriginServiceResponse
        {
        public:
            explicit GeoCountryResponse(QNetworkReply * reply);


            const QString &geoCountry() const { return mInfo.country; }
            const QString &commerceCountry() const { return mInfo.currencyCountry; }
            const QString &commerceCurrency() const { return mInfo.currency; }

        protected:
            bool parseSuccessBody(QIODevice*);

            ecommerce2::countryCurrencyByIPT mInfo;
        };
    }
}

#endif //__GEOCOUNTRYRESPONSE_H__