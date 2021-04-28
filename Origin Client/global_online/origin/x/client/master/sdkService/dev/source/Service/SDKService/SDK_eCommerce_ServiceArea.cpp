///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SDK_ECommerceServiceArea.cpp
//  SDK Service Area
//  
//  Author: Hans van Veenendaal

#include <memory>
#include "eabase/eabase.h"
#include "EbisuError.h"
#include "LSX/LsxConnection.h"
#include "Service/SDKService/SDK_eCommerce_ServiceArea.h"
#include "engine/utilities/ContentUtils.h"
#include "ecommerce2.h"
#include "ecommerce2Reader.h"
#include "ecommerce2EnumStrings.h"
#include "lsx.h"
#include "lsxreader.h"
#include "lsxwriter.h"
#include "LsxWrapper.h"
#include "ReaderCommon.h"
#include "server.h"
#include "serverEnumStrings.h"
#include "serverreader.h"
#include "QNetworkRequest.h"
#include "QNetworkReply.h"
#include "QNetworkConfigManager.h"
#include "services/log/LogService.h"
#include <QDomDocument>
#include "JsonDocument.h"
#include "TelemetryAPIDLL.h"

#include "ReaderCommon.h"

#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/dirtybits/DirtyBitsClient.h"

#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/publishing/ConversionUtil.h"
#include "services/downloader/Common.h"
#include "services/platform/PlatformService.h"

#include "engine/content/EntitlementCache.h"
#include "services/publishing/SignatureVerifier.h"

#define IGNORE_SSL_ERRORS 1
// Exported from OriginApplication.cpp
extern bool ForceLockboxURL();

namespace Origin
{
    namespace SDK
    {
        namespace //Empty
        {
            // Singleton functions
            SDK_ECommerceServiceArea* mpSingleton = NULL;
    
            QString FindAttributeValue(std::vector<server::attributeT> &attributes, const char *pName)
            {
                for(std::vector<server::attributeT>::iterator i=attributes.begin(); i!=attributes.end(); i++)
                {
                    if(i->name.compare(pName) == 0)
                    {
                        return i->value;
                    }
                }
                return "";
            }
        }


        SDK_ECommerceServiceArea* SDK_ECommerceServiceArea::create(Lsx::LSX_Handler* handler)
        {
            if (mpSingleton == NULL)
            {
                mpSingleton = new SDK_ECommerceServiceArea(handler);
            }
            return mpSingleton;
        }

        SDK_ECommerceServiceArea* SDK_ECommerceServiceArea::instance()
        {
            return mpSingleton;
        }

        void SDK_ECommerceServiceArea::destroy()
        {
            delete mpSingleton;
            mpSingleton = NULL;
        }

        SDK_ECommerceServiceArea::SDK_ECommerceServiceArea( Lsx::LSX_Handler* handler )
            : BaseService( handler, "Commerce" ), 
            m_processingEntitlements(false)
        {
            registerHandler("SelectStore", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::SelectStore );
            registerHandler("GetStore", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::GetStore );
            registerHandler("GetCatalog", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::GetCatalog );
            registerHandler("GetWalletBalance", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::GetWalletBalance );
            registerHandler("Checkout", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::DirectCheckout);
            registerHandler("QueryCategories", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::QueryCategories );
            registerHandler("QueryOffers", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::QueryOffers );
            registerHandler("QueryEntitlements", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::QueryEntitlements );
            registerHandler("QueryManifest", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::QueryManifest );
            registerHandler("ConsumeEntitlement", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::ConsumeEntitlement );
            registerHandler("ShowIGOWindow", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::ShowIGOWindow );
            registerHandler("GetLockboxUrl", ( BaseService::RequestHandler ) &SDK_ECommerceServiceArea::GetLockboxUrl );

            ORIGIN_VERIFY_CONNECT_EX(&m_EntitlementManager, &SDK_EntitlementManager::entitlementGranted, this, &SDK_ECommerceServiceArea::onEntitlementGranted, Qt::QueuedConnection);
        }

        void SDK_ECommerceServiceArea::selectStore(QString lockboxURL)
        {
            m_LockboxURL = lockboxURL;
        }

        void SDK_ECommerceServiceArea::purchaseCompleted()
        {
            // Send an event to the SDK.
            lsx::PurchaseEventT pe;

            // Create a list of ProductId's
            pe.manifest = m_ProductToBuy.toUtf8();

            sendEvent(pe);
        }

        lsx::GetLockboxUrlResponseT SDK_ECommerceServiceArea::getLockboxUrl(QString commerceProfile, QString userId, QString productId, QString context, QString contentId, bool isSdk)
        {
            lsx::GetLockboxUrlResponseT urlResponse;
            
            if(!useLegacyCatalog(contentId))
            {
                urlResponse.Url = Services::readSetting(Services::SETTING_EbisuLockboxV3URL).toString();
                urlResponse.Url.replace("{profile}", commerceProfile);
                
                // The offer/offers portion is a base64 encoded JSON of offer ID's and quantities.  For example, {"OFB-FIFA:39330":1}, or {"Origin.OFR.50.0000404":1,"Origin.OFR.50.00000403":1}.
                QString offerJSON = QString("{\"%1\":1}").arg(productId);
                QByteArray encodedOffer;
                encodedOffer.append(offerJSON);
                urlResponse.Url.replace("{offers}", encodedOffer.toBase64());

                // Remember the product that you are going to buy. NOTE: This only works for non concurrent purchases.
                m_ProductToBuy = productId;

                return urlResponse;
            }

            // Using old checkout flow, querying ec2 for lockbox url.            
            
            QUrl url(GetECommerceURL(contentId, isSdk) + "/checkout");
            QUrlQuery urlQuery(url);

            if(!m_LockboxSucceededURL.isEmpty())
                urlQuery.addQueryItem("successUrl", QUrl::toPercentEncoding(m_LockboxSucceededURL));

            if(!m_LockboxFailedURL.isEmpty())
                urlQuery.addQueryItem("failedUrl", QUrl::toPercentEncoding(m_LockboxFailedURL));

            if(!isSdk)
            {
                urlQuery.addQueryItem("offerId", QUrl::toPercentEncoding(productId));
            }
            else
            {   
                urlQuery.addQueryItem("productId", QUrl::toPercentEncoding(productId));
            }

            urlQuery.addQueryItem("userId", userId);
            urlQuery.addQueryItem("region", GetCountry());
            urlQuery.addQueryItem("lang", GetLanguage());
            urlQuery.addQueryItem("profile", commerceProfile);
            urlQuery.addQueryItem("sourceType", "IGOCheckout");
            urlQuery.addQueryItem("context", context);
            urlQuery.addQueryItem("sku", contentId);

            if(ForceLockboxURL() && !m_LockboxURL.isEmpty())
            {
                urlQuery.addQueryItem("lockboxUrl", QUrl::toPercentEncoding(m_LockboxURL));
                urlQuery.addQueryItem("overrideLockboxUrl", "true");
            }

            // Remember the product that you are going to buy. NOTE: This only works for non concurrent purchases.
            m_ProductToBuy = productId;

            QString reply;

            if(m_EWalletCategoryId.toLongLong() != 0)
                urlQuery.addQueryItem("ewalletcategoryId", m_EWalletCategoryId.toString());

            if(!m_VirtualCurrency.isEmpty())
                urlQuery.addQueryItem("currency", m_VirtualCurrency);

            url.setQuery(urlQuery);
            QNetworkRequest serverRequest(url);
            serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
            serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
            
            if(isSdk)
                serverRequest.setRawHeader("X-Origin-SDK", "true");

            QNetworkReply *serverReply;
            if(DoServerCall(serverRequest, OPERATION_GET, serverReply) == 302)
            {
                urlResponse.Url = serverReply->header(QNetworkRequest::LocationHeader).toString();
            }

            delete serverReply;

            return urlResponse;
        }

