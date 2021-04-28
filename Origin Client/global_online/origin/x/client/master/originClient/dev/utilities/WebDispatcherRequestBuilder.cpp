#include "WebDispatcherRequestBuilder.h"
#include "services/settings/SettingsManager.h"

namespace 
{
    QByteArray encodedNameValuePair(const QString &name, const QString &value)
    {
        return QUrl::toPercentEncoding(name) + "=" + QUrl::toPercentEncoding(value);
    }
}

namespace Origin
{
	namespace Client
	{
		namespace WebDispatcherRequestBuilder
		{

			WebDispatcherRequest buildRequest(const QString &exchangeTicket, const QUrl redirectUrl)
			{
				// Get our root dispatcher URL
				QUrl dispatcherUrl = QUrl(Origin::Services::readSetting(Origin::Services::SETTING_WebDispatcherURL));

				// Build our body
				QByteArray requestBody;

				requestBody  = encodedNameValuePair("redirect", "true");
				requestBody += "&";
				requestBody += encodedNameValuePair("exchangeTicket", exchangeTicket);
				requestBody += "&";
				requestBody += encodedNameValuePair("returnUrl", redirectUrl.toEncoded()); 

				// Build our request
				QNetworkRequest req(dispatcherUrl);
				req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

				return WebDispatcherRequest(req, QNetworkAccessManager::PostOperation, requestBody);
			}

			QUrl buildGETUrl(const QString &exchangeTicket, const QUrl& redirectUrl)
			{
				// Get our root dispatcher URL
				QUrl dispatcherUrl(Origin::Services::readSetting(Origin::Services::SETTING_WebDispatcherURL));
				QUrlQuery dispatcherUrlQuery(dispatcherUrl);
                
				dispatcherUrlQuery.addQueryItem("exchangeTicket", exchangeTicket);
				dispatcherUrlQuery.addQueryItem("returnUrl", redirectUrl.toString());
				dispatcherUrlQuery.addQueryItem("SessionOnly", "true");
                dispatcherUrl.setQuery(dispatcherUrlQuery);
                
                return dispatcherUrl;
			}


		}
	}
}
