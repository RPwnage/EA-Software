#include "GeoCountryResponse.h"
#include "services/platform/TrustedClock.h"
#include "Services/settings/SettingsManager.h"

#include "LSXWrapper.h"
#include "ecommerce2Reader.h"

namespace Origin
{
    namespace Services
    {
        GeoCountryResponse::GeoCountryResponse(QNetworkReply * reply) : 
            OriginServiceResponse(reply)
        {

        }

        bool GeoCountryResponse::parseSuccessBody(QIODevice* data)
        {
            // Prioritize on standard 'Date' header
            const QByteArray DATE_HEADER("Date");
            const QByteArray CURRENT_TIME_HEADER("X-Origin-CurrentTime");
            QDateTime currentServerTime;

            const QByteArray &dateHeader = reply()->rawHeader(DATE_HEADER);
            if (!dateHeader.isEmpty())
            {
                currentServerTime = QDateTime::fromString(dateHeader, Qt::RFC2822Date);
            }

            // Fall back to old 'X-Origin-CurrentTime' header
            if (!currentServerTime.isValid() && reply()->hasRawHeader(CURRENT_TIME_HEADER))
            {
                bool parsedOkay = false;
                qint64 xOriginCurrentTime = reply()->rawHeader(CURRENT_TIME_HEADER).toLongLong(&parsedOkay);

                if (parsedOkay)
                {
                    currentServerTime = QDateTime::fromMSecsSinceEpoch(xOriginCurrentTime * 1000).toUTC();
                }
            }

            if (currentServerTime.isValid())
            {
                // Update the trusted clock with the server time.
                Origin::Services::TrustedClock::instance()->initialize(currentServerTime);
            }

            QDomDocument doc;
            if(doc.setContent(data))
            {
                LSXWrapper resp(doc, doc.firstChildElement());
                ecommerce2::Read(&resp, mInfo);

                return true;
            }
            return false;
        }
    }
}

