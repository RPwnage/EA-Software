/////////////////////////////////////////////////////////////////////////////
// DownloadDebugDataManager.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/debug/DownloadDebugDataManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"
#include "services/Settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
DownloadDebugDataManager* DownloadDebugDataManager::sInstance = NULL;

DownloadDebugDataManager* DownloadDebugDataManager::instance()
{
    if(!sInstance)
        init();
    return sInstance;
}

DownloadDebugDataManager::DownloadDebugDataManager(QObject *parent /*= NULL*/)
: QObject(parent)
{
}

DownloadDebugDataManager::~DownloadDebugDataManager()
{
    QWriteLocker lock(&mLock);
    foreach(DownloadDebugDataCollector* collector, mDatabase)
    {
        collector->deleteLater();
    }
    mDatabase.clear();
}

DownloadDebugDataCollector* DownloadDebugDataManager::addDownload(const QString& productId)
{
    mLock.lockForWrite();
    if(mDatabase.contains(productId))
    {
        mDatabase[productId]->deleteLater();
        mDatabase.remove(productId);
    }

    DownloadDebugDataCollector *collector = new DownloadDebugDataCollector(this);
    mDatabase.insert(productId, collector);
    mLock.unlock();

    emit downloadAdded(productId);

    return collector;
}

void DownloadDebugDataManager::removeDownload(const QString& productId)
{
    mLock.lockForWrite();
    mDatabase.remove(productId);
    mLock.unlock();

    emit downloadRemoved(productId);
}
    
DownloadDebugDataCollector* DownloadDebugDataManager::getDataCollector(const QString& productId)
{
    QReadLocker lock(&mLock);
    return mDatabase.contains(productId) ? mDatabase[productId] : NULL;
}

} // Debug
} // Engine
} // Origin
