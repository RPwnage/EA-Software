/////////////////////////////////////////////////////////////////////////////
// UpdateDebugViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "UpdateDebugViewController.h"
#include "UpdateDebugView.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/downloader/ContentServices.h"
#include "engine/debug/CalculateUpdateDetailsHelper.h"
#include <QFileDialog>
#include <QDesktopServices>

namespace Origin
{
namespace Client
{

UpdateDebugViewController::UpdateDebugViewController(Engine::Content::EntitlementRef entitlement, UpdateDebugView* view, QObject *parent /*= NULL*/) 
: QObject(parent)
, mCalcHelper(NULL)
, mView(view)
, mEntitlement(entitlement)
{
    ORIGIN_VERIFY_CONNECT_EX(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadAdded(const QString&)), 
        this, SLOT(onDownloadAdded(const QString&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadRemoved(const QString&)), 
        this, SLOT(onDownloadRemoved(const QString&)), Qt::QueuedConnection);
    if(mEntitlement->contentConfiguration()->hasOverride() && !mEntitlement->contentConfiguration()->overrideUsesJitService())
    {
        calculateUpdateForUrl(mEntitlement->contentConfiguration()->overrideUrl());
    }
    else
    {
        Origin::Services::Publishing::DownloadUrlResponse* resp = Origin::Services::Publishing::DownloadUrlProviderManager::instance()->downloadUrl(Origin::Services::Session::SessionService::currentSession(), 
            mEntitlement->contentConfiguration()->productId(), mEntitlement->contentConfiguration()->buildIdentifierOverride(), mEntitlement->contentConfiguration()->buildReleaseVersionOverride());
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onUrlFetchFinished()));
    }
}

UpdateDebugViewController::~UpdateDebugViewController()
{
    ORIGIN_LOG_EVENT << "User has closed download debug dialog for " << mEntitlement->contentConfiguration()->productId();
    mView = NULL;

    ORIGIN_VERIFY_DISCONNECT(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadAdded(const QString&)), 
        this, SLOT(onDownloadAdded(const QString&)));
    ORIGIN_VERIFY_DISCONNECT(Engine::Debug::DownloadDebugDataManager::instance(), SIGNAL(downloadRemoved(const QString&)), 
        this, SLOT(onDownloadRemoved(const QString&)));

    // Just stop listening.  Don't clear the table until the download starts back up or the user closes the dialog.
    ORIGIN_VERIFY_DISCONNECT(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onUpdateCalculated()));
}


void UpdateDebugViewController::onDownloadAdded(const QString& productId)
{
    // Only care if the download is the one we're tracking.
    if(mEntitlement->contentConfiguration()->productId().compare(productId, Qt::CaseInsensitive) == 0)
    {
        mCollector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(productId);       
        ORIGIN_VERIFY_CONNECT_EX(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onUpdateCalculated()), Qt::QueuedConnection);
    }
}

void UpdateDebugViewController::onDownloadRemoved(const QString& productId)
{
    // Only care if the download is the one we're tracking.
    if(mEntitlement->contentConfiguration()->productId().compare(productId, Qt::CaseInsensitive) == 0)
    {
        // Just stop listening.  Don't clear the table until the download starts back up or the user closes the dialog.
        ORIGIN_VERIFY_DISCONNECT(mCollector, SIGNAL(downloadFileProgress(QList<QString>)), this, SLOT(onUpdateCalculated()));
    }
}

void UpdateDebugViewController::onUrlFetchFinished()
{
    QString url;

    Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());
    if(response != NULL)
    {
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(completeInitializeProtocolWithJitUrl()));
        response->deleteLater();

        if(response->error() == Origin::Services::restErrorSuccess)
        {
            ORIGIN_LOG_DEBUG << "Retrieved PDLC URL " << response->downloadUrl().mURL << " for product ID " << mEntitlement->contentConfiguration()->productId();
            url = response->downloadUrl().mURL;
        }
    }

    if(url.isEmpty())
    {
        // ERROR MESSAGE
    }
    else
    {
        calculateUpdateForUrl(url);
    }
}

void UpdateDebugViewController::calculateUpdateForUrl(const QString& url)
{
    if(mCalcHelper)
    {
        delete mCalcHelper;
    }

    mCalcHelper = new Origin::Engine::Debug::CalculateUpdateDetailsHelper(url, mEntitlement->contentConfiguration());

    QString cachePath;
    if (mEntitlement->contentConfiguration()->isPDLC())
    {
        Origin::Downloader::ContentServices::GetContentInstallCachePath(mEntitlement->parent(), cachePath, false);
    }
    else
    {
        Origin::Downloader::ContentServices::GetContentInstallCachePath(mEntitlement, cachePath, false);
    }

    if(!mCalcHelper->calculateUpdateDetails(mEntitlement->localContent()->dipInstallPath(), cachePath))
    {
        // ERROR MESSAGE
    }
    else
    {
        ORIGIN_LOG_DEBUG << "Calculating update details.";
    }
}

void UpdateDebugViewController::onUpdateCalculated()
{
    if(mCollector && mView && mCalcHelper)
    {
        // Populate the view.
        QString source = mCalcHelper->downloadUrl();
        if(source.contains("akamai", Qt::CaseInsensitive))
            source = tr("ebisu_client_notranslate_akamai");
        else if(source.contains("lvlt", Qt::CaseInsensitive))
            source = tr("ebisu_client_notranslate_level_3");

        quint64 totalUpdateSize = 0;

        mView->setSource(source);
        
        // Set up our progress table
        QMap<QString, Engine::Debug::DownloadFileMetadata> allFiles = mCollector->getDownloadFiles();
        mView->clear();
        mView->setFileCount(allFiles.size());

        qint32 index = 0;
        mView->setSortingEnabled(false);

        TIME_BEGIN("UpdateDebugViewController::show() adding files to table");
        for(QMap<QString, Engine::Debug::DownloadFileMetadata>::ConstIterator it = allFiles.begin(); it != allFiles.constEnd(); ++it)
        {
            mView->addFile(index, it.value().strippedFileName(), it.value().totalBytes(), it.value().fileName(), it.value().packageFileCrc(), it.value().diskFileCrc());
            totalUpdateSize += it.value().totalBytes();
            ++index;
        }
        TIME_END("UpdateDebugViewController::show() adding files to table");

        mView->setSize(totalUpdateSize);

        TIME_BEGIN("UpdateDebugViewController::show() sorting");
        mView->setSortingEnabled(true);
        TIME_END("UpdateDebugViewController::show() sorting");

        mView->showUpdateFiles(true);
    }

    if(mCalcHelper)
    {
        mCalcHelper->deleteLater();
        mCalcHelper = NULL;
    }
}

void UpdateDebugViewController::onExportRequested()
{
    QString savePath = QFileDialog::getSaveFileName(NULL, QString(), Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation), ".csv");
    if(!savePath.isEmpty() && mCollector)
    {
        mCollector->exportUpdateDiff(savePath);
    }
}

}
}
