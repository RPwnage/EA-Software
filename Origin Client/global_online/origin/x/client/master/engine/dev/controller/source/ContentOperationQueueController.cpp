/////////////////////////////////////////////////////////////////////////////
// ContentOperationQueueController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "ContentOperationQueueController.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/PlayFlow.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformJumplist.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

#define EXTRA_CONTENT_INITIAL_LOADCHECK_INTERVAL	1000	// one second between checks to see if extra content is done loading
#define EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MIN		15		// minimum number of checks to do before timing out while waiting to load extra content needed for restoring queue
#define EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MAX		120		// maximum number of checks to do before timing out while waiting to load extra content needed for restoring queue

namespace Origin
{
namespace Engine
{
namespace Content
{
	// We are using ':' in our product id string, so we cannot use it as a separator
	const char *kProductID_Separator = ";";	// not allowed in windows or mac filenames


ContentOperationQueueController* ContentOperationQueueController::create(Origin::Engine::UserWRef user)
{
   return new ContentOperationQueueController(user);
}


ContentOperationQueueController* ContentOperationQueueController::currentUserContentOperationQueueController()
{
    // it is possible for the user to be gone so check first to avoid a crash.
    Origin::Engine::UserRef user = Origin::Engine::LoginController::currentUser();
    if (user.isNull())
        return NULL;

    return user->contentOperationQueueControllerInstance();
}


ContentOperationQueueController::ContentOperationQueueController(Origin::Engine::UserWRef user)
: QObject(NULL)
, mUser(user)
, mIsQueueInitialized(false)
, mSignal_HeadBusy(false)
, mExtraContentLoadTimer(NULL)
, mExtraContentCheckCounter(0)
{
	mInitialQueueFromSettings.clear();
    mPushToTop_InProgress = Engine::Content::EntitlementRef();

	mExtraContentLoadTimer = new QTimer(this);
	mExtraContentLoadTimer->setInterval(EXTRA_CONTENT_INITIAL_LOADCHECK_INTERVAL);	// check once a second
	mExtraContentLoadTimer->stop();
	ORIGIN_VERIFY_CONNECT(mExtraContentLoadTimer, SIGNAL(timeout()), this, SLOT(onInitialContentLoadCheck()));
}


ContentOperationQueueController::~ContentOperationQueueController()
{
	mInitialQueueFromSettings.clear();

	if (mExtraContentLoadTimer)
	{
		mExtraContentLoadTimer->stop();
		ORIGIN_VERIFY_DISCONNECT(mExtraContentLoadTimer, SIGNAL(timeout()), this, SLOT(onInitialContentLoadCheck()));

		delete mExtraContentLoadTimer;
		mExtraContentLoadTimer = NULL;
	}
}

void ContentOperationQueueController::UpdateQueueSetting()
{
	// build list of product ids of current queue
	QStringList queue_list;

	QList<Engine::Content::EntitlementRef>::const_iterator it, end;
	for (it = mQueue.begin(); it != mQueue.end();it++)
	{
		queue_list = queue_list << (*it)->contentConfiguration()->productId();
	}

	QString queue = queue_list.join(kProductID_Separator);
	ORIGIN_LOG_DEBUG << "Download Queue string: " << queue;
	Origin::Services::writeSetting(Origin::Services::SETTING_DownloadQueue, queue, Origin::Engine::LoginController::currentUser()->getSession());
}

// check to see if we have everything in the queue.
void ContentOperationQueueController::onInitialContentLoadCheck()
{
	int items_missing = 0;
	{
	QReadLocker locker(&mQueueLock);

	QStringList::const_iterator it, end;
	for(it = mInitialQueueFromSettings.begin(); it != mInitialQueueFromSettings.end();it++)
	{
		Origin::Engine::Content::EntitlementRef ent = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementOrChildrenByProductId((*it));

		if (ent == NULL)
		{
			items_missing++; 
		}
	}
	}

	if ((items_missing == 0) || (mExtraContentCheckCounter == 0))	// we got everything or we ran out of tries
	{
		if (items_missing)
		{
			ORIGIN_LOG_WARNING << " initial extra content load timeout. " << items_missing << " items not yet loaded.";
		}

		mExtraContentCheckCounter = 0;
		mExtraContentLoadTimer->stop();
	}
	else
	{
		mExtraContentCheckCounter--;

		ORIGIN_LOG_DEBUG << " queued extra content not finished loading " << items_missing << " items not yet loaded.  mExtraContentCheckCounter = " << mExtraContentCheckCounter;
	}
}

void ContentOperationQueueController::LoadInitialQueue()
{
	QString queue_str = Origin::Services::readSetting(Origin::Services::SETTING_DownloadQueue, Origin::Engine::LoginController::currentUser()->getSession());
	ORIGIN_LOG_DEBUG << "ContentOperationQueueController: Download Queue string from settings: " << queue_str;

	mIsQueueInitialized = false;
	mPushToTop_InProgress = Origin::Engine::Content::EntitlementRef();
	if (queue_str.length() > 0)
	{
		mInitialQueueFromSettings = queue_str.split(kProductID_Separator);

		// set give-up counter
		mExtraContentCheckCounter = mInitialQueueFromSettings.length() * 10;
		// set constraints
		if (mExtraContentCheckCounter > EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MAX)
			mExtraContentCheckCounter = EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MAX;
		else
		if (mExtraContentCheckCounter < EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MIN)
			mExtraContentCheckCounter = EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MIN;
	}
	else
	{
		mExtraContentCheckCounter = 0;
		mInitialQueueFromSettings.clear();
	}
}

void ContentOperationQueueController::onInitialEntitlementLoad()
{
	if (mExtraContentCheckCounter <= 0)	// make sure this value is set to something positive
		mExtraContentCheckCounter = EXTRA_CONTENT_INITIAL_LOAD_TIMEOUT_MIN;

	ORIGIN_LOG_DEBUG << "onInitialEntitlementLoad timer started. mExtraContentCheckCounter = " << mExtraContentCheckCounter;

	mExtraContentLoadTimer->start();
}

// rebuild the queue when entitlements complete initial loading
void ContentOperationQueueController::buildInitialQueue()
{
	QWriteLocker locker(&mQueueLock);

    if (mIsQueueInitialized)    // already initialized?
        return;

	ORIGIN_LOG_EVENT << "buildInitialQueue called";

	QStringList::const_iterator it, end;
	for(it = mInitialQueueFromSettings.begin(); it != mInitialQueueFromSettings.end();it++)
	{
		ORIGIN_LOG_DEBUG << "trying to enqueue: " << *it;
		Origin::Engine::Content::EntitlementRef ent = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementOrChildrenByProductId((*it));

		if (ent != NULL)
		{
			enqueue_internal(ent, true);	// enqueue, but don't kick off downloads from the head yet
		}
		else
		{
			ORIGIN_LOG_ERROR << " uh oh, this entitlement: " << *it << " was not loaded from the initial queue";
		}
	}

	// in case there where errors that removed items from the list
	UpdateQueueSetting();

	mIsQueueInitialized = true;
    emit initialized();
}

void ContentOperationQueueController::postFullRefresh()
{
    // with LARS 3.0, we now must wait for the full refresh (data load plus processing for things like suppressed content) before
    // building the initial queue.  in buildInitialQueue(), it will check if already initialized and exit.  so it is safe to call here. (5/22/15)
    buildInitialQueue();    

    QWriteLocker locker(&mQueueLock);

    // check the head to see if it is in the kEnqueued state.  if so, kick it off.  currently called when returning from offline
    if (mQueue.length() > 0)
    {
        QList<Engine::Content::EntitlementRef>::const_iterator it, end;
        for (it = mQueue.begin(); it != mQueue.end();it++)
        {
            if ((*it)->isSuppressed() || ((*it)->contentConfiguration()->masterTitleId().isEmpty() && (*it)->contentConfiguration()->isPDLC()))
            {
                ORIGIN_LOG_WARNING << "Detected in queue an entitlement " <<  (*it)->contentConfiguration()->productId() << " that is suppressed.  Skipping.";
            }
        }

        Engine::Content::EntitlementRef head_ent = head_internal();
        if (head_ent->localContent()->installFlow())
        {
            if ((head_ent->localContent()->installFlow()->getFlowState() == Origin::Downloader::ContentInstallFlowState::kEnqueued) ||
                ((head_ent->localContent()->installFlow()->getFlowState() == Origin::Downloader::ContentInstallFlowState::kPaused) && head_ent->localContent()->installFlow()->isAutoResumeSet()))
            {
                head_ent->localContent()->installFlow()->startTransferFromQueue();  // kick it off

                ORIGIN_LOG_EVENT << "postFullRefresh called - head item started";
            }
        }
    }
}

Engine::Content::EntitlementRef	ContentOperationQueueController::head_internal()
{
	if (mQueue.length() == 0)
		return EntitlementRef();
	return mQueue.front();
}

Engine::Content::EntitlementRef	ContentOperationQueueController::head()
{
	QReadLocker locker(&mQueueLock);

    return head_internal();
}

Engine::Content::EntitlementRef	ContentOperationQueueController::first_child_of(Engine::Content::EntitlementRef ent)
{
    QReadLocker locker(&mQueueLock);

    if (ent.isNull() || mQueue.length() == 0 || !mQueue.contains(ent))
        return EntitlementRef();

    int index = mQueue.indexOf(ent) + 1;    // assumes children always follow parent

    if (index < mQueue.length())
    {
        Engine::Content::EntitlementRef potential_child = mQueue.at(index);
        QList<EntitlementRef> parent_list = potential_child->parentList();
        if ((parent_list.length() > 0) && (parent_list.contains(ent)))
            return potential_child;
    }

    return EntitlementRef();
}

QList<Engine::Content::EntitlementRef> ContentOperationQueueController::children_of(Engine::Content::EntitlementRef ent)
{
    QReadLocker locker(&mQueueLock);
    return children_of_internal(ent);
}

// called when a title is trying to resume - check for if it is on the queue and the head
bool ContentOperationQueueController::resume(Origin::Engine::Content::EntitlementRef ent)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action: " << ent->contentConfiguration()->productId();

