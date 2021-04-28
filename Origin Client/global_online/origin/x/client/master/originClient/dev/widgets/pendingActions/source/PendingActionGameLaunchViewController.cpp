#include "PendingActionGameLaunchViewController.h"
#include "MainFlow.h"
#include "SingleLoginFlow.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "RTPFlow.h"
#include "CommandLine.h"
#include "CommandFlow.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Client
    {
        using namespace PendingAction;

        PendingActionGameLaunchViewController::PendingActionGameLaunchViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            : PendingActionBaseViewController(queryParams, launchedFromBrowser, uuid, lookupId)
            , mAutoDownload(false)
            , mGameRestart(false)
        {
            mSSOFlowType = Origin::Client::SSOFlow::SSO_LAUNCHGAME;
            mComponentsCompleted = PopupWindow | WindowMode;
        }

        void PendingActionGameLaunchViewController::performMainAction()
        {
            if (mForceOnline && MainFlow::instance()->singleLoginFlow())
            {
                MainFlow::instance()->singleLoginFlow()->closeSingleLoginDialogs();
            }

            MainFlow::instance()->rtpFlow()->setPendingLaunch  (mIdList, mGameTitle, mCmdParams, mAutoDownload, mGameRestart, mItemSubType, mUUID, mForceOnline);

            MainFlow::instance()->setShowAgeUpFlow();

            mComponentsCompleted |= MainAction;
        }

        bool PendingActionGameLaunchViewController::parseAndValidateParams()
        {
            bool valid = true;

            QString offerIds = QString();
            mGameTitle = QObject::tr("ebisu_client_r2p_unknown_game_title");

            QString rtpFile = mQueryParams.queryItemValue(ParamRtpFile);
            if (!rtpFile.isEmpty()) //using rtpFile to specify urlGameIds and urlGametitle
            {
                //Percent encoding does not unencode spaces, need to do manually
                rtpFile = QUrl::fromPercentEncoding(rtpFile.toUtf8()).replace("%20", " ");
                // If we got a .rtp file instead of content IDs, parse the file
                if (!rtpFile.endsWith(".rtp", Qt::CaseInsensitive))
                {
                    ORIGIN_LOG_ERROR << "RtP launch aborted -- couldn't find .rtp file name.";
                    valid = false;
                }
                else if (Origin::Client::CommandFlow::instance() && Origin::Client::CommandFlow::instance()->parseRtPFile (rtpFile, offerIds, mGameTitle))
                {
                    if (offerIds.isEmpty())
                        valid = false;
                }
                else
                {
                    valid = false;
                }
            }
            else
            {
                // EBIBUGS-27909: Use QUrl::FullyDecoded to decode our url parameter. 0x3a is : in url encode-ness.
                // We need to decode that so we can find our games via offer id.
                offerIds = mQueryParams.queryItemValue(ParamOfferIdsId, QUrl::FullyDecoded);
                if(offerIds.isEmpty())
                {
                    valid = false;
                }
                else
                {
                    QString gameTitle = mQueryParams.queryItemValue(ParamTitleId, QUrl::FullyDecoded);

                    if (!gameTitle.isEmpty())
                        mGameTitle = gameTitle;
                }
            }

            if(valid)
            {
                mIdList = offerIds.split(',', QString::SkipEmptyParts);

                //to convert the unicode escape chars to original format
                Origin::Client::CommandLine::UnicodeUnescapeString (mGameTitle);

                //need QUrl::FullyDecoded, otherwise mCmdParams will be double encoded and won't get decoded correctly
                //in UnicodeUnescapeQString() in RTPFlow.cpp
                mCmdParams = mQueryParams.queryItemValue(ParamCmdParamsId, QUrl::FullyDecoded);

                mItemSubType = mQueryParams.queryItemValue(ParamItemSubTypeId);
                mForceOnline = (mQueryParams.queryItemValue(ParamForceOnlineId).toInt() == 1) || (mQueryParams.queryItemValue(ParamForceOnlineId).compare("true", Qt::CaseInsensitive) == 0);
                mGameRestart = (mQueryParams.queryItemValue(ParamRestartId).toInt() == 1) || (mQueryParams.queryItemValue(ParamRestartId).compare("true", Qt::CaseInsensitive) == 0);
                mAutoDownload = (mQueryParams.queryItemValue(ParamAutoDownloadId).toInt() == 1) || (mQueryParams.queryItemValue(ParamAutoDownloadId).compare("true", Qt::CaseInsensitive) == 0);

                //let the RTP Flow now whether this is an autodownload or not, RTP Flow uses that to determine whether to minimize or not
                if (MainFlow::instance() && MainFlow::instance()->rtpFlow())
                {
                    MainFlow::instance()->rtpFlow()->setAutoDownload(mAutoDownload);
                }

                if (mLaunchedFromBrowser || mCmdParams.contains(OpenAutomateId, Qt::CaseInsensitive))
                {
                    mCmdParams.clear();
                }
            }

            return valid;
        }

        void PendingActionGameLaunchViewController::sendTelemetry()
        {
            if (mFromJumpList)
            {
                //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                GetTelemetryInterface()->Metric_JUMPLIST_ACTION("launchgame", "jumplist", "" );
            }

            //call the base class one
            PendingActionBaseViewController::sendTelemetry();
        }

        bool PendingActionGameLaunchViewController::shouldConsumeSSOToken()
        {
            if (mForceOnline)
            {
                return Origin::Services::Connection::ConnectionStatesService::instance()->isUserOnline(Engine::LoginController::instance()->currentUser()->getSession());
            }
            else
            {
                return Engine::LoginController::isUserLoggedIn();
            }
        }

    }
}