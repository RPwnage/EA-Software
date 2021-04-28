///////////////////////////////////////////////////////////////////////////////
// DownloaderErrors.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __DOWNLOADER_ERRORS_INCLUDED_
#define __DOWNLOADER_ERRORS_INCLUDED_

#include <QMap>
#include <QVariant>
#include <QString>

#define CREATE_DOWNLOAD_ERROR_INFO(var) \
    DownloadErrorInfo var;              \
    var.mSourceFile = __FILE__;         \
    var.mSourceLine = __LINE__;

#define POPULATE_ERROR_INFO(info, errorType, errorCode) \
    info.mErrorType = errorType;                         \
    info.mErrorCode = errorCode;                         \
    info.mSourceFile = __FILE__;                         \
    info.mSourceLine = __LINE__;

#define POPULATE_ERROR_INFO_TELEMETRY(info, errorType, errorCode, productId) \
    POPULATE_ERROR_INFO(info, errorType, errorCode) \
    GetTelemetryInterface()->Metric_DL_POPULATE_ERROR(productId, Origin::Downloader::ErrorTranslator::ErrorString((Origin::Downloader::ContentDownloadError::type) errorType).toUtf8().data(), errorCode, __FILE__, __LINE__);

namespace Origin
{
	namespace Downloader
	{
        /// \brief Contains base downloader errors
		namespace ContentDownloadError
		{
			const int kHiWordMask = 0xFFFF0000;
			
            /// \brief Defines the base error enum for components within the downloader. 
			enum type
			{
				UNKNOWN = -1, ///< Used by all downloader components as a default error.
				OK = 0, ///< Used by all downloader components to communicate success.
				FlowError = (1<<16), ///< FlowError Base enum for errors originating in IContentInstallFlow
				ProtocolError = (2<<16), ///< Base enum for errors originating in IContentProtocol
				DownloadServiceError = (3<<16), ///< Base enum for errors originating in DownloadService
				UnpackServiceError = (4<<16), ///< Base enum for errors originating in UnpackService
				InstallServiceError = (5<<16), ///< Base enum for errors originating in InstallSession
                PatchBuilderError = (6<<16) ///< Base enum for errors originating in PatchBuilder
			};
		}

        /// \brief Downloader flow error namespace
		namespace FlowError
		{
            /// \brief Defines possible errors that can occur within IContentInstallFlow 
			enum type
			{
                UNKNOWN = -2, ///< An unknown flow error occurred. 
				OK = 0, ///< Successful flow operation. 
				FlowErrorStart = ContentDownloadError::FlowError,  ///< 0x10000 = 65536 Offset enum value for flow errors. Should not be used. 
				OSRequirementNotMet, ///< 65537  Minimum OS version not met. 
				JitUrlRequestFailed, ///< 65538 The install flow was unable to retrieve the content's JIT URL. 
                CommandInvalid, ///< 65539 The install flow received an invalid command for the current state. 
                DecryptionFailed, ///< 65540 The install flow was unable to decrypt the content. 
                ProcessFailed, ///< 65541 The install flow started an external process which failed. 
                LanguageSelectionEmpty, ///< 65542 The installing content has no supported languages. 
                FileDoesNotExist, ///< 65543 The install flow was unable to locate a file.
                ClientVersionRequirementNotMet, ///< 65544 The current Origin client version does not support this content.
                DiPManifestParseFailure ///< 65545 The DiP manifest could not be parsed.
			};
		}

