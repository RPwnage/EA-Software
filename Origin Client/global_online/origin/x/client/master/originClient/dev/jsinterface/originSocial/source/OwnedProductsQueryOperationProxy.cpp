#include "OwnedProductsQueryOperationProxy.h"

#include <QMetaObject>

#include "services/rest/AtomServiceClient.h"
#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/ProductArt.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            using namespace Origin::Engine::Content::ProductArt;
            using Engine::UserRef;
            using Engine::Content::EntitlementRef;

            OwnedProductsQueryOperationProxy::OwnedProductsQueryOperationProxy(QObject *parent, quint64 nucleusId) :
                QueryOperationProxyBase(parent)
                ,mNucleusID(nucleusId)
            {
            }

            void OwnedProductsQueryOperationProxy::getUserProducts()
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                if (!user)
                {
                    // Need a user
                    QMetaObject::invokeMethod(this, "failed", Qt::QueuedConnection);
                    return;
                }

                if (static_cast<quint64>(user->userId()) == mNucleusID)
                {
                    if (user->contentControllerInstance()->initialFullEntitlementRefreshed())
                    {
                        // Send our entitlements once JS can connect its signals
                        QMetaObject::invokeMethod(this, "sendCurrentUserEntitlements", Qt::QueuedConnection);
                    }
                    else
                    {
                        // Wait for our entitlements to load
                        ORIGIN_VERIFY_CONNECT(
                            user->contentControllerInstance(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)),
                            this, SLOT(sendCurrentUserEntitlements()));
                    }
                }
                else
                {
                    Services::UserGamesResponseBase* response = Services::AtomServiceClient::userGames(user->getSession(), mNucleusID);

                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(success()), this, SLOT(onUserGamesSuccess()));
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(failQuery()));
                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onQueryError(Origin::Services::HttpStatusCode)));
                }
            }

            void OwnedProductsQueryOperationProxy::sendCurrentUserEntitlements()
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                // We should only emit this once
                // Be careful because not all of our paths will connect this - ORIGIN_VERIFY_DISCONNECT will sporadically assert
                disconnect(
                    user->contentControllerInstance(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)),
                    this, SLOT(sendCurrentUserEntitlements()));

                // This is for the current user - super easy
                QVariantList products;

                QList<EntitlementRef> entitlements = user->contentControllerInstance()->entitlements();

                for(QList<EntitlementRef>::ConstIterator it = entitlements.constBegin();
                    it != entitlements.constEnd();
                    it++)
                {
                    const EntitlementRef &entRef = *it;
                    const Engine::Content::ContentConfigurationRef contentConf = entRef->contentConfiguration();

                    QVariantMap productInfo;

                    productInfo["productId"] = contentConf->productId();
                    productInfo["title"] = contentConf->displayName();

                    productInfo["bannerUrls"] = urlsForType(contentConf, OwnedGameDetailsBanner);
                    productInfo["boxartUrls"] = urlsForType(contentConf, HighResBoxArt) + urlsForType(contentConf, LowResBoxArt);
                    productInfo["thumbnailUrls"] = urlsForType(contentConf, ThumbnailBoxArt);

                    //TODO (Hans): Make sure that when we are supporting more than two platforms this code gets modified to take that into account.
                    QString achievementSetId = contentConf->achievementSet();
                    if (achievementSetId.isEmpty())
                    {
                        achievementSetId = entRef->contentConfiguration()->achievementSet(Origin::Services::PlatformService::PlatformWindows);
                    }

                    if (!achievementSetId.isEmpty())
                    {
                        productInfo["achievementSetId"] = achievementSetId;
                    }
                    else
                    {
                        productInfo["achievementSetId"] = QVariant();
                    }

                    products.append(productInfo);
                }

                emit succeeded(products);
                deleteLater();
            }

            void OwnedProductsQueryOperationProxy::onUserGamesSuccess()
            {
                Services::UserGamesResponseBase* resp = dynamic_cast<Services::UserGamesResponseBase*>(sender());

                if (!resp)
                {
                    ORIGIN_ASSERT(0);
                    deleteLater();
                    return;
                }

                QList<Services::UserGameData> games = resp->userGames();
                createPayload(games);
                resp->deleteLater();
                deleteLater();
            }

            void OwnedProductsQueryOperationProxy::createPayload(const QList<Services::UserGameData> &games )
            {
                QVariantList products;
                for(QList<Services::UserGameData>::ConstIterator it = games.constBegin();
                    it != games.constEnd();
                    it++)
                {
                    QVariantMap productInfo;

                    productInfo["productId"] = it->productId;
                    productInfo["title"] = it->displayProductName;

                    productInfo["bannerUrls"] = urlsForType(*it, OwnedGameDetailsBanner); // background
                    productInfo["boxartUrls"] = urlsForType(*it, HighResBoxArt) + urlsForType(*it, LowResBoxArt); // boxart
                    productInfo["thumbnailUrls"] = urlsForType(*it, ThumbnailBoxArt);

                    if (!it->achievementSetId.isNull())
                    {
                        productInfo["achievementSetId"] = it->achievementSetId;
                    }
                    else
                    {
                        productInfo["achievementSetId"] = QString();
                    }

                    products.append(productInfo);
                }

                emit succeeded(products);
            }

        }
    }
}
