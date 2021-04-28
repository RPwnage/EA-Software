#include "UnpackStreamFile.h"
#include "../CRC.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QDebug>

#if defined(Q_OS_WIN)
#include <io.h>
#include <windows.h>
#include <atltime.h>
#else
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#include <QDateTime>
#endif

#define ORIGIN_PC

#define ORIGIN_ASSERT(condition)

#define CRC_INITIAL_VALUE 0xffffffff

const int kDefaultMaxCompressedChunkSize = 512*1024;  // 512 KiB

const int kDefaultMaxUncompressedChunkSize = 2048*1024;  // 2 MB

const int kMaxFileOpenRetries = 3;

const int kFileOpenRetryDelayMS = 1000;

const int kMinimumAvailableDiskSpace = 100*1024*1024; // 100 MB

namespace Origin
{
namespace Services
{
namespace PlatformService
{
    int lastSystemResultCode()
    {
        return GetLastError();
    }

    void sleep(int delayInMS)
    {
        Sleep(delayInMS);
    }
}
}
namespace EnvUtils
{
    bool GetFileDetailsNative(const QString& filePath, qint64& fileSize)
    {
        // Qt handles shortcut (.lnk) in an utterly stupid way on Windows, it treats them as symlinks
        // Furthermore none of the QFile objects can operate on the .lnk file itself, they all go to the file it points to, which is incorrect for our purposes
        // Therefore we must use the native APIs
        fileSize = 0;

        WIN32_FILE_ATTRIBUTE_DATA fad;

        // If it doesn't exist, this will return false
        if (!GetFileAttributesEx((LPCWSTR)filePath.utf16(), GetFileExInfoStandard, &fad))
            return false;
        LARGE_INTEGER size;
        size.HighPart = fad.nFileSizeHigh;
        size.LowPart = fad.nFileSizeLow;
        fileSize = size.QuadPart;

        return true;
    }
    bool GetFileExistsNative(const QString& filePath)
    {
        qint64 fileSize = 0; // Unused
        return GetFileDetailsNative(filePath, fileSize);
    }
}
namespace Downloader
{

quint32 UnpackStreamFile::sMaxCompressedChunkSize = kDefaultMaxCompressedChunkSize;

quint32 UnpackStreamFile::sMaxUncompressedChunkSize = kDefaultMaxUncompressedChunkSize;

void UnpackStreamFile::setMaxChunkSizes(quint32 maxCompressedChunkSize, quint32 maxUncompressedChunkSize)
{
	if(maxCompressedChunkSize > 0)
	{
		sMaxCompressedChunkSize = maxCompressedChunkSize;
	}
	else
	{
		sMaxCompressedChunkSize = kDefaultMaxCompressedChunkSize;
	}

	if(maxUncompressedChunkSize > 0)
	{
		sMaxUncompressedChunkSize = maxUncompressedChunkSize;
	}
	else
	{
		sMaxUncompressedChunkSize = kDefaultMaxUncompressedChunkSize;
	}
}

UnpackStreamFile::UnpackStreamFile(const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc, const QString &mOfferId, bool ignoreCRCErrors ) :
    _shutdown(false),
    _productId(mOfferId),
    _filePath(path),
	_fileDate(date),
	_fileTime(time),
    _fileAttributes(fileAttributes),
    _fileTotalUnpackedBytes(unpackedSize),
	_fileTotalPackedBytes(packedSize),
    _fileSavedBytes(0),
    _fileProcessedBytes(0),
	_fileComputedCRC(CRC_INITIAL_VALUE),
    _fileCRC(crc),
    _fileUnpackType(unpackType),
    _ignoreCRCErrors(ignoreCRCErrors),
	_errorType(UnpackError::OK),
	_errorCode(0),
	_fileChunkCount(0),
	_fileDecompressor(NULL),
	_fileHandle(path)
{
	if(_fileUnpackType == UnpackType::Uncompressed)
	{
		_maxChunkSize = sMaxUncompressedChunkSize;
	}
	else
	{
		_maxChunkSize = sMaxCompressedChunkSize;
	}
}

UnpackStreamFile::~UnpackStreamFile()
{
	// Prevent us from being deleted while we're still active
	QMutexLocker lock(&_lock);

	Cleanup(false);
}

qint64 UnpackStreamFile::getTotalUnpackedBytes()
{
	return _fileTotalUnpackedBytes;
}

qint64 UnpackStreamFile::getTotalPackedBytes()
{
	return _fileTotalPackedBytes;
}

qint64 UnpackStreamFile::getSavedBytes()
{
    return _fileSavedBytes;
}

qint64 UnpackStreamFile::getProcessedBytes()
{
    return _fileProcessedBytes;
}

QString UnpackStreamFile::getFileName()
{
    return _filePath;
}

UnpackError::type UnpackStreamFile::getErrorType()
{
	return _errorType;
}

qint32 UnpackStreamFile::getErrorCode()
{
	return _errorCode;
}

// Internal members
void UnpackStreamFile::shutdown()
{
    // Set the flag before grabbing the lock to make sure the other threads see the shutdown flag
    _shutdown = true;

    // Grab the lock to block our caller until we have shutdown
	QMutexLocker lock(&_lock);
}

bool UnpackStreamFile::isShutdown()
{
    return _shutdown;
}

UnpackError::type UnpackStreamFile::tryResume()
{
	// If we're trying to resume an existing file
    qint64 fileSize;
	if (EnvUtils::GetFileDetailsNative(_filePath, fileSize))
	{
#ifdef ORIGIN_MAC
        QFile streamFile(_filePath);
        // Need to check if the file is a symbolic link
        QFileInfo finfo (_filePath);
        if( !finfo.isSymLink() ) // If not get the path size normally
        {
            _fileSavedBytes = streamFile.size();
        }
        else // Make sure we get the correct size of the relative link
        {
            char linktarget[512];
            memset(linktarget,0,sizeof(linktarget));
            _fileSavedBytes = readlink(qPrintable(_filePath), linktarget, sizeof(linktarget)-1);
        }
#endif
#ifdef ORIGIN_PC
        _fileSavedBytes = fileSize;
#endif

        qint64 diskFileBytes = _fileSavedBytes;  // Save this value, because it may be overwritten later

		// If the file is already complete, no need to do anything
		if (_fileSavedBytes == _fileTotalUnpackedBytes)
		{
			qDebug() << "No need to resume already completed file: " << _filePath;

			_fileProcessedBytes = _fileTotalPackedBytes;

			return UnpackError::AlreadyCompleted;
		}
		else if (_fileSavedBytes > _fileTotalUnpackedBytes)
		{
			qDebug() << "File was larger than expected, restarting: " << _filePath << " Saved: " << _fileSavedBytes << " Total: " << _fileTotalUnpackedBytes;

            // Reset all progress counters and wipe out the files
			ResetUnpackFileProgress();

			// TELEMETRY: Could not resume existing file, redownload.
            //GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), "RESUME_FAIL_TOO_LARGE", 0, _fileSavedBytes, _fileTotalUnpackedBytes);
		}
		else
		{
			if (_fileUnpackType == UnpackType::Deflate)
			{
				// Load the decompressor if it exists
				bool resumed = false;
				QString streamFilename = GetDecompressorFilename(_filePath);
				if (EnvUtils::GetFileExistsNative(streamFilename))
				{
					_fileDecompressor = new ZLibDecompressor::DeflateStreamDecompressor;

					int nStatus = _fileDecompressor->ResumeStream( streamFilename );
					if (nStatus == Z_OK)
					{
						qDebug() << "Loaded Stream file for: \"" << _filePath << "\" : Compressed_Bytes_Downloaded:" << _fileDecompressor->GetTotalInputBytesProcessed() << " Bytes_On_Disk:" << _fileSavedBytes << " Bytes_Confirmed:" << _fileDecompressor->GetTotalOutputBytes() << " Redownloading_Unconfirmed_Bytes: " << _fileSavedBytes - _fileDecompressor->GetTotalOutputBytes();
                        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ResumeFile", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

						// Update the counters with whatever the decompressor reports
						_fileProcessedBytes = _fileDecompressor->GetTotalInputBytesProcessed();
						_fileSavedBytes = _fileDecompressor->GetTotalOutputBytes();

                        // If the values we read back are nonsensical, we need to discard this stream
                        // Three cases which we consider nonsensical
                        //    1) If the 'processed' (compressed) bytes reported by the recomp file are
                        //        a) greater than the compressed size - The recomp file is corrupted
                        //        b) or equal to the compressed size - We have read all the compressed data but the file is still 'incomplete', so there must be a problem
                        //    2) If the 'saved' (uncompressed) bytes reported by the recomp file are greater than the total uncompressed size of the final file
                        //    3) If the 'saved' (uncompressed) bytes reported by the recomp file are more than we actually have on disk (we can have less [unconfirmed bytes], but never more)
                        if (_fileProcessedBytes >= _fileTotalPackedBytes || _fileSavedBytes > _fileTotalUnpackedBytes || _fileSavedBytes > diskFileBytes)
                        {
                            // We didn't restore any usable progress, so we'll just stay uninitialized for now
						    delete _fileDecompressor;
						    _fileDecompressor = NULL;

                            qDebug() << "Discarded invalid stream file: " << streamFilename << " Processed: " << _fileProcessedBytes << " PackedTotal: " << _fileTotalPackedBytes 
                                             << " Saved: " << _fileSavedBytes << " UnpackedTotal: " << _fileTotalUnpackedBytes << " BytesOnDisk: " << diskFileBytes;

                            QString telemStr1 = QString("File=%1,Processed=%2,TotalPacked=%3,Saved=%4,TotalUnpacked=%5,BytesOnDisk=%6").arg(streamFilename).arg(_fileProcessedBytes).arg(_fileTotalPackedBytes).arg(_fileSavedBytes).arg(_fileTotalUnpackedBytes).arg(diskFileBytes);

                            //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ResumeFile_StreamRecompInvalid", qPrintable(telemStr1), __FILE__, __LINE__);

                            // Try to remove the stream file just to make sure we never try to use it again for any reason
                            if (!QFile::remove(streamFilename))
                            {
                                qDebug() << "Unable to remove invalid stream file: " << streamFilename;

                                QString telemStr2 = QString("File=%1,OSError=%2").arg(streamFilename).arg((int)Services::PlatformService::lastSystemResultCode());

                                //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ResumeFile_StreamRecompDeleteFail", qPrintable(telemStr2), __FILE__, __LINE__);
                            }

                            resumed = false;
                        }
                        else
                        {
                            // We succeeded in resuming the decompressor
						    resumed = true;
                        }
					}
					else
					{
						qDebug() << "Decompression Stream Resume failed with zlib status code: " << nStatus << " For file: " << _filePath;

                        // TELEMETRY: Could not resume decompressing existing file, redownload.
                        ////GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), streamFilename.toLocal8Bit().constData(), "RESUME_FAIL", nStatus, _fileSavedBytes, _fileTotalUnpackedBytes);

						// We didn't restore any usable progress, so we'll just stay uninitialized for now
						delete _fileDecompressor;
						_fileDecompressor = NULL;
					}
				}
				else
				{
					qDebug() << "A decompressor should exist but didn't.  Redownloading: " << _filePath;

					// TELEMETRY: Could not resume existing file, redownload.
					////GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), "RESUME_FAIL_DECOMP_MISSING", 0, _fileSavedBytes, _fileTotalUnpackedBytes);
				}

				// If resuming failed, we have to start over
				if (!resumed)
				{
					// Reset all progress counters and wipe out the files
			        ResetUnpackFileProgress();
				}
			}
			else if (_fileUnpackType == UnpackType::Uncompressed)
			{
				// For uncompressed files, saved bytes = processed bytes
				_fileProcessedBytes = _fileSavedBytes;
			}
		}
	
	    // If file CRC checking is enabled
	    if (_fileCRC != 0)
	    {
		    quint32 crc = (quint32)-1;

		    // If we're resuming a file
		    if (_fileSavedBytes > 0)
		    {
			    // Redo the CRC for this partial file
			    qDebug() << "Regenerating CRC for partial file: " << _filePath;
			    crc = CRCMap::CalculateCRCForFile(_filePath, _fileSavedBytes, false);
			    if (crc == (quint32)-1)
			    {
				    qDebug() << "Unable to regenerate CRC.  Redownloading.";

                    QString telemStr = QString("File=%1,FileBytes=%2,ConfirmedBytes=%3,ExpectedCRC=%4,OSResult=%5").arg(_filePath).arg(diskFileBytes).arg(_fileSavedBytes).arg(_fileCRC).arg((int)Services::PlatformService::lastSystemResultCode());

                    //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ResumeFile_PartialCRCRegenFail", qPrintable(telemStr), __FILE__, __LINE__);

				    // Reset all progress counters and wipe out the files
			        ResetUnpackFileProgress();
			    }
			    else
			    {
				    // Save the partial file CRC
				    _fileComputedCRC = crc;
			    }
		    }
	    }
    }

	return UnpackError::OK;
}

void UnpackStreamFile::ResetUnpackFileProgress()
{
    // Blank out the counters so that we start over
	_fileSavedBytes = 0;
	_fileProcessedBytes = 0;

    ClobberFiles();
}

void UnpackStreamFile::ClobberFiles()
{
    // Delete the base file if it exists
    if (EnvUtils::GetFileExistsNative(_filePath))
    {
        QFile streamFile(_filePath);

        qDebug() << "Deleting: " << _filePath;

	    // We should delete the file that already existed
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ClobberFiles_DeleteBaseFile", qPrintable(_filePath), __FILE__, __LINE__);
        bool wasDeleted = streamFile.remove();
        ORIGIN_ASSERT(wasDeleted);
        if (wasDeleted == false)
        {
            qDebug() << "Unable to delete invalid file: " << _filePath << " OS Result: " << (int)streamFile.error();

            //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteBaseFile_error", qPrintable(_filePath), streamFile.error());

            // Check permissions and try again
		    QFile::Permissions perm = streamFile.permissions();

		    if (!(perm & QFile::WriteUser))
		    {
			    perm |= QFile::WriteUser;
			    streamFile.setPermissions(perm);
			    wasDeleted = streamFile.remove();	// try again
		    }

		    ORIGIN_ASSERT(wasDeleted);
		    if (wasDeleted == false)
		    {
                qDebug() << "Still unable to delete invalid file after adjusting permissions: " << _filePath << " OS Result: " << (int)streamFile.error();

                //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteBaseFile_retryerror", qPrintable(_filePath), streamFile.error());
		    }
            else
            {
                qDebug() << "Successfully deleted file after adjusting permissions: " << _filePath;

                //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteBaseFile_retrysuccess", qPrintable(_filePath), 0);
            }
        }
    }

    // Delete the stream recomposition file if it existed also
    QString streamFilename = GetDecompressorFilename(_filePath);
    if (EnvUtils::GetFileExistsNative(streamFilename))
    {
        qDebug() << "Deleting: " << streamFilename;

        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "ClobberFiles_DeleteStreamRecompFile", qPrintable(streamFilename), __FILE__, __LINE__);

        QFile recompFile(streamFilename);
        bool wasDeleted = recompFile.remove();
        if (wasDeleted == false)
        {
            qDebug() << "Unable to delete invalid stream recomposition file: " << streamFilename << " OS Result: " << (int)recompFile.error();

            //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteStreamRecompFile_error", qPrintable(streamFilename), (int)recompFile.error());

            // Check permissions and try again
		    QFile::Permissions perm = recompFile.permissions();

		    if (!(perm & QFile::WriteUser))
		    {
			    perm |= QFile::WriteUser;
			    recompFile.setPermissions(perm);
			    wasDeleted = recompFile.remove();	// try again
		    }

		    ORIGIN_ASSERT(wasDeleted);
		    if (wasDeleted == false)
		    {
                qDebug() << "Still unable to delete invalid stream file after adjusting permissions: " << streamFilename << " OS Result: " << (int)recompFile.error();

                //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteStreamRecompFile_retryerror", qPrintable(streamFilename), recompFile.error());
		    }
            else
            {
                qDebug() << "Successfully deleted stream file after adjusting permissions: " << streamFilename;

                //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ClobberFiles_DeleteStreamRecompFile_retrysuccess", qPrintable(streamFilename), 0);
            }
        }
    }
}

bool UnpackStreamFile::isPartial()
{
	return _fileSavedBytes > 0 && _fileSavedBytes < _fileTotalUnpackedBytes;
}

bool UnpackStreamFile::isComplete()
{
	return _fileSavedBytes == _fileTotalUnpackedBytes;
}

QString UnpackStreamFile::GetDecompressorFilename(const QString& baseFilename)
{
    if (baseFilename.contains('~'))
    {
        // only check if the file name not the folder names have a ~ in it.
        int start = baseFilename.lastIndexOf('/');

        if (start == -1)
            start = baseFilename.lastIndexOf('\\');	// in case path is constructed with other slash

        if (start == -1)
            start = 0;
        else 
            start++;        // skip the slash

        QString path = baseFilename.left(start);   // get the path part
        QString name = baseFilename.mid(start);    // get the filename part
        name.replace("~", "_");                 // only replace the ~ in the name part

        QString streamFileName(path + "s_t_r_" + name + "_stream");
        return streamFileName;
    }

    return baseFilename + "_stream";
}

void UnpackStreamFile::SaveDecompressor()
{
	if (_fileDecompressor)
	{
		_fileDecompressor->SaveStream(GetDecompressorFilename(_filePath));
	}
}

bool UnpackStreamFile::PeekProcessedBytes(const QString& baseFilename, UnpackType::code unpackType, qint64& bytesProcessed)
{
    if (!EnvUtils::GetFileDetailsNative(baseFilename, bytesProcessed))
        return false;

    if (unpackType == UnpackType::Deflate)
	{
        QString streamFilename = GetDecompressorFilename(baseFilename);

        if (!EnvUtils::GetFileExistsNative(streamFilename))
        {
            bytesProcessed = 0;
            return false;
        }

		ZLibDecompressor::DeflateStreamDecompressor decomp;

		int nStatus = decomp.ResumeStream( streamFilename );
		if (nStatus == Z_OK)
		{
            bytesProcessed = decomp.GetTotalInputBytesProcessed();
            return true;
        }

        bytesProcessed = 0;
        return false;
    }
    else
    {
        return true;
    }
}

void UnpackStreamFile::Cleanup(bool cleanFiles)
{
	// First, close our actual file handle if it is open
	if (_fileHandle.openMode() != QIODevice::NotOpen)
	{
		// Close the file handle
		_fileHandle.close();
	}

	if (cleanFiles)
	{
		// Make sure we wipe out any decompressor that may have been saved and wipe out our actual file if we've hit an error
		ClobberFiles();
	}
	else
	{
		if (!isComplete())
		{
			// We might be pausing, so save the decompressor if we need to
			// But don't save decompressor if the file name has a ~ in it as the file can lead to other files with ~ to grab the _stream file. (EBIBUGS-17798)
			if (_fileDecompressor && isPartial())
			{
				qDebug() << "Saving decompressor on shutdown for: " << _filePath;
                //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "SavingDecomp", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);
				SaveDecompressor();
			}
		}
	}

	if (_fileDecompressor)
	{
		delete _fileDecompressor;
		_fileDecompressor = NULL;
	}
}

// Utility function
QString GetTelemetrySafeString(QString input)
{
    QString output = input;
    output.replace("[", "+");
    output.replace("]", "+");
    output.replace("=", "~");
    output.replace(";","$");
    output.replace(":","_");

    return output;
}

QString GetURLFilePart(QString input)
{
    QFileInfo url(input);
    return url.fileName();
}

void UnpackStreamFile::DumpDiagnosticData()
{
    // snip
}

void UnpackStreamFile::Error(Origin::Downloader::DownloadErrorInfo errorInfo, bool cleanFiles, bool fatal)
{
	// Clean up after ourselves
	Cleanup(cleanFiles);

	// Save the error values
	_errorType = static_cast<UnpackError::type>(errorInfo.mErrorType);
    _errorCode = errorInfo.mErrorCode;

    qDebug() << "*** Unpack Stream File Error: " << (int)_errorType << " " << (int)_errorCode;
}

bool UnpackStreamFile::processChunk(void* data, quint32 dataLen, quint32 &bytesWritten, bool &fileComplete)
{
	// Prevent us from being deleted while we're still active
	QMutexLocker lock(&_lock);

	bool retval = true;
	
    QString szError;

    // Increment the chunk count
    ++_fileChunkCount;

    if (!prepDestinationFileForWriting())
    {
        return false;
    }

    // Decompress (if needed) and write the data
    if (_fileUnpackType == UnpackType::Deflate)
    {
        if (!writeChunk_Compressed(data, dataLen, bytesWritten))
            return false;
    }
    else
    {
        // Make sure there is still buffer to write after the tag utility might have consumed some
        if (dataLen > 0)
        {
            if (!writeChunk_Uncompressed(data, dataLen, bytesWritten))
                return false;
        }
        else
        {
            bytesWritten = 0;
        }
    }

	fileComplete = false;

    // Determine whether we processed too much data
    if (_fileProcessedBytes > _fileTotalPackedBytes)
    {
        qDebug() << "Too many bytes processed!  Processed: " << _fileProcessedBytes << " > total compressed size: " << _fileTotalPackedBytes << " File: " << _filePath;

        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "TooManyCompressedBytes", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

        DumpDiagnosticData();

        // Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::StreamStateInvalid;
        errorInfo.mErrorCode = 1;
        errorInfo.mErrorContext = _filePath;
		Error(errorInfo);

		return false;
    }
	// Determine whether the file has completed
	else if (_fileSavedBytes > _fileTotalUnpackedBytes)
	{
		qDebug() << "Too many bytes!  Saved: " << _fileSavedBytes << " > total size: " << _fileTotalUnpackedBytes << " File: " << _filePath;
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "TooManyBytes", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

        DumpDiagnosticData();

		// Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::StreamStateInvalid;
        errorInfo.mErrorCode = 2;
        errorInfo.mErrorContext = _filePath;
		Error(errorInfo);

		return false;
	}
    // Determine if we ran off the end of the compressed stream but failed to decompress enough data
    else if (_fileProcessedBytes == _fileTotalPackedBytes && _fileSavedBytes != _fileTotalUnpackedBytes)
    {
        qDebug() << "Ran out of compressed stream but did not decompress enough data!  Saved: " << _fileSavedBytes << " Total Size: " << _fileTotalUnpackedBytes << " Compressed Processed: " << _fileProcessedBytes << " File: " << _filePath;
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "CompressedStreamExhausted", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

        DumpDiagnosticData();

        // Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::StreamStateInvalid;
        errorInfo.mErrorCode = 3;
        errorInfo.mErrorContext = _filePath;
		Error(errorInfo);

		return false;
    }
    // We completed successfully
	else if (_fileSavedBytes == _fileTotalUnpackedBytes)
	{
		quint32 finalCRC = _fileComputedCRC ^ CRC_INITIAL_VALUE;
		if (_fileCRC != 0 && finalCRC != _fileCRC)
		{
			qDebug() << "CRC Mismatch [" << QString("0x%1").arg(_fileCRC, 8, 16) << " != " << QString("0x%1").arg(finalCRC, 8, 16) << "]! Redownloading File: " << _filePath;

            // TELEMETRY: CRCs didn't match
            //GetTelemetryInterface()->Metric_DL_CRC_FAILURE(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), _fileUnpackType, _fileCRC, finalCRC, _fileDate, _fileTime);

            DumpDiagnosticData();

            if (!_ignoreCRCErrors)
            {
			    // Fail this unpack file
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::CRCFailed;
                errorInfo.mErrorCode = 0;
                errorInfo.mErrorContext = _filePath;
			    Error(errorInfo);

			    return false;
            }
		}

		Q_ASSERT(_fileProcessedBytes > 0 && _fileSavedBytes > 0);

        // Trim any extra bytes from the file
        // This could happen in cases where the file was previously larger or if there was a crash or power outage previously
		bool didResize = _fileHandle.resize(_fileHandle.pos());
        ORIGIN_ASSERT(didResize);
		if (!didResize)
		{
            QFile::FileError error = _fileHandle.error();
			qDebug() << "Couldn't truncate file at the end of the download." << "\". (" << error << ")";
            //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "TruncateFailure", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);
		}

#if defined(Q_OS_WIN)
		CTime fileModDateTime((WORD) _fileDate, (WORD) _fileTime, -1);
		SYSTEMTIME sysTime;
		fileModDateTime.GetAsSystemTime(sysTime);

		FILETIME localFileTime;
		SystemTimeToFileTime(&sysTime, &localFileTime);

		FILETIME fileTime;
		LocalFileTimeToFileTime(&localFileTime, &fileTime);

		bool bSetTimeSuccess = (bool)SetFileTime((HANDLE)_get_osfhandle(_fileHandle.handle() ), &fileTime, &fileTime, &fileTime) == TRUE;

#else
		// convert the DOS date/time to Mac/Unix(seconds since midnight 1-1-1970)
		qint32 year = ((_fileDate >> 9) & 0x7f) + 1980;
		qint32 month = (_fileDate >> 5) & 0x0f;
		qint32 day = _fileDate & 0x1f;
		QDate date(year, month, day);

		qint32 hour = (_fileTime >> 11) & 0x1f;
		qint32 minute = (_fileTime >> 5) & 0x3f;
		qint32 second = (_fileTime << 1) & 0x3e;
		QTime time(hour, minute, second);
		
		QDateTime dateTime(date, time);
		qint64 timeInSeconds = dateTime.toMSecsSinceEpoch() / qint64(1000);

		struct utimbuf fileTime;
		fileTime.actime = timeInSeconds;
		fileTime.modtime = timeInSeconds;

		// if utime returns 0, then it has succeeded in modifying the date/time of the file
		bool bSetTimeSuccess = (utime(_filePath.toLocal8Bit(), &fileTime) == 0);
#endif
		if (!bSetTimeSuccess)
		{
			qDebug() << "Couldn't update the filetime for file " << _filePath << ".";
            //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "FileTimeError", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);
		}
		// Close the file handle
		_fileHandle.close();
        
#if !defined(Q_OS_WIN)
        // Set the attributes, this is particularly important since Unix-based OSs (like MacOS) depend on the execute bit
        if (_fileAttributes != 0)
        {
            // Handle symlinks
            if ((_fileAttributes & S_IFLNK) == S_IFLNK)
            {
                // remove what we just wrote, create a symlink in its place
                //
                if (_fileHandle.remove())
                {
                    QByteArray ba((const char*) data, dataLen);
                    QString link_target(ba);
                    
                    if(!_fileHandle.link(link_target, _filePath))
                    {
                        qDebug() << "Unable to create link from " << _filePath << " to " << link_target;
                        // Fail this unpack file
                        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                        errorInfo.mErrorType = UnpackError::IOWriteFailed;
                        errorInfo.mErrorCode = 0;
                        errorInfo.mErrorContext = _filePath;
                        Error(errorInfo);

                        return false;
                    }
                }
                else
                {
                    qDebug() << "Unable to clean-up temp symlink file!";
                    // Fail this unpack file
                    CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                    errorInfo.mErrorType = UnpackError::IOWriteFailed;
                    errorInfo.mErrorCode = 0;
                    errorInfo.mErrorContext = _filePath;
                    Error(errorInfo);

                    //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("UnpackStreamFile_processPartialChunk_Mac", _filePath.toUtf8().data(), _fileHandle.error());
                    
                    return false;
                }
            }
            
            // Strip everything but the execute permission bits.
            quint32 unixPermissions = _fileAttributes & 0777;
            
            // Explicitly set relaxed permissions to allow resuming downloads by other users.
            unixPermissions |= 0666;
            
            chmod(_filePath.toLocal8Bit(), unixPermissions);
        }
#endif        

		// Mark ourself complete
		fileComplete = true;

#ifdef _DEBUG
		// TODO: re-enable this
		//int64_t nFilesCompletedOrStaged = mFileProgressMap.GetTotalFilesInState(kCompleted) + mFileProgressMap.GetTotalFilesInState(kStaged);
		//EBILOGEVENT << L"File Completed Downloading: \"" << pEntry->GetFileName() << L"\"  " << pEntry->GetUncompressedSize() << L" bytes. Completed " << (int) nFilesCompletedOrStaged << L" of " << (int) mnIncludedFiles << L" files.";
#endif
	}


	if (retval)
	{
        double pct = (_fileSavedBytes / (double)_fileTotalUnpackedBytes) * 100.0;
        qDebug() << "Unpack Stream File processed " << dataLen << " bytes, wrote " << bytesWritten << " bytes. - " << pct << "%";
	}
	else
	{
		// report some telemetry on decomp error - this will correlate with range data sent in ContentProtocolPackage::onUnpackFileError() (11/9/12)
		QString errorStr;

		errorStr = QString("file: %1 ").arg(getFileName().right(180));
		errorStr += QString("offset:%1 length:%2").arg(bytesWritten).arg(dataLen);
		//GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "DecompFail", errorStr.toUtf8().data(), __FILE__, __LINE__);
	}

	return retval;
}

bool UnpackStreamFile::prepDestinationFileForWriting()
{
    	//Now open/create the file
	if (_fileHandle.openMode() == QIODevice::NotOpen)
	{
		// Make sure the directory exists
        QFileInfo fileInfo(_filePath);
        QDir fullDir(fileInfo.canonicalPath());

		if (!fullDir.exists())
		{
            qDebug() << "Directory did not exist, \"" << fullDir.canonicalPath();
            //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "DirectoryMissing", fullDir.canonicalPath().toLocal8Bit().constData(), __FILE__, __LINE__);

            // Fail this unpack file
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = UnpackError::DirectoryNotFound;
            errorInfo.mErrorCode = -1;
            errorInfo.mErrorContext = fullDir.canonicalPath();
            Error(errorInfo);

            return false;
        }

        bool wasOpened = _fileHandle.open(QIODevice::ReadWrite);

		if (!wasOpened)
		{
			// check permissions
			QFile::Permissions perm = _fileHandle.permissions();

			if (!(perm & QFile::WriteUser))
			{
				perm |= QFile::WriteUser;
				_fileHandle.setPermissions(perm);
            }

            int numRetries = 0;
            while (!wasOpened && numRetries < kMaxFileOpenRetries)
            {
			    // try again
                ++numRetries;
			    wasOpened = _fileHandle.open(QIODevice::ReadWrite);

                // If we failed, log an error
                if (!wasOpened)
                {
                    qDebug() << "Unable to open file (" << _filePath << ") for read/write.  Retry #" << numRetries << " failed.  Sleeping " << kFileOpenRetryDelayMS << "ms";

                    // Sleep.  Note that this is in a thread pool, so it should be OK to block here for some time
                    Origin::Services::PlatformService::sleep(kFileOpenRetryDelayMS);
                }
                else
                {
                    qDebug() << "Opened file (" << _filePath << ") for read/write after retry #" << numRetries << ".";
                }
            } 

			ORIGIN_ASSERT(wasOpened);
			Q_UNUSED(wasOpened);
		}
	}

	if ( _fileHandle.openMode() == QIODevice::NotOpen )
	{
        qint32 error = static_cast<qint32>(_fileHandle.error());

//#ifdef ORIGIN_PC
//        Services::PlatformService::SystemResultCode lastSystemError = (Services::PlatformService::SystemResultCode)_fileHandle.nativeError();
//#else
        // TODO: Implement the internals for QFile::nativeError() on Mac (if we care)
        int lastSystemError = Services::PlatformService::lastSystemResultCode();
//#endif
        QString lastSystemErrorString(QString("%1").arg(lastSystemError));
        QString errorDescription(QString("OpenFileError (%1) (%2:%3)").arg(error).arg(lastSystemErrorString).arg(static_cast<qint32>(lastSystemError)));

#ifdef ORIGIN_PC
        if ((int)lastSystemError == ERROR_ACCESS_DENIED) // 5
        {
            //// Grab the DACL
            //QString dacl;
            //Services::PlatformService::GetDACLStringFromPath(_filePath, true, dacl);

            //if (!dacl.isEmpty())
            //    errorDescription.append(QString("[DACL;%1]").arg(dacl));
        }
        else if ((int)lastSystemError == ERROR_SHARING_VIOLATION) // 32
        {
           /* QString rebootReason;
            QString lockingProcessesSummary;
            QList<EnvUtils::FileLockProcessInfo> processes;

            if (EnvUtils::GetFileInUseDetails(_filePath, rebootReason, lockingProcessesSummary, processes))
            {
                if (!lockingProcessesSummary.isEmpty())
                    errorDescription.append(QString("[InUse;%1/%2]").arg(rebootReason).arg(lockingProcessesSummary));
            }*/
        }
#endif

        qDebug() << "Error opening file \"" << _filePath << "\". " << errorDescription;
        
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), errorDescription.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

		// Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::IOCreateOpenFailed;
        errorInfo.mErrorCode = (int)lastSystemError;
        errorInfo.mErrorContext = _filePath;
		Error(errorInfo);
		return false;
	}

