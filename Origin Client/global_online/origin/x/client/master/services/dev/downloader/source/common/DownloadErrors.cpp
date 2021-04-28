#include "services/downloader/DownloaderErrors.h"

namespace Origin
{
	namespace Downloader
	{
        struct ErrorTranslateStruct
        {
            ErrorTranslateStruct(){}
            ErrorTranslateStruct(QString enumString, QString sentence)
            {
                mSentence = sentence;
                mEnumString = enumString;
            }
            QString mSentence;    // "Minimum OS version not met"
            QString mEnumString;  // "OSRequirementNotMet"
        };
        QMap<ContentDownloadError::type, ErrorTranslateStruct> g_ContentDownloadErrorMap;

		// Forward declare
		void InitializeContentDownloadErrorStrings();

        DownloadChunkCRC::DownloadChunkCRC() : 
            startOffset(0), 
            endOffset(0), 
            crc(0),
            timestamp(0)
        {
            // Nothing to do
        }

        DownloadChunkCRC::DownloadChunkCRC(const DownloadChunkCRC& rhs)
        {
            copy(rhs);
        }
        DownloadChunkCRC& DownloadChunkCRC::operator= (const DownloadChunkCRC &rhs)
        {
            copy(rhs);
            return *this;
        }

        void DownloadChunkCRC::copy(const DownloadChunkCRC &rhs)
        {
            startOffset = rhs.startOffset;
            endOffset = rhs.endOffset;
            crc = rhs.crc;
            timestamp = rhs.timestamp;
        }

        qint64 DownloadChunkCRC::size()
        {
            return endOffset - startOffset;
        }

        DownloadChunkCRC::ChunkRelationship DownloadChunkCRC::relationship(const DownloadChunkCRC &next, qint64 &delta)
        {
            delta = abs(next.startOffset - endOffset);
            if (endOffset > next.startOffset)
                return DownloadChunkCRC::kChunkRelationshipOverlap;
            if (endOffset < next.startOffset)
                return DownloadChunkCRC::kChunkRelationshipGap;
            return DownloadChunkCRC::kChunkRelationshipAdjacent;
        }

        QString DownloadChunkCRC::toString()
        {
            // All values in hex
            return QString("[Time=%1][Rng=%2-%3][CRC=%4]").arg(QString::number(timestamp,16)).arg(QString::number(startOffset,16)).arg(QString::number(endOffset,16)).arg(QString::number(crc,16));
        }

        DownloadChunkDiagnosticInfo::DownloadChunkDiagnosticInfo()
        {
            // Nothing to do
        }

        DownloadChunkDiagnosticInfo::DownloadChunkDiagnosticInfo(const DownloadChunkDiagnosticInfo& rhs)
        {
            copy(rhs);
        }

        DownloadChunkDiagnosticInfo& DownloadChunkDiagnosticInfo::operator= (const DownloadChunkDiagnosticInfo &rhs)
        {
            copy(rhs);
            return *this;
        }

        void DownloadChunkDiagnosticInfo::copy(const DownloadChunkDiagnosticInfo &rhs)
        {
            hostname = rhs.hostname;
            ip = rhs.ip;
            file = rhs.file;
            chunkCRC = rhs.chunkCRC;
        }

        DownloadErrorInfo::DownloadErrorInfo()
		{
			mErrorType = ContentDownloadError::UNKNOWN;
            mErrorCode = ContentDownloadError::OK;
			mSourceFile = "";
            mSourceLine = 0;
            mErrorContext = "";
		}

        DownloadErrorInfo::DownloadErrorInfo(const DownloadErrorInfo &rhs)
		{
            mErrorType = rhs.mErrorType;
            mErrorCode = rhs.mErrorCode;
            mSourceFile = rhs.mSourceFile;
            mSourceLine = rhs.mSourceLine;
            mErrorContext = rhs.mErrorContext;
		}

        DownloadErrorInfo& DownloadErrorInfo::operator= (const DownloadErrorInfo &rhs)
        {
            mErrorType = rhs.mErrorType;
            mErrorCode = rhs.mErrorCode;
            mSourceFile = rhs.mSourceFile;
            mSourceLine = rhs.mSourceLine;
            mErrorContext = rhs.mErrorContext;
            return *this;
        }

		ContentDownloadError::type ErrorTranslator::GetErrorType(ContentDownloadError::type code)
		{
			// Mask off the low bits
			return (ContentDownloadError::type)(ContentDownloadError::kHiWordMask & code);
		}
		