        QString SDK_ECommerceServiceArea::GetAccessToken()
        {
            auto user = Engine::LoginController::instance()->currentUser();
            if (user)
            {
                auto session = user->getSession();
                return Origin::Services::Session::SessionService::accessToken(session);
            }
            return "";
        }

        QString SDK_ECommerceServiceArea::GetCountry()
        {
            auto user = Origin::Engine::LoginController::currentUser();
            return user.isNull() ? "" : user->country();
        }

        QString SDK_ECommerceServiceArea::GetLanguage()
        {
            QString language = QLocale().name();

            // Add the country if not already there.
            if(language.length() == 2)
            {
                language += "_";
                language += GetCountry();
            }
            return language;
        }

        bool SDK_ECommerceServiceArea::useLegacyCatalog(const QString& contentId)
        {
            Origin::Engine::Content::ContentController *controller = Origin::Engine::Content::ContentController::currentUserContentController();

            if(controller)
            {
                Origin::Engine::Content::EntitlementRef entitlement = controller->entitlementByContentId(contentId);

                if (entitlement)
                {
                    return entitlement->contentConfiguration()->useLegacyCatalog();
                }
            }
            return false;
        }

        bool SDK_ECommerceServiceArea::returnUnpublishedContent(Lsx::Request& request)
        {
            Lsx::Connection * connection = request.connection();

            if(connection)
            {
                Origin::Engine::Content::EntitlementRef entitlement = connection->entitlement();

                if(!entitlement.isNull())
                {
                    return (entitlement->contentConfiguration()->originPermissions() >= Origin::Services::Publishing::OriginPermissions1102);
                }
            }
            return false;
        }

        bool SDK_ECommerceServiceArea::useLegacyCatalog(Lsx::Request& request)
        {
            Lsx::Connection * connection = request.connection();

            if(connection)
            {
                Origin::Engine::Content::EntitlementRef entitlement = connection->entitlement();

                if(!entitlement.isNull())
                {
                    return entitlement->contentConfiguration()->useLegacyCatalog();
                }
            }
            return false;
        }


        QString SDK_ECommerceServiceArea::GetECommerceURL(bool useLegacy)
        {
            if(useLegacy)
            {
                return Origin::Services::readSetting(Origin::Services::SETTING_EcommerceV1URL).toString();
            }
            else
            {
                return Origin::Services::readSetting(Origin::Services::SETTING_EcommerceURL).toString();
            }
        }

        QString SDK_ECommerceServiceArea::GetECommerceURL(const QString& contentId, bool isSdk)
        {
            return GetECommerceURL(isSdk && useLegacyCatalog(contentId));
        }

        QString SDK_ECommerceServiceArea::GetECommerceURL(Lsx::Request& request)
        {
            return GetECommerceURL(useLegacyCatalog(request));
        }

        void SDK_ECommerceServiceArea::SelectStore(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::SelectStoreT r;

            if(lsx::Read(request.document(), r))
            {
                m_StoreId = r.StoreId;
                m_CatalogId = r.CatalogId;
                m_EWalletCategoryId = r.EWalletCategoryId;      // This can be configured on the lockbox instance --> don't send if 0.
                m_VirtualCurrency = r.VirtualCurrency;  // This can be configured on the lockbox instance --> don't send if empty;
                m_LockboxURL = r.LockboxUrl;
                m_LockboxSucceededURL = r.SuccessUrl;
                m_LockboxFailedURL = r.FailedUrl;
            }

            // Respond with everything is OK.
            CreateErrorResponse(EBISU_SUCCESS, "", response);
        }

        void ConvertData(lsx::CatalogT &catalog, server::catalogT &scatalog)
        {
            catalog.Name = scatalog.catalogName;
            catalog.Status = scatalog.status;
            catalog.CurrencyType = scatalog.currencyType;
            catalog.Group = FindAttributeValue(scatalog.attributes, "storeGroupId");
            catalog.CatalogId = scatalog.catalogId.toULongLong();
        }

        void ConvertData(std::vector<lsx::CatalogT> &catalogs, std::vector<server::catalogT> &scatalogs)
        {
            for(unsigned int i=0; i<scatalogs.size(); i++)
            {
                lsx::CatalogT cat;

                ConvertData(cat, scatalogs[i]);

                catalogs.push_back(cat);
            }
        }

        void ConvertData(lsx::StoreT &store, server::storeT &sstore)
        {
            store.Name = sstore.storeName;
            store.Title = sstore.title;
            store.Group = FindAttributeValue(sstore.attributes, "storeGroupId");
            store.Status = sstore.status;
            store.DefaultCurrency = FindAttributeValue(sstore.attributes, "defaultCurrencyUomId");
            store.StoreId = sstore.storeId.toULongLong();
            QString sTemp = FindAttributeValue(sstore.attributes, "isDemoStore");
            store.IsDemoStore = sTemp.startsWith('y', Qt::CaseInsensitive) ? true : false;

            // These parameters are communicated as elements

            ConvertData(store.Catalog, sstore.catalogs);
        }


        void SDK_ECommerceServiceArea::GetStore(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetStoreT r;

            if(lsx::Read(request.document(), r))
            {
                if(r.StoreId != 0)
                    m_StoreId = r.StoreId;
    
                if(m_StoreId.toULongLong() != 0)
                {
                    QUrl url(GetECommerceURL(request) + "/store/" + m_StoreId.toString());
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("expanded", "true");
                    url.setQuery(urlQuery);
                    
                    QNetworkRequest serverRequest(url);
                    serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                    serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                    serverRequest.setRawHeader("X-Origin-SDK", "true");

                    QNetworkReply *serverReply;
                    if(DoServerCall(serverRequest, OPERATION_GET, serverReply) == 200)
                    {
                        QByteArray replyString = serverReply->readAll();

                        // We received our message.
                        QDomDocument doc;
                        doc.setContent(replyString);

                        LSXWrapper reply(doc);

                        server::storeT sstore;

                        if(server::Read(&reply,  sstore))
                        {
                            lsx::GetStoreResponseT store;

                            ConvertData(store.Store, sstore);

                            lsx::Write(response.document(),  store);
                        }
                        else
                        {
                            CreateErrorResponse(-1, "Server response could not be decoded.", response);
                        }
                    }
                    else
                    {
                        CreateServerErrorResponse(serverReply, response);
                    }
                    delete serverReply;
                }
                else
                {
                    CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "StoreId not specified", response);
                    return;
                }
            }
        }


        void ConvertData(lsx::EntitlementT &entitlement, server::entitlementT &sentitlement)
        {
            entitlement.Type = server::entitlementTypeTypeStrings[server::entitlementTypeType_EnumToIndexMap[sentitlement.entitlementType]];
            entitlement.ItemId = sentitlement.productId;
            entitlement.EntitlementId = sentitlement.entitlementId;
            entitlement.EntitlementTag = sentitlement.entitlementTag;
            entitlement.Group = sentitlement.groupName;
            entitlement.ResourceId = "";
            entitlement.UseCount = sentitlement.useCount;
            entitlement.Expiration = sentitlement.terminationDate;
            entitlement.GrantDate = sentitlement.grantDate;
        }

