//
//  IGOGameTracker.cpp
//  engine
//
//  Created by Frederic Meraud on 3/4/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOGameTracker.h"

#include "IGOIPC/IGOIPC.h"

#include "IGOWindowManagerIPC.h"

#include "engine/content/ContentController.h"

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

#ifdef ORIGIN_MAC
#include "IGOMacHelpers.h"
#elif defined(ORIGIN_PC)
#include <Windows.h>
#include <TlHelp32.h>
#include <QDir>
#include "IGOWinHelpers.h"
#endif

// detect concurrencies - assert if our mutex is already locked
#ifdef ORIGIN_PC

#ifdef _DEBUG_MUTEXES
#define DEBUG_MUTEX_TEST(x){\
bool s=x->tryLock();\
if(s)\
x->unlock();\
ORIGIN_ASSERT(s==true);\
}
#else
#define DEBUG_MUTEX_TEST(x) void(0)
#endif

#elif defined (ORIGIN_MAC)

#define DEBUG_MUTEX_TEST(x) void(0)

#endif // !MAC OSX

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Origin
{
namespace Engine
{

// Helper to convert a folder to a full length folder
static QString convertGameProcessFolder(const QString &folder)
{
#if defined(ORIGIN_PC)
    
    // extend path length to 32767 wide chars
    const int UNICODE_MAX_PATH = 32767;
    wchar_t exePath[UNICODE_MAX_PATH] = {0};
    
    QString prependedPath("\\\\?\\");
    prependedPath.append( folder );
    DWORD bytes = GetLongPathName(prependedPath.utf16(), exePath, UNICODE_MAX_PATH);
    if( bytes == 0 )
    {
        return QString();
    }
    
    QString fullFolder = QString::fromUtf16(exePath);
    
    // remove trailing path separators
    while ( fullFolder.endsWith(QDir::separator(), Qt::CaseInsensitive) )
        fullFolder.chop(1);
    
    return fullFolder;
    
#else
    
    // We want to only keep up to the /Contents folder
    static const QString CONTENTS_SUBFOLDER_NAME = "/Contents/";
    
    int subFolderIdx = folder.indexOf(CONTENTS_SUBFOLDER_NAME);
    return subFolderIdx > 0 ? folder.left(subFolderIdx) : folder;
    
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IGOGameTracker::IGOGameTracker(IGOWindowManagerIPC* ipc)
    : mCurrentGameId(0), mGameMappingMutex(new QMutex(QMutex::Recursive)), mIPCServer(ipc), mDisableUpdateNotification(false)
{
    
}

IGOGameTracker::~IGOGameTracker()
{
    delete mGameMappingMutex;
}

void IGOGameTracker::reset()
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    NotifyLock lock(&mDisableUpdateNotification); // don't emit gameInfoUpdated

    mCurrentGameId = 0;

    mGames.clear();
    mGameLaunchMap.clear();
}

void IGOGameTracker::addDefinition(const BaseGameLaunchInfo& gameInfo, const QString& browserDefaultURL)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);

    NotifyLock lock(&mDisableUpdateNotification); // don't emit gameInfoUpdated
    
    GameLaunchInfo launchInfo;
    launchInfo.mBase = gameInfo;
    mGameLaunchMap[gameInfo.mProductID] = launchInfo;
    
    // setup the default IGO browser URL 
    setDefaultURL(gameInfo.mProductID, browserDefaultURL); 
}

void IGOGameTracker::addGame(uint32_t gameId, const QString& gameProcessFolder)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    NotifyLock lock(&mDisableUpdateNotification); // don't emit gameInfoUpdated

    // why this code?
    // this code handles multiple game launches in Origin
    // for every game that launches, we get:
    // - default url
    // - product id
    // - game name
    // - game install folder
    // the install folder is required, to match the IGO onnection id (game process PID) to the acctual game data,
    // because the game launch and the actual connection of IGO to the game process can happen in an unspecified order, depending on how fast a game starts(or fails to start!)!!!
    QString bundleId;


#ifdef ORIGIN_MAC
    bundleId = Origin::Services::PlatformService::getBundleIdentifier(gameId);
