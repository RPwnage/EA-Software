#include "services/downloader/Common.h"
#include "services/debug/DebugService.h"
#include "engine/downloader/ContentServices.h"
#include "engine/downloader/ContentInstallFlowDiP.h"
#include "DiPManifest.h"
#include "engine/downloader/DiPUpdate.h"
#include "ContentProtocolPackage.h"

#include "services/log/LogService.h"

#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"

#include <QFinalState>
#include <QMutexLocker>
#include <QThreadPool>

namespace
{
    /// \brief Performs basic NULL pointer checks on members of the given ContentInstallFlowDiP object.
    ///
    /// May not necessarily properly define what "valid" is for any given situation.
    ///
    /// \return True if the given ContentInstallFlowDiP is in a valid/usable state, false otherwise.
    const bool dipFlowValid(const Origin::Downloader::ContentInstallFlowDiP* dipFlow)
    {
        return dipFlow 
            && dipFlow->getContent() 
            && dipFlow->getDiPManifest() 
            && dipFlow->getProtocol();
    }
}

namespace Origin
{
namespace Downloader
{
	DiPUpdate::DiPUpdate(ContentInstallFlowDiP* parent) : QStateMachine(parent),
		mDipFlow(parent),
		mInstallerChanged(false),
        mCanCancel(true)
	{
		InitializeUpdateStateMachine();
	}

    DiPUpdate::~DiPUpdate()
    {
        // If we had an open content watcher, signal it to die
        if (!mContentWatcherKillSwitch.isNull())
        {
            // Set the kill switch before acquiring the lock
            mContentWatcherKillSwitch->kill();

            // Acquire the lock to prevent a race between test (isKilled()) and use
            QMutexLocker killSwitchLock(&mContentWatcherKillSwitch->getLock());
        }
    }