	if (!mIsQueueInitialized)
	{
		if (!mInitialQueueFromSettings.contains(ent->contentConfiguration()->productId()))
		{
			mInitialQueueFromSettings.append(ent->contentConfiguration()->productId());	// add it to the end
		}

		// if not initialized, tell caller that the resume can't start now.
	}
	else
	if (!mQueue.empty() && (head_internal() == ent))
		return true;

	return false;
}

void ContentOperationQueueController::enqueueAndTryToPushToTop(Engine::Content::EntitlementRef ent)
{
	ORIGIN_LOG_DEBUG << "Queue Action: " << ent->contentConfiguration()->productId();

    // first check if entitlement is in the completed queue
    if (indexInCompleted(ent) != -1)
    {
        // if so, enqueue will place it back in the download queue and pushToTop will move it to the front
        enqueue(ent,false);
        pushToTop(ent);
    }
    else
    {
        int position = indexInQueue(ent);
        if (position == -1) // not in queue?
        {
            ORIGIN_LOG_WARNING << "Trying to move to head but title not in Download or Complete queue: " << ent->contentConfiguration()->productId();
            enqueue(ent,false);
            pushToTop(ent);
        }
        else if (position != 0) // not already head
        {
            pushToTop(ent);
        }
        // if already at head of download queue, then do nothing
    }
}

void ContentOperationQueueController::enqueue(Engine::Content::EntitlementRef ent, bool just_queue)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action: " << ent->contentConfiguration()->productId();