#endif

    QList<GameClasses::iterator> matchingGames;
    for (GameClasses::iterator iter = mGameLaunchMap.begin(); iter != mGameLaunchMap.end(); ++iter)
    {
        QString fullFolder = convertGameProcessFolder(iter.value().mBase.mInstallDir);
        QString gameBundleId = iter.value().mBase.mBundleId;
        if ((!gameBundleId.isEmpty() && gameBundleId == bundleId)           || // MAC only -  check the bundle Id in case the game gets copied in an entirely different location (for example for popcap games)
            gameProcessFolder.contains( fullFolder, Qt::CaseInsensitive )  || // found our game process
            fullFolder.contains( gameProcessFolder, Qt::CaseInsensitive ) )   // fifa12 case where the launcher is a subfolder of the game process's folder
        {
            matchingGames.append(iter);
        }
    }

    // Select best fit from our set of matching games.
    // Owned full game > owned trial > anything else
    GameClasses::iterator bestMatch = mGameLaunchMap.end();
    GameClasses::iterator ownedMatch = mGameLaunchMap.end();
    GameClasses::iterator anyMatch = mGameLaunchMap.end();
    foreach (const GameClasses::iterator& iter, matchingGames)
    {
        Content::EntitlementRef ent = Content::ContentController::currentUserContentController()->entitlementById(iter.value().mBase.mProductID);
        if(ent)
        {
            const bool owned = ent->contentConfiguration()->owned();
            const bool trial = ent->contentConfiguration()->isFreeTrial();
            if (owned && !trial)
            {
                bestMatch = iter;
                break;
            }
            else if (owned)
            {
                ownedMatch = iter;
            }
            else
            {
                anyMatch = iter;
            }
        }
    }

    if (bestMatch == mGameLaunchMap.end())
    {
        if (ownedMatch != mGameLaunchMap.end())
            bestMatch = ownedMatch;
        else
            bestMatch = anyMatch;
    }

    if(bestMatch != mGameLaunchMap.end())
    {
        // set per game data
        bool unreleased = bestMatch.value().mBase.mIsUnreleased;

        // fill in default executable name
        QString actualProcessName = bestMatch.value().mBase.mExecutable;
#if defined(ORIGIN_PC)
        // get the actual process name from the pid (gameid == pid)
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32))
            {
                do
                {
                    if (pe32.th32ProcessID == gameId)
                    {
                        actualProcessName = QString::fromWCharArray(pe32.szExeFile);
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
#endif

        GameInfo info;
        info.mIsValid = true;
        info.mBase.mProductID = bestMatch.key();
        info.mBase.mExecutable = bestMatch.value().mBase.mExecutable;
        info.mBase.mExecutablePath = bestMatch.value().mBase.mExecutablePath;

#if defined(ORIGIN_PC)
        // if our stored executable name and the actual executable name differ, use the actual executable name
        if (actualProcessName.compare(info.mBase.mExecutable, Qt::CaseInsensitive))
        {
            info.mBase.mExecutable = actualProcessName;
            if (bestMatch.value().mBase.mRefCount == 0)  // if we were launched by a launcher, set the reference to 1
                bestMatch.value().mBase.mRefCount = 1;
        }
        else
#endif
            ++bestMatch.value().mBase.mRefCount; // normal launch

        info.mBase.mTitle = unreleased ? tr("ebisu_client_notranslate_embargomode_title") : bestMatch.value().mBase.mTitle;
        info.mBase.mMasterTitle = unreleased ? tr("ebisu_client_notranslate_embargomode_title") : bestMatch.value().mBase.mMasterTitle;
        info.mMasterTitleOverride = unreleased ? tr("ebisu_client_notranslate_embargomode_title") : bestMatch.value().mMasterTitleOverride;
        info.mDefaultUrl = bestMatch.value().mDefaultUrl;
        info.mBase.mForceKillAtOwnershipExpiry = bestMatch.value().mBase.mForceKillAtOwnershipExpiry;
        info.mBase.mIsUnreleased = unreleased;
        info.mBase.mAchievementSetID = bestMatch.value().mBase.mAchievementSetID;
        info.mBase.mIsTrial = bestMatch.value().mBase.mIsTrial;
        info.mBase.mTimeRemaining = bestMatch.value().mBase.mTimeRemaining;
            
        mGames[gameId] = info;
            
        // Notify anybody who cares! only after we're done setting up, but before anything internally also queues up notifications (ie trial stuff)
        QMetaObject::invokeMethod(this, "gameAdded", Qt::QueuedConnection, Q_ARG(uint32_t, gameId));
            
        setTitle(gameId);
        setMasterTitle(gameId);
#if defined(ORIGIN_PC)
        setMasterTitleOverride(gameId, mGames[gameId].mBase.mMasterTitle);
#else
        setMasterTitleOverride(gameId);
#endif
        setDefaultURL(gameId);
        setProductId(gameId);
        setAchievementSetId(gameId);
        setExecutablePath(gameId);
        setIsTrial(gameId);
        setTrialTimeRemaining(gameId);
    
        // Notify the game we're done sending over the initial info
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameInitialInfoComplete());
        mIPCServer->send(msg);

        // do not delete the game data, some games disconnect & reconnect when you alt-tab out/in of them like SWTOR
        // so we keep it here until Origin is gone!

        return;
    }
    
    // handle special case of Origin SDK games, that are not in the catalog, so we receive the event from the game, without having the Origin RTP flow!!!
    // + unrecognized games (client crashes + reconnect case?)
    // set per game data - minimal set only
    GameInfo info;
    
    bool embargoModeDisabled = Origin::Services::readSetting(Origin::Services::SETTING_OverridesEnabled).toQVariant().toBool() && Origin::Services::readSetting(Origin::Services::SETTING_DisableEmbargoMode);
    
    info.mIsValid = false;
    info.mBase.mTitle = (!embargoModeDisabled) ? tr("ebisu_client_notranslate_embargomode_title") : "none";
    info.mBase.mMasterTitle = (!embargoModeDisabled) ? tr("ebisu_client_notranslate_embargomode_title") : "none";
    info.mMasterTitleOverride = (!embargoModeDisabled) ? tr("ebisu_client_notranslate_embargomode_title") : "none";
    info.mBase.mIsUnreleased = !embargoModeDisabled && !Origin::Services::readSetting(Origin::Services::SETTING_DisableTwitchBlacklist);
    
    mGames[gameId] = info;
    
    // Notify the game to send us info IF this was a broken connection
    eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameRequestInfo());
    mIPCServer->send(msg);
    
    // We are going to wait until we get the restore message from the game to notify about the new game
}

