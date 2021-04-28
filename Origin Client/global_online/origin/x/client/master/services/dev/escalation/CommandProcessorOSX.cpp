///////////////////////////////////////////////////////////////////////////////
// CommandProcessorOSX.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "CommandProcessorOSX.h"
#include "PlatformService.h"

#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>

namespace Origin
{

    namespace Escalation
    {
        static const QString AssistiveDevicesOnScriptName = "AssistiveDevicesOn";
        static const char* AssistiveDevicesOnScriptText = "tell application \"System Events\"\n\
                                                           activate\n\
                                                           set UI elements enabled to true\n\
                                                           end tell";
        
        
        CommandProcessorOSX::CommandProcessorOSX(QObject* parent)
            : ICommandProcessor(parent)
        {
        }


        CommandProcessorOSX::CommandProcessorOSX(const QString& sCommandText, QObject* parent)
            : ICommandProcessor(sCommandText, parent)
        {
        }


        bool CommandProcessorOSX::isOriginUserAdmin()
        {
            bool isAdmin = true;


            return isAdmin;
        }

        int CommandProcessorOSX::executeProcess()
        {
            ProcessId processId = 0;
            bool bResult = false;
            
            if(mCommandArray.size() > 1) // If there is a process path string at mCommandArray[1]...
            {
                bResult = true;

				// these variables are not currently being used
                const QString & pProcessPath      = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                const QString & pProcessArguments = ((mCommandArray.size() > 2) && !mCommandArray[2].isEmpty()) ? mCommandArray[2] : QString();
                const QString & pProcessDirectory = ((mCommandArray.size() > 3) && !mCommandArray[3].isEmpty()) ? mCommandArray[3] : QString();

				Q_UNUSED(pProcessPath);
				Q_UNUSED(pProcessArguments);
				Q_UNUSED(pProcessDirectory);

                if(bResult)
                {
				}
                else
                {
                    //mSystemError = GetLastError();
                    doError(kCommandErrorProcessExecutionFailure, "CommandProcessorOSX::ExecuteProcess: ShellExecuteExW failed", true);
                }
            }
            else
                doError(kCommandErrorProcessExecutionFailure, "CommandProcessor::ExecuteProcess: Invalid command.", false);

            if(bResult)
                doSuccess();

            // Write the process id result to the response text.
            mResponseText += QString::number(processId, 16);
            mResponseText += '\0';
            return mError;
        }