        void ConvertData(lsx::OfferT &offer, server::productT &soffer, const QString &country)
        {
            offer.Type = soffer.productType;
            offer.OfferId = soffer.productId;
            offer.Name = FindAttributeValue(soffer.attributes, "Product Name");
            offer.Description = FindAttributeValue(soffer.attributes, "Long Description");
            offer.ImageId = FindAttributeValue(soffer.attributes, "imageTemplate");
    
            offer.bIsOwned = soffer.isOwned;
            offer.bCanPurchase = soffer.canPurchase;
            offer.bHidden = FindAttributeValue(soffer.attributes, "Is Displayable") != "Y";

            OriginTimeT s;
            memset(&s, 0, sizeof(s));

            // TODO: Get proper values here.
            offer.PurchaseDate = s;
            offer.DownloadDate = s;
            offer.PlayableDate = s;
            offer.DownloadSize = 0;
            offer.bIsDiscounted = false;
            offer.OriginalPrice = 0;
            offer.LocalizedOriginalPrice = "";

            if(soffer.pricePoints.freeProduct == true)
            {
                offer.Currency = "Free";
                offer.Price = 0;
                offer.LocalizedPrice = "";
            }
            else
            {
                if(soffer.pricePoints.pricepoint.size()>=1)
                {
                    bool assigned = false;


                    for(unsigned int i=0; i<soffer.pricePoints.pricepoint.size(); i++)
                    {
                        if(country == soffer.pricePoints.pricepoint[i].locale)
                        {
                            offer.Currency = soffer.pricePoints.pricepoint[i].currency;
                            offer.Price = soffer.pricePoints.pricepoint[i].price;
                            offer.LocalizedPrice = QVariant(offer.Price).toString().toLatin1();

                            assigned = true;

                            break;
                        }
                    }

                    if(!assigned)
                    {
                        offer.Currency = soffer.pricePoints.pricepoint[0].currency;
                        offer.Price = soffer.pricePoints.pricepoint[0].price;
                        offer.LocalizedPrice = QVariant(offer.Price).toString().toLatin1();
                    }
                }
                else
                {
                    offer.Currency = "Unknown";
                    offer.Price = -1;
                    offer.LocalizedPrice = "Unknown";
                }
            }

            offer.InventoryCap = -1;
            offer.InventorySold = -1;
            offer.InventoryAvailable = -1;

            if(FindAttributeValue(soffer.attributes, "isLimitedQuantity") == "Y")
            {
                // TODO: Get proper values here.
                offer.InventoryCap = 100;
                offer.InventorySold = 0;
                offer.InventoryAvailable = 100;
            }
        }

        void ConvertData(lsx::CategoryT &category, server::categoryT &scategory, bool includeSubCategories)
        {
            category.Name = scategory.name;
            category.CategoryId = scategory.categoryId;
            category.Description = FindAttributeValue(scategory.attributes, "Category Description");
            category.ImageId = FindAttributeValue(scategory.attributes, "Image");
            category.MostPopular = FindAttributeValue(scategory.attributes, "Most Popular") == "true";
            category.ParentId = FindAttributeValue(scategory.attributes, "Parent");
            category.Type = scategory.categoryType;

            /*if(includeSubCategories)
            {*/
                //for(unsigned int i=0; i<scategory.categories.category.size(); i++)
                //{
                //  lsx::Category cat;

                //  ConvertDataCategory(cat, scategory.categories.category[i]);

                //  category.Categories.push_back(cat);
                //}
            /*}*/
        }

        void SDK_ECommerceServiceArea::sslErrors ( QNetworkReply * reply, const QList<QSslError> & errors )
        {
            if(IGNORE_SSL_ERRORS)
            {
                reply->ignoreSslErrors();
            }
        }

        void SDK_ECommerceServiceArea::onEntitlementGranted(Origin::Services::Publishing::NucleusEntitlementServiceResponse* resp)
        {
            lsx::QueryEntitlementsResponseT e;
            foreach(const Services::Publishing::NucleusEntitlementRef& ent, resp->entitlements())
            {
                e.Entitlements.push_back(ent->toLSX());
            }
            sendEvent(e);
            resp->deleteLater();
        }