	enqueue_internal(ent, just_queue);
}

void ContentOperationQueueController::enqueue_internal(Engine::Content::EntitlementRef ent, bool just_queue)
{
	if (!just_queue && !mIsQueueInitialized)
	{
		if (!mInitialQueueFromSettings.contains(ent->contentConfiguration()->productId()))
		{
			ORIGIN_LOG_DEBUG << "adding to mInitialQueueFromSettings: " << ent->contentConfiguration()->productId();
			mInitialQueueFromSettings.append(ent->contentConfiguration()->productId());	// add it to the end
		}

		// if not initialized yet, just pre-queue it and return
		return;
	}
	
	// If it was in the queue (maybe in completed) remove it.
    remove_internal(ent, true);

    if(ent->localContent() == NULL)
    {
        ORIGIN_LOG_ERROR << "Trying to enqueue an entitlement " << ent->contentConfiguration()->productId() << " local content is NULL.";
        return;
    }
    else if(ent->localContent()->installFlow() == NULL)
    {
        ORIGIN_LOG_ERROR << "Trying to enqueue an entitlement " << ent->contentConfiguration()->productId() << " install flow is NULL.";
        return;
    }

    if (ent->isSuppressed() || (ent->contentConfiguration()->masterTitleId().isEmpty() && ent->contentConfiguration()->isPDLC()))
    {
        ORIGIN_LOG_WARNING << "Trying to enqueue an entitlement " << ent->contentConfiguration()->productId() << " that is suppressed.  Skipping.";
        return;
    }

    if(ent->localContent()->installFlow()->operationType() == Downloader::kOperation_None)
    {
        ORIGIN_LOG_ERROR << "Trying to enqueue an entitlement " << ent->contentConfiguration()->productId() << " that doesn't have an operation type. See IContentInstallFlow::operationType().";
        return;
    }

    Origin::Downloader::ContentInstallFlowState currentState = ent->localContent()->installFlow()->getFlowState();
	if ((currentState != Origin::Downloader::ContentInstallFlowState::kEnqueued) && (currentState != Origin::Downloader::ContentInstallFlowState::kPaused) && (currentState != Origin::Downloader::ContentInstallFlowState::kInstalling))
	{
		ORIGIN_LOG_ERROR << "Trying to enqueue an entitlement " << ent->contentConfiguration()->productId() << " that is not in kEnqueued or kPaused or kInstalling state.  Current state: " << currentState.toString();
		return;
	}

	mQueue.push_back(ent);

	// check if items at the bottom of queue are paused due to unretryable errors
	int length = mQueue.length();
	int new_index = length - 1;
	for (int i = length - 2; i >= 0; i--)
	{
		if (mQueue.at(i)->localContent()->installFlow()->pausedForUnretryableError())
		{
			mQueue.swap(i, new_index);
			new_index = i;
		}
		else
			break;
	}
	UpdateQueueSetting();

    emit enqueued(ent);

    if (new_index == 0)
    {
        if(!just_queue)
        {
            ent->localContent()->installFlow()->startTransferFromQueue();
        }

		if (length > 1)
		{
			emit enqueued(mQueue.at(1));
			changeQueueHead(ent, mQueue.at(1));
		}
		else
			changeQueueHead(ent, EntitlementRef());
    }
    GetTelemetryInterface()->Metric_QUEUE_ENTITLMENT_COUNT(mQueue.count());
}

void ContentOperationQueueController::removeFromQueue(Engine::Content::EntitlementRef ent, const bool& removeChilren)
{
    QWriteLocker locker(&mQueueLock);

    ORIGIN_LOG_DEBUG << "Queue Action from queue: " << ent->contentConfiguration()->productId();

    removeFromQueue_internal(ent, removeChilren);
}

void ContentOperationQueueController::removeFromCompleted(Engine::Content::EntitlementRef ent)
{
    QWriteLocker locker(&mQueueLock);

    ORIGIN_LOG_DEBUG << "Queue Action from completed: " << ent->contentConfiguration()->productId();

    removeFromCompleted_internal(ent);
}

void ContentOperationQueueController::remove(Engine::Content::EntitlementRef ent, const bool& removeChilren)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action from all: " << ent->contentConfiguration()->productId();

