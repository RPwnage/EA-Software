#include "io/UnpackStream.h"
#include "services/downloader/UnpackStreamFile.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/Common.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

#include "UnpackServiceInternal.h"

#include <QDebug>
#include <QSharedPointer>

namespace Origin
{
namespace Downloader
{

UnpackStream::UnpackStream(const QString& productId, const QString& path, const QMap<QString,QString>& contentTagBlock, QThreadPool *threadPool) :
	_unpackIds(0),
	_unpackDir(path),
    _productId(productId),
    _contentTagBlock(contentTagBlock),
    _fatalError(false),
	_threadPool(threadPool)
{
	if (!_unpackDir.endsWith("\\"))
		_unpackDir.append("\\");
}

UnpackStream::~UnpackStream()
{
	// Clean up after ourselves
	ORIGIN_LOG_DEBUG << "Unpack Stream cleaning up.  Outstanding files: " << _unpackFiles.count();

    // Tell all the unpack files to shut down if they are still outstanding
	for (QMap<UnpackStreamFileId, QSharedPointer<UnpackStreamFile> >::Iterator it = _unpackFiles.begin(); it != _unpackFiles.end(); it++)
	{
		it.value()->shutdown();
	}

    // These will get deleted by their QSharedPointers if they are not in use
	_unpackFiles.clear();
}

// IUnpackStream members
UnpackStreamFileId UnpackStream::getNewId()
{
	return ++_unpackIds;
}

UnpackError::type UnpackStream::createUnpackFile(UnpackStreamFileId id, QString &productId, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc, bool ignoreCRCErrors, bool clearExistingFiles)
{
    QString filePath = CFilePath::Absolutize( _unpackDir,  path ).ToString();

    // We don't support anything else at the moment
	if (unpackType != UnpackType::Deflate && unpackType != UnpackType::Uncompressed)
	{
		ORIGIN_LOG_ERROR << "Unsupported compression format [" << unpackType << "] for file:" << filePath;
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(_productId.toLocal8Bit().constData(), "UnsupportedZipFormat", filePath.toLocal8Bit().constData(), __FILE__, __LINE__);

		return UnpackError::StreamTypeInvalid;
	}

    // Get a QWeakPointer self-reference from the Unpack Service
    // This is a workaround for the fact that in Qt 5.0+, QWeakPointers are only constructable from the parent QSharedPointer
    // So UnpackStream cannot create a QWeakPointer<UnpackStream> directly from its own 'this' pointer.
    QWeakPointer<UnpackStream> weakSelfRef;
    if (UnpackService::GetInternalData())
    {
        weakSelfRef = UnpackService::GetInternalData()->GetStreamWeakRef(this);
    }

	QSharedPointer<UnpackStreamFile> file = QSharedPointer<UnpackStreamFile>(new UnpackStreamFile(weakSelfRef, id, filePath, packedSize, unpackedSize, date, time, fileAttributes, unpackType, crc, productId, ignoreCRCErrors));
	UnpackError::type ret = UnpackError::OK; 

    // Pass on the content tag block if it exists
    if (!_contentTagBlock.empty())
    {
        file->setContentTagBlock(_contentTagBlock);
    }

    if (clearExistingFiles)
    {
        // Clobber all the files
        file->Cleanup(true);
    }
    else
    {
        // Try to resume normally
        ret = file->tryResume();
    }

	if (ret == UnpackError::OK)
	{
		_unpackFiles[id] = file;
		return ret;
	}
	else
	{
        // The QSharedPointer will clean up automatically
		return ret;
	}
}

IUnpackStreamFile* UnpackStream::getUnpackFileById(UnpackStreamFileId id)
{
	if (_unpackFiles.contains(id))
	{
        // Our API clients do not need a shared pointer because they use our APIs for managing the lifetimes of UnpackStreamFile objects
		return _unpackFiles[id].data();
	}
	return NULL;
}

void UnpackStream::closeUnpackFile(UnpackStreamFileId id)
{
	if (_unpackFiles.contains(id))
	{
        // The QSharedPointer will delete the UnpackStreamFile automatically if it is not in use
        _unpackFiles[id]->shutdown();
		_unpackFiles.remove(id);
	}
}

void UnpackStream::queueChunk(UnpackStreamFileId id, void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData)
{
	// Queue the chunk with the thread pool
	if (_unpackFiles.contains(id))
	{
		// Queue a runnable object (holds a weak pointer to our UnpackStreamFile object)
		UnpackStreamFileChunk *chunk = new UnpackStreamFileChunk(_unpackFiles[id], data, dataLen, diagData);
		_threadPool->start(chunk);
	}
}

// Used by the unpack files
void UnpackStream::reportUnpackFileChunkProcessed(UnpackStreamFileId id, qint64 downloadedByteCount, qint64 processedByteCount, bool completed)
{
	emit (UnpackFileChunkProcessed(id, downloadedByteCount, processedByteCount, completed));
}

void UnpackStream::reportUnpackFileError(UnpackStreamFileId id, Origin::Downloader::DownloadErrorInfo errorInfo, bool fatal)
{
    // If the error is fatal
    if (fatal)
    {
        // Save the 'fatalness' of the error
        _fatalError = true;
    }

	emit (UnpackFileError(id, errorInfo));
}

const QString &UnpackStream::productId() const
{
    return _productId;
}

bool UnpackStream::fatalError() const
{
    return _fatalError;
}

}//namespace
}
