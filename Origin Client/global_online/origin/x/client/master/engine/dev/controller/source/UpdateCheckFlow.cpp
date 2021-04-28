///////////////////////////////////////////////////////////////////////////////
// Engine::UpdateCheckFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////     

/// \brief TBD.
#include "UpdateCheckFlow.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/connection/ConnectionStatesService.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"

#include "TelemetryAPIDLL.h"

#define OFFER_UPDATE_CHECK_TIMEOUT 5000

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {

            /// \brief The PlayFlow constructor.
            ///
            /// \param content TBD.
            UpdateCheckFlow::UpdateCheckFlow(Origin::Engine::Content::EntitlementRef content) : 
                mContent(content)
            {    
                mOfferUpdateCheckTimer.setInterval(OFFER_UPDATE_CHECK_TIMEOUT);
                mOfferUpdateCheckTimer.setSingleShot(true);
                ORIGIN_VERIFY_CONNECT(&mOfferUpdateCheckTimer, SIGNAL(timeout()), this, SLOT(onOfferVersionCheckTimeout()));
            }

            /// \brief The PlayFlow destructor; releases the resources used by a PlayFlow class instance.
            UpdateCheckFlow::~UpdateCheckFlow()
            {

            }
                    
            /// \brief TBD.
            void UpdateCheckFlow::begin()
            {
                // Make sure we are up to date with catalog
                Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
                if(contentController != NULL)
                {
                    ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(offerVersionUpdateCheckComplete(QString, bool)), 
                        this, SLOT(onOfferVersionUpdateCheckComplete(QString, bool)));

                    mOfferUpdateCheckInProgress = true;
                    mOfferUpdateCheckTimer.start();
                    contentController->checkIfOfferVersionIsUpToDate(mContent);
                }
                else
                {
                    emit noUpdateAvailable();
                }
            }
            
            void UpdateCheckFlow::onOfferVersionUpdateCheckComplete(QString offerId, bool isOfferUpToDate)
            {
                if(offerId != mContent->contentConfiguration()->productId())
                {
                    return;
                }

                // We already timed out...
                if(!mOfferUpdateCheckInProgress)
                {
                    return;
                }

                mOfferUpdateCheckTimer.stop();
                mOfferUpdateCheckInProgress = false;

                Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
                if(contentController != NULL)
                {
                    ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(offerVersionUpdateCheckComplete(QString, bool)), 
                                    this, SLOT(onOfferVersionUpdateCheckComplete(QString, bool)));
                }

                if(isOfferUpToDate)
                {
                    emit noUpdateAvailable();
                }
                else
                {
                    emit updateAvailable();
                }
            }

            void UpdateCheckFlow::onOfferVersionCheckTimeout()
            {
                // We already timed out...
                if(!mOfferUpdateCheckInProgress)
                {
                    return;
                }

                ORIGIN_LOG_EVENT << "Software version update check timed out for product [" << mContent->contentConfiguration()->productId() << "].";

                GetTelemetryInterface()->Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), 
                    mContent->localContent()->installedVersion().toLocal8Bit().constData());

                mOfferUpdateCheckTimer.stop();
                mOfferUpdateCheckInProgress = false;
                emit noUpdateAvailable();
            }

            /// \brief TBD.
            void UpdateCheckFlow::cancel()
            {
                mOfferUpdateCheckTimer.stop();
                mOfferUpdateCheckInProgress = false;
                emit noUpdateAvailable();
            }
        }
    }
}
