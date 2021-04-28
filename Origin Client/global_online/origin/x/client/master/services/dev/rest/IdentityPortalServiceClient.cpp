///////////////////////////////////////////////////////////////////////////////
// IdentityPortalServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/IdentityPortalServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace
{
	const static QString contentTypeHeader("application/x-www-form-urlencoded");
}

namespace Origin
{
    namespace Services
    {

        IdentityPortalUserResponse* IdentityPortalServiceClient::retrieveUserInfoByToken(const QString& accessToken)
        {
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->retrieveUserInfoByTokenPriv("/identity/pids/me", accessToken);
        }

        IdentityPortalUserResponse* IdentityPortalServiceClient::retrieveUserInfoByPid(const QString& accessToken, qint64 userPid)
        {
            QString servicePath = QString("/identity/pids/%1").arg(userPid);
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->retrieveUserInfoByTokenPriv(servicePath, accessToken);
        }

        IdentityPortalUserProfileUrlsResponse* IdentityPortalServiceClient::retrieveUserProfileUrls(const QString& accessToken, qint64 userPid,
            IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category /*= IdentityPortalUserProfileUrlsResponse::All*/)
        {
            QString servicePath = QString("/identity/pids/%1/profileinfo").arg(userPid);

            return OriginClientFactory<IdentityPortalServiceClient>::instance()->retrieveUserProfileUrlsPriv(servicePath, accessToken, category);
        }

        IdentityPortalExpandedNameProfilesResponse* IdentityPortalServiceClient::retrieveExpandedNameProfiles(const QString& accessToken, qint64 userPid)
        {
            QString servicePath = QString("/identity/pids/%1/profileinfo").arg(userPid);

            return OriginClientFactory<IdentityPortalServiceClient>::instance()->retrieveExpandedNameProfilesPriv(servicePath, accessToken);
        }

        IdentityPortalPersonasResponse* IdentityPortalServiceClient::personas(const QString& accessToken, qint64 userPid)
        {
            QString servicePath = "/identity/pids/%1/personas";
            servicePath = servicePath.arg(userPid);
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->personasPriv(servicePath, accessToken);
        }

        IdentityPortalExpandedPersonasResponse* IdentityPortalServiceClient::expandedPersonas(const QString& accessToken, qint64 userPid)
        {
            QString servicePath = "/identity/pids/%1/personas";
            servicePath = servicePath.arg(userPid);
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->expandedPersonasPriv(servicePath, accessToken);
        }

        IdentityPortalPersonasResponse2* IdentityPortalServiceClient::personas2(const QString& accessToken, const QString& servicePath)
        {
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->personasPriv2(servicePath, accessToken);
        }

        IdentityPortalPersonasResponse* IdentityPortalServiceClient::personasByEAID(const QString& accessToken, const QString& eaId)
        {
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->personasByEAIDPriv("/identity/personas", accessToken, eaId);
        }


        IdentityPortalUserNameResponse* IdentityPortalServiceClient::retrieveUserName(const QString& servicePath, const QString& accessToken)
        {
            QString fullServicePath = QString("/identity%1").arg(servicePath);
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->retrieveUserNamePriv(fullServicePath, accessToken);
        }

        IdentityPortalUserProfileUrlsResponse* IdentityPortalServiceClient::updateUserName(const QString& servicePath, const QString& accessToken,
                const QString& firstName, const QString& lastName)
        {
            QString fullServicePath = QString("/identity%1").arg(servicePath);
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->updateUserNamePriv(fullServicePath, accessToken, firstName, lastName);

        }

        IdentityPortalUserIdResponse* IdentityPortalServiceClient::pidByEmail(const QString& accessToken, const QString& email)
        {
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->pidByEmailPriv("/identity/pids", accessToken, email);
        }

        IdentityPortalUserIdResponse* IdentityPortalServiceClient::pidByEAID(const QString&  accessToken, const QString& eaid)
        {
            return OriginClientFactory<IdentityPortalServiceClient>::instance()->pidByEAIDPriv("/identity/personas", accessToken, eaid);
        }