		QString ErrorTranslator::Translate(ContentDownloadError::type err)
		{
			QString translation = "Undefined";
			if (g_ContentDownloadErrorMap.isEmpty())
			{
				InitializeContentDownloadErrorStrings();
			}

			QMap<ContentDownloadError::type, ErrorTranslateStruct>::const_iterator i = g_ContentDownloadErrorMap.find(err);
			if (i != g_ContentDownloadErrorMap.constEnd())
			{
				translation = i.value().mSentence;
			}

			return translation;
		}

        QString ErrorTranslator::ErrorString(ContentDownloadError::type err)
        {
            QString translation = "Undefined";
            if (g_ContentDownloadErrorMap.isEmpty())
            {
                InitializeContentDownloadErrorStrings();
            }

            QMap<ContentDownloadError::type, ErrorTranslateStruct>::const_iterator i = g_ContentDownloadErrorMap.find(err);
            if (i != g_ContentDownloadErrorMap.constEnd())
            {
                translation = i.value().mEnumString;
            }

            return translation;
        }

		void InitializeContentDownloadErrorStrings()
		{
            g_ContentDownloadErrorMap.insert(ContentDownloadError::OK, ErrorTranslateStruct("OK", "No Error"));
			g_ContentDownloadErrorMap.insert(ContentDownloadError::UNKNOWN, ErrorTranslateStruct("ContentDownloadError::UNKNOWN", "Unknown error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::UNKNOWN, ErrorTranslateStruct("FlowError::UNKNOWN", "Unknown downloader flow error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::UNKNOWN, ErrorTranslateStruct("DownloadError::UNKNOWN", "Unknown downloader service error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::UNKNOWN, ErrorTranslateStruct("UnpackError::UNKNOWN", "Unknown unpack service error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::UNKNOWN, ErrorTranslateStruct("ProtocolError::UNKNOWN", "Unknown download protocol error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::UNKNOWN, ErrorTranslateStruct("InstallError::UNKNOWN", "Unknown downloader install service error"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::UNKNOWN, ErrorTranslateStruct("PatchBuilderError::UNKNOWN", "Unknown downloader patch builder error"));
			
			// flow
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::OSRequirementNotMet, ErrorTranslateStruct("OSRequirementNotMet", "Minimum OS version not met"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::JitUrlRequestFailed, ErrorTranslateStruct("JitUrlRequestFailed", "The install flow was unable to retrieve the content's JIT URL"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::CommandInvalid, ErrorTranslateStruct("CommandInvalid", "The install flow received an invalid command for the current state"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::DecryptionFailed, ErrorTranslateStruct("DecryptionFailed", "The install flow was unable to decrypt the content"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::ProcessFailed, ErrorTranslateStruct("ProcessFailed", "The install flow started an external process which failed"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::LanguageSelectionEmpty, ErrorTranslateStruct("LanguageSelectionEmpty", "The installing content has no supported languages"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::FileDoesNotExist, ErrorTranslateStruct("FileDoesNotExist", "The install flow was unable to locate a file: %1"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::ClientVersionRequirementNotMet, ErrorTranslateStruct("ClientVersionRequirementNotMet", "The current Origin client version does not support this content"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)FlowError::DiPManifestParseFailure, ErrorTranslateStruct("DiPManifestParseFailure", "The DiP Manifest could not be parsed."));

			// protocol
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::CommandInvalid, ErrorTranslateStruct("CommandInvalid", "The protocol received a command that was invalid for the current state."));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::NotReady, ErrorTranslateStruct("NotReady", "The protocol is not initialized."));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ContentInvalid, ErrorTranslateStruct("ContentInvalid", "The content being downloaded is invalid"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::DownloadSessionFailure, ErrorTranslateStruct("DownloadSessionFailure", "The download session failed"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EscalationFailureAdminRequired, ErrorTranslateStruct("EscalationFailureAdminRequired", "The protocol was not able to acquire administrator rights"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EscalationFailureUnknown, ErrorTranslateStruct("EscalationFailureUnknown", "The protocol was unable to escalate to create the unpack folder"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ContentFolderError, ErrorTranslateStruct("ContentFolderError", "The protocol was unable to create the unpack folder"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::DestageFailed, ErrorTranslateStruct("DestageFailed", "The protocol was unable to atomically destage all staged files"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::OutOfMemory, ErrorTranslateStruct("OutOfMemory", "The system ran out of memory during protocol operations"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ZipOffsetCalculationFailed, ErrorTranslateStruct("ZipOffsetCalculationFailed", " The protocol failed to calculate zip offsets for ITO"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::DiscEjected, ErrorTranslateStruct("DiscEjected", "The disc being read was ejected (ITO)"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ZipContentInvalid, ErrorTranslateStruct("ZipContentInvalid", "The zip directory is empty"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::RedownloadRequestFailed, ErrorTranslateStruct("RedownloadRequestFailed", "Redownload request could not be issued"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::RedownloadTooManyRetries, ErrorTranslateStruct("RedownloadTooManyRetries", "Redownload request could not be issued because the number of retries was exhausted."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ZipFileOffsetsInvalid, ErrorTranslateStruct("ZipFileOffsetsInvalid", "Download request could not be issued because the file offsets are invalid."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EndOfDisc, ErrorTranslateStruct("EndOfDisc", "Download request could not be issued because the file is beyond the end of the disc."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ComputedFileOffsetsInvalid, ErrorTranslateStruct("ComputedFileOffsetsInvalid", "Download request could not be issued because the file is beyond the end of the disc."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EscalationFailureUserCancelled, ErrorTranslateStruct("EscalationFailureUserCancelled", "The user cancelled the UAC prompt."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EscalationFailureUACTimedOut, ErrorTranslateStruct("EscalationFailureUACTimedOut", "The protocol was unable to escalate to create the unpack folder due to UAC timeout"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::EscalationFailureUACRequestAlreadyPending, ErrorTranslateStruct("EscalationFailureUACRequestAlreadyPending", "The protocol was unable to escalate because there is already a UAC request waiting on the user"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::PartialDataReceived, ErrorTranslateStruct("PartialDataReceived", "The protocol received only a portion of the total data requested."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)ProtocolError::ExcessDataReceived, ErrorTranslateStruct("ExcessDataReceived", "The protocol received more data than was requested."));

			// download service
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::DownloadWorkerCancelled, ErrorTranslateStruct("DownloadWorkerCancelled", "The download session cancelled a download worker"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::InvalidDownloadRequest, ErrorTranslateStruct("InvalidDownloadRequest", "The download session received an invalid request"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::ClusterRead, ErrorTranslateStruct("ClusterRead", "The download session failed to read a data cluster"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::TimeOut, ErrorTranslateStruct("TimeOut", "The download session encountered a timeout"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::ContentUnavailable, ErrorTranslateStruct("ContentUnavailable", "The download session could not access the content"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::FileRead, ErrorTranslateStruct("FileRead", "The download session failed to read a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::FileWrite, ErrorTranslateStruct("FileWrite", "The download session failed to write to a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::FileOpen, ErrorTranslateStruct("FileOpen", "The download session failed to open a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::FileSeek, ErrorTranslateStruct("FileSeek", "The download session could not perform a random access on a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::FilePermission, ErrorTranslateStruct("FilePermission", "The download session did not have the proper permissions to access a file"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkConnectionRefused, ErrorTranslateStruct("NetworkConnectionRefused", "The remote server refused the download session connect request"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkRemoteHostClosed, ErrorTranslateStruct("NetworkRemoteHostClosed", "The remote server closed the download session connection prematurely"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkHostNotFound, ErrorTranslateStruct("NetworkHostNotFound", "The download session tried to connect to an invalid hostname"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkOperationCanceled, ErrorTranslateStruct("NetworkOperationCanceled", "The download session closed the connection before a request was finished"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::SslHandshakeFailed, ErrorTranslateStruct("SslHandshakeFailed", "The download session could not establish an encrypted channel due to SSL/TLS handshake failure"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::TemporaryNetworkFailure, ErrorTranslateStruct("TemporaryNetworkFailure", "The download session network connection was broken"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkProxyConnectionRefused, ErrorTranslateStruct("NetworkProxyConnectionRefused", "The proxy server refused the download session connect request"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkProxyConnectionClosed, ErrorTranslateStruct("NetworkProxyConnectionClosed", "The proxy server closed the download session connection prematurely"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkProxyNotFound, ErrorTranslateStruct("NetworkProxyNotFound", "The download session tried to connect to an invalid proxy server"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NetworkAuthenticationRequired, ErrorTranslateStruct("NetworkAuthenticationRequired", "The download session did not provide valid credentials for authorization"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::HttpError, ErrorTranslateStruct("HttpError", "The download session received an http server error"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::RangeRequestInvalid, ErrorTranslateStruct("RangeRequestInvalid", "The download session received an invalid range request error"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::NotConnectedError, ErrorTranslateStruct("NotConnectedError", "The download session received an http server error"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)DownloadError::ContentLength, ErrorTranslateStruct("ContentLength", "The download session could not get the contentlength"));

			// unpack service
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::IOCreateOpenFailed, ErrorTranslateStruct("IOCreateOpenFailed", "The unpack service was unable to create or open a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::IOWriteFailed, ErrorTranslateStruct("IOWriteFailed", "The unpack service was unable to write to a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::StreamTypeInvalid, ErrorTranslateStruct("StreamTypeInvalid", "The unpack service received a stream with an unsupported compression type"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::StreamStateInvalid, ErrorTranslateStruct("StreamStateInvalid", "The unpack service detected a stream with an invalid state"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::StreamUnpackFailed, ErrorTranslateStruct("StreamUnpackFailed", "The unpack service was unable to process the stream"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::CRCFailed, ErrorTranslateStruct("CRCFailed", "The unpack service detected that the CRC did not match for a file"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::AlreadyCompleted, ErrorTranslateStruct("AlreadyCompleted", "The file was already completed"));
			g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::DirectoryNotFound, ErrorTranslateStruct("DirectoryNotFound", "The directory does not exist"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)UnpackError::VolumeUnavailable, ErrorTranslateStruct("VolumeUnavailable","The volume is unavailable or is not writable"));

            // installer
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::Escalation, ErrorTranslateStruct("Escalation", "The install service was unable to escalate and run the content installer"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InstallerExecution, ErrorTranslateStruct("InstallerExecution", "The install service was able to escalate but unable to run the content installer"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InvalidHandle, ErrorTranslateStruct("InvalidHandle", "The install service was unable to retrieve the handle of the content installer process"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InstallerFailed, ErrorTranslateStruct("InstallerFailed", "The content installer was able to start but could not complete"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InstallerBlocked, ErrorTranslateStruct("InstallerBlocked", "The content installer was blocked from executing by another active installer."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InstallerHung, ErrorTranslateStruct("InstallerHung", "The content installer process is not responding."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)InstallError::InstallCheckFailed, ErrorTranslateStruct("InstallCheckFailed", "The installer reported success, but the content is not installed. User may have canceled the installer."));

            // patch builder
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::SyncPackageUnavailable, ErrorTranslateStruct("SyncPackageUnavailable", "The sync package was not found on the server/disk"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::SyncPackageInvalid, ErrorTranslateStruct("SyncPackageInvalid", "The sync package was invalid, did not match the base package, or did not contain the requested files"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::RejectFileNonOptimal, ErrorTranslateStruct("RejectFileNonOptimal", "The patch builder has determined that patching this file would be slower than downloading it outright from the compressed source."));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileOpenFailure,  ErrorTranslateStruct("FileOpenFailure", "393220 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileReadFailure,  ErrorTranslateStruct("FileReadFailure", "393221 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileReadTruncate, ErrorTranslateStruct("FileReadTruncate", "393222 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileWriteFailure, ErrorTranslateStruct("FileWriteFailure", "393223 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FilePosFailure,   ErrorTranslateStruct("FilePosFailure", "393224 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileCloseFailure, ErrorTranslateStruct("FileCloseFailure", "393225 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileCorrupt,      ErrorTranslateStruct("FileCorrupt", "393226 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileRemoveError,  ErrorTranslateStruct("FileRemoveError", "393227 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileNameLength,   ErrorTranslateStruct("FileNameLength", "393228 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FilePathLength,   ErrorTranslateStruct("FilePathLength", "393229 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileNotFound,     ErrorTranslateStruct("FileNotFound", "393230 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileNotSpecified, ErrorTranslateStruct("FileNotSpecified", "393231 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileRenameError,  ErrorTranslateStruct("FileRenameError", "393232 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::FileCreateError,  ErrorTranslateStruct("FileCreateError", "393233 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::DirCreateError,   ErrorTranslateStruct("DirCreateError", "393234 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::DirReadError,     ErrorTranslateStruct("DirReadError", "393235 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::DirRenameError,   ErrorTranslateStruct("DirRenameError", "393236 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::DirRemoveError,   ErrorTranslateStruct("DirRemoveError", "393237 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::XMLError,         ErrorTranslateStruct("XMLError", "393238 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::MemoryFailure,    ErrorTranslateStruct("MemoryFailure", "393239 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::URLError,         ErrorTranslateStruct("URLError", "393240 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::ThreadError,      ErrorTranslateStruct("ThreadError", "393241 Internal error from EAPatchClient"));
            g_ContentDownloadErrorMap.insert((ContentDownloadError::type)PatchBuilderError::ServerError,      ErrorTranslateStruct("ServerError", "393242 Internal error from EAPatchClient"));
        }
	}                                                                    
}