	remove_internal(ent, removeChilren);
}

bool ContentOperationQueueController::removeFromQueue_internal(Engine::Content::EntitlementRef ent, const bool& removeChilren)
{
    if(ent.isNull() || ent->localContent() == NULL || ent->localContent()->installFlow() == NULL)
        return false;

    if(mQueue.contains(ent))
    {
        bool was_head = false,
             was_paused = false;

        if (mQueue.front() == ent)
        {
            was_head = true;
            was_paused = false;
            if (ent->localContent()->installFlow())
            {
                was_paused = ent->localContent()->installFlow()->lastFlowAction().flowState == Downloader::ContentInstallFlowState::kPaused;
            }
        }

        ent->localContent()->installFlow()->setEnqueuedAndPaused(false);

        mQueue.removeOne(ent);
        UpdateQueueSetting();

        if (was_head)
        {
            if (mQueue.isEmpty() == false)	// anything left in queue?
            {
                if (mQueue.front()->localContent()->installFlow())
				{
	                if(was_paused == false)
                        mQueue.front()->localContent()->installFlow()->startTransferFromQueue();	// get it started
                    else
                        mQueue.front()->localContent()->installFlow()->setEnqueuedAndPaused(true);
                }
                changeQueueHead(mQueue.front(), ent);
            }
            else
            {
                changeQueueHead(EntitlementRef(), ent);
            }
        }
        emit removed(ent->contentConfiguration()->productId(), removeChilren);
        return true;
    }
    return false;
}

