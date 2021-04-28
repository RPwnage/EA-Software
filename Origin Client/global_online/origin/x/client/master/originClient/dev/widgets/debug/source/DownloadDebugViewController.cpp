/////////////////////////////////////////////////////////////////////////////
// DownloadDebugViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "DownloadDebugViewController.h"
#include "DownloadDebugView.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/DownloaderErrors.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"

#include "originwindow.h"
#include "originwindowmanager.h"
#include "originscrollablemsgbox.h"

#include <QFileDialog>
#include <QDesktopServices>

namespace Origin
{
namespace Client
{

DownloadDebugViewController::DownloadDebugViewController(Engine::Content::EntitlementRef entitlement, DownloadDebugView* view, QObject *parent /*= NULL*/) 
: QObject(parent)
, mView(view)
, mEntitlement(entitlement)
{
    ORIGIN_VERIFY_CONNECT_EX(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadAdded(const QString&)), 
        this, SLOT(onDownloadAdded(const QString&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadRemoved(const QString&)), 
        this, SLOT(onDownloadRemoved(const QString&)), Qt::QueuedConnection);

    ORIGIN_VERIFY_CONNECT(mView, SIGNAL(selectionChanged(const QString&)), this, SLOT(onSelectionChanged(const QString&)));
    onDownloadAdded(mEntitlement->contentConfiguration()->productId());
}

DownloadDebugViewController::~DownloadDebugViewController()
{
    ORIGIN_LOG_EVENT << "User has closed download debug dialog for " << mEntitlement->contentConfiguration()->productId();
    mView = NULL;

    ORIGIN_VERIFY_DISCONNECT(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadAdded(const QString&)), 
        this, SLOT(onDownloadAdded(const QString&)));
    ORIGIN_VERIFY_DISCONNECT(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadRemoved(const QString&)), 
        this, SLOT(onDownloadRemoved(const QString&)));

    // Just stop listening.  Don't clear the table until the download starts back up or the user closes the dialog.
    ORIGIN_VERIFY_DISCONNECT(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onFileProgress(QList<QString>)));
}

void DownloadDebugViewController::onDownloadAdded(const QString& productId)
{
    // Only care if the download is the one we're tracking.
    if(mEntitlement->localContent()->installFlow() 
        && mEntitlement->contentConfiguration()->productId().compare(productId, Qt::CaseInsensitive) == 0)
    {
        mCollector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(productId);
        
        if(mCollector)
        {
            ORIGIN_VERIFY_CONNECT_EX(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onFileProgress(QList<QString>)), Qt::QueuedConnection);
        }

        mView->setSource(mEntitlement->localContent()->installFlow()->getLogSafeUrl());
        mView->setDestination(mEntitlement->localContent()->installFlow()->getDownloadLocation());

        // Set up our progress table
        QMap<QString, Engine::Debug::DownloadFileMetadata> allFiles;
        if(!mCollector.isNull())
            allFiles = mCollector->getDownloadFiles();
        mView->clear();
        mView->setFileCount(allFiles.size());

        qint32 index = 0;
        qint64 elapsed = 0;
        mView->setSortingEnabled(false);

        TIME_BEGIN("DownloadDebugViewController::show() adding files to table");
        for(QMap<QString, Engine::Debug::DownloadFileMetadata>::ConstIterator it = allFiles.begin(); it != allFiles.constEnd(); ++it)
        {
            mView->addFile(index, it.value().strippedFileName(), it.value().totalBytes(), it.value().fileName(), it.value().fileStatus());
            ++index;
        }
        elapsed = TIME_END("DownloadDebugViewController::show() adding files to table");
        ORIGIN_LOG_EVENT << "DownloadDebugViewController::show() adding files to table took " << elapsed << " milliseconds.";

        TIME_BEGIN("DownloadDebugViewController::show() sorting");
        mView->setSortingEnabled(true);
        elapsed = TIME_END("DownloadDebugViewController::show() sorting");
        ORIGIN_LOG_EVENT << "DownloadDebugViewController::show() sorting files took " << elapsed << " milliseconds.";
    }
}

void DownloadDebugViewController::onDownloadRemoved(const QString& productId)
{
    // Only care if the download is the one we're tracking.
    if(mEntitlement->contentConfiguration()->productId().compare(productId, Qt::CaseInsensitive) == 0)
    {
        // Just stop listening.  Don't clear the table until the download starts back up or the user closes the dialog.
        ORIGIN_VERIFY_DISCONNECT(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onFileProgress(QList<QString>)));
    }
}

void DownloadDebugViewController::onFileProgress(QList<QString> fileNames)
{
    if(mCollector && mView)
    {
        for(QList<QString>::const_iterator it = fileNames.constBegin(); it != fileNames.constEnd(); ++it)
        {
            Engine::Debug::DownloadFileMetadata metadata = mCollector->getDownloadFile(*it);

            // Update status column.
            mView->updateStatus(metadata.fileName(), metadata.fileStatus());
        }
    }
}

void DownloadDebugViewController::onSelectionChanged(const QString& installLocation)
{
    if(mCollector && mView)
    {
        Engine::Debug::DownloadFileMetadata metadata = mCollector->getDownloadFile(installLocation);

        // Update file info table
        Downloader::ContentDownloadError::type error = (Downloader::ContentDownloadError::type)metadata.errorCode();
        mView->setFileInfo(metadata.strippedFileName(), metadata.errorCode(), Downloader::ErrorTranslator::Translate(error));
    }
}

void DownloadDebugViewController::onExportRequested()
{
    QString savePath = QFileDialog::getSaveFileName(NULL, QString(), Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation), ".csv");
    if(!savePath.isEmpty() && mCollector)
    {
        mCollector->exportProgress(savePath);
    }
}

}
}