uint32_t IGOGameTracker::removeGame(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    NotifyLock lock(&mDisableUpdateNotification); // don't emit gameInfoUpdated

    // Let's be safe in case this is called mutiple times
    if (mGames.find(gameId) != mGames.end())
    {
        emit gameRemoved(gameId);

        // Update the base launch map - this is to help with the scenario where the user may have a trial and the actual game, and they may
        // be installed in the same location (not cool)
        GameClasses::iterator classIter = mGameLaunchMap.find(mGames[gameId].mBase.mProductID);
        if (classIter != mGameLaunchMap.end())
        {
#if defined(ORIGIN_PC)
            // only decrement the ref count, if we actually have the real process, not just a launcher!
            // this will cause an intentional "leak" in mGameLaunchMap for each product id that uses a launcher which we do not track!  
            if (classIter->mBase.mExecutablePath.contains(mGames[gameId].mBase.mExecutable, Qt::CaseInsensitive))
#endif
                --classIter->mBase.mRefCount;

            if (classIter->mBase.mRefCount == 0)
                mGameLaunchMap.erase(classIter);
        }

        mGames.erase(gameId);
    }

    return mGames.size();
}

void IGOGameTracker::setCurrentGame(uint32_t gameId)
{
    // Feels like we got way too many mutexes here!
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    mCurrentGameId = gameId;
    
    emit gameFocused(gameId);
}

size_t IGOGameTracker::gameCount() const
{
    return mGames.size();
}

uint32_t IGOGameTracker::currentGameId() const
{
    return mCurrentGameId;
}

IGOGameTracker::GameInfo IGOGameTracker::gameInfo(uint32_t gameId) const
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    GameInstances::const_iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        return iter->second;
    
    return GameInfo();
}

IGOGameTracker::GameInfo IGOGameTracker::currentGameInfo() const
{
    return gameInfo(mCurrentGameId);
}

