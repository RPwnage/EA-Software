#include "GamesManagerProxy.h"
#include "GameProxy.h"

#include "services/debug/DebugService.h"

#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/ContentController.h"
#include "engine/login/LoginController.h"

#include "EntitlementInstallFlowProxy.h"
#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"

#include "services/log/LogService.h"

using namespace Origin::Engine::Content;
using namespace Origin::Services;
namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            GamesManagerProxy::GamesManagerProxy(QObject *parent)
                :QObject(parent)
            {

                Origin::Engine::Content::ContentOperationQueueController *cQController = Origin::Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
                if (cQController)
                {
                    //not enough to listen for just updateFinished from ContentController, we need to wait for queue to be initialized
                    ORIGIN_VERIFY_CONNECT_EX (cQController, SIGNAL(initialized()), 
                                              this, 
                                              SLOT(onDownloadQueueInitialized()),
                                              Qt::QueuedConnection);

                    //for now all we want to know is that the queue changed so have them all go to the same slot
                    ORIGIN_VERIFY_CONNECT_EX(cQController, 
                                             SIGNAL(enqueued(Origin::Engine::Content::EntitlementRef)), 
                                             this, 
                                             SLOT (onDownloadQueueChangedEnqueued(Origin::Engine::Content::EntitlementRef)),
                                             Qt::QueuedConnection);

                    ORIGIN_VERIFY_CONNECT_EX(cQController, 
                                             SIGNAL(removed(const QString&, const bool&, const bool&)), 
                                             this, 
                                             SLOT (onDownloadQueueChangedRemoved(const QString&, const bool&, const bool&)),
                                             Qt::QueuedConnection);

                    ORIGIN_VERIFY_CONNECT_EX(cQController, 
                                             SIGNAL(headChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), 
                                             this, 
                                            SLOT (onDownloadQueueChangedHeadChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)),
                                            Qt::QueuedConnection);

                    ORIGIN_VERIFY_CONNECT_EX(cQController,
                                             SIGNAL(headBusy(const bool&)),
                                             this,
                                             SLOT(onDownloadQueueChangedHeadBusy(const bool&)),
                                             Qt::QueuedConnection);
                }

                // Connect the NOG controller.
                NonOriginContentController *nogController = ContentController::currentUserContentController()->nonOriginController();
                ORIGIN_VERIFY_CONNECT(nogController, SIGNAL(nonOriginGameAdded(Origin::Engine::Content::EntitlementRef)),
                                      this, SLOT(onNogUpdated()));
                ORIGIN_VERIFY_CONNECT(nogController, SIGNAL(nonOriginGameRemoved(Origin::Engine::Content::EntitlementRef)),
                                      this, SLOT(onNogUpdated()));
                ORIGIN_VERIFY_CONNECT(nogController, SIGNAL(nonOriginGameModified(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)),
                                      this, SLOT(onNogUpdated()));

            }
            
            void GamesManagerProxy::onNogUpdated()
            {
                emit basegamesupdated(QVariantMap());
            }
            
            QVariantList GamesManagerProxy::getNonOriginGames()
            {
                // Prepare to loop over all NOGs.
                QVariantList nogs;
                NonOriginContentController *nogController = ContentController::currentUserContentController()->nonOriginController();
                
                // For each NOG,
                foreach (const EntitlementRef &e, nogController->nonOriginGames())
                {
                    QVariantMap entitlement, catalog, i18n, platforms, platform, combined;
                    
                    // Create entitlement data.
                    entitlement["offerId"] = e->contentConfiguration()->productId();
                    entitlement["contentId"] = e->contentConfiguration()->contentId();
                    entitlement["masterTitleId"] = e->contentConfiguration()->masterTitle();
                    entitlement["multiplayerId"] = e->contentConfiguration()->multiplayerId();
                    entitlement["originPermissions"] = e->contentConfiguration()->originPermissions();
            
                    // Create Catalog Data
                    i18n["displayName"] = e->contentConfiguration()->displayName();
                    catalog["offerId"] =e->contentConfiguration()->productId();
                    catalog["i18n"] = i18n;
                    
                    // Create Platform Data
                    platforms[Origin::Services::PlatformService::catalogStringFromSupportedPlatfom()] = platform;
                    catalog["platforms"] = platforms;
                    
                    combined["entitlement"] = entitlement;
                    combined["catalog"] = catalog;
                    nogs.append(combined);
                }
                
                return nogs;
            }

            void GamesManagerProxy::onGameProxyChanged(QJsonObject data)
            {
                QJsonArray dataArray;
                dataArray.append(data);
                emit changed(dataArray.toVariantList());
            }

            void GamesManagerProxy::onGameProxyProgressChanged(QJsonObject data)
            {
                QJsonArray dataArray;
                dataArray.append(data);
                emit progressChanged(dataArray.toVariantList());
            }

            void GamesManagerProxy::onGameProxyOperationFailed(QJsonObject data)
            {
                QJsonArray dataArray;
                dataArray.append(data);
                emit operationFailed(dataArray.toVariantList());
            }

            GamesManagerProxy::~GamesManagerProxy()
            {
            }

            bool GamesManagerProxy::initialRefreshCompleted()
            {
                ContentController *cc = ContentController::currentUserContentController();
                return cc ? cc->initialEntitlementRefreshCompleted() : false;
            }

            bool GamesManagerProxy::isGamePlaying()
            {
                //check and see if we've finished processing all the entitlements; if not, the nreturn an empty array
                Origin::Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
                if (contentController)
                {
                    if (contentController->initialEntitlementRefreshCompleted())
                    {
                        //MF TODO: remove once QA has verified
                        ORIGIN_LOG_EVENT << "isGamePlaying: initial entitlement refresh completed";
                        foreach(Engine::Content::EntitlementRef ref, Engine::Content::ContentController::currentUserContentController()->entitlements())
                        {
                            if (ref->localContent()->playing())
                            {
                                return true;
                            }
                        }
                    }
                    else
                    {
                        //MF TODO: remove once QA has verified
                        ORIGIN_LOG_EVENT << "isGamePlaying: initial entitlement refresh NOT completed";
                    }
                }

                return false;
            }

            QVariant GamesManagerProxy::games(const QString &productId)
            {
                Engine::Content::EntitlementRef ent = ContentController::currentUserContentController()->entitlementByProductId(productId);
                if(ent)
                {
                    return QVariant::fromValue<QObject*>(gameProxyForEntitlement(ent));
                }
                return QVariant();
            }

            QVariantList GamesManagerProxy::requestGamesStatus()
            {
                QJsonArray array;

                //check and see if we've finished processing all the entitlements; if not, the nreturn an empty array
                Origin::Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
                if (contentController) 
                {
                    if (contentController->initialEntitlementRefreshCompleted())
                    {
                        //MY TODO: remove once QA has verified
                        ORIGIN_LOG_EVENT << "RequestGameStatus: initial entitlement refresh completed";
                        foreach (Engine::Content::EntitlementRef ref, Engine::Content::ContentController::currentUserContentController()->entitlements())
                        {
                            array.append(QJsonValue(gameProxyForEntitlement(ref)->createStateObj()));
                        }

                    } 
                    //MY TODO: remove once QA has verified
                    else 
                    {
                        ORIGIN_LOG_EVENT << "RequestGameStatus: initial entitlement refresh NOT completed";
                    }
                }
                return array.toVariantList();
            }

            GameProxy* GamesManagerProxy::gameProxyForEntitlement(Origin::Engine::Content::EntitlementRef e)
            {
                GameProxy *proxy = mGameProxies[e.data()];

                if (!proxy)
                {
                    proxy = new GameProxy(e, this);
                    mGameProxies[e.data()] = proxy;
                }

                return proxy;
            }

            QVariantMap GamesManagerProxy::onRemoteGameAction(QString productId, QString action, QVariantMap actionParams)
            {
                QVariantMap returnArgs;

                ORIGIN_LOG_EVENT << "Action requested: " << action << ", prodId =" << productId;
      
                Origin::Engine::Content::EntitlementRef entRef = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
                GameProxy *gameProxy = gameProxyForEntitlement(entRef);

                if(gameProxy)
                {
                    bool success = QMetaObject::invokeMethod(gameProxy, action.toLocal8Bit(), Qt::DirectConnection, Q_RETURN_ARG(QVariantMap, returnArgs),  Q_ARG(QVariantMap, actionParams));
                    if(gameProxy->metaObject() && gameProxy->metaObject()->className())
                    {
                        ORIGIN_ASSERT_MESSAGE(success, (QString(gameProxy->metaObject()->className()) +"::" + action + " could not be invoked.\nMake sure the function name, parameters and return args match.\n").toLocal8Bit());
                    }
                }
                else
                {
                    ORIGIN_ASSERT_MESSAGE(false, "Cannot find game proxy");
                }

                return returnArgs;
            }

            void GamesManagerProxy::onDownloadQueueInitialized()
            {
                //we're here when we know that it's ok for the SPA to request status of all games; the info we send back will be valid
                //for now, we just want to know when the entitlement refresh happens
                QJsonObject obj;

                //MY TODO: remove once QA has verified
                ORIGIN_LOG_EVENT << "onDownloadQueueInitialized, firing basegamesupdated";                
                emit basegamesupdated(obj.toVariantMap());
            }

            void GamesManagerProxy::onDownloadQueueChanged()
            {
                //pass back the updated queue information
                QJsonArray array;

                //MY TODO: remove once QA has verified
                ORIGIN_LOG_EVENT << "onDownloadQueueChanged";

                Origin::Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
                if (contentController) 
                {
                    foreach (Engine::Content::EntitlementRef ref, Engine::Content::ContentController::currentUserContentController()->entitlements())
                    {
                        array.append(QJsonValue(gameProxyForEntitlement(ref)->createDownloadQueueObj()));
                    }
                }

                emit downloadQueueChanged(array.toVariantList());
            }

            void GamesManagerProxy::onDownloadQueueChangedEnqueued(Origin::Engine::Content::EntitlementRef entRef)
            {
                ORIGIN_LOG_EVENT << "onDownloadQueueChangedEnqueued: " << entRef->contentConfiguration()->productId();
                onDownloadQueueChanged();
            }

            void GamesManagerProxy::onDownloadQueueChangedRemoved(const QString& productId, const bool& removeChildren, const bool& enqueueNext = true)
            {
                ORIGIN_LOG_EVENT << "onDownloadQueueChangedRemoved: " << productId << " removeChildren: " << removeChildren << " enqueueNext: " << enqueueNext;
                onDownloadQueueChanged();
            }

            void GamesManagerProxy::onDownloadQueueChangedHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead)
            {
                //newHead may be null because nothing in the Queue, onDownloadQueueChanged would be triggered by removed
                if (newHead)
                {
                    ORIGIN_LOG_EVENT << "onDownloadQueueChagedHeadChanged: new head" << newHead->contentConfiguration()->productId();
                }

                if (oldHead) 
                { 
                    ORIGIN_LOG_EVENT << ", old head = " << oldHead->contentConfiguration()->productId();

                    //if newHead exists, but oldHead doesn't, then that enQueue of the newHead would trigger onDownloadQueueChanged
                    if (newHead)
                    {
                        //only send event that queue changed if an actual switch occurred -- if oldHead is NULL then the change would have been caught by enqueued signal
                        onDownloadQueueChanged();
                    }
                }
            }

            void GamesManagerProxy::onDownloadQueueChangedHeadBusy(const bool& busy)
            {
                ORIGIN_LOG_EVENT << "onDownloadQueueChangedHeadBusy: " << busy;
                onDownloadQueueChanged();
            }
        }
    }
}
