//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.

#include "lsxWriter.h"
#include "lsxReader.h"
#include "EbisuError.h"

#include "Service/SDKService/SDK_EntitlementManager.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "engine/content/ContentController.h"
#include "engine/content/EntitlementCache.h"
#include "engine/content/NucleusEntitlementController.h"
#include "engine/login/LoginController.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "services/session/SessionService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/publishing/NucleusEntitlementServiceClient.h"
#include "services/publishing/SignatureVerifier.h"

#include "TelemetryAPIDLL.h"


namespace Origin
{

namespace SDK
{

SDK_EntitlementManager::SDK_EntitlementManager()
{
    ORIGIN_VERIFY_CONNECT_EX(Engine::LoginController::instance(), &Engine::LoginController::userLoggedIn, this, &SDK_EntitlementManager::onUserLoggedIn, Qt::QueuedConnection);
}

SDK_EntitlementManager::~SDK_EntitlementManager()
{
}


void CreateErrorResponse(int code, const char *description, Lsx::Response &response)
{
    lsx::ErrorSuccessT resp;
    resp.Code = code;
    resp.Description = description;
    lsx::Write(response.document(), resp);
}

int GetServerErrorCodeAndDescription(QNetworkReply * pReply, QString &description)
{
    int status = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString error = pReply->readAll();
    QString errorString = pReply->errorString();

    description = QString("Request: ") + pReply->url().toString() + "\nInvalid Response Code = " + QVariant(status).toString() + "\n" + error + "\n" + errorString;

    ORIGIN_LOG_ERROR << description;

    return status;
}

void CreateServerErrorResponse(QNetworkReply * pReply, Lsx::Response& response)
{
    QString description;
    int code = GetServerErrorCodeAndDescription(pReply, description);

    CreateErrorResponse((int)(EBISU_ERROR_PROXY + code), description.toUtf8(), response);
}

void SDK_EntitlementManager::QueryEntitlements(Lsx::Request& request, Lsx::Response& response)
{
    lsx::QueryEntitlementsT r;

    if(lsx::Read(request.document(), r))
    {
        QueryEntitlements(r, response);
    }
    else
    {
        CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
    }
}


void SDK_EntitlementManager::QueryEntitlements(lsx::QueryEntitlementsT request, Lsx::Response& response)
{
    if(Engine::LoginController::instance() == NULL || Engine::LoginController::instance()->currentUser() == NULL)
    {
        CreateErrorResponse(EBISU_ERROR_NOT_LOGGED_IN, "The user is not logged into Origin.", response);
        return;
    }

    int *jobs = new int;
    *jobs = 1;  //< set the job count to 1 to prevent early deletion of the gatherWrapper, because the offline cache is not doing a network call.

    SDK_EntitlementManager::QueryEntitlementsPayload<lsx::QueryEntitlementsResponseT> payload;
    lsx::QueryEntitlementsResponseT *entitlementResponse = new lsx::QueryEntitlementsResponseT();
    payload.entitlements = &entitlementResponse->Entitlements;
    payload.request = request;
    payload.gatherWrapper = new ResponseWrapperEx<lsx::QueryEntitlementsResponseT *, SDK_EntitlementManager, int>
        (entitlementResponse, this, &SDK_EntitlementManager::GatherResponse, jobs, response);

    // Move the singular group into the filter group.
    if(payload.request.Group.length() > 0)
    {
        payload.request.FilterGroups.push_back(payload.request.Group);
    }

    std::vector<QString> offers;
    std::vector<QString> groups;

    // Only support one offer at the time.
    if(request.FilterOffers.size() > 1)
    {
        offers = request.FilterOffers;
        request.FilterOffers.clear();
    }

    // If the first offer is a comma separated list of offers.
    if(request.FilterOffers.size() == 1)
    {
        QStringList offerList = request.FilterOffers[0].split(",", QString::SkipEmptyParts);

        if(offerList.size() > 0)
        {
            foreach(QString offer, offerList)
            {
                offers.push_back(offer);
            }
            request.FilterOffers.clear();
        }
    }


    // If Enumerate child groups, and more than one group is specified.
    if(request.FilterGroups.size() > 1 && request.includeChildGroups)
    {
        groups = request.FilterGroups;
        request.FilterGroups.clear();
    }

    // Convert the Filters to a back end friendly format.
    for(std::vector<QString>::iterator i = payload.request.FilterCategories.begin(); i != payload.request.FilterCategories.end(); i++) { payload.FilterCategories.push_back(*i); }
    for(std::vector<QString>::iterator i = payload.request.FilterItems.begin(); i != payload.request.FilterItems.end(); i++) { payload.FilterItems.push_back(*i); }

    if(groups.size() > 0)
    {
        if(offers.size() > 0)
        {
            // vary offers, and groups
            for(std::vector<QString>::iterator offer = offers.begin(); offer != offers.end(); offer++)
            {
                for(std::vector<QString>::iterator group = groups.begin(); group != groups.end(); group++)
                {
                    payload.request.FilterOffers.clear();
                    payload.request.FilterOffers.push_back(*offer);
                    payload.request.FilterGroups.clear();
                    payload.request.FilterGroups.push_back(*group);

                    ++(*jobs);

                    QueryEntitlementsImpl(payload, response);
                }
            }
        }
        else
        {
            // vary groups
            for(std::vector<QString>::iterator group = groups.begin(); group != groups.end(); group++)
            {
                payload.request.FilterGroups.clear();
                payload.request.FilterGroups.push_back(*group);

                ++(*jobs);

                QueryEntitlementsImpl(payload, response);
            }
        }
    }
    else
    {
        if(offers.size() > 0)
        {
            // vary offers
            for(std::vector<QString>::iterator offer = offers.begin(); offer != offers.end(); offer++)
            {
                payload.request.FilterOffers.clear();
                payload.request.FilterOffers.push_back(*offer);

                ++(*jobs);

                QueryEntitlementsImpl(payload, response);
            }
        }
        else
        {
            if(Services::readSetting(Services::SETTING_DisableEntitlementFilter) ||
               request.FilterItems.size() > 0 || request.FilterGroups.size() > 0 || request.Group.length() > 0)
            {
                ++(*jobs);

                QueryEntitlementsImpl(payload, response);
            }
            else
            {
                if(response.connection()->CompareSDKVersion(9, 6, 0, 0) >= 0)
                {
                    lsx::ErrorSuccessT resp;
                    resp.Code = EBISU_ERROR_INVALID_ARGUMENT;
                    resp.Description = "Must filter by at least one of group, offer, or item";

                    lsx::Write(response.document(), resp);
                }
                else
                {
                    ++(*jobs);

                    QueryEntitlementsImpl(payload, response);

                    QString offerId = response.connection()->productId();
                    GetTelemetryInterface()->Metric_ORIGIN_SDK_UNFILTERED_ENTITLEMENTS_REQUEST(offerId.toLatin1().data(), response.connection()->sdkVersion().toLatin1().data());
                }
            }
        }

        --(*jobs);
        if(*jobs <= 0)
        {
            payload.gatherWrapper->finished();
        }
    }
}

void SDK_EntitlementManager::GetProductEntitlements(uint64_t userId, QString productId, Lsx::Response &response)
{
    if(productId.isEmpty())
    {
        CreateErrorResponse(EBISU_ERROR_NOT_FOUND, "No Product Found!", response);
        return;
    }

    SDK_EntitlementManager::QueryEntitlementsPayload<lsx::QueryManifestResponseT> payload;
    int *jobs = new int;
    *jobs = 1;

    lsx::QueryManifestResponseT * manifestResponse = new lsx::QueryManifestResponseT();
    payload.request.UserId = userId;
    payload.request.FilterOffers.push_back(productId);
    payload.entitlements = &manifestResponse->Entitlements;
    payload.gatherWrapper = new ResponseWrapperEx<lsx::QueryManifestResponseT *, SDK_EntitlementManager, int>(manifestResponse, this, &SDK_EntitlementManager::GatherResponse, jobs, response);

    QueryEntitlementsImpl(payload, response);
}


template <typename T> void SDK_EntitlementManager::QueryEntitlementsImpl(QueryEntitlementsPayload<T> &payload, Lsx::Response &response)
{
    using namespace Origin::Services::Publishing;
    using Engine::Content::EntitlementCache;
    using Engine::Content::CachedResponseData;

    // Transform into a back end compatible container.
    payload.FilterOffers.clear();
    for(std::vector<QString>::iterator i = payload.request.FilterOffers.begin(); i != payload.request.FilterOffers.end(); i++) { payload.FilterOffers.push_back(*i); }
    payload.FilterGroups.clear();
    for(std::vector<QString>::iterator i = payload.request.FilterGroups.begin(); i != payload.request.FilterGroups.end(); i++) { payload.FilterGroups.push_back(*i); }

    Services::Session::SessionRef session = Services::Session::SessionService::currentSession();
    if(!Services::Connection::ConnectionStatesService::isUserOnline(session))
    {
        QueryCachedEntitlements(payload, response);
        payload.gatherWrapper->finished();
        return;
    }

    NucleusEntitlementServiceResponse *resp =
        NucleusEntitlementServiceClient::sdkEntitlements(session, payload.FilterGroups, payload.FilterOffers, payload.FilterItems, QStringList() ,payload.request.includeChildGroups);

    ResponseWrapperEx<QueryEntitlementsPayload<T>, SDK_EntitlementManager, NucleusEntitlementServiceResponse> * wrapper =
        new ResponseWrapperEx<QueryEntitlementsPayload<T>, SDK_EntitlementManager, NucleusEntitlementServiceResponse>
            (payload, this, &SDK_EntitlementManager::QueryEntitlementsProcessResponse, resp, response);

    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), wrapper, SLOT(finished()));
}