        IdentityPortalServiceClient::IdentityPortalServiceClient(const QUrl& baseUrl, NetworkAccessManager* nam)
            : OriginServiceClient(nam) 
        {
            mClientId = Origin::Services::readSetting(Origin::Services::SETTING_ClientId).toString();
            mClientSecret = Origin::Services::readSetting(Origin::Services::SETTING_ClientSecret).toString();

            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString identityPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_IdentityPortalBaseUrl).toString();
                setBaseUrl(QUrl(identityPortalBaseUrl));
            }
        }

        IdentityPortalPersonasResponse* IdentityPortalServiceClient::personasPriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("namespaceName", "cem_ea_id");
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalPersonasResponse(getNonAuth(request));    

        }

        IdentityPortalExpandedPersonasResponse* IdentityPortalServiceClient::expandedPersonasPriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("namespaceName", "cem_ea_id");
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("X-Expand-Results", "true");
            setHeader(accessToken, request);

            return new IdentityPortalExpandedPersonasResponse(getNonAuth(request));  
        }

        IdentityPortalPersonasResponse* IdentityPortalServiceClient::personasByEAIDPriv(const QString& servicePath, const QString& accessToken, const QString& eaId)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("namespaceName", "cem_ea_id");
            serviceUrlQuery.addQueryItem("displayName", eaId.toUtf8());
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalPersonasResponse(getNonAuth(request)); 
        }

        IdentityPortalPersonasResponse2* IdentityPortalServiceClient::personasPriv2(const QString& servicePath, const QString& accessToken)
        {
            QString burl = baseUrl().toString() + QString("/identity") + servicePath;
            QUrl serviceUrl(burl);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("namespaceName", "cem_ea_id");
//             serviceUrlQuery.addQueryItem("client_id", "client1);
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalPersonasResponse2(getNonAuth(request));

        }

        IdentityPortalUserResponse* IdentityPortalServiceClient::retrieveUserInfoByTokenPriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);
            request.setRawHeader("X-Include-Underage", "true");

            return new IdentityPortalUserResponse(getNonAuth(request));    
        }

        IdentityPortalUserProfileUrlsResponse* IdentityPortalServiceClient::retrieveUserProfileUrlsPriv(const QString& servicePath, const QString& accessToken,
            IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category)
        {
            QString profileInfoCategory = profileInfoCategoryToString(category);

            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);

            if (!profileInfoCategory.isEmpty())
            {
                serviceUrlQuery.addQueryItem("profileInfoCategory", profileInfoCategory);
            }
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalUserProfileUrlsResponse(getNonAuth(request));
        }

        IdentityPortalExpandedNameProfilesResponse* IdentityPortalServiceClient::retrieveExpandedNameProfilesPriv(const QString& servicePath, const QString& accessToken)
        {
            QString profileInfoCategory = profileInfoCategoryToString(IdentityPortalUserProfileUrlsResponse::Name);

            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            if (!profileInfoCategory.isEmpty())
            {
                serviceUrlQuery.addQueryItem("profileInfoCategory", profileInfoCategory);
            }
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("X-Expand-Results", "true");
            setHeader(accessToken, request);

            return new IdentityPortalExpandedNameProfilesResponse(getNonAuth(request));
        }

        QString IdentityPortalServiceClient::profileInfoCategoryToString(IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category) const
        {
            QString profileInfoCategory;
            switch (category)
            {
            case IdentityPortalUserProfileUrlsResponse::MobileInfo:
                profileInfoCategory = "MOBILE_INFO";
                break;
            case IdentityPortalUserProfileUrlsResponse::Name:
                profileInfoCategory = "NAME";
                break;
            case IdentityPortalUserProfileUrlsResponse::Address:
                profileInfoCategory = "ADDRESS";
                break;
            case IdentityPortalUserProfileUrlsResponse::SecurityQA:
                profileInfoCategory = "SECURITY_QA";
                break;
            case IdentityPortalUserProfileUrlsResponse::Gender:
                profileInfoCategory = "GENDER";
                break;
            case IdentityPortalUserProfileUrlsResponse::BillingAddress:
                profileInfoCategory = "BILLING_ADDRESS";
                break;
            case IdentityPortalUserProfileUrlsResponse::ShippingAddress:
                profileInfoCategory = "SHIPPING_ADDRESS";
                break;
            case IdentityPortalUserProfileUrlsResponse::UserDefaults:
                profileInfoCategory = "USER_DEFAULTS";
                break;
            case IdentityPortalUserProfileUrlsResponse::SafeChildInfo:
                profileInfoCategory = "SAFE_CHILD_INFO";
                break;
            default:;
            }

            return profileInfoCategory;
         }

        IdentityPortalUserNameResponse* IdentityPortalServiceClient::retrieveUserNamePriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalUserNameResponse(getNonAuth(request));
        }

        IdentityPortalUserProfileUrlsResponse* IdentityPortalServiceClient::updateUserNamePriv(const QString& servicePath, const QString& accessToken,
                const QString& firstName, const QString& lastName)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            /*
            {
                "nameInfo" : {
                 "first_name" : "John",
                 "last_name" : "smith1"
                }
            }
            */
            QString json = QString("{\"nameInfo\":{\"first_name\":\"%1\",\"last_name\":\"%2\"}}").arg(firstName).arg(lastName);
            QByteArray payload(json.toUtf8());

            return new IdentityPortalUserProfileUrlsResponse(putNonAuth(request, payload));
        }

        IdentityPortalUserIdResponse* IdentityPortalServiceClient::pidByEmailPriv(const QString& servicePath, const QString& accessToken, const QString& email)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("email", QUrl::toPercentEncoding(email.toUtf8()));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalUserIdResponse(getNonAuth(request));
        }

        IdentityPortalUserIdResponse* IdentityPortalServiceClient::pidByEAIDPriv(const QString& servicePath, const QString& accessToken, const QString& eaid)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("namespaceName", "cem_ea_id");
            serviceUrlQuery.addQueryItem("displayName", eaid.toUtf8());
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            setHeader(accessToken, request);

            return new IdentityPortalUserIdResponse(getNonAuth(request));
        }

        void IdentityPortalServiceClient::setHeader( const QString& accessToken, QNetworkRequest &request )
        {
            QByteArray ba;
            ba.append("Bearer ");
            ba.append(accessToken);
            request.setRawHeader("Authorization", ba);
        }

    }
}