        /// \brief Downloader protocol error namespace
		namespace ProtocolError
		{
            /// \brief Defines possible errors that can occur within IContentProtcol 
			enum type
			{
                UNKNOWN = -3, ///< An unknown protocol error occurred. 
				OK = 0, ///< Successful protocol operation. 
				ProtocolErrorStart = ContentDownloadError::ProtocolError, ///< 0x2000 = 131072 Offset enum value for protocol errors. Should not be used. 
				CommandInvalid, ///< 131073 The protocol received a command that was invalid for the current state. 
				NotReady, ///< 131074 The protocol is not initialized. 
				ContentInvalid, ///< 131075 The content being downloaded is invalid. 
				DownloadSessionFailure, ///< 131076 The download session failed. 
				EscalationFailureAdminRequired, ///< 131077 The protocol was not able to acquire administrator rights. 
				EscalationFailureUnknown, ///< 131078 The protocol was unable to escalate to create the unpack folder. 
				ContentFolderError, ///< 131079 The protocol was unable to create the unpack folder. 
				DestageFailed, ///< 131080 The protocol was unable to atomically destage all staged files. 
                OutOfMemory, ///< 131081 The system ran out of memory during protocol operations.
                ZipOffsetCalculationFailed, ///< 131082 The protocol failed to calculate zip offsets for ITO
                DiscEjected, ///< 131083 The disc being read was ejected (ITO)
                ZipContentInvalid, ///< 131084 The zip directory is empty. 
                RedownloadRequestFailed, ///< 131085 Redownload request could not be issued. 
                RedownloadTooManyRetries, ///< 131086 Redownload request could not be issued because the number of retries was exhausted.
                ZipFileOffsetsInvalid, ///< 131087 Download request could not be issued because the file offsets are invalid.
                EndOfDisc, ///< 131088 Download request could not be issued because the file is beyond the end of the disc.
                ComputedFileOffsetsInvalid, ///< 131089 Download request could not be issued because the file is beyond the end of the disc.
                EscalationFailureUserCancelled, ///< 131090 The user cancelled the UAC prompt.
				EscalationFailureUACTimedOut, ///< 131091 The protocol was unable to escalate to create the unpack folder due to UAC timeout. 
                PartialDataReceived, ///< 131092 The protocol received only a portion of the total data requested.
                ExcessDataReceived, ///< 131093 The protocol received more data than was requested.
                EscalationFailureUACRequestAlreadyPending ///<131094 The protocal was unable to escalate because there was already a separate UAC request pending/waiting on the user.
			};
		}

        /// \brief Download service error namespace
		namespace DownloadError
		{
            /// \brief Defines possible errors that can occur within DownloadService 
			enum type
			{
                UNKNOWN = -4, ///< An unknown download service error occurred. 
				OK = 0, ///< Successful download service operation. 
				DownloadErrorStart = ContentDownloadError::DownloadServiceError, ///< 0x30000 = 196608 Offset enum value for download service errors. Should not be used. 
				DownloadWorkerCancelled, ///< 196609 The download session cancelled a download worker. 
				InvalidDownloadRequest, ///< 196610 The download session received an invalid request. 
				ClusterRead, ///< 196611 The download session failed to read a data cluster. 
                ContentUnavailable, ///< 196612 The download session could not access the content. 
				TimeOut, ///< 196613 The download session encountered a timeout. 
				FileRead, ///< 196614 The download session failed to read a file. 
				FileWrite, ///< 196615 The download session failed to write to a file. 
				FileOpen, ///< 196616 The download session failed to open a file. 
				FileSeek, ///< 196617 The download session could not perform a random access on a file. 
				FilePermission, ///< 196618 The download session did not have the proper permissions to access a file. 
				NetworkConnectionRefused, ///< 196619 The remote server refused the download session connect request. 
				NetworkRemoteHostClosed, ///< 196620 The remote server closed the download session connection prematurely. 
				NetworkHostNotFound, ///< 196621 The download session tried to connect to an invalid hostname. 
				NetworkOperationCanceled, ///< 196622 The download session closed the connection before a request was finished. 
				SslHandshakeFailed, ///< 196623 The download session could not establish an encrypted channel due to SSL/TLS handshake failure. 
				TemporaryNetworkFailure, ///< 196624 The download session network connection was broken. 
				NetworkProxyConnectionRefused, ///< 196625 The proxy server refused the download session connect request. 
				NetworkProxyConnectionClosed, ///< 196626 The proxy server closed the download session connection prematurely. 
				NetworkProxyNotFound, ///< 196627 The download session tried to connect to an invalid proxy server. 
				NetworkAuthenticationRequired, ///< 196628 The download session did not provide valid credentials for authorization.  
                HttpError, ///< 196629 = 0x3|0x21 The download session received an http server error. 
				RangeRequestInvalid, ///< 196630 The download session received an invalid range request error. 
				NotConnectedError, ///< 196631 The download session received an http server error. 
                ContentLength ///< 196632 The download session could not get the contentlength 
			};
		}