template <typename T> void SDK_EntitlementManager::QueryCachedEntitlements(QueryEntitlementsPayload<T> &payload, Lsx::Response &response)
{
    using namespace Origin::Services::Publishing;
    using Engine::Content::EntitlementCache;
    using Engine::Content::CachedResponseData;

    NucleusEntitlementList entitlements;

    EntitlementCache *cache = EntitlementCache::instance();
    QHash<QString, CachedResponseData>* sdkEntitlementsCache = cache->sdkEntitlementsCacheData(Engine::LoginController::instance()->currentUser()->getSession());

    bool noUser = Engine::LoginController::instance()->currentUser().isNull();
    bool noCache = sdkEntitlementsCache == nullptr || cache == nullptr;

    if (noUser || noCache)
    {
        CreateErrorResponse(EBISU_ERROR_COMMERCE_INVALID_REPLY, "Couldn't decode server reply.", response);
    }
    else
    {
        const QString &cacheKey = constructCacheKey(payload);

        if(sdkEntitlementsCache->contains(cacheKey))
        {
            const QByteArray &key = PlatformService::machineHashAsString().toUtf8();
            const Origin::Engine::Content::CachedResponseData& cacheData = sdkEntitlementsCache->value(cacheKey);
            const QByteArray responseSignature = QByteArray::fromBase64(cacheData.m_responseSignature);

            if(SignatureVerifier::verify(key, cacheData.m_responseData, responseSignature))
            {
                if(NucleusEntitlement::parseJson(cacheData.m_responseData, entitlements))
                {
                    foreach(const Services::Publishing::NucleusEntitlementRef& ent, entitlements)
                    {
                        payload.entitlements->push_back(ent->toLSX());
                    }
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Failed to parse cached response";
                    CreateErrorResponse(EBISU_ERROR_CORE_RECEIVE_FAILED, "Offline cache cannot be read.", response);
                }
            }
            else
            {
                ORIGIN_LOG_WARNING << "Cached response failed verification.";
                CreateErrorResponse(EBISU_ERROR_CORE_RECEIVE_FAILED, "Offline cache cannot be read.", response);
            }
        }
    }
}