	//Restore File pointer
    bool didSeek = _fileHandle.seek(_fileSavedBytes);
    ORIGIN_ASSERT(didSeek);
    
	if (!didSeek)
	{
        QFile::FileError error = _fileHandle.error();

//#ifdef ORIGIN_PC
//        Services::PlatformService::SystemResultCode lastSystemError = (Services::PlatformService::SystemResultCode)_fileHandle.nativeError();
//#else
        // TODO: Implement the internals for QFile::nativeError() on Mac (if we care)
        int lastSystemError = Services::PlatformService::lastSystemResultCode();
//#endif
        QString lastSystemErrorString(QString("%1").arg(lastSystemError));
        QString errorDescription(QString("SetFilePtr (%1) (%2:%3)").arg(error).arg(lastSystemErrorString).arg(static_cast<qint32>(lastSystemError)));

		qDebug() << "Couldn't set file pointer to correct offset into destination file \"" << _filePath << "\". (" << errorDescription << ")";
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "SetFilePtr", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

		// Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::IOWriteFailed;
        errorInfo.mErrorCode = (int)lastSystemError;
        errorInfo.mErrorContext = _filePath;
		Error(errorInfo);

		return false;
	}


    return true;
}

bool UnpackStreamFile::writeChunk_Compressed(void* data, quint32 dataLen, quint32& bytesWritten)
{
		// Make a new decompressor if necessary
		if (_fileDecompressor == NULL)
		{
			_fileDecompressor = new ZLibDecompressor::DeflateStreamDecompressor;
			_fileDecompressor->Init();
		}

		// Decompress the data
		if ((qint64)_fileDecompressor->GetTotalInputBytesProcessed() != _fileProcessedBytes)
		{
			// If for any reason the number of bytes processed by the decompressor does not match the number of bytes downloaded,
			// Don't do anything with the data.
			qDebug() << "Decompression stream doesn't match downloaded stream....redownloading: " << _filePath << " Processed: " << _fileDecompressor->GetTotalInputBytesProcessed() << " Downloaded: " << _fileProcessedBytes;

			// TELEMETRY: Data is missing or corrupted, redownload.
            //GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), "BYTES_PROCESSED_MISMATCH", 0, _fileSavedBytes, _fileTotalUnpackedBytes);

            DumpDiagnosticData();

			// Fail this unpack file
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = UnpackError::StreamStateInvalid;
            errorInfo.mErrorCode = 0;
            errorInfo.mErrorContext = _filePath;
			Error(errorInfo);

			return false;
		}

        // Initialize the decompressor with the buffer
        _fileDecompressor->StartBuffer((uint8_t*)data, dataLen);

        //qDebug() << "decompress.....file:" << _filePath << " input bytes processed:" << _fileDecompressor->GetTotalInputBytesProcessed() << " dataLen:" << dataLen;

        // Decompress loop.
        // The decompressor may run out of output buffer, so loop until the decompressor has no more output
        int32_t nDecompStatus;
        do
        {
	        nDecompStatus = _fileDecompressor->Decompress();

		    if (!(nDecompStatus == Z_OK || nDecompStatus == Z_STREAM_END))      // if nStatus is anything but OK or END
		    {
			    qDebug() << "Couldn't decompress downloaded data:  Error code: " << nDecompStatus << "!\nRedownloading file: " << _filePath << "\n";

			    // TELEMETRY: Could not decompress downloaded data, redownload.
                //GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), "DECOMPRESSION_FAIL", nDecompStatus, _fileSavedBytes, _fileTotalUnpackedBytes);

                DumpDiagnosticData();

			    // Fail this unpack file
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::StreamUnpackFailed;
                errorInfo.mErrorCode = nDecompStatus;
                errorInfo.mErrorContext = _filePath;
			    Error(errorInfo);

			    return false;
		    }

		    int32_t nDecompressedSize = _fileDecompressor->GetDecompressedBytes();
		    uint8_t* pDecompressedBuffer = _fileDecompressor->GetDecompressedBuffer();


            if (nDecompressedSize > 0 && pDecompressedBuffer)
            {

                // Update the CRC (if CRC checking is enabled)
                if (_fileCRC != 0)
                {
                    _fileComputedCRC = CRCMap::CalculateCRC(_fileComputedCRC, pDecompressedBuffer, nDecompressedSize);
                }

                // Write the chunk that corresponds to this file
                if (!writeChunkToDisk(reinterpret_cast<const char*>(pDecompressedBuffer), nDecompressedSize))
                    return false;
            }

            //	Update counters
            _fileSavedBytes += nDecompressedSize;
            bytesWritten += nDecompressedSize;    // for reporting back
            _fileProcessedBytes = _fileDecompressor->GetTotalInputBytesProcessed();

        } while (_fileDecompressor->HasMoreOutput() && !_shutdown);

        // Determine if this is a multi-part file
        bool multipart = false;
        if (dataLen < _fileTotalPackedBytes)
        {
            multipart = true;
        }

        // Save the decompressor for multi-chunk files every so often (every 5 chunks)
        if (multipart && (_fileChunkCount % 5) == 0 && _fileDecompressor->NeedsMoreInput())
        {
            SaveDecompressor();
        }

        if (_fileDecompressor->NeedsMoreInput() == false)   // end of file
        {
            // Clear out the decompressor if we've hit the end of the stream
            delete _fileDecompressor;
            _fileDecompressor = NULL;

            // Delete the stream file if there is one
            QString sStreamPath = GetDecompressorFilename(_filePath);
            if (EnvUtils::GetFileExistsNative(sStreamPath))
            {
                QFile fileToDelete(sStreamPath);
                bool wasDeleted = fileToDelete.remove();
                ORIGIN_ASSERT(wasDeleted);
                if (wasDeleted == false)
                {
                    //GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("UnpackStreamFile_processPartialChunk", sStreamPath.toUtf8().data(), fileToDelete.error());
                }
            }

            if ( _fileSavedBytes != _fileTotalUnpackedBytes )
            {
                qDebug() << "Decompression State is Z_STREAM_END but bytes saved: " << _fileSavedBytes << " != total size:" << _fileTotalUnpackedBytes << " File: " << _filePath;

                // TELEMETRY: File unpacked successfully but size mismatch, redownload.
                //GetTelemetryInterface()->Metric_DL_REDOWNLOAD(_productId.toLocal8Bit().constData(), _filePath.toLocal8Bit().constData(), "BYTES_WRITTEN_MISMATCH", 0, _fileSavedBytes, _fileTotalUnpackedBytes);

                // Fail this unpack file
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::StreamStateInvalid;
                errorInfo.mErrorCode = 0;
                errorInfo.mErrorContext = _filePath;
                Error(errorInfo);

                return false;
            }
        }



        return true;
}