        /// \brief Unpack service error namespace
		namespace UnpackError
		{
            /// \brief Defines possible errors that can occur within UnpackService 
			enum type
			{
                UNKNOWN = -5, ///< An unknown unpack service error occurred. 
				OK = 0, ///< Successful unpack service operation. 
				UnpackErrorStart = ContentDownloadError::UnpackServiceError, ///< 0x40000 = 262144 Offset enum value for unpack service errors. Should not be used. 
				IOCreateOpenFailed, ///< 262145 The unpack service was unable to create or open a file. 
				IOWriteFailed, ///< 262146 The unpack service was unable to write to a file. 
				StreamTypeInvalid, ///< 262147 TThe unpack service received a stream with an unsupported compression type. 
				StreamStateInvalid, ///< 262148 The unpack service detected a stream with an invalid state. 
				StreamUnpackFailed, ///< 262149 The unpack service was unable to process the stream. 
				CRCFailed, ///< 262150 The unpack service detected that the CRC did not match for a file. 
				AlreadyCompleted, ///< 262151 The file was already completed 
				DirectoryNotFound, ///< 262152 The directory does not exist
                VolumeUnavailable ///<262153 The volume is unavailable or is not writable.
			};
		}

        /// \brief Install session error namespace
		namespace InstallError
		{
            /// \brief Defines possible errors that can occur within InstallSession 
			enum type
			{
                UNKNOWN = -6, ///< An unknown install session error occurred. 
				OK = 0, ///< Successful install session operation. 
				InstallErrorStart = ContentDownloadError::InstallServiceError, ///< 0x50000 = 327680  Offset enum value for install session errors. Should not be used. 
                Escalation, ///< 327681 The install session was unable to escalate and run the content installer. 
                InstallerExecution, ///< 327682 The install session was able to escalate but unable to run the content installer. 
                InvalidHandle, ///< 327683 The install service was unable to retrieve the handle of the content installer process. 
                InstallerFailed, ///< 327684 The content installer was able to start but could not complete. 
                InstallerBlocked, ///< 327685 The content installer was blocked from executing by another active installer. 
                InstallerHung, ///< 327686 The content installer process is not responding. 
                InstallCheckFailed ///< 327687 The installer reported success, but the content is not installed. User may have canceled the installer.
			};
		}

        /// \brief Install session error namespace
		namespace PatchBuilderError
		{
            /// \brief Defines possible errors that can occur within InstallSession 
			enum type
			{
                UNKNOWN = -7, ///< An unknown patch builder error occurred. 
				OK = 0, ///< Successful patch builder operation. 
				PatchBuilderErrorStart = ContentDownloadError::PatchBuilderError, ///< 0x60000 = 393216  Offset enum value for patch builder errors. Should not be used. 
                SyncPackageUnavailable, ///< 393217 The sync package was not found on the server/disk
                SyncPackageInvalid, ///< 393218 The sync package was invalid, did not match the base package, or did not contain the requested files
                RejectFileNonOptimal, ///< 393219 The patch builder has determined that patching this file would be slower than downloading it outright from the compressed source.
                FileOpenFailure,    ///< 393220 Internal error from EAPatchClient
                FileReadFailure,    ///< 393221 Internal error from EAPatchClient
                FileReadTruncate,   ///< 393222 Internal error from EAPatchClient
                FileWriteFailure,   ///< 393223 Internal error from EAPatchClient
                FilePosFailure,     ///< 393224 Internal error from EAPatchClient
                FileCloseFailure,   ///< 393225 Internal error from EAPatchClient
                FileCorrupt,        ///< 393226 Internal error from EAPatchClient
                FileRemoveError,    ///< 393227 Internal error from EAPatchClient
                FileNameLength,     ///< 393228 Internal error from EAPatchClient
                FilePathLength,     ///< 393229 Internal error from EAPatchClient
                FileNotFound,       ///< 393230 Internal error from EAPatchClient
                FileNotSpecified,   ///< 393231 Internal error from EAPatchClient
                FileRenameError,    ///< 393232 Internal error from EAPatchClient
                FileCreateError,    ///< 393233 Internal error from EAPatchClient
                DirCreateError,     ///< 393234 Internal error from EAPatchClient
                DirReadError,       ///< 393235 Internal error from EAPatchClient
                DirRenameError,     ///< 393236 Internal error from EAPatchClient
                DirRemoveError,     ///< 393237 Internal error from EAPatchClient
                XMLError,           ///< 393238 Internal error from EAPatchClient
                MemoryFailure,      ///< 393239 Internal error from EAPatchClient
                URLError,           ///< 393240 Internal error from EAPatchClient
                ThreadError,        ///< 393241 Internal error from EAPatchClient
                ServerError         ///< 393242 Internal error from EAPatchClient
			};
		}

