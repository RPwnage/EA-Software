#include "PendingActionLibraryOpenViewController.h"
#include "ClientFlow.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    using namespace Engine::Content;

    namespace Client
    {
        using namespace PendingAction;

        PendingActionLibraryOpenViewController::PendingActionLibraryOpenViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            :PendingActionBaseViewController(queryParams, launchedFromBrowser, uuid, lookupId)
            ,mEntitlementLookupId(Engine::Content::ContentController::EntByOfferId)
            ,mShowClientPage(NULL)
        {

        }

        void PendingActionLibraryOpenViewController::showGameDetails()
        {
            //mEntitlementIds and  mEntitlementLookupId are set in parseAndValidateParams
            EntitlementRef entToShowDetails = findEntitlementToShow(mEntitlementIds, mEntitlementLookupId);

            if(entToShowDetails)
                ClientFlow::instance()->showGameDetails(entToShowDetails);
            else
                ClientFlow::instance()->showMyGames();
        }

        void PendingActionLibraryOpenViewController::showMyProfile()
        {
            bool ok = false;

            //mProfileId is set in parseAndValidateParams
            qint64 id = mProfileId.toLongLong(&ok);

            if(mProfileId.isEmpty() || !ok)
            {
                ClientFlow::instance()->showMyProfile(Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
            else
            {

                ClientFlow::instance()->showProfile(id);
            }
        }

        void PendingActionLibraryOpenViewController::showAchievements()
        {
            ClientFlow::instance()->showAchievementsHome();
        }

        void PendingActionLibraryOpenViewController::showSettingsGeneral()
        {
            ClientFlow::instance()->showSettingsGeneral();
        }

        void PendingActionLibraryOpenViewController::showSettingsInGame()
        {
            ClientFlow::instance()->showSettingsInGame();
        }

        void PendingActionLibraryOpenViewController::showSettingsNotifications()
        {
            ClientFlow::instance()->showSettingsNotifications();
        }

        void PendingActionLibraryOpenViewController::showSettingsAdvanced()
        {
            ClientFlow::instance()->showSettingsAdvanced();
        }

        void PendingActionLibraryOpenViewController::showAboutMe()
        {
            ClientFlow::instance()->view()->onMainMenuAccount();
        }

        void PendingActionLibraryOpenViewController::showOrderHistory()
        {
            ClientFlow::instance()->showOrderHistory();
        }

        void PendingActionLibraryOpenViewController::showAccountProfilePrivacy()
        {
            ClientFlow::instance()->showAccountProfilePrivacy();
        }

        void PendingActionLibraryOpenViewController::showAccountProfileSecurity()
        {
            ClientFlow::instance()->showAccountProfileSecurity();
        }

        void PendingActionLibraryOpenViewController::showAccountProfilePaymentShipping()
        {
            ClientFlow::instance()->showAccountProfilePaymentShipping();
        }

        void PendingActionLibraryOpenViewController::showAccountProfileRedeem()
        {
            ClientFlow::instance()->showAccountProfileRedeem();
        }

        void PendingActionLibraryOpenViewController::performMainAction()
        {
            ensureClientVisible();
            refreshEntitlementsIfRequested();

            if (Engine::LoginController::isUserLoggedIn() && ClientFlow::instance())
            {
                if(mShowClientPage)
                {
                    (this->*mShowClientPage)();
                }
                else    //since no specific client page was specified, show mygames
                {
                    ClientFlow::instance()->showMyGames();
                }
            }
            mComponentsCompleted |= MainAction;
        }

        QString PendingActionLibraryOpenViewController::startupTab()
        {
            return PendingAction::StartupTabMyGamesId;
        }

        bool PendingActionLibraryOpenViewController::parseAndValidateParams()
        {
            bool valid = PendingActionBaseViewController::parseAndValidateParams();
            
            mClientPage = mQueryParams.queryItemValue(PendingAction::ParamClientPageId);

            //grab the string thats in the id field, could be a list of ids or a single id
            //its used for showing offer pages and the same field is used for profile
            QString ids = mQueryParams.queryItemValue(PendingAction::ParamIdId, QUrl::FullyDecoded);
 
            if((mClientPage.compare(ClientPageGameDetailsByOfferId, Qt::CaseInsensitive) == 0) ||
                (mClientPage.compare(ClientPageGameDetailsByContentId, Qt::CaseInsensitive) == 0) ||
                (mClientPage.compare(ClientPageGameDetailsByUnknownTypeId, Qt::CaseInsensitive) == 0))
            {
                mRequiresEntitlements = true;

                //if the id param is empty but we have specified a show game details page action, the request is not valid.
                //otherwise assign showGameDetails function to the function pointer
                if(!ids.isEmpty())
                {
                    mEntitlementIds = ids;
                    mShowClientPage = &PendingActionLibraryOpenViewController::showGameDetails;
                }
                else
                    valid = false;
            }

            if(!mClientPage.isEmpty())
            {
                if(mClientPage.compare(ClientPageGameDetailsByOfferId, Qt::CaseInsensitive) == 0)
                {
                    mEntitlementLookupId = Engine::Content::ContentController::EntByOfferId;
                }
                else
                if(mClientPage.compare(ClientPageGameDetailsByContentId, Qt::CaseInsensitive) == 0)
                {
                    mEntitlementLookupId = Engine::Content::ContentController::EntByContentId;
                }
                else
                if(mClientPage.compare(ClientPageGameDetailsByUnknownTypeId, Qt::CaseInsensitive) == 0)
                {
                    mEntitlementLookupId = Engine::Content::ContentController::EntById;
                }
                else
                if(mClientPage.compare(ClientPageProfileId, Qt::CaseInsensitive) == 0) 
                {
                    //we only expect one id for profiles
                    mProfileId = ids;
                    mShowClientPage = &PendingActionLibraryOpenViewController::showMyProfile;
                }
                else
                if(mClientPage.compare(ClientPageAchievementsId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAchievements;
                }
                else
                if(mClientPage.compare(ClientPageSettingsGeneralId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showSettingsGeneral;
                }
                else
                if(mClientPage.compare(ClientPageSettingsOIGId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showSettingsInGame;
                }
                else
                if(mClientPage.compare(ClientPageSettingsNotificationsId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showSettingsNotifications;
                }
                else
                if(mClientPage.compare(ClientPageSettingsAdvancedId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showSettingsAdvanced;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfileAboutMeId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAboutMe;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfileOrderHistoryId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showOrderHistory;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfilePrivacyId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAccountProfilePrivacy;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfileSecurityId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAccountProfileSecurity;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfilePaymentId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAccountProfilePaymentShipping;
                }
                else
                if(mClientPage.compare(ClientPageAccountAndProfileOrderRedeemId, Qt::CaseInsensitive) == 0) 
                {
                    mShowClientPage = &PendingActionLibraryOpenViewController::showAccountProfileRedeem;
                }
                else
                {
                    valid = false;
                }
            }
            return valid;
        }

        EntitlementRef PendingActionLibraryOpenViewController::findEntitlementToShow(QString ids, Engine::Content::ContentController::EntitlementLookupType lookup)
        {
            QStringList idList = ids.split(',', QString::SkipEmptyParts);

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if(contentController)
            {
                return contentController->bestMatchedEntitlementByIds(idList, lookup, true);
            }

            return EntitlementRef();
        }

        void PendingActionLibraryOpenViewController::sendTelemetry()
        {
            if (mFromJumpList)
            {
                //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                GetTelemetryInterface()->Metric_JUMPLIST_ACTION("library", "jumplist", "" );
            }

            if( (mClientPage.compare(ClientPageSettingsGeneralId, Qt::CaseInsensitive) == 0) ||
                (mClientPage.compare(ClientPageSettingsOIGId, Qt::CaseInsensitive) == 0) ||
                (mClientPage.compare(ClientPageSettingsNotificationsId, Qt::CaseInsensitive) == 0) ||
                (mClientPage.compare(ClientPageSettingsAdvancedId, Qt::CaseInsensitive) == 0) )
            {
                //for now, send telemetry regardless of mFromJumpList because before refactor, we were assuming that origin://settings was only coming from jumplist
                //and origin:// doesn't differentiate the source; should refactor PlatformJumpList to use origin2:// and then we can pass in the jump parameter
//              if (mFromJumpList)
                {
                    //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                    GetTelemetryInterface()->Metric_JUMPLIST_ACTION(TelemetrySettings, "jumplist", "" );
                }
            }
            else if(mClientPage.compare(ClientPageProfileId, Qt::CaseInsensitive) == 0) 
            {
                //for now, send telemetry regardless of mFromJumpList because before refactor, we were assuming that origin://settings was only coming from jumplist
                //and origin:// doesn't differentiate the source; should refactor PlatformJumpList to use origin2:// and then we can pass in the jump parameter
//              if (mFromJumpList)
                {
                    //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                    GetTelemetryInterface()->Metric_JUMPLIST_ACTION(TelemetryProfile, "jumplist", "" );
                }

            }

            //call the baseclass one
            PendingActionBaseViewController::sendTelemetry();
        }

    }
}