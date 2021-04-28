/////////////////////////////////////////////////////////////////////////////
// ContentOperationQueueController.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CONTENTOPERATIONQUEUECONTROLLER_H
#define CONTENTOPERATIONQUEUECONTROLLER_H

#include <QObject>
#include "engine/content/Entitlement.h"

namespace Origin
{
namespace Engine
{
namespace Content
{

class ContentOperationQueueController : public QObject
{
    Q_OBJECT

public:
    static ContentOperationQueueController* create(Origin::Engine::UserWRef user);
    /// \returns ContentController Returns a pointer to the newly created entitlement controller.
    static ContentOperationQueueController* currentUserContentOperationQueueController();
    Engine::Content::EntitlementRef	head();

    /// \returns first child entitlement in queue of entitlement ent - null ref if not found
    Engine::Content::EntitlementRef	first_child_of(Engine::Content::EntitlementRef ent);
    QList<Engine::Content::EntitlementRef> children_of(Engine::Content::EntitlementRef ent);

    void enqueueAndTryToPushToTop(Engine::Content::EntitlementRef ent);
    void enqueue(Engine::Content::EntitlementRef ent, bool just_queue);
    /// \brief Remove entitlement from queue list if it is there.
    void removeFromQueue(Engine::Content::EntitlementRef ent, const bool& removeChilren);
    /// \brief Remove entitlement from completed list if it is there.
    void removeFromCompleted(Engine::Content::EntitlementRef ent);
    /// \brief Remove entitlement from whatever list it is in.
    void remove(Engine::Content::EntitlementRef ent, const bool& removeChilren);
    QList<Engine::Content::EntitlementRef> entitlementsQueued();
    QList<Engine::Content::EntitlementRef> entitlementsCompleted();

    /// \returns Pushes entitlement to top of the queue. Returns true if successful.
    bool pushToTop(const QString& productId);
    bool pushToTop(Origin::Engine::Content::EntitlementRef entitlement);

	void moveToBottom (Origin::Engine::Content::EntitlementRef entitlement);

    /// \returns Can this entitlement be pushed to the top of the queue?
    bool queueSkippingEnabled(const QString& productId);
    bool queueSkippingEnabled(Origin::Engine::Content::EntitlementRef entitlement);

    /// \returns Returns the index position item in the queue. Returns -1 if no item matched.
    int indexInQueue(const QString& productId);
    int indexInQueue(Origin::Engine::Content::EntitlementRef entitlement);

    /// \returns Returns the index position item in the completed list. Returns -1 if no item matched.
    int indexInCompleted(const QString& productId);
    int indexInCompleted(Origin::Engine::Content::EntitlementRef entitlement);

    // Clears items in completed list
    void clearCompleteList();
	void clearDownloadQueue();

	void onInitialEntitlementLoad();
	void LoadInitialQueue();	// load queue string list from settings when the first set of entitlements are loaded
	bool IsQueueInitialized() { return mIsQueueInitialized; }
	bool resume(Origin::Engine::Content::EntitlementRef entitlement);

	void addToCompleted(Origin::Engine::Content::EntitlementRef entitlement);

	void postFullRefresh();	// after a full refresh (login or returning from offline) kick off download at head if state is kEnqueued

    /// \returns Returns if the head entitlement can't be moved in queue.
    bool isHeadBusy();

public slots:
    void dequeue(Engine::Content::EntitlementRef ent);
	void pushToTop_finalize();

signals:
    void initialized();
    void enqueued(Origin::Engine::Content::EntitlementRef entitlement);
    void removed(const QString& productId, const bool& removeChildren, const bool& enqueueNext = true);
    void addedToComplete(Origin::Engine::Content::EntitlementRef entitlement);
    void completeListCleared();
    void headChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead);
    void headBusy(const bool& isBusy);
    void triedToAddGameWhileBusy(Origin::Engine::Content::EntitlementRef head, Origin::Engine::Content::EntitlementRef gameTriedToAdd);


private slots:
    void onHeadProgressUpdate();
	void onInitialContentLoadCheck();

private:
    //struct
    //{
    //    Engine::Content::EntitlementRef mEnt;
    //    const ContentOperation mOperation;
    //};
    ContentOperationQueueController(Origin::Engine::UserWRef user);
    ~ContentOperationQueueController();

	void UpdateQueueSetting();
	void enqueue_internal(Engine::Content::EntitlementRef ent, bool just_queue);	// used internally so we can lock it
    bool remove_internal(Engine::Content::EntitlementRef ent, const bool& removeChilren);
    bool removeFromQueue_internal(Engine::Content::EntitlementRef ent, const bool& removeChilren);
	bool removeFromCompleted_internal(Engine::Content::EntitlementRef ent);
	bool pushToTop_internal(Origin::Engine::Content::EntitlementRef entitlement);
	void pushToTop_finalize_internal();
	int indexInQueue_internal(Origin::Engine::Content::EntitlementRef entitlement);
	int indexInCompleted_internal(Origin::Engine::Content::EntitlementRef entitlement);
	Engine::Content::EntitlementRef	head_internal();
	bool queueSkippingEnabled_internal(Origin::Engine::Content::EntitlementRef entitlement);
    void changeQueueHead(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead);
    bool isHeadBusy_internal();
    void onHeadProgressUpdate_internal(bool busy);
    QList<Engine::Content::EntitlementRef> baseGroup_internal(Origin::Engine::Content::EntitlementRef entitlement);
    QList<Engine::Content::EntitlementRef> children_of_internal(Origin::Engine::Content::EntitlementRef entitlement);

	void buildInitialQueue();

    Engine::UserWRef mUser;
    QList<Engine::Content::EntitlementRef> mQueue;
    QList<Engine::Content::EntitlementRef> mCompleted;
	QStringList mInitialQueueFromSettings;
	bool mIsQueueInitialized;
	Engine::Content::EntitlementRef mPushToTop_InProgress;	// flag so nothing will interfere with the old head pausing before kicking off new head's action
    bool mSignal_HeadBusy; // flag to stop us from spamming headBusy() signal

	QTimer *mExtraContentLoadTimer;	// timer to wait for loading of extra content and check again
	int mExtraContentCheckCounter;	// number of intervals to wait before giving up and just initialize the queue

	QReadWriteLock mQueueLock;	// to make sure queue isn't accessed by multiple threads at the same time
};

}
}
}
#endif