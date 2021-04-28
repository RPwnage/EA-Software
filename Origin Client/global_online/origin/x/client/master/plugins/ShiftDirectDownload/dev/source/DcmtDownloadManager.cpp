// DcmtFileDownloader.cpp
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.

#include "ShiftDirectDownload/DcmtDownloadManager.h"
#include "DcmtDownloadViewController.h"

#include <QFileDialog>

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

DcmtDownloadManager* DcmtDownloadManager::sInstance = NULL;

void DcmtDownloadManager::destroy()
{
    if (nullptr != sInstance)
    {
        delete sInstance;
        sInstance = nullptr;
    }
}


DcmtDownloadManager* DcmtDownloadManager::instance()
{
    if (!sInstance)
    {
        init();
    }

    return sInstance;
}


DcmtDownloadManager::DcmtDownloadManager(QObject* parent)
    : QObject(parent)
{
}


DcmtDownloadManager::~DcmtDownloadManager()
{
    for(DownloaderHash::iterator it = mDownloaders.begin(); it != mDownloaders.end(); ++it)
    {
        DcmtFileDownloader *downloader = (*it);
        if(downloader)
        {
            ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(error(const QString&)), this, SLOT(removeDownload(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(finished(const QString&)), this, SLOT(removeDownload(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(canceled(const QString&)), this, SLOT(removeDownload(const QString&)));

            delete downloader;
        }
    }

    mDownloaders.clear();
}


void DcmtDownloadManager::startDownload(const QString& buildId, const QString& buildVersion)
{
    if(mDownloaders.contains(buildId))
    {
        return;
    }

    QFileDialog saveDialog(nullptr);

    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setConfirmOverwrite(true);
    saveDialog.setDefaultSuffix(".zip");
    saveDialog.setDirectory(Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation));
    saveDialog.selectFile(buildVersion + ".zip");

    if (saveDialog.exec())
    {
        QStringList fileNames = saveDialog.selectedFiles();
        DcmtFileDownloader *downloader = new DcmtFileDownloader(buildId, fileNames[0]);
        mDownloaders[buildId] = downloader;

        ORIGIN_VERIFY_CONNECT(downloader, SIGNAL(error(const QString&)), this, SLOT(removeDownload(const QString&)));
        ORIGIN_VERIFY_CONNECT(downloader, SIGNAL(finished(const QString&)), this, SLOT(removeDownload(const QString&)));
        ORIGIN_VERIFY_CONNECT(downloader, SIGNAL(canceled(const QString&)), this, SLOT(removeDownload(const QString&)));

        DcmtDownloadViewController *viewController = new DcmtDownloadViewController(downloader);
        viewController->show();
    }
}

void DcmtDownloadManager::removeDownload(const QString& buildId)
{
    if(mDownloaders.contains(buildId))
    {
        DcmtFileDownloader *downloader = mDownloaders[buildId];
        
        ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(error(const QString&)), this, SLOT(removeDownload(const QString&)));
        ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(finished(const QString&)), this, SLOT(removeDownload(const QString&)));
        ORIGIN_VERIFY_DISCONNECT(downloader, SIGNAL(canceled(const QString&)), this, SLOT(removeDownload(const QString&)));

        downloader->deleteLater();

        mDownloaders.remove(buildId);
    }
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