        void SDK_ECommerceServiceArea::GetCatalog(Lsx::Request& request, Lsx::Response& response)
        {
            lsx::GetCatalogT r;

            if(lsx::Read(request.document(), r))
            {
                // Make a request to Ebisu Proxy
                QUrl url(GetECommerceURL(request) + "/catalog/" + m_CatalogId.toString());
                QUrlQuery urlQuery(url);
                urlQuery.addQueryItem("expanded", "true");
                url.setQuery(urlQuery);
                
                QNetworkRequest serverRequest(url);
                serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                serverRequest.setRawHeader("X-Origin-SDK", "true");
        
                QNetworkReply *serverReply;
                if(DoServerCall(serverRequest, OPERATION_GET, serverReply) == 200)
                {
                    QByteArray replyString = serverReply->readAll();

                    // We received our message.
                    QDomDocument doc;
                    doc.setContent(replyString);

                    LSXWrapper qreply(doc);

                    server::catalogT catalog;
                    if(server::Read(&qreply, catalog))
                    {
                        lsx::GetCatalogResponseT s;
                
                        for(unsigned int c=0; c<catalog.categories.size();c++)
                        {
                            lsx::CategoryT category;

                            ConvertData(category, catalog.categories[c], true);

                            s.Catalog.Categories.push_back(category);
                        }

                        lsx::Write(response.document(), s);
                    }
                    else
                    {
                        CreateErrorResponse(EBISU_ERROR_LSX_INVALID_RESPONSE, "Couldn't decode server reply.", response);
                    }
                }
                else
                {
                    CreateServerErrorResponse(serverReply, response);
                }
                delete serverReply;
            }
            else
            {
				CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        void SDK_ECommerceServiceArea::GetWalletBalance(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetWalletBalanceT r;

            if(lsx::Read(request.document(), r))
            {
                QUrl url(GetECommerceURL(request) + "/billingaccount/" + QVariant(r.UserId).toString() + "/wallet");
                QUrlQuery urlQuery(url);
                urlQuery.addQueryItem("currency", r.Currency);
                url.setQuery(urlQuery);
                
                QNetworkRequest serverRequest(url);
                serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                serverRequest.setRawHeader("X-Origin-SDK", "true");

                DoServerCall(serverRequest, OPERATION_GET, &SDK_ECommerceServiceArea::GetWalletBalanceProcessResponse, response);
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        bool SDK_ECommerceServiceArea::GetWalletBalanceProcessResponse(QNetworkReply *serverReply, Lsx::Response &response)
        {
            if(serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                QByteArray replyString = serverReply->readAll();

                // We received our message.
                QDomDocument doc;
                doc.setContent(replyString);

                LSXWrapper qreply(doc);

                server::billingaccountsT accounts;
                if(server::Read(&qreply, accounts))
                {
                    lsx::GetWalletBalanceResponseT s;
                    s.Balance = 0;

                    for(unsigned int i = 0; i < accounts.walletAccount.size(); i++)
                    {
                        server::walletAccountT &account = accounts.walletAccount[i];

                        if(account.status == "ACTIVE")
                        {
                            s.Balance = account.balance;
                            break;
                        }
                    }

                    lsx::Write(response.document(), s);
                }
                else
                {
                    CreateErrorResponse(EBISU_ERROR_LSX_INVALID_RESPONSE, "Couldn't understand the server response", response);
                }
            }
            else
            {
                CreateServerErrorResponse(serverReply, response);
            }

            return true;
        }

        bool SDK_ECommerceServiceArea::isUserUnderAge() const
        {
            return Origin::Engine::LoginController::currentUser()->isUnderAge();
        }

        void SDK_ECommerceServiceArea::DirectCheckout(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::CheckoutT r;

            if(lsx::Read(request.document(), r))
            {
                if(r.Offers.size() >= 1)
                {
                    QUrl url(GetECommerceURL(request) + "/vccheckout/" + QVariant(r.UserId).toString());
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("currency", r.Currency);
                    urlQuery.addQueryItem("profile", request.connection()->commerceProfile());
                    url.setQuery(urlQuery);
                    
                    QNetworkRequest serverRequest(url);
                    serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                    serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                    serverRequest.setRawHeader("X-Origin-SDK", "true");
                    serverRequest.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

                    QDomDocument doc;

                    QDomElement checkout = doc.createElement("checkout"); 
                    QDomElement lineItems = doc.createElement("lineItems"); checkout.appendChild(lineItems);
                    for(unsigned int i=0; i<r.Offers.size(); i++)
                    {
                        QDomElement lineItem = doc.createElement("lineItem"); lineItems.appendChild(lineItem);
                        QDomElement productId = doc.createElement("productId"); lineItem.appendChild(productId);
                        QDomText offer = doc.createTextNode(r.Offers[i]); productId.appendChild(offer);
                        QDomElement quantity = doc.createElement("quantity"); lineItem.appendChild(quantity);
                        QDomText val = doc.createTextNode("1"); quantity.appendChild(val);
                    }
                    doc.appendChild(checkout);

                    DoServerCall(serverRequest, OPERATION_POST, &SDK_ECommerceServiceArea::DirectCheckoutProcessResponse, response, doc.toByteArray());
                }
                else
                {
					CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "No offer specified.", response);
                }
            }
            else
            {
				CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        bool SDK_ECommerceServiceArea::DirectCheckoutProcessResponse(QNetworkReply *serverReply, Lsx::Response &response)
        {
            if(serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                QByteArray replyString = serverReply->readAll();

                // We received our message.
                QDomDocument doc;
                doc.setContent(replyString);

                LSXWrapper qreply(doc);

                server::checkoutT checkout;
                if(server::Read(&qreply, checkout))
                {
                    CreateErrorResponse(checkout.entitlements.size() > 0 ? EBISU_SUCCESS : EBISU_ERROR_PROXY_NOT_FOUND, "No Entitlements awarded", response);

                    // Pull any newly added entitlements
                    Origin::Engine::Content::ContentController *controller = Origin::Engine::Content::ContentController::currentUserContentController();
                    if(controller)
                    {
                        controller->refreshUserGameLibrary(Engine::Content::ContentUpdates);
                    }
                }
                else
                {
                    CreateErrorResponse(EBISU_ERROR_LSX_INVALID_RESPONSE, "Couldn't understand the server response", response);
                }
            }
            else
            {
                CreateServerErrorResponse(serverReply, response);
            }

            return true;
        }

        void SDK_ECommerceServiceArea::QueryCategories(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::QueryCategoriesT r;

            if(lsx::Read(request.document(), r))
            {
        
                // Make a request to Ebisu Proxy

                QUrl url(GetECommerceURL(request) + "/catalog/" + m_CatalogId.toString());
                QUrlQuery urlQuery(url);
                urlQuery.addQueryItem("expanded", "true");
                url.setQuery(urlQuery);
                
                QNetworkRequest serverRequest(url);
                serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                serverRequest.setRawHeader("X-Origin-SDK", "true");

                QNetworkReply *serverReply;
                if(DoServerCall(serverRequest, OPERATION_GET, serverReply) == 200)
                {
                    QByteArray replyString = serverReply->readAll();

                    // We received our message.
                    QDomDocument doc;
                    doc.setContent(replyString);

                    LSXWrapper qreply(doc);

                    server::catalogT catalog;
                    if(server::Read(&qreply, catalog))
                    {
                        lsx::QueryCategoriesResponseT qcr;
                
                        if(r.FilterCategories.size() == 0)
                        {
                            for(unsigned int c=0; c<catalog.categories.size();c++)
                            {
                                lsx::CategoryT category;

                                ConvertData(category, catalog.categories[c], true);

                                qcr.Categories.push_back(category);
                            }
                        }
                        else
                        {
                            // Only include the filtered categories.

                            //TODO: implement this.
                        }

                        lsx::Write(response.document(), qcr);
                    }
                    else
                    {
                        CreateErrorResponse(EBISU_ERROR_LSX_INVALID_RESPONSE, "Couldn't decode server reply", response);
                    }
                }
                else
                {
                    CreateServerErrorResponse(serverReply, response);
                }
                delete serverReply;
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        bool SDK_ECommerceServiceArea::ProcessEC1ProductsResponse( QDomDocument &doc, lsx::QueryOffersResponseT &qor, const QString country )
        {
            // Process as an EC1 response.
            LSXWrapper qreply(doc);

            std::vector<server::productT> products;
            if(server::Read(&qreply, products))
            {
                for(unsigned int p=0; p<products.size();p++)
                {
                    lsx::OfferT product;

                    ConvertData(product, products[p], country);

                    qor.Offers.push_back(product);
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        void ConvertData(lsx::OfferT &offer, ecommerce2::offerT &soffer, const QString &country)
        {
            offer.Type =  ecommerce2::offerTypeEnumStrings[soffer.offerType];
            offer.OfferId = soffer.offerId;
            offer.Name = soffer.localizableAttributes.displayName;
            offer.Description = soffer.localizableAttributes.longDescription;
            offer.ImageId = soffer.localizableAttributes.originSdkImage;

            offer.bIsOwned = soffer.ecommerceAttributes.isOwned;
            offer.bCanPurchase = soffer.ecommerceAttributes.userCanPurchase;
            offer.bHidden = !soffer.ecommerceAttributes.isDisplayable; 

            OriginTimeT s;
            memset(&s, 0, sizeof(s));

            // TODO: Get proper values here.
            offer.PurchaseDate = s;
            offer.DownloadDate = soffer.publishing.publishingAttributes.publishedDate;
            offer.PlayableDate = s;
            offer.DownloadSize = 0;


            // Find proper software control dates
            ecommerce2::softwarePlatformEnumT curPlatform;

            switch(Origin::Services::PlatformService::runningPlatform())
            {
            case Origin::Services::PlatformService::PlatformMacOS:
                curPlatform = ecommerce2::SOFTWAREPLATFORMENUM_MAC;
                break;
            case Origin::Services::PlatformService::PlatformWindows:
                curPlatform = ecommerce2::SOFTWAREPLATFORMENUM_PC;
                break;
            default:
                curPlatform = ecommerce2::SOFTWAREPLATFORMENUM_PC;
                break;
            }

            for(unsigned int i = 0; i < soffer.publishing.softwareControlDates.softwareControlDate.size(); i++)
            {
                ecommerce2::softwareControlDateT& softwareControlDate = soffer.publishing.softwareControlDates.softwareControlDate.at(i);

                if(curPlatform == softwareControlDate.platform)
                {
                    offer.DownloadDate = softwareControlDate.downloadStartDate;
                    offer.PlayableDate = softwareControlDate.releaseDate;
                    break;
                }
            }

            // find country
            ecommerce2::countryT c;

            bool bDataSet = false;      // Indicator that we found a result
            bool bUseFirst = false;     // Retry indicator when no axact match is found.   

            while(!bDataSet && soffer.countries.country.size()>0)
            {
                for(unsigned int i=0; i<soffer.countries.country.size(); i++)
                {
                    c = soffer.countries.country[i];

                    if(c.countryCode == country || bUseFirst)
                    {
                        if(c.attributes.freeProduct)
                        {
                            offer.Currency = "Free";
                            offer.Price = 0;
                            offer.LocalizedPrice = "";
                            offer.bIsDiscounted = false;
                            offer.OriginalPrice = 0;
                            offer.LocalizedOriginalPrice = "";

                            bDataSet = true;
                            break;
                        }
                        else
                        {
                            offer.bIsDiscounted = false;

                            for(unsigned int p = 0; p<c.prices.price.size(); p++)
                            {
                                ecommerce2::priceT & price = c.prices.price[p];

                                if(price.priceType == "SALE_PRICE" || price.priceType == "DEFAULT")
                                {
                                    offer.Currency = price.currency;

                                    offer.OriginalPrice = price.price;
                                    offer.LocalizedOriginalPrice = Origin::Services::Publishing::ConversionUtil::FormatPrice(price.price, price.decimalPattern, price.orientation == ecommerce2::PRICEORIENTATIONENUM_LEFT ? "L" : "R", price.symbol, price.currency);

                                    if(!bDataSet)
                                    {
                                        offer.Price = price.price;
                                        offer.LocalizedPrice = offer.LocalizedOriginalPrice;
                                    }

                                    bDataSet = true;
                                }

                                if(price.priceType == "REDUCED_PRICE")
                                {
                                    offer.bIsDiscounted = true;

                                    offer.Currency = price.currency;
                                    offer.Price = price.price;

                                    offer.LocalizedPrice = Origin::Services::Publishing::ConversionUtil::FormatPrice(price.price, price.decimalPattern, price.orientation == ecommerce2::PRICEORIENTATIONENUM_LEFT ? "L" : "R", price.symbol, price.currency);

                                    if(!bDataSet)
                                    {
                                        offer.OriginalPrice = price.originalPrice;
                                        offer.LocalizedOriginalPrice = Origin::Services::Publishing::ConversionUtil::FormatPrice(price.originalPrice, price.decimalPattern, price.orientation == ecommerce2::PRICEORIENTATIONENUM_LEFT ? "L" : "R", price.symbol, price.currency);
                                    }

                                    bDataSet = true;
                                }
                            }
                        }
                    }
                }

                bUseFirst = true;
            }

            if(!bDataSet)
            {
                offer.Currency = "Unknown";
                offer.Price = -1;
                offer.LocalizedPrice = "Unknown";
                offer.OriginalPrice = -1;
                offer.LocalizedOriginalPrice = "Unknown";
                offer.bIsDiscounted = false;
            }
    
            offer.InventoryCap = -1;
            offer.InventorySold = -1;
            offer.InventoryAvailable = -1;
        }

        bool SDK_ECommerceServiceArea::ProcessEC2ProductsResponse( QDomDocument &doc, lsx::QueryOffersResponseT &qor, const QString country, bool returnUnpublishedContent)
        {
            LSXWrapper qreply(doc);

            ecommerce2::offersT offers;
            if(ecommerce2::Read(&qreply, offers))
            {
                for(unsigned int p=0; p<offers.offer.size();p++)
                {
                    lsx::OfferT offer;

                    if(offers.offer[p].publishing.publishingAttributes.isPublished || returnUnpublishedContent)
                    {
                        ConvertData(offer, offers.offer[p], country);

                        qor.Offers.push_back(offer);
                    }
                }

                return true;
            }
            else
            {
                return false;
            }
        }


        void SDK_ECommerceServiceArea::QueryOffers(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::QueryOffersT r;

            if(lsx::Read(request.document(), r))
            {
                // If no categories are specified, and no masterTitleId is requested, than add the master title of the connection.
                if(r.FilterCategories.size() == 0 && r.FilterMasterTitleIds.size() == 0 && request.connection()->entitlement()!=NULL)
                {
                    r.FilterMasterTitleIds.push_back(request.connection()->entitlement()->contentConfiguration()->masterTitleId());
                }

                QVariant userId = r.UserId;

                QueryOffersPayload payload;
                int * jobCount = new int();
                *jobCount = 0;

                payload.qor = new lsx::QueryOffersResponseT();
                payload.gatherWrapper = new ResponseWrapperEx<lsx::QueryOffersResponseT *, SDK_ECommerceServiceArea, int>(payload.qor, this, &SDK_ECommerceServiceArea::GatherResponse, jobCount, response);
                payload.country = GetCountry();
                payload.bReturnUnpublishedContent = returnUnpublishedContent(request);
                
                for(unsigned int i=0; i<r.FilterCategories.size(); i++)
                {
                    // Make a request to Ebisu Proxy
                    QUrl url(GetECommerceURL(request) + "/products");
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("expanded", "true");
                    urlQuery.addQueryItem("categoryId", r.FilterCategories[i]);
                    urlQuery.addQueryItem("userId", userId.toString());

                    if(r.FilterOffers.size()>0)
                    {
                        QString products;

                        for(unsigned int p=0; p<r.FilterOffers.size(); p++)
                        {
                            products += r.FilterOffers[p];
                            products += p+1<r.FilterOffers.size() ? "," : "";
                        }
                        urlQuery.addQueryItem("productIds", products);
                    }

                    url.setQuery(urlQuery);
                    QNetworkRequest serverRequest(url);
                    serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                    serverRequest.setRawHeader("X-Origin-SDK", "true");
                    serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                    serverRequest.setRawHeader("Accept", QString("application/xml").toUtf8());

                    DoServerCall(payload, serverRequest, OPERATION_GET, &SDK_ECommerceServiceArea::QueryOffersProcessResponse, response);
                    (*jobCount)++;
                }

                for(unsigned int i=0; i<r.FilterMasterTitleIds.size(); i++)
                {

                    // Make a request to Ebisu Proxy
                    QUrl url(GetECommerceURL(request) + "/products");
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("expanded", "true");
                    urlQuery.addQueryItem("masterTitleId", r.FilterMasterTitleIds[i]);
                    urlQuery.addQueryItem("userId", userId.toString());

                    if(r.FilterOffers.size()>0)
                    {
                        QString products;

                        for(unsigned int p=0; p<r.FilterOffers.size(); p++)
                        {
                            products += r.FilterOffers[p];
                            products += p+1<r.FilterOffers.size() ? "," : "";
                        }
                        urlQuery.addQueryItem("productIds", products);
                    }

                    url.setQuery(urlQuery);
                    QNetworkRequest serverRequest(url);
                    serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                    serverRequest.setRawHeader("X-Origin-SDK", "true");
                    serverRequest.setRawHeader("locale", GetLanguage().toLatin1());
                    serverRequest.setRawHeader("Accept", QString("application/xml").toUtf8());

                    DoServerCall(payload, serverRequest, OPERATION_GET, &SDK_ECommerceServiceArea::QueryOffersProcessResponse, response);
                    (*jobCount)++;
                }
            }
            else
            {
				CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        bool SDK_ECommerceServiceArea::QueryOffersProcessResponse(QueryOffersPayload &payload, QNetworkReply * serverReply, Lsx::Response & response)
        {
            if(serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                bool bErrorOccurred = false;
                QByteArray replyString = serverReply->readAll();

                // We received our message.
                QDomDocument doc;
                doc.setContent(replyString);

                QDomElement infoList = doc.documentElement();
                if(!infoList.isNull() && (infoList.tagName().compare("offers", Qt::CaseInsensitive) == 0))
                {
                    if(infoList.hasAttribute("schemaVersion") && infoList.attribute("schemaVersion") == "2")
                    {
                        bErrorOccurred = !ProcessEC2ProductsResponse(doc, *payload.qor, payload.country, payload.bReturnUnpublishedContent);
                    }
                    else
                    {
                        bErrorOccurred = !ProcessEC1ProductsResponse(doc, *payload.qor, payload.country);
                    }
                }
                else
                {
                    bErrorOccurred = !ProcessEC1ProductsResponse(doc, *payload.qor, payload.country);
                }

                if(bErrorOccurred)
                {
                    CreateErrorResponse(EBISU_ERROR_COMMERCE_INVALID_REPLY, "Processing the returned offers failed.", response);
                }
            }
            else
            {
                CreateServerErrorResponse(serverReply, response);
            }

            payload.gatherWrapper->finished();

            return true;
        }

        template <typename T> bool SDK_ECommerceServiceArea::GatherResponse(T *& data, int * jobsPending, Lsx::Response& response)
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

        void SDK_ECommerceServiceArea::QueryEntitlements(Lsx::Request& request, Lsx::Response& response )
        {
            m_EntitlementManager.QueryEntitlements(request, response);
        }

        bool SDK_ECommerceServiceArea::IsEntitlementInProductList(server::entitlementT &entitlement, std::vector<server::productT> &products)
        {
            for(unsigned int p=0; p<products.size(); p++)
            {
                server::productT &product = products[p];

                if(product.productType != "BUNDLE")
                {
                    QString groupName;
                    QString entitlementTag;

                    for(unsigned int a=0; a<product.attributes.size(); a++)
                    {
                        server::attributeT &attr = product.attributes[a];

                        if(attr.name == "groupName") 
                        {
                            groupName = attr.value;
                        }
                        else if(attr.name == "entitlementTag")
                        {
                            entitlementTag = attr.value;
                        }
                    }

                    if(entitlement.entitlementTag == entitlementTag && entitlement.groupName == groupName)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        void SDK_ECommerceServiceArea::QueryManifest(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::QueryManifestT r;

            if(lsx::Read(request.document(), r))
            {
                m_EntitlementManager.GetProductEntitlements(r.UserId, r.Manifest, response);
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't understand request", response);
            }
        }

        void SDK_ECommerceServiceArea::ConsumeEntitlement(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::ConsumeEntitlementT r;

            if(lsx::Read(request.document(), r))
            {
                // Make a request to Ebisu Proxy Server.

                QVariant userId = r.UserId;

                //[PUT] http://10.10.78.156:10093/ecommerce/entitlements/<userId>?entitlementTag=<entitlementTag>&groupName=<groupName>
                // &productId=<productId>&currentUseCount=<useCount>&consumeCount=<consumeCount>&overUse=<overuse>
        
                QUrl url(GetECommerceURL(request) + "/entitlements/" + userId.toString());
                QUrlQuery urlQuery(url);
                urlQuery.addQueryItem("expanded", "true");

                if(r.Entitlement.EntitlementTag.length()>0)
                {
                    urlQuery.addQueryItem("entitlementTag", r.Entitlement.EntitlementTag);
                }
                if(r.Entitlement.Group.length()>0)
                {
                    urlQuery.addQueryItem("groupName", r.Entitlement.Group);
                }
                if(r.Entitlement.ItemId.length()>0)
                {
                    urlQuery.addQueryItem("productId", r.Entitlement.ItemId);
                }
                if(r.Entitlement.UseCount>0)
                {
                    urlQuery.addQueryItem("currentUseCount", QVariant(r.Entitlement.UseCount).toString());
                }
                if(r.Uses>0)
                {
                    urlQuery.addQueryItem("consumeCount", QVariant(r.Uses).toString());
                }
                urlQuery.addQueryItem("overUse", r.bOveruse ? "true" : "false");
        
                url.setQuery(urlQuery);
                QNetworkRequest serverRequest(url);
                serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                serverRequest.setRawHeader("X-Origin-SDK", "true");
                serverRequest.setRawHeader("locale", GetLanguage().toLatin1());

                DoServerCall(serverRequest, OPERATION_PUT, &SDK_ECommerceServiceArea::ConsumeEntitlementProcessResults, response);
            }
            else
            {
				CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request.", response);
            }
        }

        bool SDK_ECommerceServiceArea::ConsumeEntitlementProcessResults(QNetworkReply * serverReply, Lsx::Response &response)
        {
            if(serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                lsx::ConsumeEntitlementResponseT cer;

                QByteArray replyString = serverReply->readAll();

                // We received our message.
                QDomDocument doc;
                doc.setContent(replyString);

                QDomElement node = doc.firstChildElement("entitlements");

                LSXWrapper qreply(doc);

                if(!node.isNull())
                {
                    std::vector<server::entitlementT> entitlements;
                    if(server::Read(&qreply, entitlements))
                    {
                        for(unsigned int e = 0; e < entitlements.size(); e++)
                        {
                            ConvertData(cer.Entitlement, entitlements[e]);

                            // Bail on the first element.
                            if(e == 0)
                                break;
                        }

                    }
                }
                else
                {
                    server::entitlementT entitlement;
                    if(server::Read(&qreply, entitlement))
                    {
                        ConvertData(cer.Entitlement, entitlement);
                    }
                }
                lsx::Write(response.document(), cer);
            }
            else
            {
                CreateServerErrorResponse(serverReply, response);
            }

            return true;
        }


        bool IsIGOEnabled(const Lsx::Request& request)
        {
            if(request.connection() && !request.connection()->entitlement().isNull())
            {
                return request.connection()->entitlement()->contentConfiguration()->isIGOEnabled();
            }
            return Origin::Engine::IGOController::isEnabled();
        }

        void SDK_ECommerceServiceArea::ShowIGOWindow(Lsx::Request& request, Lsx::Response& response )
        {
            Origin::Engine::Content::EntitlementRef entitlement = request.connection()->entitlement();
            QString contentId = !entitlement.isNull() ? entitlement->contentConfiguration()->contentId() : request.connection()->productId();

            //Is IGO enabled and available (The igo is available when it is connected, and the screen is large enough)?
            if (IsIGOEnabled(request) &&
                Origin::Engine::IGOController::instance()->isConnected() &&
                Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough())
            {
                // Check if user is under age.
                if ( isUserUnderAge() )
                {
                    // Show under age error.
                    Origin::Engine::IGOController::instance()->EbisuShowErrorUI("ebisu_underage_sdk_show_store_error_title", "ebisu_underage_sdk_show_store_error_text");

                    CreateErrorResponse(EBISU_ERROR_COMMERCE_UNDERAGE_USER, "", response);
                }
                else /* Non underage user */
                {
                    lsx::ShowIGOWindowT r;

                    if(lsx::Read(request.document(), r))
                    {
                        if(r.WindowId == lsx::IGOWINDOW_CHECKOUT)
                        {
                            if(r.Offers.size() >= 1)
                            {
                                lsx::GetLockboxUrlResponseT res;

                                QStringList offers;

                                for(unsigned int i = 0; i < r.Offers.size(); i++)
                                {
                                    offers.append(r.Offers[i]);
                                }

                                GetLockboxUrlEx(request, request.connection()->commerceProfile(), QVariant(r.UserId).toString(), offers, "IGO", contentId, true, res, response);

                                if(!res.Url.isNull() && !res.Url.isEmpty())
                                    Origin::Engine::IGOController::instance()->EbisuShowCheckoutUI(res.Url.toLatin1());
                            }
                        }
                        else if(r.WindowId == lsx::IGOWINDOW_STORE)
                        {
                            QStringList offers;
                            QStringList categories;
                            QStringList masterTitleIds;

                            for(unsigned int i=0; i<r.Offers.size(); i++)
                            {
                                offers.append(r.Offers[i]);                            
                            }

                            for(unsigned int i=0; i<r.Categories.size(); i++)
                            {
                                categories.append(r.Categories[i]);                            
                            }

                            for(unsigned int i=0; i<r.MasterTitleIds.size(); i++)
                            {
                                masterTitleIds.append(r.MasterTitleIds[i]);                            
                            }

                            Origin::Engine::Content::EntitlementRef entitlement = request.connection()->entitlement();

                            if(!entitlement.isNull() && entitlement->contentConfiguration()->useLegacyCatalog())
                            {
                                // We are using EC1 or the title requires the legacy catalog.
                                if(categories.size())
                                {
                                    Origin::Engine::IGOController::instance()->EbisuShowStoreUIByCategoryIds(QString::number(r.UserId), contentId, categories.join(","), offers.join(","));
                                }
                                else
                                {
                                    CreateErrorResponse(EBISU_ERROR_COMMERCE_NO_CATEGORIES, "No categories provided for opening a store.", response);
                                    return;
                                }
                            }
                            else
                            {
                                // Game specified a masterTitleId
                                if(masterTitleIds.size()>0)
                                {
                                    Origin::Engine::IGOController::instance()->EbisuShowStoreUIByMasterTitleId(QString::number(r.UserId), contentId, masterTitleIds.join(","), offers.join(","));
                                }
                                // We are running against EC2 or newer
                                else if(!entitlement.isNull() && !entitlement->contentConfiguration()->masterTitleId().isEmpty())
                                {
                                    Origin::Engine::IGOController::instance()->EbisuShowStoreUIByMasterTitleId(QString::number(r.UserId), contentId, entitlement->contentConfiguration()->masterTitleId(), offers.join(","));
                                }
                                else
                                {
                                    if(categories.size()>0)
                                    {
                                        // The game specifies categories. For now redirect to EC1
                                        Origin::Engine::IGOController::instance()->EbisuShowStoreUIByCategoryIds(QString::number(r.UserId), contentId, categories.join(","), offers.join(","));
                                    }   
                                    else
                                    {
                                        if(!entitlement.isNull())
                                        {
                                            // Use the configured master title id to show the addon store.
                                            Origin::Engine::IGOController::instance()->EbisuShowStoreUIByMasterTitleId(QString::number(r.UserId), contentId, entitlement->contentConfiguration()->masterTitleId(), offers.join(","));
                                        }
                                        else
                                        {
                                            CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "No categories/masterTitleId is provided for opening a store and the SDK connection is not associated with an entitlement to get a default store.", response);
                                            return;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                CreateErrorResponse(EBISU_SUCCESS, "", response);
            }
            else
            {
                int code = EBISU_ERROR_IGO_NOT_AVAILABLE;
                QString description;
                if(Origin::Engine::IGOController::instantiated() != NULL)
                {
                    if(!Origin::Engine::IGOController::instance()->isConnected())
                    {
                        description = "The IGO is not connected.";
                    }
                    else
                    {
                        if(!Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough())
                        {
                                    description = "The IGO doesn't fit on the screen.";
                        }
                        else
                        {
                                    description = "The IGO is not available for undetermined reason.";
                        }
                    }
                }
                else
                {
                    description = "No IGO instance initialized.";
                }
                CreateErrorResponse(code, description, response);
            }
        }

        QString SDK_ECommerceServiceArea::GetProfile(QString contentId, QString multiplayerId)
        {
            typedef Origin::Engine::Content::EntitlementRefList list_t;
            list_t const& content = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
            list_t::const_iterator match = content.end();
            list_t::const_iterator bestmatch = content.end();
            for ( list_t::const_iterator i = content.begin(); i != content.end(); ++i )
            {
                if ( (*i)->contentConfiguration()->multiplayerId() == multiplayerId )
                {
                    match = i;
                }
                if ( (*i)->contentConfiguration()->contentId() == contentId )
                {
                    bestmatch = i;
                    break;
                }
            }

            QString ret;

            if(bestmatch != content.end())
                ret = (*bestmatch)->contentConfiguration()->commerceProfile();

            if (ret == "" && match != content.end())
                ret = (*match)->contentConfiguration()->commerceProfile();

            if (ret == "")
                ret = "unknown";

            return ret;
        }

        void SDK_ECommerceServiceArea::GetLockboxUrl(Lsx::Request& request, Lsx::Response& response)
        {
            QString contentId = request.connection()->entitlement() ? request.connection()->entitlement()->contentConfiguration()->contentId() : request.connection()->productId();

            lsx::GetLockboxUrlT r;

            if(lsx::Read(request.document(),  r))
            {
                lsx::GetLockboxUrlResponseT res;

                QString nucleusId = Origin::Services::Session::SessionService::nucleusUser(Engine::LoginController::instance()->currentUser()->getSession());
        
                QString commerceProfile;
                QString contentId;
        
                if(contentId.isEmpty())
                {
                    commerceProfile = r.CommerceProfile;
                    contentId = r.ContentId;
                }
                else
                {
                    commerceProfile = request.connection()->commerceProfile();
                }
        
                QStringList offers;
                offers.append(r.ProductId);

                GetLockboxUrlEx(request, commerceProfile, nucleusId, offers, r.Context, contentId, r.IsSDK, res, response);

                if(res.Url.length()>0)
                {
                    lsx::Write(response.document(), res);
                }
            }
        }

        void SDK_ECommerceServiceArea::GetLockboxUrlEx(Lsx::Request &request, QString commerceProfile, QString userId, QStringList offers, QString context, QString contentId, bool isSdk, lsx::GetLockboxUrlResponseT& urlResponse, Lsx::Response& response)
        {
            if(!useLegacyCatalog(request))
            {
                urlResponse.Url = Services::readSetting(Services::SETTING_EbisuLockboxV3URL).toString();
                urlResponse.Url.replace("{profile}", commerceProfile);
                
                // The offer/offers portion is a base64 encoded JSON of offer ID's and quantities.  For example, {"OFB-FIFA:39330":1}, or {"Origin.OFR.50.0000404":1,"Origin.OFR.50.00000403":1}.

                QStringList offersToBuy;

                for(int i = 0; i < offers.size(); i++)
                {
                    offersToBuy.append(QString("\"%1\":1").arg(offers[i]));
                }

                urlResponse.Url.replace("{offers}", QString("{%1}").arg(offersToBuy.join(",")).toLatin1().toBase64());

                m_ProductToBuy = offers.join(",");

                return;
            }

            if(offers.size()>0)
            {
                // Using old checkout flow, querying ec2 for lockbox url.

                QUrl url(GetECommerceURL(request) + "/checkout");
                QUrlQuery urlQuery(url);

                if(!m_LockboxSucceededURL.isEmpty())
                    urlQuery.addQueryItem("successUrl", QUrl::toPercentEncoding(m_LockboxSucceededURL));

                if(!m_LockboxFailedURL.isEmpty())
                    urlQuery.addQueryItem("failedUrl", QUrl::toPercentEncoding(m_LockboxFailedURL));

                urlQuery.addQueryItem("productId", QUrl::toPercentEncoding(offers[0])); // Only take the first offer.
                urlQuery.addQueryItem("userId", userId);
                urlQuery.addQueryItem("region", GetCountry());
                urlQuery.addQueryItem("lang", GetLanguage());
                urlQuery.addQueryItem("profile", commerceProfile);
                urlQuery.addQueryItem("sourceType", "IGOCheckout");
                urlQuery.addQueryItem("context", context);
                urlQuery.addQueryItem("sku", contentId);

                if(ForceLockboxURL() && !m_LockboxURL.isEmpty())
                {
                    urlQuery.addQueryItem("lockboxUrl", QUrl::toPercentEncoding(m_LockboxURL));
                    urlQuery.addQueryItem("overrideLockboxUrl", "true");
                }

                // Remember the product that you are going to buy. NOTE: This only works for non concurrent purchases.
                m_ProductToBuy = offers[0]; // Remember the first offer.

                QString reply;

                if(m_EWalletCategoryId.toLongLong() != 0)
                    urlQuery.addQueryItem("ewalletcategoryId", m_EWalletCategoryId.toString());

                if(!m_VirtualCurrency.isEmpty())
                    urlQuery.addQueryItem("currency", m_VirtualCurrency);

                url.setQuery(urlQuery);

                QNetworkRequest serverRequest(url);
                serverRequest.setRawHeader("AuthToken", GetAccessToken().toLatin1());
                serverRequest.setRawHeader("locale", GetLanguage().toLatin1());

                if(isSdk)
                    serverRequest.setRawHeader("X-Origin-SDK", "true");

                QNetworkReply *serverReply;
                if(DoServerCall(serverRequest, OPERATION_GET, serverReply) == 302)
                {
                    urlResponse.Url = serverReply->header(QNetworkRequest::LocationHeader).toString();
                }
                else
                {
                    CreateServerErrorResponse(serverReply, response);
                }

                delete serverReply;
            }
        }


        int SDK_ECommerceServiceArea::GetServerErrorCodeAndDescription(QNetworkReply * pReply, QString &description)
        {
            int status = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString error = pReply->readAll();
            QString errorString = pReply->errorString();

            description = QString("Request: ") + pReply->url().toString() + "\nInvalid Response Code = " + QVariant(status).toString() + "\n" + error + "\n" + errorString;

            ORIGIN_LOG_ERROR << description;

            return status;
        }

        void SDK_ECommerceServiceArea::CreateServerErrorResponse(QNetworkReply * pReply, Lsx::Response& response)
        {   
            QString description;
            int code = GetServerErrorCodeAndDescription(pReply, description);

            CreateErrorResponse((int)(EBISU_ERROR_PROXY + code), description.toUtf8(), response);
        }

        void SDK_ECommerceServiceArea::CreateErrorResponse(int code, const QString &description, Lsx::Response& response)
        {
            lsx::ErrorSuccessT e;
            e.Code = code;
            e.Description = description;
            lsx::Write(response.document(), e);
        }


        int SDK_ECommerceServiceArea::DoServerCall(QNetworkRequest &serverRequest, NetworkMethod method,  QNetworkReply *&serverReply, QDomDocument *pDoc)
        {
            // Wait for the reply to come back.
            QEventLoop eventLoop;

            Origin::Services::NetworkAccessManager *pNetworkManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

            switch(method)
            {
            case OPERATION_GET:
                serverReply = pNetworkManager->get(serverRequest);
                break;
            case OPERATION_PUT:
                if(pDoc)
                {
                    serverReply = pNetworkManager->put(serverRequest, pDoc->toByteArray());
                }
                else
                {
                    serverReply = pNetworkManager->put(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_POST:
                if(pDoc)
                {
                    serverReply = pNetworkManager->post(serverRequest, pDoc->toByteArray());
                }
                else
                {
                    serverReply = pNetworkManager->post(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_DELETE:
                serverReply = pNetworkManager->deleteResource(serverRequest);
                break;
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Invalid Network Method");
                break;
            }

            connect(serverReply, SIGNAL(finished()), &eventLoop, SLOT(quit()));

            eventLoop.exec();

            return serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }

        template <typename T> void SDK_ECommerceServiceArea::DoServerCall(T & data, QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ECommerceServiceArea::*callback)(T &, QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &doc)
        {
            Origin::Services::NetworkAccessManager *pNetworkManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

            QNetworkReply * serverReply = NULL;

            switch(method)
            {
            case OPERATION_GET:
                serverReply = pNetworkManager->get(serverRequest);
                break;
            case OPERATION_PUT:
                if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->put(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->put(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_POST:
                if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->post(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->post(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_DELETE:
                serverReply = pNetworkManager->deleteResource(serverRequest);
                break;
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Invalid Network Method");
                break;
            }

            ResponseWrapperEx<T, SDK_ECommerceServiceArea, QNetworkReply> * resp = 
                new ResponseWrapperEx<T, SDK_ECommerceServiceArea, QNetworkReply>
                    (data, this, callback, serverReply, response);

            ORIGIN_VERIFY_CONNECT(serverReply, SIGNAL(finished()), resp, SLOT(finished()));
        }

        void SDK_ECommerceServiceArea::DoServerCall(QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ECommerceServiceArea::*callback)(QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &doc)
        {
            Origin::Services::NetworkAccessManager *pNetworkManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

            QNetworkReply * serverReply = NULL;

            switch(method)
            {
            case OPERATION_GET:
                serverReply = pNetworkManager->get(serverRequest);
                break;
            case OPERATION_PUT:
                if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->put(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->put(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_POST:
                if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->post(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->post(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_DELETE:
                serverReply = pNetworkManager->deleteResource(serverRequest);
                break;
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Invalid Network Method");
                break;
            }

            ResponseWrapper<SDK_ECommerceServiceArea, QNetworkReply> * resp = 
                new ResponseWrapper<SDK_ECommerceServiceArea, QNetworkReply>
                    (this, callback, serverReply, response);

            ORIGIN_VERIFY_CONNECT(serverReply, SIGNAL(finished()), resp, SLOT(finished()));
        }
    }
}



