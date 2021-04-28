#ifndef UNPACKSTREAMFILE_H
#define UNPACKSTREAMFILE_H

#include "services/downloader/UnpackService.h"
#include "services/downloader/DownloaderErrors.h"

#include "services/downloader/zlibDecompressor.h"
#include "services/plugin/PluginAPI.h"

#include <QFile>
#include <QRunnable>
#include <QMutex>
#include <QPointer>

namespace Origin
{
    namespace Downloader
    {
		class UnpackStream;
        class ContentTagUtility;

        class ORIGIN_PLUGIN_API UnpackStreamFile : public IUnpackStreamFile
        {
            Q_OBJECT
        public:
			static void setMaxChunkSizes(quint32 maxCompressedChunkSize, quint32 maxUncompressedChunkSize);

            UnpackStreamFile(QWeakPointer<UnpackStream> stream, UnpackStreamFileId id, const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc, const QString &productId, bool ignoreCRCErrors = true);
			~UnpackStreamFile();

            // IUnpackStreamFile members
            virtual UnpackStreamFileId getId();

            virtual qint64 getTotalUnpackedBytes();
			virtual qint64 getTotalPackedBytes();
            virtual qint64 getSavedBytes();
            virtual qint64 getProcessedBytes();

            virtual QString getFileName();

			virtual UnpackError::type getErrorType();
			virtual qint32 getErrorCode();

			virtual bool processChunk(void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData, quint32 &bytesProcessed, bool &fileComplete);

            // Internal members
            void shutdown();
            bool isShutdown();
			UnpackError::type tryResume();
            void ResetUnpackFileProgress();
            void ClobberFiles();
			bool isPartial();
			bool isComplete();

            void setContentTagBlock(const QMap<QString,QString>& contentTagBlock);

			static QString GetDecompressorFilename(const QString& baseFilename);
			void SaveDecompressor();

            static bool PeekProcessedBytes(const QString& baseFilename, UnpackType::code unpackType, qint64& bytesProcessed);

			void Cleanup(bool cleanFiles);
            void DumpDiagnosticData();
			void Error(Origin::Downloader::DownloadErrorInfo errorInfo, bool cleanFiles = true, bool fatal = false);
        private:

            bool prepDestinationFileForWriting();
            bool writeChunk_Compressed(void* data, quint32 dataLen, quint32& bytesWritten);
            bool writeChunk_Uncompressed(void* data, quint32 dataLen, quint32& bytesWritten);
            bool writeChunkToDisk(const char* data, qint64 len);


			static quint32 sMaxCompressedChunkSize;
			static quint32 sMaxUncompressedChunkSize;

			QWeakPointer<UnpackStream> _stream;
            volatile bool _shutdown;

            const QString _productId;

            UnpackStreamFileId _id;
            QString _filePath;
			quint16 _fileDate;
			quint16 _fileTime;
            quint32 _fileAttributes;
            qint64 _fileTotalUnpackedBytes;
			qint64 _fileTotalPackedBytes;
            qint64 _fileSavedBytes;
            qint64 _fileProcessedBytes;
			quint32 _fileComputedCRC;
            quint32 _fileCRC;
            UnpackType::code _fileUnpackType;
            bool    _ignoreCRCErrors;

			quint32 _maxChunkSize;

			UnpackError::type _errorType;
			qint32 _errorCode;

			int _fileChunkCount;
			ZLibDecompressor::DeflateStreamDecompressor* _fileDecompressor;
			QFile _fileHandle;

            QMap<QString,QString> _contentTagBlock;
            ContentTagUtility* _contentTagUtility;

            QString _diagFile;
            QString _diagHostname;
            QMap<QString, QList<DownloadChunkCRC> > _diagChunkCRCs; // IP -> CRC chunks map
            DownloadChunkCRC _diagLastChunk;

			QMutex	_lock;
        };

		class ORIGIN_PLUGIN_API UnpackStreamFileChunk : public QRunnable
		{
			public:
				UnpackStreamFileChunk(QWeakPointer<UnpackStreamFile> file, void* data, quint32 dataLen, QSharedPointer<DownloadChunkDiagnosticInfo> diagData);

				virtual void run();

			private:
                QWeakPointer<UnpackStreamFile> _file;
				void			 *_data;
				quint32			  _dataLen;
                QSharedPointer<DownloadChunkDiagnosticInfo> _diagData;
		};
    }
}

#endif // UNPACKSTREAMFILE_H
