#include "ITEPrepare.h"
#include "ITEUIManager.h"

#include "OriginApplication.h"
#include "services/settings/SettingsManager.h"

#include "engine/ito/InstallFlowManager.h"
#include "TelemetryAPIDLL.h"

#include "services/log/LogService.h"

#include "services/debug/DebugService.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"

#include "ITOViewController.h"
#include "ITOFlow.h"

namespace Origin
{
    namespace Client
    {
        static ITEUIManager* _instance = NULL;

        //keys that will be passed in from the state machine
        static const QString sInstallProgress("ITEInstallProgress");


        ITEUIManager* ITEUIManager::GetInstance()
        {
            if(_instance == NULL)
                _instance = new ITEUIManager();

            return _instance;
        }

        void ITEUIManager::DeleteInstance()
        {	
        #if TODO //replace with new main window call
            //if(!EbisuDlg::instance()->isMaximized())
            //    EbisuDlg::instance()->showNormal();

            //EbisuDlg::instance()->activateWindow();
        #endif
            if(_instance != NULL)
            {
                delete _instance;
                _instance = NULL;
            }
        }

        ITEUIManager::ITEUIManager()
        : mVC(NULL)
        {
            Init();
        }

        ITEUIManager::~ITEUIManager()
        {
            Shutdown();
        }

        void ITEUIManager::Init()
        {
            ORIGIN_VERIFY_CONNECT(&OriginApplication::instance(), SIGNAL(generalCoreError()), this, SLOT(onCoreError()));
        }

        void ITEUIManager::Shutdown()
        {
            ORIGIN_VERIFY_DISCONNECT(&OriginApplication::instance(), SIGNAL(generalCoreError()), this, SLOT(onCoreError()));
        }

        void ITEUIManager::onInsertDisk(const Origin::Downloader::InstallArgumentRequest& request)
        {
            if(request.wrongDiscNum < 0)
            {
                if(mVC)
                    mVC->showInsertDisk(request.nextDiscNum, request.totalDiscNum);
                ORIGIN_LOG_EVENT << "Install Through Ebisu: Insert Disk " << request.nextDiscNum;
            }
            else
            {
                if(mVC)
                    mVC->showWrongDisk(request.nextDiscNum);
                ORIGIN_LOG_EVENT << "Install Through Ebisu: Wrong disc was inserted. " << request.wrongDiscNum << " was inserted, expecting " << request.nextDiscNum;;
            }
        }

        void ITEUIManager::onSwitchToDownloadFromServer()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            //we need to wait for the cancel to finish, but we can't seem to check for kReadyToStart at this point because we're ahead of the installflow state machine
            //so we're catching it in kReadyToStart state before it has a chance to process the cancel request
            //if(mEntitlement->localContent()->installFlow()->getFlowState() == Downloader::kReadyToStart)
            //    Engine::InstallFlowManager::GetInstance().SetStateCommand("ReadyToDownloadFromServer");
            //else
            {
                //but if not, then we need to wait
                //we need to connect a signal to wait for the InstallContentFlow to reinitialize
                Engine::InstallFlowManager::GetInstance().SetVariable("WaitingCommand", "ReadyToDownloadFromServer");
                if (mEntitlement->localContent()->installFlow())
                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onContentInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState)), Qt::QueuedConnection);
            }

            Engine::InstallFlowManager::GetInstance().SetVariable("SuppressCancelState", true);
            Engine::InstallFlowManager::GetInstance().SetStateCommand("ITE_Download");

            resetFlow();
        }

        void ITEUIManager::resetFlow()
        {
            if(mVC)
            {
                mVC->disconnect();
                mVC->deleteLater();
                mVC = NULL;
            }

            //make sure on the signals are reset as when we switch to download we will call showITEDialog again
            ORIGIN_VERIFY_DISCONNECT(&OriginApplication::instance(), SIGNAL(silentAutoUpdateDownloadError()), this, SLOT(onSilentAutoUpdateDownloadError()));

            const QString contentID = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentID);
            if(entitlement)
            {
                if (entitlement->localContent()->installFlow())
                {
                    ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(installDidNotComplete()), this, SLOT(onTouchupInstallerError()));
                    ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(pendingDiscChange(const Origin::Downloader::InstallArgumentRequest&)), this, SLOT(onInsertDisk(const Origin::Downloader::InstallArgumentRequest&)));
                    ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(discChanged()), mVC, SLOT(closeWindow()));
                    ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), this, SLOT(onError(qint32, qint32, QString, QString)));
                    ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(finishedState()), this, SLOT(onInstallFinished()));
                }
                ORIGIN_VERIFY_DISCONNECT(entitlement->localContent(), SIGNAL(installFlowCanceled()), this, SLOT(onInstallFlowCanceled()));
            }
        }

        void ITEUIManager::onDiscMsgRetry()
        {
            //ORIGIN_LOG_EVENT << "ITEDiskInstallProgress::onDiscMsgRetry()";

            //if(mWasDiskDialogVisible)
            //{
            //    if(mEntitlement->localContent()->state() == Engine::Content::LocalContent::Downloading)
            //                hideDiskDialogs();
            //    else
            //    {
            //        ORIGIN_LOG_EVENT << "Calling... showInsertDisk()";
            //        showInsertDisk(mExpectedDiskIndex);
            //    }
            //}
            //mExpectedDiskIndex = -1;
        }

        void ITEUIManager::onError(qint32 errorType, qint32 errorCode, QString errorMessage, QString key)
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

            //check to see if we're in the update phase of ITO, can't use localContent()->updating() because if it gets a JIT Error, e.g., it won't consider it "Active" so updating() wil lreturn false
            if(mEntitlement && mEntitlement->localContent()->installFlow() && mEntitlement->localContent()->installFlow()->isUpdate())
            {
                //any error we get in the middle of update is considered ignorable and ITO flow should just finish
                Engine::InstallFlowManager::GetInstance().SetStateCommand("Installed");
                return;
            }

            //filter out the ErrorType, if it isn't a read disk error, then InstallerViewController::onInstallError will handle it
            if(ITOFlow::isReadDiscErrorType(errorType))
            {
                //MY: do this BEFORE showing dialog or anything else, just in case it's going to switch states where "Failed" may be referenced
                Engine::InstallFlowManager::GetInstance().SetVariable("Failed", true);

                const TYPE_S8 reasonStr = "Disk Read Error";
                GetTelemetryInterface()->Metric_ITE_CLIENT_FAILED(reasonStr);

                //TODO
                mVC->showReadDiscError();

                ORIGIN_LOG_EVENT << "Install Through Ebisu: Read Disk Error";
            }