void SDK_EntitlementManager::onEntitlementGranted(const QByteArray payload)
{   
    using namespace Origin::Services::Publishing;
    QScopedPointer<INodeDocument> doc(CreateJsonDocument());

    if (doc && doc->Parse(payload))
    {
        server::dirtybitEntitlementDataT data;

        if (server::Read(doc.data(), data))
        {
            QStringList entitlementIds;
            entitlementIds.append(data.data.entId);
            Services::Session::SessionRef session = Services::Session::SessionService::currentSession();

            NucleusEntitlementServiceResponse *resp =
                NucleusEntitlementServiceClient::sdkEntitlements(session, QStringList(), QStringList(), QStringList(), entitlementIds, false);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(GrantEntitlementsProcessResponse()));
        }
    }

}

void SDK_EntitlementManager::GrantEntitlementsProcessResponse()
{
    Origin::Services::Publishing::NucleusEntitlementServiceResponse* resp = (Origin::Services::Publishing::NucleusEntitlementServiceResponse*)sender();
    emit entitlementGranted(resp);
}

template <typename T> QString SDK_EntitlementManager::constructCacheKey(const QueryEntitlementsPayload<T> &payload) const
{
    QUrl cacheUrl(QString("sdkentitlements:%1").arg(QString::number(payload.request.UserId)));
    QUrlQuery query;

    if (!payload.FilterGroups.isEmpty())
    {
        query.addQueryItem("groupName", payload.FilterGroups.join(','));
    }

    if (!payload.FilterOffers.isEmpty())
    {
        query.addQueryItem("productId", payload.FilterOffers.at(0));
    }

    if (!payload.FilterItems.isEmpty())
    {
        query.addQueryItem("entitlementTag", payload.FilterItems.at(0));
    }

    if (payload.request.includeChildGroups)
    {
        query.addQueryItem("includeChildGroups", "true");
    }

    query.addQueryItem("machine_hash", PlatformService::machineHashAsString());
    cacheUrl.setQuery(query);

    return QString::fromUtf8(QCryptographicHash::hash(cacheUrl.toString().toUtf8(), QCryptographicHash::Sha1).toHex().toUpper());
}


