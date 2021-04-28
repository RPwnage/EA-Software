// DcmtFileDownloader.cpp
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "engine/content/ContentController.h"
#include "engine/downloader/ContentServices.h"

#include "ContentProtocols.h"
#include "ContentProtocolPackage.h"

#include "ShiftDirectDownload/DcmtFileDownloader.h"

#include "Qt/originwindow.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

DcmtFileDownloader::DcmtFileDownloader(const QString &buildId, const QString &filename)
    : mStatus("PENDING")
    , mDownloadUrl(nullptr)
    , mBuildId(buildId)
    , mLocalFileName(filename)
    , mDownloadProtocol(nullptr)
{
    mDownloadUrl = new DcmtDownloadUrlResponse(QString(), buildId, Services::Session::SessionRef());
    ORIGIN_VERIFY_CONNECT(mDownloadUrl, SIGNAL(finished()), this, SLOT(onUrlReady()));
    ORIGIN_VERIFY_CONNECT(mDownloadUrl, SIGNAL(requestedDelivery()), this, SLOT(onDeliveryRequested()));
}


DcmtFileDownloader::~DcmtFileDownloader()
{
    deleteDownloadUrl();
    deleteDownloadProtocol();
}


void DcmtFileDownloader::begin(const QString &url)
{
    if (mDownloadProtocol != nullptr || 0 == url.length())
    {
        emit error(mBuildId);
        return;
    }

    Engine::UserRef user = Engine::Content::ContentController::currentUserContentController()->user();
    ORIGIN_ASSERT(user);

    mDownloadProtocol = new Downloader::ContentProtocolSingleFile("", user->getSession());
    mDownloadProtocol->setGameTitle(mLocalFileName);

    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Started()), this, SLOT(onProtocolStarted()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Finished()), this, SLOT(onProtocolFinished()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Paused()), this, SLOT(onProtocolPaused()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Canceled()), this, SLOT(onProtocolCanceled()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SIGNAL(downloadProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);

    Downloader::ContentProtocols::RegisterProtocol(mDownloadProtocol);

    mDownloadProtocol->Initialize(url, mLocalFileName, false);
}


void DcmtFileDownloader::cancel()
{
    QString title = "CANCEL DOWNLOAD?";
    QString text = QString("You are about to cancel your download of <b>%1</b>. All progress will be lost if you continue.").arg(fileName());
    Origin::UIToolkit::OriginWindow *confirm = Origin::UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, title, text, "Yes", "No");

    ORIGIN_VERIFY_CONNECT(confirm, SIGNAL(accepted()), this, SLOT(cancelInternal()));
    ORIGIN_VERIFY_CONNECT(confirm, SIGNAL(accepted()), confirm, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(confirm, SIGNAL(rejected()), confirm, SLOT(close()));

    confirm->showWindow();
}


void DcmtFileDownloader::cancelInternal()
{
    ASYNC_INVOKE_GUARD;
    if (mDownloadProtocol)
    {
        mDownloadProtocol->Cancel();
    }
    else
    {
        emit canceled(mBuildId);
    }
}


QString DcmtFileDownloader::fileName() const
{
    QFileInfo fileInfo(mLocalFileName);
    return fileInfo.fileName();
}


void DcmtFileDownloader::onProtocolInitialized()
{
    mStatus = "INITIALIZED";
    mDownloadProtocol->Start();
}


void DcmtFileDownloader::onProtocolStarted()
{
    mStatus = "DOWNLOADING";
    emit started(mBuildId);
}


void DcmtFileDownloader::onProtocolFinished()
{
    mStatus = "FINISHED";
    emit finished(mBuildId);
}


void DcmtFileDownloader::onProtocolPaused()
{
    // EA_TODO("jonkolb", "2013/03/27", "Need to add support for resume.  Until then, need to cancel paused downloads.")
    mDownloadProtocol->Cancel();
}


void DcmtFileDownloader::onProtocolCanceled()
{
    // EA_TODO("jonkolb", "2013/03/27", "Delete partial file")
    emit canceled(mBuildId);

    deleteDownloadProtocol();
}


void DcmtFileDownloader::onProtocolError(Downloader::DownloadErrorInfo errorInfo)
{
    QString errorDescription = Downloader::ErrorTranslator::Translate((Downloader::ContentDownloadError::type)errorInfo.mErrorType);
    ORIGIN_LOG_ERROR << "DcmtFileDownloader: errortype = " << errorInfo.mErrorType << ", errorDesc = " << errorDescription << ", errorcode = ," << errorInfo.mErrorCode;

    emit error(mBuildId);
}


void DcmtFileDownloader::onUrlReady()
{
    if (mDownloadUrl && Services::restErrorSuccess == mDownloadUrl->error())
    {
        begin(mDownloadUrl->downloadUrl().mURL);
        deleteDownloadUrl();
    }
    else
    {
        emit error(mBuildId);
    }
}


void DcmtFileDownloader::onDeliveryRequested()
{
    emit deliveryRequested(mBuildId);
}


void DcmtFileDownloader::deleteDownloadUrl()
{
    if (mDownloadUrl)
    {
        ORIGIN_VERIFY_DISCONNECT(mDownloadUrl, SIGNAL(finished()), this, SLOT(onUrlReady()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadUrl, SIGNAL(requestedDelivery()), this, SLOT(onDeliveryRequested()));

        mDownloadUrl->deleteLater();
        mDownloadUrl = nullptr;
    }
}


void DcmtFileDownloader::deleteDownloadProtocol()
{
    if (mDownloadProtocol)
    {
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(Started()), this, SLOT(onProtocolStarted()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(Finished()), this, SLOT(onProtocolFinished()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(Paused()), this, SLOT(onProtocolPaused()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(Canceled()), this, SLOT(onProtocolCanceled()));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)));
        ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SIGNAL(downloadProgress(qint64, qint64, qint64, qint64)));

        mDownloadProtocol->deleteLater();
        mDownloadProtocol = nullptr;
    }
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