        struct DownloadChunkCRC
        {
            enum ChunkRelationship { kChunkRelationshipAdjacent = 0, kChunkRelationshipGap, kChunkRelationshipOverlap };

            public:
                DownloadChunkCRC();

                DownloadChunkCRC(const DownloadChunkCRC& rhs);
                DownloadChunkCRC& operator= (const DownloadChunkCRC &rhs);

                qint64 size();
                ChunkRelationship relationship(const DownloadChunkCRC &next, qint64 &delta);

                QString toString();
            private:
                void copy(const DownloadChunkCRC &rhs);

            public:
                qint64 startOffset;
                qint64 endOffset;
                quint32 crc;
                qint64 timestamp;
        };

        class DownloadChunkDiagnosticInfo : public QObject
        {
            Q_OBJECT
            
            public:
                DownloadChunkDiagnosticInfo();

                DownloadChunkDiagnosticInfo(const DownloadChunkDiagnosticInfo& rhs);
                DownloadChunkDiagnosticInfo& operator= (const DownloadChunkDiagnosticInfo &rhs);

            private:
                void copy(const DownloadChunkDiagnosticInfo &rhs);

            public:
                QString hostname;
                QString ip;
                QString file;
                DownloadChunkCRC chunkCRC;
        };

        struct DownloadErrorInfo
        {
            /// \brief The type of error encountered.
            ///
            /// This will correspond to a value in the ContentDownloadError enumeration (i.e. ContentDownloadError::FlowError).
            qint32 mErrorType;
            
            /// \brief The specific code of the error we encountered.
            ///
            /// This will correspond to a value in the other DownloaderErrors enumerations (i.e. FlowError::OSRequirementNotMet).
            qint32 mErrorCode;

            /// \brief The file in the source code in which this error was generated.
            QString mSourceFile;

            /// \brief The line in the source code on which this error was generated.
            quint32 mSourceLine;

            /// \brief Contains error-specific context data.  For example, for I/O errors, the file or path that experienced the error.  Etc.
            QString mErrorContext;

            /// \brief The DownloadErrorInfo constructor.
            ///
            /// \note Please use the CREATE_DOWNLOAD_ERROR_INFO macro to create DownloadErrorInfo objects.
            DownloadErrorInfo();

            /// \brief The DownloadErrorInfo copy constructor.
            /// 
            /// \param rhs A DownloadErrorInfo object from which we should copy.
            DownloadErrorInfo(const DownloadErrorInfo &rhs);

            /// \brief Overridden "=" operator
            /// 
            /// \param rhs A DownloadErrorInfo object from which we should copy.
            DownloadErrorInfo& operator= (const DownloadErrorInfo &rhs);
        };
        
        /// \brief Used to retrieve string representations of downloader errors or to convert a downloader error to its base type.
		class ErrorTranslator
		{
			public:
                /// \brief Translates a specific error code (i.e. FlowError::OSRequirementNotMet) into a generic error type (i.e. ContentDownloadError::FlowError).
                ///
                /// \param type The specific error code.
                ///
                /// \return A generic error type.
				static ContentDownloadError::type GetErrorType(ContentDownloadError::type type);

                /// \brief Translates a specific error code into a human-readable string describing the error.
                ///
                /// \param type The specific error code.
                ///
                /// \return A string describing the error.  Note that this string is not localized and should only be used for logging and debugging purposes.
				static QString Translate(ContentDownloadError::type type);

                /// \brief Translates a specific error code into a human-readable string the enumeration
                ///
                /// \param err The specific error code.
                ///
                /// \return String form of the error enum
                static QString ErrorString(ContentDownloadError::type err);
		};

	}
}

Q_DECLARE_METATYPE(Origin::Downloader::DownloadErrorInfo);

#endif // __DOWNLOADER_ERRORS_INCLUDED_