bool ContentOperationQueueController::removeFromCompleted_internal(Engine::Content::EntitlementRef ent)
{
    if(mCompleted.contains(ent))
    {
        mCompleted.removeOne(ent);
        emit removed(ent->contentConfiguration()->productId(), false);
        return true;
    }
    return false;
}

bool ContentOperationQueueController::remove_internal(Engine::Content::EntitlementRef ent, const bool& removeChilren)
{
    return removeFromQueue_internal(ent, removeChilren) || removeFromCompleted_internal(ent);
}

void ContentOperationQueueController::dequeue(Engine::Content::EntitlementRef ent)
{
	QWriteLocker locker(&mQueueLock);

	if(mQueue.length() && mQueue.contains(ent))	// don't dequeue if not in the queue (fixes OFM-1264)
    {
        Engine::Content::EntitlementRef oldHead = mQueue.front();

		ORIGIN_LOG_DEBUG << "Queue Action: " << oldHead->contentConfiguration()->productId();
		
		if (oldHead == ent)
		{
			mQueue.pop_front();
			mCompleted.push_back(oldHead);
			emit addedToComplete(oldHead);
			UpdateQueueSetting();

			if (mQueue.isEmpty() == false)	// anything left in queue?
			{
                if (mQueue.front()->localContent()->installFlow())
                {
                    mQueue.front()->localContent()->installFlow()->startTransferFromQueue();	// get it started
                }
				changeQueueHead(mQueue.front(), oldHead);
			}
			else
			{
				changeQueueHead(EntitlementRef(), oldHead);
			}
		}
		else
		{
			ORIGIN_LOG_WARNING << "Trying to Dequeue " << ent->contentConfiguration()->productId() << " but it is not the head.(" << oldHead->contentConfiguration()->productId() << ")";

			mQueue.removeOne(ent);
			mCompleted.push_back(ent);
			emit addedToComplete(ent);
			UpdateQueueSetting();
		}
    }
    emit headBusy(false);
}

// for adding initial entitlement to completed queue when ready to install 
void ContentOperationQueueController::addToCompleted(Origin::Engine::Content::EntitlementRef ent)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action: " << ent->contentConfiguration()->productId();

	// If it was in the queue (maybe in completed) remove it.
	remove_internal(ent, true);
#if 0
    if (!ent->localContent->installFlow())
        return;

	Origin::Downloader::ContentInstallFlowState currentState = ent->localContent()->installFlow()->getFlowState();
	if (currentState != Origin::Downloader::ContentInstallFlowState::kReadyToInstall)
	{
		ORIGIN_LOG_ERROR << "Trying to add an entitlement to the completed list " << ent->contentConfiguration()->productId() << " that is not in the kReadyToInstall state.  Current state: " << currentState.toString();
		return;
	}
#endif
	mCompleted.push_back(ent);
	emit addedToComplete(ent);
}

/// \returns Returns if the head entitlement can't be moved in queue.
bool ContentOperationQueueController::isHeadBusy()
{
    QReadLocker locker(&mQueueLock);

    return isHeadBusy_internal();
}

bool ContentOperationQueueController::isHeadBusy_internal()
{
    if(mQueue.length() == 0)
        return false;
    if(!head_internal().isNull() && head_internal()->localContent() && head_internal()->localContent()->installFlow())
    {
        Downloader::IContentInstallFlow* installFlow = head_internal()->localContent()->installFlow();
        if(installFlow->isDynamicDownload() && head_internal()->localContent()->playing())
            return true;
        if(Services::Connection::ConnectionStatesService::isUserOnline(mUser.toStrongRef()->getSession()) == false)
            return false;
        return (installFlow->canPause() || installFlow->canResume()) == false;
    }
    return false;
}