bool UnpackStreamFile::writeChunk_Uncompressed(void* data, quint32 dataLen, quint32& bytesWritten)
{
    // No compression... just save the buffer straight
    uint8_t* pDecompressedBuffer = (uint8_t*)data;


    // Update the CRC (if CRC checking is enabled)
    if (_fileCRC != 0)
    {
        _fileComputedCRC = CRCMap::CalculateCRC(_fileComputedCRC, pDecompressedBuffer, dataLen);
    }

    // Write the chunk that corresponds to this file
    if (!writeChunkToDisk(reinterpret_cast<const char*>(pDecompressedBuffer), dataLen))
        return false;

    //	Update counters
    bytesWritten = dataLen;
    _fileSavedBytes += dataLen;
    _fileProcessedBytes += dataLen;

    return true;
}

bool UnpackStreamFile::writeChunkToDisk(const char* data, qint64 len)
{
    // Write the chunk
    qint64 numBytesWritten = _fileHandle.write(data, len);
    ORIGIN_ASSERT(numBytesWritten == len);

    if ( numBytesWritten != len )
    {
        // Get the Qt error (which is probably fairly generic)
        QFile::FileError qterror = _fileHandle.error();
        int errorCode = qterror;
        QString errorString = _fileHandle.errorString();

//#ifdef ORIGIN_PC
//        Services::PlatformService::SystemResultCode lastSystemError = (Services::PlatformService::SystemResultCode)_fileHandle.nativeError();
//#else
        // TODO: Implement the internals for QFile::nativeError() on Mac (if we care)
        int lastSystemError = Services::PlatformService::lastSystemResultCode();
//#endif
        QString lastSystemErrorString(QString("%1").arg(lastSystemError));
        errorString = QString("WriteChunkToDisk (%1) (%2:%3)").arg(qterror).arg(lastSystemErrorString).arg(static_cast<qint32>(lastSystemError));

        bool fatalError = false;
        bool cleanFiles = true;

#ifdef ORIGIN_PC
        // On PC, we have QFile::nativeError() which actually retrieves the real GetLastError()
        errorCode = (int)lastSystemError;
#else
        // Check for low disk space specifically (since we don't have QFile::nativeError() implemented for Mac right now)
        bool isLowDiskSpace = (qterror == QFile::ResourceError);
        
        if (qterror == QFile::WriteError) {
            qint64 freeSpace = Services::PlatformService::GetFreeDiskSpace(_filePath);
            isLowDiskSpace = (freeSpace != -1 && freeSpace < kMinimumAvailableDiskSpace);
        }
        
        if (isLowDiskSpace) {
            // Override the error code and string
            errorCode = Services::PlatformService::ErrorDiskFull;
            errorString = QString("WriteChunkToDisk (%1) (%2:%3)").arg(qterror).arg(Services::PlatformService::systemResultCodeString(Services::PlatformService::ErrorDiskFull)).arg(static_cast<qint32>(errorCode));
        }
#endif

        // If it was a disk full error
        if (errorCode == 112)
        {
            // No need to wipe everything out for disk full errors
            cleanFiles = false;

            // But it is a fatal error
            fatalError = true;

            // Clear the decompressor so that the stream state doesn't get saved because the stream is now ahead of the file
            if (_fileDecompressor)
            {
				delete _fileDecompressor;
				_fileDecompressor = NULL;
            }
        }

        qDebug() << "Couldn't write to file \"" << _filePath << "\". (" << errorString << ") ";
        //GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "WriteError", _filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

        // Fail this unpack file
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = UnpackError::IOWriteFailed;
        errorInfo.mErrorCode = errorCode;
        errorInfo.mErrorContext = _filePath;
        Error(errorInfo, cleanFiles, fatalError);

        return false;
    }

    return true;
}

}//namespace
}