#if TODO
            // TODO: need to handle install errors
            QByteArray pFunctionName = lsxObj->BeginFunctionRead();
            if(pFunctionName == "InstallError")
            {
                //MY: do this BEFORE showing dialog or anything else, just in case it's going to switch states where "Failed" may be referenced
                Engine::InstallFlowManager::GetInstance().SetVariable("Failed", false);

                ORIGIN_LOG_EVENT << "Install Through Ebisu: Install Error";

                // need to do this last so that the "Failed" value gets set BEFORE switching to cancel state where it checks for the value in the var
                Engine::InstallFlowManager::GetInstance().SetStateCommand("Cancel");
                return;
            }
#endif
        }

        void ITEUIManager::onInstallFinished()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

            Engine::InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::Success);
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Installed");

            bool basegameInstalled = Engine::InstallFlowManager::GetInstance().GetVariable("BaseGameInstalled").toBool();
            if(!basegameInstalled) //so base game just finished installing
            {
                Engine::InstallFlowManager::GetInstance().SetVariable("BaseGameInstalled", true);

                //check to see if we're already in kReadyToStart state, if so, we can just kick off the update
                if(mEntitlement->localContent()->installFlow() && mEntitlement->localContent()->installFlow()->getFlowState() == Downloader::ContentInstallFlowState::kReadyToStart)
                    Engine::InstallFlowManager::GetInstance().SetStateCommand("ReadyToPatch");
                else
                {
                    //but if not, then we need to wait
                    Engine::InstallFlowManager::GetInstance().SetVariable("WaitingCommand", "ReadyToPatch");

                    //we need to connect a signal to wait for the InstallContentFlow to reinitialize
                    if (mEntitlement->localContent()->installFlow())
                        ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onContentInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState)), Qt::QueuedConnection);
                }
            }
        }

        void ITEUIManager::onContentInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState newState)
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

            if(newState == Downloader::ContentInstallFlowState::kReadyToStart)
            {
                if (mEntitlement->localContent()->installFlow())
                    ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onContentInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState)));

                QString waitingCommand = Engine::InstallFlowManager::GetInstance().GetVariable("WaitingCommand").toString();
                if(!waitingCommand.isEmpty())
                    Engine::InstallFlowManager::GetInstance().SetStateCommand(waitingCommand);
            }
        }

        void ITEUIManager::onTouchupInstallerError()
        {
            //if we get a touch up installer error, lets fail silently
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Cancel");
            Engine::InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::Success);
        }

        void ITEUIManager::onCancel()
        {
            const QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            if(entitlement && entitlement->localContent()->installFlow()) 
                entitlement->localContent()->installFlow()->cancel();

			onInstallFlowCanceled();
        }

        void ITEUIManager::onInstallFlowCanceled()
        {
            if(!Engine::InstallFlowManager::GetInstance().GetVariable("SuppressCancelState").toBool())
            {
                Engine::InstallFlowManager::GetInstance().SetStateCommand("Cancel");
                Engine::InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::Canceled);
            }
            Engine::InstallFlowManager::GetInstance().SetVariable("SuppressCancelState", false);
        }

        void ITEUIManager::onRetryDisc()
        {
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Retry");
        }

        //get this signal when the download for auto-patch fails
        void ITEUIManager::onSilentAutoUpdateDownloadError()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

            // this was intended only for updates but will be called on any core error so we need to check
            // to see if we are in an update, otherwise pop out.

            //if we are updating -- if we are already installed and the installflow is active then we are updating
            if(mEntitlement->localContent()->installed() && mEntitlement->localContent()->installFlow() && mEntitlement->localContent()->installFlow()->isActive())
            {
                return;
            }

            ORIGIN_LOG_ERROR << "ITE auto-update download failed; treated as silent error";

            //the failure of the patch download isn't considered blocking error, so just consider the install done
            Engine::InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::Success);
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Installed");
        }

        void ITEUIManager::StateChanged(QString newStateName)
        {
            ORIGIN_LOG_EVENT << "ITE GUI Changing to state " << newStateName.toUtf8().constData();

            if(newStateName == "Initial")
            {
                resetFlow();
                mVC = new ITOViewController(Engine::InstallFlowManager::GetInstance().GetVariable("GameName").toString(), this);
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(redemptionDone()), this, SLOT(onRedemptionDone()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(cancel()), this, SLOT(onCancel()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(finished()), this, SLOT(onInstallFinished()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(retryInstallFlow()), this, SLOT(onRetryInstallFlow()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(stopDownloadFlow()), this, SLOT(onStopDownloadFlow()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(downloadGame()), this, SLOT(onDownloadGame()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(retryDisc()), this, SLOT(onRetryDisc()));
                ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(resetFlow()), this, SLOT(resetFlow()));
            }
            else if(newStateName == "ITEInstallProgress" && mVC)
            {
                QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
                Engine::Content::EntitlementRef mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

                if (mEntitlement->localContent()->installFlow())
                {
                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(pendingDiscChange(const Origin::Downloader::InstallArgumentRequest&)), this, SLOT(onInsertDisk(const Origin::Downloader::InstallArgumentRequest&)), Qt::QueuedConnection);

                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(discChanged()), mVC, SLOT(closeWindow()), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), this, SLOT(onError(qint32, qint32, QString, QString)), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(finishedState()), this, SLOT(onInstallFinished()), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent()->installFlow(), SIGNAL(installDidNotComplete()), this, SLOT(onTouchupInstallerError()), Qt::QueuedConnection);
                }
                ORIGIN_VERIFY_CONNECT_EX(mEntitlement->localContent(), SIGNAL(installFlowCanceled()), this, SLOT(onInstallFlowCanceled()), Qt::QueuedConnection);


                ORIGIN_VERIFY_CONNECT(&OriginApplication::instance(), SIGNAL(silentAutoUpdateDownloadError()), this, SLOT(onSilentAutoUpdateDownloadError()));
            }
            else if(newStateName == "ITEComplete" && mVC)
            {
                switch(Engine::InstallFlowManager::GetInstance().GetVariable("CompleteDialogType").toInt())
                {
                case ITOFlow::InstallFailed:
                    mVC->showInstallFailed();
                    break;
                case ITOFlow::FailedNoGameName:
                    mVC->showFailedNoGameName();
                    break;
                case ITOFlow::FreeToPlayNoEntitlement:
                    mVC->showFreeToPlayFailNoEntitlement();
                    break;
                case ITOFlow::SysReqFailed:
                    mVC->showSystemReqFailed();
                    break;
                case ITOFlow::DLCSysReqFailed:
                    mVC->showDLCSystemReqFailed();
                    break;
                // The designers don't want to show any UI for this. If a game installs correctly 
                // but the update that installs at the same time fails, then it should appear that 
                // the game has installed correctly. If the user then tries to launch the game that 
                // has an update, we should follow our standard update flow.
                case ITOFlow::PatchFailed:
                case ITOFlow::Canceled:
                case ITOFlow::Success:
                default:
                    Engine::InstallFlowManager::GetInstance().SetStateCommand("Finish");
                    resetFlow();
                    break;
                };
            }
            else if(newStateName == "ITERedemptionCode" && mVC)
            {
                mVC->showRedeem();
            }
            else if(newStateName == "ITEWrongCode" && mVC)
            {
                mVC->showWrongCode();
            }
            else if(newStateName == "ITEInitial_DiscCheck" && mVC)
            {
                mVC->showDiscCheck();
            }

	        //we have this check here because we need a place that guarantees the contentId is good but before
	        //the grey market language select dialog comes up

	        ////we want to connect the cancel signal so we will pick up and cancel from teh Language select and the ITE Prepare
	        //if(newStateName == "LanguageAndITEPrepare")
	        //{
         //       mVC->showPrepare();
		       // //ITEPrepare *prepareDialog =(ITEPrepare *)mDialogFunctionMap[sPrepareDialogID];
		       // //prepareDialog->connectToCancelAndErrorSignal();    //need to listen for cancel from the tile, can't do it at dialog creation because contentID hasn't been set yet
	        //}

        }

        void ITEUIManager::onRedemptionDone()
        {
            if( Engine::InstallFlowManager::GetInstance().GetVariable("CodeRedeemed").toBool() )
            {
                Engine::InstallFlowManager::GetInstance().SetStateCommand("Next");

                if(mVC)
                    mVC->closeWindow();
            }
            else
            {
                ORIGIN_LOG_EVENT << "ITE - User cancelling from ITERedemptionDialog.";
                onCancel();
            }
        }

        void ITEUIManager::onCoreError()
        {
            ORIGIN_LOG_ERROR << "ITE failed due to an unknown Core error";

            Engine::InstallFlowManager::GetInstance().SetVariable("Failed", true);

            const TYPE_S8 reasonStr = "Failure due to core error";
            GetTelemetryInterface()->Metric_ITE_CLIENT_FAILED(reasonStr);

            //MY: DT 14790 - need to do this last so that the "Failed" value gets set BEFORE switching to cancel state where it checks for the value in the var
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Cancel");
        }

        void ITEUIManager::onLoggedOut()
        {
            Services::SettingsManager::instance()->unset(Services::SETTING_COMMANDLINE);
            DeleteInstance();
        }

        //the prepare dialog is not driven as part of the state machine but rather shown when receiving a message from core
        //this is because we need core to tell the dialog about the optional components and the actual download size




        void ITEUIManager::showNotReadyToDownloadError(const QString& contentId)
        {
            Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentId);
            if(entitlement && !entitlement->localContent()->installed(true))
            {
                if(entitlement->localContent()->state() == Engine::Content::LocalContent::Unreleased && mVC)
                {
                    mVC->showNotReadyToDownload();
                }
            }
        }

        void ITEUIManager::onDownloadGame()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            if(!entRef.isNull() && entRef->localContent()->installFlow())
            {
                if(entRef->localContent()->installFlow()->canCancel())
                {
                    entRef->localContent()->installFlow()->cancel();
                    onSwitchToDownloadFromServer();
                }
                else if(entRef->localContent()->installFlow()->getFlowState() == Downloader::ContentInstallFlowState::kReadyToStart)
                {
                    onSwitchToDownloadFromServer();
                }
                else
                {
                    ORIGIN_VERIFY_CONNECT(entRef->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChanged()));
                    // in case you can't cancel, wait for the downloader to be in a kPaused state 
                    if(mVC)
                        mVC->showDownloadPreparing();
                }
            }
        }

        void ITEUIManager::onInstallFlowStateChanged()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            if(!entRef.isNull() && entRef->localContent()->installFlow())
            {
                if(entRef->localContent()->installFlow()->canCancel())
                {
                    onSwitchToDownloadFromServer();
                    if(mVC)
                        mVC->closeWindow();
                    ORIGIN_VERIFY_DISCONNECT(entRef->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChanged()));
                    // regardless of the outcome(canCancel() returned true or user hit X), cancel the ITO flow
                    entRef->localContent()->installFlow()->requestCancel();
                }
            }
        }

        void ITEUIManager::onRetryInstallFlow()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            //try again logic
            if(!entRef.isNull() && entRef->localContent()->installFlow())
            {
                //check canResume, if so resume, otherwise we need to call begin()
                if(entRef->localContent()->installFlow()->canResume())
                    entRef->localContent()->installFlow()->resume();
                else
                    entRef->localContent()->installFlow()->begin();
            }
        }

        void ITEUIManager::onStopDownloadFlow()
        {
            QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
            Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
            if(!entRef.isNull() && entRef->localContent()->installFlow())
            {
                entRef->localContent()->installFlow()->requestCancel();
            }
        }
    }
}

