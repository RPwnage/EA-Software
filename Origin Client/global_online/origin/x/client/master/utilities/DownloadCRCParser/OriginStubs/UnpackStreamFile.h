#ifndef UNPACKSTREAMFILE_H
#define UNPACKSTREAMFILE_H

#include "DownloaderErrors.h"

#include "zlibDecompressor.h"

#include <QFile>
#include <QRunnable>
#include <QMutex>
#include <QPointer>

namespace Origin
{
    namespace Downloader
    {
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

        class UnpackStreamFile
        {
        public:
			static void setMaxChunkSizes(quint32 maxCompressedChunkSize, quint32 maxUncompressedChunkSize);

            UnpackStreamFile(const QString& path, qint64 packedSize, qint64 unpackedSize, quint16 date, quint16 time, quint32 fileAttributes, UnpackType::code unpackType, quint32 crc, const QString &productId, bool ignoreCRCErrors = true);
			~UnpackStreamFile();

            qint64 getTotalUnpackedBytes();
			qint64 getTotalPackedBytes();
            qint64 getSavedBytes();
            qint64 getProcessedBytes();

            QString getFileName();

			UnpackError::type getErrorType();
			qint32 getErrorCode();

			bool processChunk(void* data, quint32 dataLen, quint32 &bytesProcessed, bool &fileComplete);

            // Internal members
            void shutdown();
            bool isShutdown();
			UnpackError::type tryResume();
            void ResetUnpackFileProgress();
            void ClobberFiles();
			bool isPartial();
			bool isComplete();

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

            volatile bool _shutdown;

            const QString _productId;

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

			QMutex	_lock;
        };
    }
}

#endif // UNPACKSTREAMFILE_H