QList<Engine::Content::EntitlementRef> ContentOperationQueueController::entitlementsQueued()
{
	QReadLocker locker(&mQueueLock);

    return mQueue;
}


QList<Engine::Content::EntitlementRef> ContentOperationQueueController::entitlementsCompleted()
{
	QReadLocker locker(&mQueueLock);

    return mCompleted;
}

bool ContentOperationQueueController::pushToTop(Origin::Engine::Content::EntitlementRef entitlement)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action: " << entitlement->contentConfiguration()->productId();

	return pushToTop_internal(entitlement);
}

bool ContentOperationQueueController::pushToTop(const QString& productId)
{
	QWriteLocker locker(&mQueueLock);

	ORIGIN_LOG_DEBUG << "Queue Action: " << productId;

    return pushToTop_internal(ContentController::currentUserContentController()->entitlementByProductId(productId, AllContent));
}

bool ContentOperationQueueController::pushToTop_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
    if (queueSkippingEnabled_internal(entitlement))
    {
        Origin::Engine::Content::EntitlementRef oldHead = head_internal();	// head_internal() is guarded against an empty queue by checks in queueSkippingEnabled_internal()

		if (oldHead->localContent()->installFlow() && oldHead->localContent()->installFlow()->canPause())
		{
			mPushToTop_InProgress = oldHead;	// flag so old head can finish pausing before we start new head's actions (also prevents other items from being placed on top)
			ORIGIN_LOG_DEBUG << "Connecting to paused " << oldHead->contentConfiguration()->productId();

			ORIGIN_VERIFY_CONNECT_EX(oldHead->localContent()->installFlow(), SIGNAL(paused()), this, SLOT(pushToTop_finalize()), Qt::QueuedConnection);

			oldHead->localContent()->installFlow()->pause(false);
		}
		else
			mPushToTop_InProgress = Origin::Engine::Content::EntitlementRef();

        //////// MOVE THE ELEMENT + CHILDREN
        // Get the group of the elements changing
        QList<Origin::Engine::Content::EntitlementRef> newHeads;
        // Only look for the similar entitlements if it's a parent entitlement. OFM-4359, OFM-4437
        if(entitlement->parent().isNull() || indexInQueue_internal(entitlement->parent()) != -1)
            newHeads = baseGroup_internal(entitlement);
        else
            newHeads.append(entitlement);

        for(int i = 0; i < newHeads.count(); i++)
        {
            // Remove the group we moving (new heads)
            mQueue.removeOne(newHeads[i]);
        }

        // Append the new head to the front of the list
        mQueue = newHeads + mQueue;

        //// THIS IS THE UPDATING UI PART:
        // Add the old head group to the new head because we need to emit removed and enqueued to update the UI
        newHeads += baseGroup_internal(oldHead);

        // WE HAVE TO REMOVE ALL THE ELEMENTS BEFORE WE CAN ADD THEM BACK. If not, the UI will be confused
        for(int i = 0; i < mQueue.count(); i++)
        {
            // Remove the UI element, but don't enqueue the next on the list.
            emit removed(mQueue[i]->contentConfiguration()->productId(), true, false);
        }

        // Tell the UI to add elements at the respected indexes.
        for(int i = 0; i < mQueue.count(); i++)
        {
            emit enqueued(mQueue[i]);
        }
        ////
        ////////
        
		UpdateQueueSetting();

        // Tell other things there has been a change
        changeQueueHead(entitlement, oldHead);

		if (mPushToTop_InProgress.isNull())
        {
			pushToTop_finalize_internal();
        }

        return true;
    }
	else if ((head_internal() != entitlement) && (mQueue.length() > 0))
	{	// if the head is busy, move to the second place in line
		if (// entitlement is in the list
			(indexInQueue_internal(entitlement) != -1)
			// entitlement isn't already in second place
			&& (indexInQueue_internal(entitlement) != 1))
		{
			// TODO: MOVE CHILDREN
			mQueue.move(indexInQueue_internal(entitlement), 1);
			UpdateQueueSetting();

			// A pseudo remove. As far as the UI is concerned we removed the entitlement and enqueued it in a different position
			emit removed(entitlement->contentConfiguration()->productId(), true);
			emit enqueued(entitlement);
			return true;
		}

        LocalContent* lc = (entitlement.isNull() == false && entitlement->localContent() && entitlement->localContent()) ? entitlement->localContent() : NULL;
        const Downloader::ContentOperationType opt_type = (lc && lc->installFlow()) ? lc->installFlow()->operationType() : Downloader::kOperation_None;
        if((lc && lc->playFlow() && lc->playFlow().data()->isRunning())
            || (opt_type == Downloader::kOperation_Install || opt_type == Downloader::kOperation_ITO))
        {
            emit triedToAddGameWhileBusy(mQueue.front(), entitlement);
        }
	}

	return false;
}

