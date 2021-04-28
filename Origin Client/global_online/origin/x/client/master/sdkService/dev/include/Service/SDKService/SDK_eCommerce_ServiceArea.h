///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SDK_ECommerceServiceArea.h
//	SDK Service Area
//	
//	Author: Hans van Veenendaal

#ifndef _SDK_ECOMMERCE_SERVICE_AREA_
#define _SDK_ECOMMERCE_SERVICE_AREA_

#include "Service/BaseService.h"
#include "Service/SDKService/SDK_EntitlementManager.h"

#include <string>
#include "QVariant.h"
#include "services/network/NetworkAccessManager.h"
#include "services/settings/Setting.h"
#include "EbisuError.h"

#include "server.h"
#include "lsx.h"

#include "engine/login/LoginController.h"

class LSXWrapper;

namespace Origin
{
    namespace SDK
    {
        class SDK_ECommerceServiceArea : public BaseService
        {
            Q_OBJECT
        public:
            // Singleton functions
            static SDK_ECommerceServiceArea* instance();
            static SDK_ECommerceServiceArea* create(Lsx::LSX_Handler* handler);
            void destroy();
            
            static bool getAutostart();

            // Methods for pdlc helper
            Q_INVOKABLE void selectStore(QString lockboxURL);
            Q_INVOKABLE void purchaseCompleted();
            Q_INVOKABLE lsx::GetLockboxUrlResponseT getLockboxUrl(QString commerceProfile, QString userId, QString productId, QString context, QString contentId, bool isSdk);
            
            bool useLegacyCatalog(Lsx::Request& request);
            bool useLegacyCatalog(const QString& contentId);


        public slots:
            void sslErrors ( QNetworkReply * reply, const QList<QSslError> & errors );

        private slots:
            void onEntitlementGranted(Origin::Services::Publishing::NucleusEntitlementServiceResponse* resp);

        private:
            // block constructors
            SDK_ECommerceServiceArea( Lsx::LSX_Handler* handler );

            //	Handlers

            void SelectStore(Lsx::Request& request, Lsx::Response& response );
            void GetStore(Lsx::Request& request, Lsx::Response& response );
            void GetCatalog(Lsx::Request& request, Lsx::Response& response );
            void GetWalletBalance(Lsx::Request& request, Lsx::Response& response );
            bool GetWalletBalanceProcessResponse(QNetworkReply *serverReply, Lsx::Response &response);
            void QueryCategories(Lsx::Request& request, Lsx::Response& response );

            struct QueryOffersPayload
            {
                QString country;
                bool bReturnUnpublishedContent;
                lsx::QueryOffersResponseT * qor;
                ResponseWrapperEx<lsx::QueryOffersResponseT *, SDK_ECommerceServiceArea, int> * gatherWrapper;
            };

            void QueryOffers(Lsx::Request& request, Lsx::Response& response );
            bool QueryOffersProcessResponse(QueryOffersPayload &payload, QNetworkReply * serverReply, Lsx::Response & response);
            template <typename T> bool GatherResponse(T *& data, int * jobsPending, Lsx::Response & response);

            QString GetECommerceURL(bool useLegacy);
            QString GetECommerceURL(Lsx::Request& request);
            QString GetECommerceURL(const QString& contentId, bool isSdk);

            bool returnUnpublishedContent(Lsx::Request& request);

            bool ProcessEC1ProductsResponse( QDomDocument &doc, lsx::QueryOffersResponseT &qor, const QString country);
            bool ProcessEC2ProductsResponse( QDomDocument &doc, lsx::QueryOffersResponseT &qor, const QString country, bool returnUnpublishedContent);

            void QueryEntitlements(Lsx::Request& request, Lsx::Response& response);

            void QueryManifest(Lsx::Request& request, Lsx::Response& response );
            void ConsumeEntitlement(Lsx::Request& request, Lsx::Response& response );
            bool ConsumeEntitlementProcessResults(QNetworkReply * serverReply, Lsx::Response &response);

            void ShowIGOWindow(Lsx::Request& request, Lsx::Response& response );
            void GetLockboxUrl(Lsx::Request& request, Lsx::Response& response );

            void GetLockboxUrlEx(Lsx::Request &request, QString commerceProfile, QString userId, QStringList ProductIds, QString context, QString contentId, bool isSdk, lsx::GetLockboxUrlResponseT& urlResponse, Lsx::Response& response);
            QString GetProfile(QString contentId, QString multiplayerId);

            void DirectCheckout(Lsx::Request& request, Lsx::Response& response );
            bool DirectCheckoutProcessResponse(QNetworkReply *serverReply, Lsx::Response &response);

            bool IsEntitlementInProductList(server::entitlementT &entitlement, std::vector<server::productT> &products);

            void GetProductEntitlements(uint64_t userId, QString productId, Lsx::Response &response);

            enum NetworkMethod
            {
                OPERATION_GET,
                OPERATION_PUT,
                OPERATION_POST,
                OPERATION_DELETE,
            };

            
            int DoServerCall(QNetworkRequest &serverRequest, NetworkMethod method,  QNetworkReply *&serverReply, class QDomDocument *doc = NULL);
            template <typename T> void DoServerCall(T & data, QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ECommerceServiceArea::*callback)(T &, QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &doc = QByteArray());
            void DoServerCall(QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ECommerceServiceArea::*callback)(QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &doc = QByteArray());

            int GetServerErrorCodeAndDescription(QNetworkReply * pReply, QString &description);
            void CreateErrorResponse(int code, const QString & description, Lsx::Response& response);
            void CreateServerErrorResponse(QNetworkReply * pReply, Lsx::Response& response);

            QVariant m_StoreId;
            QVariant m_CatalogId;
            QVariant m_EWalletCategoryId;
            QString m_VirtualCurrency;
            QString m_LockboxURL;
            QString m_LockboxSucceededURL;
            QString m_LockboxFailedURL;
            QString m_ProductToBuy;
            //QString GetAuthToken();
            QString GetAccessToken();
            QString GetCountry();
            QString GetLanguage();

            SDK_EntitlementManager m_EntitlementManager;

            QList<QByteArray> m_entitlementGrantList;
            bool m_processingEntitlements;

            bool isUserUnderAge() const;
        };
    }
}

#endif