	void DiPUpdate::InitializeUpdateStateMachine()
	{
		QState* eligibilityCheck = new QState(this);
		ORIGIN_VERIFY_CONNECT(eligibilityCheck, SIGNAL(entered()), this, SLOT(onEligibilityCheck()));

		QState* pendingEulaInput = new QState(this);
		ORIGIN_VERIFY_CONNECT(pendingEulaInput, SIGNAL(entered()), this, SLOT(onPendingEulaInput()));

		QState* finalizeUpdate = new QState(this);
		ORIGIN_VERIFY_CONNECT(finalizeUpdate, SIGNAL(entered()), this, SLOT(onFinalizeUpdate()));

		QState* paused = new QState(this);
		ORIGIN_VERIFY_CONNECT(paused, SIGNAL(entered()), this, SLOT(onPaused()));

		QFinalState* complete = new QFinalState(this);
        ORIGIN_VERIFY_CONNECT(complete, SIGNAL(entered()), this, SLOT(onComplete()));
        
		setInitialState(eligibilityCheck);

		eligibilityCheck->addTransition(this, SIGNAL(eligibleForUpdate()), pendingEulaInput);
		eligibilityCheck->addTransition(this, SIGNAL(pauseUpdate()), paused);
		paused->addTransition(this, SIGNAL(resumeUpdate()), eligibilityCheck);
		pendingEulaInput->addTransition(mDipFlow, SIGNAL(eulaStateSet()), finalizeUpdate);
		pendingEulaInput->addTransition(this, SIGNAL(eulaInputReceived()), finalizeUpdate);
        finalizeUpdate->addTransition(mDipFlow, SIGNAL(dynamicDownloadRequiredChunksComplete()), complete);
		finalizeUpdate->addTransition(mDipFlow->getProtocol(), SIGNAL(Finalized()), complete);

		ORIGIN_VERIFY_CONNECT(mDipFlow->getProtocol(), SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo) ), this, SLOT(onFlowError()));
	}

	bool DiPUpdate::InstallerChanged() const
	{
		return mInstallerChanged;
	}

    bool DiPUpdate::CanCancel()
    {
        QMutexLocker lock(&mMutex);
        return mCanCancel;
    }

	void DiPUpdate::onFlowError()
	{
		stop();
		mCanCancel = true;
	}

	void DiPUpdate::SetContentInactive()
	{
        ORIGIN_LOG_EVENT << "[Entitlement:" << mDipFlow->getContent()->contentConfiguration()->productId() << "] DiPUpdate resuming as content is no longer active.";
		mDipFlow->getInstallManifest().SetBool("paused", false);
		ContentServices::SaveInstallManifest(mDipFlow->getContent(), mDipFlow->getInstallManifest());

		emit resumeUpdate();
	}

    void DiPUpdate::onComplete()
    {
        QMutexLocker lock(&mMutex);
        mCanCancel = true;
    }

	void DiPUpdate::onEligibilityCheck()
	{
        // Determine if any of the contents that share this master title ID are playing (to account for standard and limited edition, for example)
        bool relatedPlaying = false;

        // Lets check only for whether the update is for a basegame. DLC should be updatable. 
        if (mDipFlow->getContent()->contentConfiguration()->isBaseGame())
        {
            Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if (contentController)
            {
                Origin::Engine::Content::EntitlementRefList ents = contentController->entitlementsByMasterTitleId(mDipFlow->getContent()->contentConfiguration()->masterTitleId());
                foreach(Origin::Engine::Content::EntitlementRef ent, ents)
                {
                    if (ent->contentConfiguration()->isBaseGame() && ent->localContent())
                    {
                        bool playing = ent->localContent()->state().testFlag(Origin::Engine::Content::LocalContent::Playing);
                        if (playing)
                        {
                            ORIGIN_LOG_EVENT << "[Entitlement:" << mDipFlow->getContent()->contentConfiguration()->productId() << "] DiPUpdate must wait while content is playing: " << ent->contentConfiguration()->productId();
                        }
                        relatedPlaying |= playing;
                    }
                }
            }
        }

        if (relatedPlaying)
		{
			emit pauseUpdate();
		}
		else
		{
            Parameterizer installManifest;
		    ContentServices::LoadInstallManifest(mDipFlow->getContent(), installManifest);
		    installManifest.SetBool("installerChanged", false);
		    ContentServices::SaveInstallManifest(mDipFlow->getContent(), installManifest);

			emit eligibleForUpdate();
		}
	}

	void DiPUpdate::onPendingEulaInput()
	{
        // If the DiP flow itself is NULL, or some of its members that we require are NULL,
        // silently exit this state machine transition. Note that the state machine will 
        // effectively end here. We assume that if the DiPFlow is in a NULL state at this
        // point, then the user is likely exiting or logging out.
        if (!dipFlowValid(mDipFlow))
        {
            ORIGIN_LOG_EVENT << "Dropping DiPUpdate state transitions";

            return;
        }

        mEulaStateRequest.productId = mDipFlow->getContent()->contentConfiguration()->productId();
		mEulaStateRequest.eulas.listOfEulas.clear();

        //PDLC also goes this through this flow, so lets make sure that this is an update
        //before showing the EULA at the end, otherwise PDLC with EULAs will show at the beginning and end
        if(mDipFlow->isUpdate())
        {
		    for (EulaInfoList::const_iterator i = mDipFlow->getDiPManifest()->GetEulas(mDipFlow->currentLanguage()).begin();
			    i != mDipFlow->getDiPManifest()->GetEulas(mDipFlow->currentLanguage()).end(); i++)
		    {
                quint32 fileSignature = 0;
			    QString stagedPath;
                if (mDipFlow->getProtocol()->IsStaged(i->mFileName, stagedPath, fileSignature))
			    {
                    QString eulaStrName = mDipFlow->getCleanEulaID(i->mFileName);
                    quint32 eulaSignature = (quint32)mDipFlow->getInstallManifest().Get64BitInt(eulaStrName);

                    if (eulaSignature == fileSignature)
                    {
                        ORIGIN_LOG_EVENT << "(Update) Skipping already accepted EULA: " << i->mFileName << " (" << eulaSignature << ")";
                        continue;
                    }

                    // The new EULA dialog can read in anything. This tells us whether 
                    // or not we should use our old RTF dialog. By the time it gets to the
                    // UI layer the file is a temp file.
                    const bool isRtfFile = i->mFileName.contains(".rtf"); 
				    Eula eula(i->mName, stagedPath, isRtfFile, false);
				    eula.IsOptional = false;
				    mEulaStateRequest.eulas.listOfEulas.push_front(eula);
			    }
		    }
        }

		if (!mEulaStateRequest.eulas.listOfEulas.isEmpty())
		{
            mEulaStateRequest.isUpdate = true;
            mEulaStateRequest.eulas.isAutoPatch = "1";
			emit pendingUpdatedEulas(mEulaStateRequest);
		}
		else
		{
			emit eulaInputReceived();
		}
	}

	void DiPUpdate::onFinalizeUpdate()
	{
        QMutexLocker lock(&mMutex);
        mCanCancel = false;

        qint64 nChangedFileCount = mDipFlow->getProtocol()->stagedFileCount();
        mDipFlow->getInstallManifest().Set64BitInt("stagedFileCount", nChangedFileCount);
		mDipFlow->getInstallManifest().SetBool("eulasaccepted", true);

        quint32 unused = 0;
		QString stagedInstaller;
		QString archiveInstallerPath = mDipFlow->getDiPManifest()->GetInstallerPath();
        mInstallerChanged = mDipFlow->getProtocol()->IsStaged(archiveInstallerPath, stagedInstaller, unused);
        mDipFlow->getInstallManifest().SetBool("installerChanged", mInstallerChanged);

        ContentServices::SaveInstallManifest(mDipFlow->getContent(), mDipFlow->getInstallManifest());

        emit finalizeUpdate();
	}

	void DiPUpdate::onCancel()
	{
        ORIGIN_LOG_EVENT << "DiPUpdate for Entitlement " << mDipFlow->getContent()->contentConfiguration()->productId() << " was canceled.";
        
        if (isRunning())
		    stop();
	}

	void DiPUpdate::onPaused()
	{
        ORIGIN_LOG_EVENT << "[Entitlement:" << mDipFlow->getContent()->contentConfiguration()->productId() << "] DiPUpdate paused because user is actively playing the content.";
		Parameterizer installState;
		ContentServices::LoadInstallManifest(mDipFlow->getContent(), installState);
		installState.SetBool("paused", true);
		ContentServices::SaveInstallManifest(mDipFlow->getContent(), installState);

        // Tell the DiP flow to emit a signal indicating that we're waiting
        mDipFlow->onPendingContentExit();

        // Dispose of any existing kill switch and create a new one
        if (!mContentWatcherKillSwitch.isNull())
        {
            mContentWatcherKillSwitch->kill();
        }
        mContentWatcherKillSwitch = QSharedPointer<ActiveContentWatcherKillSwitch>(new ActiveContentWatcherKillSwitch());

		ActiveContentWatcher* watcher = new ActiveContentWatcher(this, mContentWatcherKillSwitch, mDipFlow->getContent());
		watcher->setAutoDelete(true);
		QThreadPool::globalInstance()->start(watcher);
	}

    ActiveContentWatcherKillSwitch::ActiveContentWatcherKillSwitch() :
        mKilled(false)
    {
    }

    ActiveContentWatcherKillSwitch::~ActiveContentWatcherKillSwitch()
    {
    }

    void ActiveContentWatcherKillSwitch::kill()
    {
        mKilled = true;
    }

    bool ActiveContentWatcherKillSwitch::isKilled()
    {
        return mKilled;
    }

    QMutex& ActiveContentWatcherKillSwitch::getLock()
    {
        return mLock;
    }

    DiPUpdate::ActiveContentWatcher::ActiveContentWatcher(DiPUpdate* parent, QSharedPointer<ActiveContentWatcherKillSwitch> killSwitch, const Origin::Engine::Content::EntitlementRef content) :
		mParent(parent),
        mKillSwitch(killSwitch),
		mContent(content)
	{

	}

	void DiPUpdate::ActiveContentWatcher::run()
	{
		while (1)
		{
            // Acquire a lock to make sure we prevent a test vs. use race condition
            QMutexLocker killSwitchLock(&mKillSwitch->getLock());

            // If we were killed by the parent, bail now
            if (mKillSwitch->isKilled())
                return;

            Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if (!contentController)
                return;

            // Determine if any of the contents that share this master title ID are playing (to account for standard and limited edition, for example)
            bool relatedPlaying = false;
            Origin::Engine::Content::EntitlementRefList ents = contentController->entitlementsByMasterTitleId(mContent->contentConfiguration()->masterTitleId());
            foreach(Origin::Engine::Content::EntitlementRef ent, ents)
            {
                if(ent->contentConfiguration()->isBaseGame() && ent->localContent())
                {
                    relatedPlaying |= ent->localContent()->state().testFlag(Origin::Engine::Content::LocalContent::Playing);
                }
            }

            if (relatedPlaying && mParent->isRunning())
            {
                // Pause for a little bit, user is playing the game and may take awhile so we don't 
                // want to loop too frequently here.
                // Release the lock while we sleep
                killSwitchLock.unlock();
			    Services::PlatformService::sleep(2000);
                continue;
            }

            // The content is no longer running, see if the DiPUpdate flow still is
            if (mParent->isRunning())
            {
		        mParent->SetContentInactive();
            }

            break;
		}
	}

} // namespace Downloader
} // namespace Origin