void IGOGameTracker::setDefaultURL(const QString& productID, const QUrl &url)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    // update gameLaunchMap
    if (mGameLaunchMap.contains(productID))
        mGameLaunchMap[productID].mDefaultUrl = url;
    
    // update mDefaultUrlMap
    for(GameInstances::iterator iter = mGames.begin(); iter!=mGames.end(); ++iter)
    {
        if (iter->second.mBase.mProductID == productID)
        {
            mGames[iter->first/*connection id*/].mDefaultUrl = url;
            setDefaultURL(iter->first/*connection id*/);
            break;
        }
    }
    
}
    
void IGOGameTracker::setFreeTrialTimerInfo(const QString &productID, const qint64 timeRemaining, QElapsedTimer* elapsedTimer)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    
    if (productID.isEmpty())
        return;
    
    if (mGameLaunchMap.contains(productID))
    {
        mGameLaunchMap[productID].mBase.mTimeRemaining = timeRemaining;
        mGameLaunchMap[productID].mElapsedTimer = elapsedTimer;
        mGameLaunchMap[productID].mBase.mIsTrial = true;

        // update mTrialTimeRemainingMap
        for(GameInstances::iterator iter = mGames.begin(); iter!=mGames.end(); ++iter)
        {
            if (iter->second.mBase.mProductID == productID)
            {
                iter->second.mBase.mIsTrial = true;
                iter->second.mBase.mTimeRemaining = timeRemaining;
                iter->second.mElapsedTimer = elapsedTimer;
                setTrialTimeRemaining(iter->first/*connection id*/);
                break;
            }
        }
    }
}

void IGOGameTracker::setTitle(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameTitle((char16_t*) iter->second.mBase.mTitle.utf16(), iter->second.mBase.mTitle.length() * sizeof(char16_t)));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setMasterTitle(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameMasterTitle((char16_t*) iter->second.mBase.mMasterTitle.utf16(), iter->second.mBase.mMasterTitle.length() * sizeof(char16_t)));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setMasterTitleOverride(uint32_t gameId, const QString &overrideName)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        if (overrideName.isEmpty())
        {
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameMasterTitleOverride((char16_t*) iter->second.mMasterTitleOverride.utf16(), iter->second.mMasterTitleOverride.length() * sizeof(char16_t)));
            mIPCServer->send(msg);
        }
        
        else
        {
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameMasterTitleOverride((char16_t*) overrideName.utf16(), overrideName.length() * sizeof(char16_t)));
            mIPCServer->send(msg);
        }
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setDefaultURL(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        QString url = iter->second.mDefaultUrl.toString();
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameDefaultURL((char16_t*) url.utf16(), url.length() * sizeof(char16_t)));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setProductId(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameProductId((char16_t*)iter->second.mBase.mProductID.utf16(), iter->second.mBase.mProductID.length() * sizeof(char16_t)));
        mIPCServer->send(msg);

        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setAchievementSetId(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameAchievementSetId((char16_t*) iter->second.mBase.mAchievementSetID.utf16(), iter->second.mBase.mAchievementSetID.length() * sizeof(char16_t)));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setExecutablePath(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameExecutablePath((char16_t*) iter->second.mBase.mExecutablePath.utf16(), iter->second.mBase.mExecutablePath.length() * sizeof(char16_t)));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setIsTrial(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameIsTrial(iter->second.mBase.mForceKillAtOwnershipExpiry));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameInfoUpdated(gameId);
    }
}

void IGOGameTracker::setTrialTimeRemaining(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        QString productId = iter->second.mBase.mProductID;
        if (productId.isEmpty())
            return;
        
        //this is time remaining based on the last server time we got
        qint64 timeRemainingBase = mGameLaunchMap[productId].mBase.mTimeRemaining;
        
        //the  elapsed timer is how much time has passed since the last server time refresh
        QElapsedTimer *trialElapsedTimer = mGameLaunchMap[productId].mElapsedTimer;
        
        //this is the real time remaining, initialize to -1 which means undefined
        qint64 realTimeRemain = -1;
        
        //if we have a valid timer calculate the time remaining
        if(trialElapsedTimer)
        {
            realTimeRemain = timeRemainingBase - trialElapsedTimer->elapsed();

            // Clamp the value since we use a negative value for an not-defined-yet timer
            if (realTimeRemain < 0)
                realTimeRemain = 0;
        }
        else
        if(timeRemainingBase == 0)
        {
            //if timeRemainingBase == 0 means we've already told the game to quit because we didn't have a valid termination date at initial launch
            //so even if we don't have a valid timer we want to quit
            realTimeRemain = 0;
        }

        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgGameTrialTimeRemaining(realTimeRemain));
        mIPCServer->send(msg);
        
        if (!mDisableUpdateNotification)
            emit gameFreeTrialInfoUpdated(gameId, timeRemainingBase, trialElapsedTimer);

        else
        {
            // We stil want to send a notification, but we want to make sure it happens after the game info is completely setup
            QMetaObject::invokeMethod(this, "gameFreeTrialInfoUpdated", Qt::QueuedConnection, Q_ARG(uint32_t, gameId), Q_ARG(qint64, timeRemainingBase), Q_ARG(QElapsedTimer*, trialElapsedTimer));
        }
    }
}

