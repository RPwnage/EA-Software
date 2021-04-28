//  LoginFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "flows/NonOriginGameFlow.h"
#include "flows/ClientFlow.h"
#include "flows/MainFlow.h"
#include "ContentFlowController.h"
#include "StoreUrlBuilder.h"

#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/NonOriginContentController.h"
#include "engine/subscription/SubscriptionManager.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "EbisuSystemTray.h"

#include "TelemetryAPIDLL.h"
#include "ClientSystemMenu.h"

namespace Origin
{
namespace Client
{

    NonOriginGameFlow::NonOriginGameFlow() :
        AbstractFlow(),
        mViewController(NULL),
        mGame(NULL),
        mNonOriginController(NULL),
        mOperation(OpNotSet)
    {

    }

    NonOriginGameFlow::~NonOriginGameFlow()
    {

    }

    void NonOriginGameFlow::initialize(FlowOperation op, Engine::Content::EntitlementRef game /*= Engine::Content::EntitlementRef(NULL)*/)
    {
        mOperation = op;
        mGame = game;

        mViewController = new NonOriginGameViewController(this);
        mViewController->initialize();

        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(propertiesChanged(const Origin::Engine::Content::NonOriginGameData&, bool)), this,
            SLOT(onPropertiesChanged(const Origin::Engine::Content::NonOriginGameData&, bool)));
        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(removeBrokenGame()), this, SLOT(onRemoveBrokenGame()));
        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(redeemGameCode()), this, SLOT(onShowRedemptionPage()));
        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(showSubscriptionPage()), this, SLOT(onSubscriptionPage()));
        mNonOriginController = Engine::Content::ContentController::currentUserContentController()->nonOriginController();

        if (mNonOriginController)
        {  
            ORIGIN_VERIFY_CONNECT_EX(mNonOriginController, SIGNAL(nonOriginGameRemoved(Origin::Engine::Content::EntitlementRef)), this, SLOT(onCacheUpdated()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mNonOriginController, SIGNAL(nonOriginGameModified(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), 
                this, SLOT(onNonOriginGameModified(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
        }
    }

    NonOriginGameFlow::FlowOperation NonOriginGameFlow::currentOperation() const
    {
        return mOperation;
    }

    void NonOriginGameFlow::start()
    {
        if (!mViewController)
        {
            ORIGIN_LOG_ERROR << "Flow has not been initialized.";
            NonOriginGameFlowResult result;
            result.result = FLOW_FAILED;
            emit finished(result);
            return;
        }

        if (mOperation == OpAdd)
        {
            mViewController->onShowAddGameDialog();
        }
        else if (mOperation == OpRemove && !mGame.isNull())
        {
            mViewController->onShowConfirmRemoveGame(mGame);
        }
        else if (mOperation == OpModify && !mGame.isNull())
        {
            mViewController->onShowPropertiesDialog(mGame);
        }
        else if (mOperation == OpRepair && !mGame.isNull())
        {
            mViewController->onShowGameRepairDialog(mGame);
        }
    }

    void NonOriginGameFlow::focus()
    {
        if(mViewController)
        {
            mViewController->focus();
        }
    }

    void NonOriginGameFlow::onRemoveBrokenGame()
    {
        if (!mGame.isNull())
        {
            mOperation = OpRemove;
            mViewController->onShowConfirmRemoveGame(mGame);
        }
    }

    void NonOriginGameFlow::onRemoveConfirmed()
    {
        if (mNonOriginController && !mGame.isNull())
        {
            mNonOriginController->removeContentByProductId(mGame->contentConfiguration()->productId());
        }
    }

    void  NonOriginGameFlow::onPropertiesChanged(const  Origin::Engine::Content::NonOriginGameData& gameData, bool forceUpdate)
    {
        if (mNonOriginController && !mGame.isNull())
        {
            QString productId = mGame->contentConfiguration()->productId();

            QString backslashPath = gameData.getExecutableFile();
            
#ifdef ORIGIN_PC
            backslashPath.replace("/", "\\");
#endif
            
            bool exePathChanged = mGame->contentConfiguration()->executePath() != backslashPath;
            if (exePathChanged && !isValidExecutablePath(backslashPath))
            {
                QFileInfo fileInfo(backslashPath);
                mViewController->onInvalidFileType(fileInfo.fileName());
            }
            else
            {
                bool displayNameChanged = mGame->contentConfiguration()->displayName() != gameData.getDisplayName();
                bool exeParamsChanged = mGame->contentConfiguration()->executeParameters() != gameData.getExecutableParameters();
                bool igoPermissionChanged = mGame->contentConfiguration()->igoPermission() != gameData.getIgoPermission();

                bool somethingChanged = forceUpdate || displayNameChanged || exeParamsChanged || exePathChanged || igoPermissionChanged;

                if (somethingChanged)
                {
                    if (exePathChanged && mNonOriginController->wasAdded(backslashPath))
                    {
                        mViewController->onGameAlreadyExists(backslashPath);
                    }
                    else
                    {
                        Origin::Engine::Content::NonOriginGameData newGameData = gameData;
                        newGameData.setExecutableFile(backslashPath);
                        mNonOriginController->modifyContent(mGame->contentConfiguration()->productId(), newGameData);

                        // only send the telemetry event if the any of the data sent has changed
                        if( exePathChanged || exeParamsChanged || igoPermissionChanged )
                        {
                            // use the last 256 characters of the exe path
                            QString exePathTruncated = newGameData.getExecutableFile();
                            int length = newGameData.getExecutableFile().length();
                            if( length > 256 )
                            {
                                exePathTruncated = newGameData.getExecutableFile().remove(0, length - 256);
                            }

                            // truncate the exe params to 256 characters
                            QString exeParamsTruncated = newGameData.getExecutableParameters();
                            exeParamsTruncated.truncate(256);

                            // applied changes to Non-Origin Game properties
                            GetTelemetryInterface()->Metric_GAMEPROPERTIES_APPLY_CHANGES(
                                true,
                                productId.toUtf8().data(),
                                exePathTruncated.toUtf8().data(),
                                exeParamsTruncated.toUtf8().data(),
                                newGameData.getIgoPermission() == Services::Publishing::IGOPermissionBlacklisted);
                        }

                        return; // don't want to stop until my games is refreshed
                    }
                }
            }
        }

        stop();
    }

    void NonOriginGameFlow::onCacheUpdated()
    {
        ClientSystemMenu* systemMenu = dynamic_cast<ClientSystemMenu*>(EbisuSystemTray::instance()->trayMenu());
        if (systemMenu)
        {
            systemMenu->updateRecentPlayedGames();
        }
        stop();
    }

    void NonOriginGameFlow::onShowRedemptionPage()
    {
        ClientFlow::instance()->showRedemptionPage(RedeemBrowser::Origin, RedeemBrowser::OriginCodeClient, QString());

		// Finish but don't show My Games and steal focus from redeem
        stop(false);
    }

    void NonOriginGameFlow::onNonOriginGameModified(Origin::Engine::Content::EntitlementRef originalNog, Origin::Engine::Content::EntitlementRef modifiedNog)
    {      
        if(originalNog == mGame)
        {
            if (mOperation == OpRepair && !mGame.isNull())
            {
                Origin::Client::MainFlow::instance()->contentFlowController()->startPlayFlow(modifiedNog->contentConfiguration()->productId(), true);
            }

            onCacheUpdated();
        }
    }

    void NonOriginGameFlow::onSubscriptionPage()
    {
        ClientFlow::instance()->showUrlInStore(StoreUrlBuilder().subscriptionVaultUrl());
        // Finish but don't show My Games and steal focus from redeem
        stop(false);
    }
    void NonOriginGameFlow::onScanForGamesSelected()
    {
        // TODO
        // nonOriginContentController->scanForGames()
    }

    void NonOriginGameFlow::onGameSelectionComplete(const QStringList& executablePaths)
    {
        if (mNonOriginController)
        {
            QStringList validExecutables;
            for (QStringList::const_iterator i = executablePaths.constBegin(); i != executablePaths.constEnd(); ++i)
            {
                QString exePath(*i);
                bool isURL = false;
                if (!exePath.contains("://"))   // no URL
                {
                    #if defined(ORIGIN_PC)
                        exePath.replace("/", "\\");
                    #endif
                }
                else
                {
                    isURL = true;
                }

                QFileInfo fileInfo(exePath);

                if (!isURL && !isValidExecutablePath(exePath))
                {
                    mViewController->onInvalidFileType(fileInfo.fileName());
                }
                else if (mNonOriginController->wasAdded(exePath))
                {
                    mViewController->onGameAlreadyExists(fileInfo.fileName());
                }
                else
                {
                    validExecutables.push_back(exePath);
                }
            }

            if (!validExecutables.isEmpty())
            {
                if (mOperation == OpAdd)
                {
                    mNonOriginController->addContent(validExecutables);
                }
                else if (mOperation == OpRepair && !mGame.isNull())
                {
                    Origin::Engine::Content::NonOriginGameData gameData;
                    gameData.setDisplayName(mGame->contentConfiguration()->displayName());
                    gameData.setExecutableFile(validExecutables.front());
                    gameData.setIgoEnabled(mGame->contentConfiguration()->igoPermission() == Services::Publishing::IGOPermissionSupported);
                    onPropertiesChanged(gameData, false);
                }
            }
        }

        stop();

    }

    bool NonOriginGameFlow::isValidExecutablePath(const QString& path)
    {
        QFileInfo fileInfo(path);

        bool isOriginApp;
        
        //Not allowed to add Origin as non-Origin content
#ifdef ORIGIN_MAC
        if(fileInfo.isBundle())
        {
            // Check the bundle name
            isOriginApp = (fileInfo.bundleName().compare("Origin", Qt::CaseInsensitive) == 0);
        }
        else
        {
            // Make sure it's not the explicit path to the Origin exe in the bundle
            isOriginApp = (path.endsWith("Contents/MacOS/Origin", Qt::CaseInsensitive));
        }
#else
        isOriginApp = fileInfo.fileName().compare("origin.exe", Qt::CaseInsensitive) == 0;
#endif
        
        bool isRunnable = fileInfo.isExecutable() || fileInfo.isBundle();
        bool exists = fileInfo.exists();

        return !isOriginApp && isRunnable && exists;
    }

    void NonOriginGameFlow::onScanComplete()
    {
        // TODO
        // nonOriginContentController->getScannedContent();
    }

    void NonOriginGameFlow::cancel()
    {
        NonOriginGameFlowResult result;
        result.result = FLOW_FAILED;
        emit finished(result);
    }
    
    void NonOriginGameFlow::stop(bool showMyGames)
    {
		if (showMyGames)
		{
			// Call this here so that we always show My Games after adding a NOG
			ClientFlow::instance()->showMyGames();
		}
        NonOriginGameFlowResult result;
        result.result = FLOW_SUCCEEDED;
        emit finished(result);
    }

}
}
