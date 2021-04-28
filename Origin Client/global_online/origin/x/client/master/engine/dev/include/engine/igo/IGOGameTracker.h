//
//  IGOGameTracker.h
//  engine
//
//  Created by Frederic Meraud on 3/4/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef engine_IGOGameTracker_h
#define engine_IGOGameTracker_h

#if defined(ORIGIN_PC)
#define NOMINMAX
#endif

#include <QUrl>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QElapsedTimer>

#include "EASTL/hash_map.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{

// Fwd decls
class IGOController;
class IGOWindowManager;
class IGOWindowManagerIPC;
    
// Manages the info associated with a specific game instance currently running.
class ORIGIN_PLUGIN_API IGOGameTracker : public QObject
{
    Q_OBJECT
    
public:
    // Game launch info as seen from Content manager
    struct ORIGIN_PLUGIN_API BaseGameLaunchInfo
    {
        BaseGameLaunchInfo()
        {
            mForceKillAtOwnershipExpiry = false;
            mIsUnreleased = false;
            mIsTrial = false;
            mTimeRemaining = -1;

            mRefCount = 0;
        }
        
        QString mProductID;
        QString mInstallDir;
        QString mExecutable;
        QString mExecutablePath;
        QString mBundleId;           // MAC specific bundle identifier
        QString mTitle;
        QString mMasterTitle;
        QString mAchievementSetID;
        QString mLocale;
        
        uint32_t mRefCount;

        bool mForceKillAtOwnershipExpiry;
        bool mIsUnreleased;
        bool mIsTrial;
        qint64 mTimeRemaining;
    };

    // Details of a specific game.
    struct ORIGIN_PLUGIN_API GameLaunchInfo
    {
        GameLaunchInfo()
        {
            mElapsedTimer  = NULL;
        }
        
        BaseGameLaunchInfo mBase; // use a member instead of base class to make initialization "safer"
        
        
        QUrl mDefaultUrl;
        QString mMasterTitleOverride;
        QElapsedTimer *mElapsedTimer;
    };
    
    // Game info at run-time for each game instance
    struct ORIGIN_PLUGIN_API GameInfo : public GameLaunchInfo
    {
        GameInfo()
        {
            mIsValid = false;
            
            mIsRestart = false;
            
            mIsBroadcasting = false;
            mBroadcastingStreamId = 0;
            mMinBroadcastingViewers = 0;
            mMaxBroadcastingViewers = 0;
        }
        
        bool isValid() const { return mIsValid; }
        
        QString  mBroadcastingChannel;
        uint64_t mBroadcastingStreamId;
        uint32_t mMinBroadcastingViewers;
        uint32_t mMaxBroadcastingViewers;
        bool mIsBroadcasting;

        bool mIsValid;
        bool mIsRestart;    // i.e. we started the game, stop the Origin client, restarted the Origin client, and reattached to the game
    };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
    ~IGOGameTracker();
    
    size_t gameCount() const; // number of games currently running
    uint32_t currentGameId() const;
    GameInfo gameInfo(uint32_t gameId) const;
    GameInfo currentGameInfo() const;
    
    void setFreeTrialTimerInfo(const QString& productID, const qint64 timeRemaining, QElapsedTimer* elapsedTimer);
    void setMasterTitleOverride(uint32_t gameId, const QString &overrideName = QString());

    bool resetBroadcastStats(uint32_t gameId);
    
signals:
    void gameAdded(uint32_t gameId);    // emitted when a new game has just started - use the gameId to query for specific game info if necessary
    void gameFocused(uint32_t gameId);  // emitted when a game regains focus (ie not emitted on startup)
    void gameResized(uint32_t gameId);  // emitted when the current game resolution has changed
    void gameRemoved(uint32_t gameId);  // emitted when a game ended

    void gameInfoUpdated(uint32_t gameId);    // emitted when somebody manually changed the metadata of a running game
    void gameFreeTrialInfoUpdated(uint32_t gameId, qint64 timeRemaining, QElapsedTimer* elapsedTimer);    // emitted when somebody modifies the free trial info and it's still applying
                                                                                                        // This is separate from the general gameInfoUpdated because this code has to be 
                                                                                                        // reworked next time we need free trials!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    IGOGameTracker(IGOWindowManagerIPC* ipc);
    IGOGameTracker(const IGOGameTracker& tracker);
    IGOGameTracker& operator=(const IGOGameTracker& tracker);

    void reset();
    
    void addDefinition(const BaseGameLaunchInfo& gameInfo, const QString& browserDefaultURL);
    void addGame(uint32_t gameId, const QString& gameProcessFolder);
    uint32_t removeGame(uint32_t gameId);
    void setCurrentGame(uint32_t gameId);

    // From client to game
    void setDefaultURL(const QString& productID, const QUrl &url);
    void setDefaultURL(uint32_t gameId);
    void setTitle(uint32_t gameId);
    void setMasterTitle(uint32_t gameId);
    void setProductId(uint32_t gameId);
    void setAchievementSetId(uint32_t gameId);
    void setExecutablePath(uint32_t gameId);
    void setTrialTimeRemaining(uint32_t gameId);
    void setIsTrial(uint32_t gameId);
    
    // From game to client
    bool setBroadcastStats(uint32_t gameId, bool isBroadcasting, uint64_t streamId, uint32_t minViewers, uint32_t maxViewers, const char16_t* channel, size_t channelLength);
  
    // From game to client, restoration of state after client was closed with the game still running
    void restoreProductId(uint32_t gameId, const char16_t* productId, size_t length);
    void restoreAchievementSetId(uint32_t gameId, const char16_t* achievementSetId, size_t length);
    void restoreExecutablePath(uint32_t gameId, const char16_t* executablePath, size_t length);
    void restoreDefaultURL(uint32_t gameId, const char16_t* url, size_t length);
    void restoreTitle(uint32_t gameId, const char16_t* title, size_t length);
    void restoreMasterTitle(uint32_t gameId, const char16_t* title, size_t length);
    void restoreMasterTitleOverride(uint32_t gameId, const char16_t* title, size_t length);
    void restoreComplete(uint32_t gameId);
    
    
    typedef QHash <QString, GameLaunchInfo> GameClasses;
    typedef eastl::hash_map<uint32_t, GameInfo> GameInstances;
    GameClasses mGameLaunchMap;
    GameInstances mGames;
    
    uint32_t mCurrentGameId;
    mutable QMutex* mGameMappingMutex;
    IGOWindowManagerIPC* mIPCServer;
    
    class ORIGIN_PLUGIN_API NotifyLock
    {
    public:
        NotifyLock(bool* lock) { mLock = lock; *lock = true; }
        ~NotifyLock() { *mLock = false; }

    private:
        bool* mLock;
    };
    bool mDisableUpdateNotification;

    friend NotifyLock;
    friend IGOController;
    friend IGOWindowManager;
    friend IGOWindowManagerIPC;
};
    
} // Engine
} // Origin

#endif
