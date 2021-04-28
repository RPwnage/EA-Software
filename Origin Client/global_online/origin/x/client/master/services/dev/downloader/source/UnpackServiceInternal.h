#ifndef UNPACKSERVICEINTERNAL_H
#define UNPACKSERVICEINTERNAL_H

#include "io/UnpackStream.h"
#include "services/downloader/UnpackStreamFile.h"

#include <QSharedPointer>
#include <QThreadPool>
#include <QMutex>

namespace Origin
{
namespace Downloader
{

class UnpackServiceData
{
public:
    UnpackServiceData();
    ~UnpackServiceData();

    void AddUnpackStream(QSharedPointer<UnpackStream> unpackStream);
    void RemoveUnpackStream(void* pUnpackStream);

    QWeakPointer<UnpackStream> GetStreamWeakRef(const UnpackStream* pUnpackStream);

    void AddSingleUnpackStreamFile(QSharedPointer<UnpackStreamFile> unpackStreamFile);
    void RemoveSingleUnpackStreamFile(void* pUnpackStreamFile);

    QThreadPool* GetThreadPool();
private:
    QMutex _mapLock;
    QMap<void*,QSharedPointer<UnpackStream> > _unpackStreams;
    QMap<void*,QSharedPointer<UnpackStreamFile> > _unpackSingleFiles;
    QThreadPool _unpackThreadPool;
};

} // Downloader

} // Origin
#endif // UNPACKSERVICEINTERNAL_H