void ContentOperationQueueController::pushToTop_finalize()
{
	QReadLocker locker(&mQueueLock);

	pushToTop_finalize_internal();
}

void ContentOperationQueueController::pushToTop_finalize_internal()
{
	if ((mQueue.length() > 1) && (!mPushToTop_InProgress.isNull()))
	{
		Origin::Engine::Content::EntitlementRef oldHead = mPushToTop_InProgress;

		ORIGIN_LOG_DEBUG << "Disconnecting from paused " << oldHead->contentConfiguration()->productId();

        if (oldHead->localContent()->installFlow())
        {
		    ORIGIN_VERIFY_DISCONNECT(oldHead->localContent()->installFlow(), SIGNAL(paused()), this, SLOT(pushToTop_finalize()));
        }
	}

    mPushToTop_InProgress = Origin::Engine::Content::EntitlementRef();

    if (Origin::Services::Connection::ConnectionStatesService::isUserOnline(mUser.toStrongRef()->getSession()) && (mQueue.length() > 0))
    {
        Origin::Engine::Content::EntitlementRef entitlement = head_internal();

        if (entitlement->localContent()->installFlow())
        {
            // If this entitlement has progress - resume. Otherwise, take it out of enqueued and start the transfer 
            if (entitlement->localContent()->installFlow()->canResume() && (entitlement->localContent()->installFlow()->getFlowState() != Downloader::ContentInstallFlowState::kEnqueued) )
            {
                entitlement->localContent()->installFlow()->resume();
            }
            else
            {
                entitlement->localContent()->installFlow()->startTransferFromQueue();	// get it started
            }
        }
    }
}

void ContentOperationQueueController::moveToBottom(Origin::Engine::Content::EntitlementRef entitlement)
{
	QWriteLocker locker(&mQueueLock);

	int length = mQueue.length();

	ORIGIN_LOG_DEBUG << "Queue Action: " << entitlement->contentConfiguration()->productId();

	if ((length > 1) && mQueue.contains(entitlement))
	{
		mQueue.move(indexInQueue_internal(entitlement), length - 1);
		UpdateQueueSetting();

		Origin::Engine::Content::EntitlementRef newHead = head_internal();

		// A pseudo remove. As far as the UI is concerned we removed the entitlement and enqueued it in a different position
		emit removed(entitlement->contentConfiguration()->productId(), true);
		emit enqueued(entitlement);
		emit enqueued(newHead);
		changeQueueHead(newHead, entitlement);
        if (newHead->localContent()->installFlow())
        {
            // If this entitlement has progress - resume. Otherwise, take it out of enqueued and start the transfer 
            if (newHead->localContent()->installFlow()->canResume() && (newHead->localContent()->installFlow()->getFlowState() != Downloader::ContentInstallFlowState::kEnqueued) )
            {
                if (!newHead->localContent()->installFlow()->pausedForUnretryableError())
                {
                    newHead->localContent()->installFlow()->resume();
                }
            }
            else
            {
                newHead->localContent()->installFlow()->startTransferFromQueue();	// get it started
            }
        }
    }
}

bool ContentOperationQueueController::queueSkippingEnabled(const QString& productId)
{
	QWriteLocker locker(&mQueueLock);

    return queueSkippingEnabled_internal(ContentController::currentUserContentController()->entitlementByProductId(productId, AllContent));
}

bool ContentOperationQueueController::queueSkippingEnabled(Origin::Engine::Content::EntitlementRef entitlement)
{
	QWriteLocker locker(&mQueueLock);

	return queueSkippingEnabled_internal(entitlement);
}

