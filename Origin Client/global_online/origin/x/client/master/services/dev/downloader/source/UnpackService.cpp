#include "services/downloader/UnpackService.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"

#include "io/UnpackStream.h"
#include "UnpackServiceInternal.h"
#include "services/downloader/UnpackStreamFile.h"

#include <QMetaType>
#include <QThreadPool>

namespace Origin
{
namespace Downloader
{

// -1 here indicates let QThreadPool decide based on the number of cores
const int kDefaultMaxUnpackThreads = -1;

static UnpackService* g_unpackServiceInstance = NULL;

UnpackServiceData::UnpackServiceData()
{

}

UnpackServiceData::~UnpackServiceData() 
{
    QMutexLocker lock(&_mapLock);

    _unpackStreams.clear(); 
    _unpackSingleFiles.clear(); 
}

void UnpackServiceData::AddUnpackStream(QSharedPointer<UnpackStream> unpackStream)
{
    QMutexLocker lock(&_mapLock);

    _unpackStreams[static_cast<void*>(unpackStream.data())] = unpackStream;
}
void UnpackServiceData::RemoveUnpackStream(void* pUnpackStream)
{
    QMutexLocker lock(&_mapLock);

    // QSharedPointer will take care of the cleanup
    _unpackStreams.remove(pUnpackStream);
}

QWeakPointer<UnpackStream> UnpackServiceData::GetStreamWeakRef(const UnpackStream* pUnpackStream)
{
    QMutexLocker lock(&_mapLock);

    void *mapKey = static_cast<void*>(const_cast<UnpackStream*>(pUnpackStream));

    if (_unpackStreams.contains(mapKey))
    {
       return _unpackStreams[mapKey].toWeakRef();
    }
    return QWeakPointer<UnpackStream>();
}

void UnpackServiceData::AddSingleUnpackStreamFile(QSharedPointer<UnpackStreamFile> unpackStreamFile)
{
    QMutexLocker lock(&_mapLock);

    _unpackSingleFiles[static_cast<void*>(unpackStreamFile.data())] = unpackStreamFile;
}
void UnpackServiceData::RemoveSingleUnpackStreamFile(void* pUnpackStreamFile)
{
    QMutexLocker lock(&_mapLock);

    // QSharedPointer will take care of the cleanup
    _unpackSingleFiles.remove(pUnpackStreamFile);
}

QThreadPool* UnpackServiceData::GetThreadPool()
{
    return &_unpackThreadPool;
}

UnpackService::UnpackService() :
    _data(new UnpackServiceData())
{

}

UnpackService::~UnpackService()
{
    delete _data;
}

UnpackServiceData* UnpackService::GetInternalData()
{
    // This should be impossible unless something has failed catastrophically
    if (!g_unpackServiceInstance)
		return NULL;

    return g_unpackServiceInstance->_data;
}

void UnpackService::init()
{
	if (g_unpackServiceInstance)
		return;
	g_unpackServiceInstance = new UnpackService();

	qRegisterMetaType<UnpackStreamFileId> ("UnpackStreamFileId");

	// Do initialization here

	int maxUnpackThreads = Origin::Services::readSetting(Origin::Services::SETTING_MaxUnpackServiceThreads);

	if(maxUnpackThreads < 1)
	{
		maxUnpackThreads = kDefaultMaxUnpackThreads;
	}
	else
	{
		//TODO: We can't log in service initialization because logging hasn't been initialized yet. revisit
		//ORIGIN_LOG_EVENT << "[DownloaderTuning] Using maximum threads for unpack service [" << maxUnpackThreads << "]";
	}

	// Set the max number of threads, if necessary
	if (maxUnpackThreads != -1)
		GetInternalData()->GetThreadPool()->setMaxThreadCount(maxUnpackThreads);

	int maxUnpackCompressedChunkSize = Origin::Services::readSetting(Origin::Services::SETTING_MaxUnpackCompressedChunkSize);
	int maxUnpackUncompressedChunkSize = Origin::Services::readSetting(Origin::Services::SETTING_MaxUnpackUncompressedChunkSize);

	if(maxUnpackCompressedChunkSize > 0)
	{
		//TODO: We can't log in service initialization because logging hasn't been initialized yet. revisit
		//ORIGIN_LOG_EVENT << "[DownloaderTuning] Using maximum compressed chunk size for unpack service [" << maxUnpackCompressedChunkSize << "]";
	}

	if(maxUnpackUncompressedChunkSize > 0)
	{
		//TODO: We can't log in service initialization because logging hasn't been initialized yet. revisit
		//ORIGIN_LOG_EVENT << "[DownloaderTuning] Using maximum uncompressed chunk for unpack service [" << maxUnpackUncompressedChunkSize << "]";
	}

	UnpackStreamFile::setMaxChunkSizes(maxUnpackCompressedChunkSize, maxUnpackUncompressedChunkSize);
}

void UnpackService::release()
{
	if (!g_unpackServiceInstance)
		return;
	delete g_unpackServiceInstance;
    g_unpackServiceInstance = NULL;
}

IUnpackStream* UnpackService::GetUnpackStream(const QString& productId, const QString& unpackDir, const QMap<QString,QString>& contentTagBlock)
{
	if (!GetInternalData())
		return NULL;

    QSharedPointer<UnpackStream> stream = QSharedPointer<UnpackStream>(new UnpackStream(productId, unpackDir, contentTagBlock, GetInternalData()->GetThreadPool()));
    GetInternalData()->AddUnpackStream(stream);
	return stream.data();
}

void UnpackService::DestroyUnpackStream(IUnpackStream* stream)
{
    if (!GetInternalData())
		return;

	// QSharedPointer will take care of the cleanup
    GetInternalData()->RemoveUnpackStream(static_cast<void*>(stream));
}

IUnpackStreamFile* UnpackService::GetStandaloneUnpackStreamFile(const QString &productId, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc)
{
	if (!GetInternalData())
		return NULL;

    QSharedPointer<UnpackStreamFile> unpackFile = QSharedPointer<UnpackStreamFile>(new UnpackStreamFile(QWeakPointer<UnpackStream>(), -1, path, packedSize, unpackedSize, date, time, fileAttributes, unpackType, crc, productId));
    GetInternalData()->AddSingleUnpackStreamFile(unpackFile);
	return unpackFile.data();
}

void UnpackService::DestroyStandaloneUnpackStreamFile(IUnpackStreamFile* file)
{
    if (!GetInternalData())
		return;
    
    // QSharedPointer will take care of the cleanup
    GetInternalData()->RemoveSingleUnpackStreamFile(static_cast<void*>(file));
}

}//namespace
}