        // Creates the directory with unrestricted permissions for all users.  Also creates any non-existant intermediate
        // directories in the path, and sets unrestricted permissions for the newly created directories.  And sets 
        // unrestricted permissions for any pre-existing directories in the path - unless they are a member of the prohibited 
        // system directory list contained in the QList<QDir> 'systemDirs'.  Assigns ownership to the user for all
        // directories in which permissions are changed.
        int CommandProcessorOSX::createDirectory()
        {
            if(mCommandArray.size() > 1) // If there is a directory path string at mCommandArray[1]...
            {
                bool bResult = true;

                // initialize IDs to root
                int userID = 0;
                int groupID = 0;
                
                if(mCommandArray.size() > 2)
                {
                    // On Mac, mCommandArray[2] holds the unix user id and group id, so that ownership of the created directory can
                    // be transferred to the user.
					QStringList IDs = mCommandArray[2].split(';');
                    if (IDs.size() > 0)
                    {
                        userID = IDs[0].toInt();
                    }
                    if (IDs.size() > 1)
                    {
                        groupID = IDs[1].toInt();
                    }
				}
                
                // Create the list of system directories.  Permissions on these directories will not be changed.
                QList<QDir> systemDirs;
                
                // These are the primary system directories to worry about, since the escalation service is only currently
                // being used to create directories in the Origin .app bundle, and in /Library/Application Support/
                systemDirs << QDir("/"); // Just being paranoid, not really necessary
                systemDirs << QDir("/Applications/");
                systemDirs << QDir("/Library/");
                systemDirs << QDir("/Library/Application Support/");
                
                // Additional system directories that might as well be included for future-proofing.  Based on the 
                // prohibited download folder list in ContentFolders.cpp, in engine.
                systemDirs << QDir("/.DocumentRevisions-V100/");
                systemDirs << QDir("/.Spotlight-V100/");
                systemDirs << QDir("/.Trashes/");
                systemDirs << QDir("/.vol/");
                systemDirs << QDir("/Applications/Utilities/");
                systemDirs << QDir("/Network/");
                systemDirs << QDir("/System/");
                systemDirs << QDir("/Volumes/");
                systemDirs << QDir("/bin/");
                systemDirs << QDir("/cores/");
                systemDirs << QDir("/dev/");
                systemDirs << QDir("/home/");
                systemDirs << QDir("/net/");
                systemDirs << QDir("/private/");
                systemDirs << QDir("/sbin/");
                systemDirs << QDir("/usr/");
                
                // Create the path and set permissions for all directories unless they are in systemDirs.
                QString cleanedPath = QDir::cleanPath(mCommandArray[1]);
                QStringList splitPath = cleanedPath.split('/');
                QDir dir("/");
                for (int i = 0; i < splitPath.size(); i++)
                {
                    if (splitPath[i].isEmpty())
                        continue;
                    
                    QDir nextDir(dir.absolutePath() + QDir::separator() + splitPath[i]);
                    if (!nextDir.exists())
                    {
                        // Need to create a subdirectory
                        bResult = dir.mkdir(splitPath[i]);
                        if (!bResult)
                        {
                            // Error in the creation of the subdirectory
                            break;
                        }
                    }
                    dir.cd(splitPath[i]);
                    if (!systemDirs.contains(dir))
                    {
                        // Set permissions
                        QFile::setPermissions(dir.absolutePath(), (QFlags<QFile::Permission>)0x7777);
                        // Set ownership
                        if (chown(dir.absolutePath().toUtf8().constData(), userID, groupID) != 0)
                        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                            ORIGIN_LOG_EVENT << "Failed to set ownership on " << dir.absolutePath();
#endif
                        }
                    }
                }
                        
                if(bResult) 
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "Successfully created directory [" << mCommandArray[1] << "]";
#endif
                    doSuccess();
                }
                else
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to create directory in escalation service.";
#endif
                    //mSystemError = bResult;
                    doError(kCommandErrorProcessExecutionFailure, "CommandProcessorOSX::CreateDirectory: Creation failure.", true);
                }
            }
            else
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to create directory in escalation service, error was invalid command.";
#endif
                doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::CreateDirectory: Invalid command.", true);
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }
        
        // Deletes an OOA license from within a Mac bundle.  Used by ODT's "purge licenses" setting.  This function
        // will only delete files with the extension .dlf, from a specific bundle subdirectory path.
        int CommandProcessorOSX::deleteBundleLicense()
        {
            if(mCommandArray.size() == 2) // We need one argument: the bundle path
            {
                bool bResult = true;
                
                // Ensure that the license folder exists in the bundle, and that it contains at least one license file.
                QDir licenseDir(mCommandArray[1] + "/__Installer/OOA/License");
                if (licenseDir.exists())
                {
                    QStringList fileList(licenseDir.entryList(QStringList("*.dlf"), QDir::Files));
                    if (!fileList.isEmpty())
                    {
                        foreach(QString licenseFile, fileList)
                        {
                            if (!licenseDir.remove(licenseFile))
                            {
                                bResult = false;
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                                ORIGIN_LOG_ERROR << "Failed to delete a bundle license in the escalation service: " << licenseDir.absoluteFilePath(licenseFile);
#endif
                            }
                        }
                    }
                    else
                    {
                        bResult = false;
                    }
                }
                else
                {
                    bResult = false;
                }
                
                if(bResult)
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "Successfully deleted bundle license(s) in [" << licenseDir.absolutePath()<< "]";
#endif
                    doSuccess();
                }
                else
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to delete bundle license in the escalation service, for bundle path: " << mCommandArray[1];
#endif
                    doError(kCommandErrorProcessExecutionFailure, "CommandProcessorOSX::DeleteBundleLicense: Deletion failure.", false);
                }
            }
            else
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to delete bundle license in escalation service, error was invalid command.";
#endif
                doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::DeleteBundleLicense: Invalid command.", false);
            }
            
            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }
        
        // Command used to run a compiled AppleScript.
        int CommandProcessorOSX::runScript()
        {
            if(mCommandArray.size() == 2) // We need one argument: the name of the script to run.
            {
                //For security purposes/because we really only use this for one purpose right now, 
                // we hardcoded the content of the script and refer to it by name.
                if (0) // Not able to run AppleScript to enable assistive devices from daemon - mCommandArray[1].compare(AssistiveDevicesOnScriptName, Qt::CaseInsensitive) == 0)
                {
                    char errorMsg[256];
                    if(Origin::Services::PlatformService::runAppleScript(AssistiveDevicesOnScriptText, errorMsg, sizeof(errorMsg)))
                    {
                       doSuccess();
                    }
                    
                    else
                    {
                        char tmp[256];
                        errorMsg[0] = '\0';
                        
                        snprintf(tmp, sizeof(tmp), "CommandProcessorOSX::RunScript: %s", errorMsg);
                        doError(kCommandErrorProcessExecutionFailure, tmp, false);
                    }
                }
                
                else
                    doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::RunScript: Unknown script name", false);
            }
            else
                doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::RunScript: Invalid command.", false);
            
            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;            
        }

        // Command used to enable assistive devices setting.        
        int CommandProcessorOSX::enableAssistiveDevices()
        {
            if(mCommandArray.size() == 1) // No argument
            {
                if (Origin::Services::PlatformService::enableAssistiveDevices())
                    doSuccess();
                
                else
                    doError(kCommandErrorProcessExecutionFailure, "CommandProcessorOSX::EnableAssistiveDevices: Failed to turn on setting.", false);
            }
            else
                doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::EnableAssistiveDevices: Invalid command.", false);
            
            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;            
        }

        // Command used to delete a directory.
        int CommandProcessorOSX::deleteDirectory()
        {
            if(mCommandArray.size() == 2) // We need two arguments: the unique process id to inject code into and the absolute path to the injection code
            {
                const QString path = mCommandArray[1];
                QFileInfo directory(path);
                
                // For the time being, limit this method to the deletion of bundles and the ODT directory only
                if (directory.isDir() && directory.isAbsolute() && (directory.isBundle() || path.contains(Services::PlatformService::ODT_FOLDER_NAME)))
                {
                    char errorMsg[256];
                    QByteArray folderName = directory.filePath().toUtf8();
                    if(Origin::Services::PlatformService::deleteDirectory(folderName.constData(), errorMsg, sizeof(errorMsg)))
                    {
                        doSuccess();
                    }
                    
                    else
                    {
                        char tmp[256];
                        errorMsg[0] = '\0';
                        
                        snprintf(tmp, sizeof(tmp), "CommandProcessorOSX::DeleteDirectory: %s", errorMsg);
                        doError(kCommandErrorProcessExecutionFailure, tmp, false);
                    }                    
                }
                else
                    doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::DeleteDirectory: Not an absolute path.", false);
            }
            else
                doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::DeleteDirectory: Invalid command.", false);
            
            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }
        
       void CommandProcessorOSX::getSystemErrorText(int systemErrorId, QString& errorText)
        {
        }

       int CommandProcessorOSX::terminateProcess()
       {
           return 0;
       }

       int CommandProcessorOSX::closeProcess()
       {
           return 0;
       }

        int CommandProcessorOSX::addToMonitorList()
        {
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString processIdString = ((mCommandArray.size() > 2) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                const QString exeName = ((mCommandArray.size() > 2) && !mCommandArray[2].isEmpty()) ? mCommandArray[2] : QString();

                mResponseText = "addMonitorList," + processIdString +"," + exeName;
            }
            return 0;
        }

        int CommandProcessorOSX::removeFromMonitorList()
        {
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString processIdString = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();

                mResponseText = "removeMonitorList," + processIdString;
            }
            return 0;
        }

        int CommandProcessorOSX::injectIGOIntoProcess()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "injectIGOIntoProcess command is not supported on Mac";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::injectIGOIntoProcess: Not supported.", false);

            mResponseText += '\0';
            return mError;
        }

        int CommandProcessorOSX::setRegistryStringValue()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "setRegistryStringValue command is not supported on Mac";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::setRegistryStringValue: Not supported.", false);

            mResponseText += '\0';
            return mError;
        }

        int CommandProcessorOSX::setRegistryIntValue()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "setRegistryIntValue command is not supported on Mac";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::setRegistryIntValue: Not supported.", false);

            mResponseText += '\0';
            return mError;
        }

        int CommandProcessorOSX::deleteRegistrySubKey()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "deleteRegistrySubKey command is not supported on Mac";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorOSX::deleteRegistrySubKey: Not supported.", false);

            mResponseText += '\0';
            return mError;
        }

    } // namespace Escalation
} // namespace Origin