bool ContentOperationQueueController::queueSkippingEnabled_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
	if (!mPushToTop_InProgress.isNull())	// if we are not finished pushing something to the top don't allow anything to replace the top spot
		return false;

        // If we have a list
    return (mQueue.length()
        // entitlement is in the list
        && indexInQueue_internal(entitlement) != -1)
        // entitlement isn't the head already
        && head_internal() != entitlement
        // We are online (EBIBUGS-26948) or the game is trying to be played (EBIBUGS-26953)
        && (Services::Connection::ConnectionStatesService::isUserOnline(mUser.toStrongRef()->getSession()) || entitlement->localContent()->playFlow()->isRunning())
        // head isn't busy
        && isHeadBusy_internal() == false;
}

int ContentOperationQueueController::indexInQueue(Origin::Engine::Content::EntitlementRef entitlement)
{
	QReadLocker locker(&mQueueLock);

	return indexInQueue_internal(entitlement);
}

int ContentOperationQueueController::indexInQueue(const QString& productId)
{
	QReadLocker locker(&mQueueLock);

    return indexInQueue_internal(ContentController::currentUserContentController()->entitlementByProductId(productId, AllContent));
}

int ContentOperationQueueController::indexInQueue_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
    if(entitlement.isNull() == false)
        return mQueue.indexOf(entitlement);
    return -1;
}

int ContentOperationQueueController::indexInCompleted(Origin::Engine::Content::EntitlementRef entitlement)
{
	QReadLocker locker(&mQueueLock);

	return indexInCompleted_internal(entitlement);
}

int ContentOperationQueueController::indexInCompleted(const QString& productId)
{
	QReadLocker locker(&mQueueLock);

    return indexInCompleted_internal(ContentController::currentUserContentController()->entitlementByProductId(productId, AllContent));
}

int ContentOperationQueueController::indexInCompleted_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
	return mCompleted.indexOf(entitlement);
}


void ContentOperationQueueController::clearCompleteList()
{
	QWriteLocker locker(&mQueueLock);

    mCompleted.clear();
    emit completeListCleared();
}

void ContentOperationQueueController::clearDownloadQueue()
{
	QWriteLocker locker(&mQueueLock);

	mQueue.clear();
}

void ContentOperationQueueController::changeQueueHead(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead)
{
    emit headChanged(newHead, oldHead);
    if(oldHead.isNull() == false && oldHead->localContent() && oldHead->localContent()->installFlow())
    {
        ORIGIN_VERIFY_DISCONNECT(oldHead->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onHeadProgressUpdate()));
    }
    if(newHead.isNull() == false && newHead->localContent() && newHead->localContent()->installFlow() && mQueue.length())
    {
        ORIGIN_VERIFY_CONNECT(newHead->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onHeadProgressUpdate()));

        if (!Services::Connection::ConnectionStatesService::isUserOnline(mUser.toStrongRef()->getSession()))
        {
            newHead->localContent()->installFlow()->setAutoResumeFlag(true); // if changed to head while offline - set it to auto resume (EBIBUGS-27321)
        }
    }
    onHeadProgressUpdate_internal(isHeadBusy_internal());
}

void ContentOperationQueueController::onHeadProgressUpdate()
{
    onHeadProgressUpdate_internal(isHeadBusy());
}

void ContentOperationQueueController::onHeadProgressUpdate_internal(bool busy)
{
    if(mSignal_HeadBusy != busy)
    {
        mSignal_HeadBusy = busy;
        emit headBusy(mSignal_HeadBusy);
    }
}

QList<Engine::Content::EntitlementRef> ContentOperationQueueController::baseGroup_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
    QList<Engine::Content::EntitlementRef> ret;
    ret.append(entitlement);
    ret.append(children_of_internal(entitlement));
    return ret;
}

QList<Engine::Content::EntitlementRef> ContentOperationQueueController::children_of_internal(Origin::Engine::Content::EntitlementRef entitlement)
{
    QList<Engine::Content::EntitlementRef> ret;
    for (int i = indexInQueue_internal(entitlement) + 1; i < mQueue.count(); i++)
    {
        if (mQueue[i] && (mQueue[i]->parent() == entitlement || ((mQueue[i]->parent() == entitlement->parent()) && (!entitlement->parent().isNull()))))
        {
            ret.append(mQueue[i]);
        }
        else
        {
            break;
        }
    }
    return ret;
}

}
}
}
