#ifndef UNPACKSERVICE_H
#define UNPACKSERVICE_H

#include <QObject>
#include <QString>

#include "services/downloader/DownloaderErrors.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Downloader
    {
        class IUnpackStream;
		class IUnpackStreamFile;

        typedef qint64 UnpackStreamFileId;

		namespace UnpackType
		{
			enum code
			{
				Uncompressed = 0,
				Shrink = 1,
				Implode = 6,
				Deflate = 8,
				BZIP2 = 12,
				LZMA = 14
			};
		}

        class UnpackServiceData;
        class ORIGIN_PLUGIN_API UnpackService
        {
            friend class UnpackStream;
        public:
			static void init();
			static void release();

            static IUnpackStream* GetUnpackStream(const QString& productId, const QString& unpackDir, const QMap<QString,QString>& contentTagBlock);
            static void DestroyUnpackStream(IUnpackStream* stream);

			static IUnpackStreamFile* GetStandaloneUnpackStreamFile(const QString &productId, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType = UnpackType::Uncompressed, quint32 crc = 0);
			static void DestroyStandaloneUnpackStreamFile(IUnpackStreamFile* file);

        private:
            UnpackService();
            ~UnpackService();

            static UnpackServiceData* GetInternalData();
        private:
            UnpackServiceData* _data;
        };

		class IUnpackStream : public QObject
        {
            Q_OBJECT
        signals:
            void UnpackFileChunkProcessed(UnpackStreamFileId id, qint64 downloadedByteCount, qint64 processedByteCount, bool completed);
            void UnpackFileError(UnpackStreamFileId id, Origin::Downloader::DownloadErrorInfo errorInfo);

        public:
			virtual ~IUnpackStream() {}

			virtual UnpackStreamFileId getNewId() = 0;
            virtual UnpackError::type createUnpackFile(UnpackStreamFileId id, QString &productId, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType = UnpackType::Uncompressed, quint32 crc = 0, bool ignoreCRCErrors = true, bool clearExistingFiles = false) = 0;
            virtual IUnpackStreamFile* getUnpackFileById(UnpackStreamFileId id) = 0;
            virtual void closeUnpackFile(UnpackStreamFileId id) = 0;

            virtual void queueChunk(UnpackStreamFileId id, void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData) = 0;

        protected:
            IUnpackStream() {} // Not constructable
        };

		class ORIGIN_PLUGIN_API IUnpackStreamFile : public QObject
        {
            Q_OBJECT

        public:
			virtual ~IUnpackStreamFile() {}

            virtual UnpackStreamFileId getId() = 0;

            virtual qint64 getTotalUnpackedBytes() = 0;
			virtual qint64 getTotalPackedBytes() = 0;
            virtual qint64 getSavedBytes() = 0;
            virtual qint64 getProcessedBytes() = 0;

            virtual QString getFileName() = 0;

			virtual UnpackError::type getErrorType() = 0;
			virtual qint32 getErrorCode() = 0;

			virtual bool processChunk(void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData, quint32 &bytesProcessed, bool &fileComplete) = 0;

        protected:
            IUnpackStreamFile() {} // Not constructable
        };
    }
}

#endif // UNPACKSERVICE_H