template <typename T> bool SDK_EntitlementManager::QueryEntitlementsProcessResponse(QueryEntitlementsPayload<T> &payload, Origin::Services::Publishing::NucleusEntitlementServiceResponse *serverReply, Lsx::Response &response)
{
    using namespace Origin::Services::Publishing;
    using Engine::Content::EntitlementCache;
    using Engine::Content::CachedResponseData;

    auto status = serverReply->refreshStatus();

    switch(status)
    {
    case EntitlementRefreshStatusOK:
        {
            // Response is valid and signed, cache its contents.
            EntitlementCache *cache = EntitlementCache::instance();
            if(nullptr != cache)
            {
                Services::Session::SessionRef session = Services::Session::SessionService::currentSession();

                const QByteArray &signature = serverReply->reply()->rawHeader(SignatureVerifier::SIGNATURE_HEADER);
                const QString &cacheKey = constructCacheKey(payload);
                CachedResponseData cacheData(Engine::Content::ResponseTypeSdkEntitlements, serverReply->responseBody(), signature, cacheKey);

                cache->insert(session, signature, cacheData);
            }

            // Return the entitlements.
            foreach(const Services::Publishing::NucleusEntitlementRef& ent, serverReply->entitlements())
            {
                if(!ent.isNull())
                {
                    payload.entitlements->push_back(ent->toLSX());
                }
            }
        }
        break;  
    case Services::Publishing::EntitlementRefreshStatusSignatureInvalid:
        {
            ORIGIN_LOG_ERROR << "Signature verification for QueryEntitlements failed.";
            CreateServerErrorResponse(serverReply->reply(), response);
        }
        break;
    case Services::Publishing::EntitlementRefreshStatusServerError:
        {
            QString description;
            int code = GetServerErrorCodeAndDescription(serverReply->reply(), description);

            ORIGIN_LOG_EVENT << "Returning offline cache entitlements as server returns: " << code << ", " << description;

            QueryCachedEntitlements(payload, response);
        }
        break;

    case Services::Publishing::EntitlementRefreshStatusUnknownError:
    case Services::Publishing::EntitlementRefreshStatusBadPayload:
    default:
        {
            CreateErrorResponse(EBISU_ERROR_COMMERCE_INVALID_REPLY, "Couldn't decode server reply.", response);
        }
        break;
    }

    payload.gatherWrapper->finished();

    return true;
}

template <typename T> bool SDK_EntitlementManager::GatherResponse(T *& data, int * jobsPending, Lsx::Response& response)
{
    (*jobsPending)--;

    if(*jobsPending <= 0)
    {
        // Check if there is already a response written into the response.
        if(response.isPending())
        {
            lsx::Write(response.document(), *data);
        }

        // The gather process needs to delete the document.
        delete data;
        return true;
    }
    return false;
}

void SDK_EntitlementManager::onUserLoggedIn()
{
    Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onEntitlementGranted", "entitlement");
}

} // namespace SDK

} // namespace Origin
