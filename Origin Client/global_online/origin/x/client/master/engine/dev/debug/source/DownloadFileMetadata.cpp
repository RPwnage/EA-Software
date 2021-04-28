/////////////////////////////////////////////////////////////////////////////
// DownloadFileMetadata.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/debug/DownloadFileMetadata.h"
#include "services/downloader/DownloaderErrors.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
QString fileStatusAsString(FileStatus status)
{
    QString ret;
    switch(status)
    {
    case kFileStatusFailure:
        ret = QObject::tr("ebisu_client_notranslate_failure");
        break;

    case kFileStatusInProgress:
        ret = QObject::tr("ebisu_client_notranslate_in_progress");
        break;

    case kFileStatusSuccess:
        ret = QObject::tr("ebisu_client_notranslate_success");
        break;

    case kFileStatusExcluded:
        ret = QObject::tr("ebisu_client_notranslate_excluded");
        break;

    case kFileStatusQueued:
    default:
        ret = QObject::tr("ebisu_client_notranslate_queued");
        break;
    }
    return ret;
}

DownloadFileMetadata::DownloadFileMetadata() 
{
    reset(); 
}

DownloadFileMetadata::DownloadFileMetadata(const DownloadFileMetadata& rhs)
{
    *this = rhs;
}

DownloadFileMetadata::~DownloadFileMetadata() 
{
}

void DownloadFileMetadata::reset()
{
    mFileName = QString(); 
    mBytesSaved = 0; 
    mTotalBytes = 0; 
    mErrorCode = Downloader::ContentDownloadError::OK; 
    mPackageFileCrc = 0;
    mDiskFileCrc = 0;
    mIncluded = true;
}

FileStatus DownloadFileMetadata::fileStatus() const
{
    if(mBytesSaved == mTotalBytes)
        return kFileStatusSuccess;
    else if(!mIncluded)
        return kFileStatusExcluded;
    else if(mErrorCode != Downloader::ContentDownloadError::OK)
        return kFileStatusFailure;
    else if(mBytesSaved != 0)
        return kFileStatusInProgress;
    else
        return kFileStatusQueued;
}

QString DownloadFileMetadata::strippedFileName() const
{
    return fileName().remove(0, fileName().lastIndexOf('/') + 1);
}

DownloadFileMetadata& DownloadFileMetadata::operator=(const DownloadFileMetadata& rhs)
{
    mFileName = rhs.mFileName;
    mBytesSaved = rhs.mBytesSaved;
    mTotalBytes = rhs.mTotalBytes;
    mErrorCode = rhs.mErrorCode;
    mPackageFileCrc = rhs.mPackageFileCrc;
    mDiskFileCrc = rhs.mDiskFileCrc;
    mIncluded = rhs.mIncluded;
	return *this;
}

} // Debug
} // Engine
} // Origin
