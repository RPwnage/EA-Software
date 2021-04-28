///////////////////////////////////////////////////////////////////////////////
// DownloadFileMetadata.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DOWNLOADFILEMETADATA_H_
#define _DOWNLOADFILEMETADATA_H_

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
    enum FileStatus
    {
        kFileStatusFailure,     ///< The file has encountered a terminal error.
        kFileStatusInProgress,  ///< The file is being downloaded.
        kFileStatusSuccess,     ///< The file has downloaded successfully.
        kFileStatusQueued,      ///< The file is queued for download.
        kFileStatusExcluded     ///< The file has been excluded from the download.
    };

    /// \brief Converts a file status to a human-readable string.
    /// \param status The file status.
    /// \return A human-readable string describing the file status.
    ORIGIN_PLUGIN_API QString fileStatusAsString(FileStatus status);

/// \brief Stores information about the state of a download file.
class ORIGIN_PLUGIN_API DownloadFileMetadata
{
public:
    /// \brief Constructor
    DownloadFileMetadata();
    
    /// \brief Copy constructor
    /// \param rhs Object to copy from
    DownloadFileMetadata(const DownloadFileMetadata& rhs);

    /// \brief Destructor
    ~DownloadFileMetadata();

    /// \brief Resets the data back to its initial state.
    void reset();

    /// \brief Sets the file name
    /// \param fileName The file name
    void setFileName(const QString& fileName) { mFileName = fileName; }

    /// \brief Retrieves the file name
    /// \return The file name
    QString fileName() const { return mFileName; }
    
    /// \brief Sets the number of bytes saved
    /// \param bytesSaved The number of bytes saved
    void setBytesSaved(qint64 bytesSaved) { mBytesSaved = bytesSaved; }
    
    /// \brief Retrieves the number of bytes saved
    /// \return The number of bytes saved
    qint64 bytesSaved() const { return mBytesSaved; }
    
    /// \brief Sets the total number of bytes
    /// \param totalBytes The total number of bytes
    void setTotalBytes(qint64 totalBytes) { mTotalBytes = totalBytes; }
    
    /// \brief Retrieves the total number of bytes
    /// \return The total number of bytes
    qint64 totalBytes() const { return mTotalBytes; }
    
    /// \brief Sets the error code
    /// \param errorCode The error code
    void setErrorCode(qint32 errorCode) { mErrorCode = errorCode; }
    
    /// \brief Retrieves the error code
    /// \return The error code
    qint32 errorCode() const { return mErrorCode; }
        
    /// \brief Sets the package file crc
    /// \param crc The crc
    void setPackageFileCrc(quint32 crc) { mPackageFileCrc = crc; }
    
    /// \brief Retrieves package file crc
    /// \return The crc
    quint32 packageFileCrc() const { return mPackageFileCrc; }
         
    /// \brief Sets the disk file crc
    /// \param crc The crc
    void setDiskFileCrc(quint32 crc) { mDiskFileCrc = crc; }
    
    /// \brief Retrieves disk file crc
    /// \return The crc
    quint32 diskFileCrc() const { return mDiskFileCrc; }

    /// \brief Sets whether the file is included in the download
    /// \param included True if the file is included in the download
    void setIncluded(bool included) { mIncluded = included; }
    
    /// \brief Checks if the file is included in the download
    /// \return True if the file is included in the download
    bool included() const { return mIncluded; }
    
    /// \brief Returns the file download state.
    /// \return The file download state.
    FileStatus fileStatus() const;

    /// \brief Retrieves the file name without prepended directories.
    /// \return The file name without prepended directories.
    QString strippedFileName() const;

    /// \brief Overloaded assignment operator
    /// \param rhs Object to copy from
    /// \return The object that was copied to
    DownloadFileMetadata& operator=(const DownloadFileMetadata& rhs);

private:
    QString mFileName; ///< The name of this file.
    qint64 mBytesSaved; ///< The number of bytes saved to disk so far.
    qint64 mTotalBytes; ///< The total number of bytes to download.
    qint32 mErrorCode; ///< The error encountered while attempting to download this file.
    quint32 mPackageFileCrc; ///< CRC of file in DiP package.    
    quint32 mDiskFileCrc; ///< Populated if there's a file already on disk to be updated

    bool mIncluded; ///< True if the file is included in the download.
};

} // namespace Debug
} // namespace Engine
} // namespace Origin

#endif