bool IGOGameTracker::setBroadcastStats(uint32_t gameId, bool isBroadcasting, uint64_t streamId, uint32_t minViewers, uint32_t maxViewers, const char16_t* channel, size_t channelLength)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mIsBroadcasting = isBroadcasting;
        iter->second.mBroadcastingStreamId = streamId;
        iter->second.mMinBroadcastingViewers = minViewers;
        iter->second.mMaxBroadcastingViewers = maxViewers;
        iter->second.mBroadcastingChannel = QString::fromUtf16(reinterpret_cast<const ushort*>(channel), channelLength / 2);
        
        return true;
    }
    
    return false;
}

bool IGOGameTracker::resetBroadcastStats(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcastResetStats());
        mIPCServer->send(msg);
        
        return true;
    }
    
    return false;
}

void IGOGameTracker::restoreProductId(uint32_t gameId, const char16_t* productId, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        iter->second.mBase.mProductID = QString::fromUtf16(reinterpret_cast<const ushort*>(productId), length / 2);
}

void IGOGameTracker::restoreAchievementSetId(uint32_t gameId, const char16_t* achievementSetId, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        iter->second.mBase.mAchievementSetID = QString::fromUtf16(reinterpret_cast<const ushort*>(achievementSetId), length / 2);
}

void IGOGameTracker::restoreExecutablePath(uint32_t gameId, const char16_t* executablePath, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        iter->second.mBase.mExecutablePath = QString::fromUtf16(reinterpret_cast<const ushort*>(executablePath), length / 2);
}

void IGOGameTracker::restoreDefaultURL(uint32_t gameId, const char16_t* url, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mDefaultUrl = QString::fromUtf16(reinterpret_cast<const ushort*>(url), length / 2);
    }
}
    
void IGOGameTracker::restoreTitle(uint32_t gameId, const char16_t* title, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mBase.mTitle = QString::fromUtf16(reinterpret_cast<const ushort*>(title), length / 2);
    }
}
    
void IGOGameTracker::restoreMasterTitle(uint32_t gameId, const char16_t* title, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mBase.mMasterTitle = QString::fromUtf16(reinterpret_cast<const ushort*>(title), length / 2);
    }
}
    
void IGOGameTracker::restoreMasterTitleOverride(uint32_t gameId, const char16_t* title, size_t length)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mMasterTitleOverride = QString::fromUtf16(reinterpret_cast<const ushort*>(title), length / 2);
    }
}

void IGOGameTracker::restoreComplete(uint32_t gameId)
{
    DEBUG_MUTEX_TEST(mGameMappingMutex); QMutexLocker locker(mGameMappingMutex);
    GameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
    {
        iter->second.mIsValid = true;
        iter->second.mIsRestart = true;
     
        // Restore base constructor too for things like trial timers, etc...
        GameClasses::iterator classIter = mGameLaunchMap.find(iter->second.mBase.mProductID);
        if (classIter == mGameLaunchMap.end())
        {
            GameLaunchInfo launchInfo;
            launchInfo.mBase = iter->second.mBase;
            launchInfo.mBase.mRefCount = 1;
            launchInfo.mDefaultUrl = iter->second.mDefaultUrl;
            launchInfo.mMasterTitleOverride = iter->second.mMasterTitleOverride;

            mGameLaunchMap[iter->second.mBase.mProductID] = launchInfo;
        }

        else
        {
            ++classIter->mBase.mRefCount;
        }

        emit gameAdded(gameId);
    }
}
    
} // Engine
} // Origin
