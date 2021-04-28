#include "services/common/DiskUtil.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/escalation/IEscalationClient.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Util
    {
        namespace DiskUtil
        {
            bool DeleteFileWithRetry( QString sFileName )
            {
                if (sFileName.isEmpty())
                    return false;

                if (!QFile::exists(sFileName))
                {
                    ORIGIN_LOG_DEBUG << "EnvUtils::DeleteFileWithRetry - Called with no file on disk: " << sFileName;
                    return false;
                }

                // Simple case
                QFile fileToDelete(sFileName);
                if (fileToDelete.remove())
                {
                    // No problems
                    return true;
                }

                // Simple deletion failed.
                ORIGIN_LOG_ERROR << "Failed to delete \"" << qPrintable(fileToDelete.fileName()) << "\" FileError:" << fileToDelete.error() << " NativeError:" << fileToDelete.nativeError();  // The useful error should be nNativeError however we'll put both in the log
                GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("DiskUtil_DeleteFileWithRetry", qPrintable(fileToDelete.fileName()), fileToDelete.nativeError());  

                // Try clearing READ-ONLY
                QFile::Permissions perm = fileToDelete.permissions();
                ORIGIN_LOG_ERROR << "Old Permissions:" << perm;
                perm |= QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser;
                ORIGIN_LOG_ERROR << "New Permissions:" << perm;
                fileToDelete.setPermissions(perm);
                if (fileToDelete.remove())
                {
                    // Success
                    ORIGIN_LOG_EVENT << "Succeeded in deleting file after changing permissions.";
                    return true;
                }

                // Changing READ-ONLY attribute failed to remove the file.
                ORIGIN_LOG_ERROR << "Failed to delete \"" << qPrintable(fileToDelete.fileName()) << "\" FileError:" << fileToDelete.error() << " NativeError:" << fileToDelete.nativeError();  // The useful error should be nNativeError however we'll put both in the log
                GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("DiskUtil_DeleteFileWithRetry", qPrintable(fileToDelete.fileName()), fileToDelete.nativeError());  

                // Ensure proper ACLs are set in the containing folder and try again.  This is something that should have already been a prerequisite to any file the client may write.
                // Use EscalationClient's ability to set permissions on existing folders.
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;

                // Get the file's path
                QString sPath(fileToDelete.fileName());
                sPath.replace('\\', '/');
                sPath = sPath.left(sPath.lastIndexOf("/"));

                QString escalationReasonStr = "CreateDirectoryAllAccess (" + sPath + ")";

                if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                {
                    ORIGIN_LOG_ERROR << "Failed to Escalate \"" << qPrintable(fileToDelete.fileName()) << "\" EscalationError:" << escalationError;
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("DiskUtil_DeleteFileWithRetry", qPrintable(fileToDelete.fileName()), escalationError);  

                    return false;
                }

                QString szSD("D:(A;OICI;GA;;;WD)");
                int err = escalationClient->createDirectory(sPath, szSD);       // The Escalation Client will also set ACL on existing folders so we use this call here to ensure it's set correctly
                if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", err, escalationClient->getSystemError()))
                {
                    ORIGIN_LOG_ERROR << "Failed to Delete \"" << qPrintable(fileToDelete.fileName()) << "\" due to being unable to set ACL";
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("DiskUtil_DeleteFileWithRetry", qPrintable(fileToDelete.fileName()), escalationClient->getSystemError());  

                    return false;
                }

                // Succeeded in setting ACL for path.  Last try to delete.
                if (fileToDelete.remove())
                {
                    // Success!  Now why are ACLs to our files being changed?
                    ORIGIN_LOG_EVENT << "Succeeded in deleting file after setting ACL on path.";
                    return true;
                }

                // Nothing worked.  Log the error and report failure dejectedly. 
                ORIGIN_LOG_ERROR << "Failed to delete \"" << qPrintable(fileToDelete.fileName()) << "\" FileError:" << fileToDelete.error() << " NativeError:" << fileToDelete.nativeError();  // The useful error should be nNativeError however we'll put both in the log
                GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("DiskUtil_DeleteFileWithRetry", qPrintable(fileToDelete.fileName()), fileToDelete.nativeError());  

                return false;
            }
        }
    }
}