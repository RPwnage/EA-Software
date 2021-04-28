#ifndef UNPACKSTREAM_H
#define UNPACKSTREAM_H

#include "services/downloader/UnpackService.h"
#include "services/downloader/DownloaderErrors.h"

#include <QMap>
#include <QThreadPool>

namespace Origin
{
    namespace Downloader
    {
		class UnpackStreamFile;
        class IUnpackStreamFile;

        class UnpackStream : public IUnpackStream
        {
            Q_OBJECT

        public:
            UnpackStream(const QString& productId, const QString& path, const QMap<QString,QString>& contentTagBlock, QThreadPool *threadPool);
			~UnpackStream();

            // IUnpackStream members
			virtual UnpackStreamFileId getNewId();
            virtual UnpackError::type createUnpackFile(UnpackStreamFileId id, QString &productId, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc, bool ignoreCRCErrors, bool clearExistingFiles);
            virtual IUnpackStreamFile* getUnpackFileById(UnpackStreamFileId id);
            virtual void closeUnpackFile(UnpackStreamFileId id);

            virtual void queueChunk(UnpackStreamFileId id, void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData);

			// Internal members
			void reportUnpackFileChunkProcessed(UnpackStreamFileId id, qint64 downloadedByteCount, qint64 processedByteCount, bool completed);
            void reportUnpackFileError(UnpackStreamFileId id, Origin::Downloader::DownloadErrorInfo errorInfo, bool fatal);
            const QString &productId() const;
            bool fatalError() const;

        private:
			qint64 _unpackIds;
            QString _unpackDir;
            QString _productId;
            QMap<QString,QString> _contentTagBlock;
            bool _fatalError;

			QMap<UnpackStreamFileId, QSharedPointer<UnpackStreamFile> > _unpackFiles;

			QThreadPool *_threadPool;
        };
    }
}


#endif // UNPACKSTREAM_H
