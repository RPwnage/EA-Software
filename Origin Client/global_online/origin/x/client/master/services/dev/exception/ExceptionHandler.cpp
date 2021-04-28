/////////////////////////////////////////////////////////////////////////////
// ExceptionHandler.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ExceptionHandler.h"
#include "base64.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "services/exception/CrashReportData.h"
#include "services/exception/ExceptionReportWriter.h"
#include "services/settings/SettingsManager.h"
#include "version/version.h"
#include "TelemetryAPIDLL.h"

#include <EACallstack/Context.h>
#include <EAStdC/EAProcess.h>
#include <BugSentry/bugsentryconfig.h>

#include <QApplication>

// For uploading core log
#if defined(ORIGIN_PC)
#include <ExceptionHandler/Win32/ExceptionHandlerWin32.h>
#include <Windows.h> //for OutputDebugStringA
#elif defined (ORIGIN_MAC)
#include <ExceptionHandler/Apple/ExceptionHandlerApple.h>
#include <cstdio>
#include <libproc.h>
#include <unistd.h>
#endif

//	Flag to enable mini-dump to be included in the BugSentry crash report
#define ENABLE_INCLUDEMINIDUMPOUTPUT

// Defines the maximum number of bytes to upload from the bootstrap log.
static const int BOOTSTRAP_LOG_MAX_UPLOAD_BYTES = 6 * 1024;

//	Max BugSentry upload size to server:  500KB
//  We Base64 encode prior to sending to the server.  This increases the size
//  of the report, so lower max upload size here accordingly.
//  Leave 5KB wiggle room for user-added comments, etc.
static const int BUG_SENTRY_MAX_UPLOAD_SIZE_BYTES = Base64DecodedSize(495 * 1024);

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            /////////////////////////////////////////////////////////////////////////
            // ExceptionHandler
            /////////////////////////////////////////////////////////////////////////        
            ExceptionHandler::ExceptionHandler(bool minidumpEnabled) : 
              EA::Debug::ExceptionHandler()
            {
                // Set report directory
                wchar_t reportPathW[1024] = { 0 };
                QString reportPath = Origin::Services::PlatformService::logDirectory();
                reportPath.toWCharArray(reportPathW);

                SetReportDirectory(reportPathW);

                // Set report preferences
                int flags = EA::Debug::ExceptionHandler::kReportTypeException; // We always enable this.
                if(minidumpEnabled)
                {
                    flags |= EA::Debug::ExceptionHandler::kReportTypeMinidump;
                }
                EnableReportTypes(flags); 

                //  Exception build signature = "R.9,0,0,0", 16 byte limit
                QString build = EBISU_BUILD_STR;
                QString exceptionBuildSignature = QString("%1.%2").arg(build[0]).arg(EBISU_VERSION_STR); 
                exceptionBuildSignature.replace(QRegExp(" "), "_");
                SetBaseName(exceptionBuildSignature.toUtf8().data());
            }

            /////////////////////////////////////////////////////////////////////////
            // ExceptionHandlerClient
            /////////////////////////////////////////////////////////////////////////
            ExceptionHandlerClient::ExceptionHandlerClient() : 
              EA::Debug::ExceptionHandlerClient()
            {
            }
        
            void ExceptionHandlerClient::Notify(EA::Debug::ExceptionHandler* pHandler, EA::Debug::ExceptionHandler::EventId id)
            {
                static volatile bool CrashOnce = false;

                bool fullMemoryDump = Services::SettingsManager::isLoaded() ? Services::readSetting(Services::SETTING_FullMemoryDump).toQVariant().toBool() : 0;
                if((fullMemoryDump == true) && (pHandler != NULL))
                    pHandler->SetMiniDumpFlags(2 /*MiniDumpWithFullMemory */);

                if(id != EA::Debug::ExceptionHandler::kExceptionHandlingEnd) return; // only process ExceptionHandlingEnd

                //	Clients are sometimes spamming BugSentry so check if report sent once already.
                if (CrashOnce == true)
                {
                    ORIGIN_LOG_EVENT << "Crash re-entry detected";
                    return;
                }

                //	Logging for crash
                ORIGIN_LOG_EVENT << "Crash handler called";
                CrashOnce = true;

#if defined(_DEBUG)
                // Handy dialog to "pause" execution so you can attach and debug
                //MessageBox(NULL, L"Attach debugger to step through crash reporting code.", L"Debug", MB_ICONEXCLAMATION|MB_OK);
#endif
            
                Services::Exception::ExceptionReportWriter *pReportWriter = static_cast<Services::Exception::ExceptionReportWriter*>(pHandler->GetReportWriter());
                Services::Exception::CrashReportData crashData;

                // The Exception report writer includes ~30KB of standard data in the report, so subtract the existing data from our buffer size
                int uploadBufferRemaining = BUG_SENTRY_MAX_UPLOAD_SIZE_BYTES - EA::StdC::Strlen(pReportWriter->GetReportText());
                
                // Populate the crash data.
                crashData.mLocale = QLocale().name();
				// HACK for Norwegian since Origin only knows about no_NO
				if (crashData.mLocale == "nb_NO")
					crashData.mLocale = "no_NO";
                crashData.mOriginPath = QApplication::applicationFilePath();
                crashData.mLocalizedFaqUrl = "file:///" + Services::PlatformService::clientLogFilename();
                crashData.mOptOutMode = Services::SettingsManager::isLoaded() ? Services::readSetting(Services::SETTING_CRASH_DATA_OPTOUT).toQVariant().toInt() : Services::AskMe;
                
                bool useTestServer = Services::SettingsManager::isLoaded() ? Services::readSetting(Services::SETTING_UseTestServer).toQVariant().toBool() : 0;
                crashData.mBugSentryMode = useTestServer ? EA::BugSentry::BUGSENTRYMODE_TEST : EA::BugSentry::BUGSENTRYMODE_PROD;
                
                //  Build SKU = "ea.ears.ebisu.Windows_on_X86"
                crashData.mSKU = QString("ea.ears.ebisu.%1").arg(EA_PLATFORM_DESCRIPTION);
                crashData.mSKU.replace(QRegExp(" "), "_");

                //  Build signature = "Release.9,0,0,0"
                crashData.mBuildSignature = QString("%1.%2").arg(EBISU_BUILD_STR).arg(EBISU_VERSION_STR); 
                crashData.mBuildSignature.replace(QRegExp(" "), "_");
            
#ifdef ORIGIN_PC
            
                // We want to send the crash telemetry regardless of the user's decision to send the report.
                void* platformExceptionInfo = pHandler->GetPlatformExceptionInfo();
            
                // On Windows platformExceptionInfo is LPEXCEPTION_POINTERS.
                // Get the category id and use the last four digits
                LPEXCEPTION_POINTERS pWin32Pointers = reinterpret_cast<LPEXCEPTION_POINTERS>(platformExceptionInfo);
                void* crashIP = pWin32Pointers->ExceptionRecord->ExceptionAddress;
                int categoryId = ( reinterpret_cast<unsigned int>( crashIP ) ) & 0x0000ffff;
                char categoryIdString[16];
                _snprintf( categoryIdString, sizeof( categoryIdString ) - 1, "%04x", categoryId );
                crashData.mCategoryIdString = categoryIdString;
                
                ORIGIN_LOG_EVENT << "Crash Code: " << crashData.mCategoryIdString;
            
#elif defined(ORIGIN_MAC)
            
                // Obtain the exception callstack
                EA::Callstack::CallstackContext crashCallstackContext;
                void* callstack[2];
                GetCallstackContext( crashCallstackContext, pHandler->GetExceptionContext() );
                GetCallstack( callstack, 2, &crashCallstackContext );
            
                // Convert the top address into a string (the "Category ID String").
                int categoryId = ( reinterpret_cast<unsigned int>( callstack[0 ]) ) & 0x00000fff;
                char categoryIdString[16];
                snprintf( categoryIdString, sizeof( categoryIdString ) - 1, "%03x", categoryId );
                crashData.mCategoryIdString = categoryIdString;
                
                ORIGIN_LOG_EVENT << "Crash Code: " << crashData.mCategoryIdString;
            
#endif
                
                //	Write the log files and EACore.ini to the crash report
                uploadBufferRemaining -= pReportWriter->WriteEACoreIniToCrashReport();
            
#ifdef ENABLE_INCLUDEMINIDUMPOUTPUT
                QString minidumpPath;
#if defined(ORIGIN_PC)
                EA::Debug::ExceptionHandlerWin32* pWin32Handler = pHandler->GetPlatformHandler<EA::Debug::ExceptionHandlerWin32*>();
                minidumpPath = pWin32Handler->mMiniDumpPath;
#endif

                //	Write the mini-dump to the crash report
                uploadBufferRemaining -= pReportWriter->WriteMiniDumpToCrashReport(minidumpPath);
#endif
                // Write the bootstrap log to the crash report
                uploadBufferRemaining -= pReportWriter->WriteBootstrapLogToCrashReport(BOOTSTRAP_LOG_MAX_UPLOAD_BYTES);

                QString logFile;
                Origin::Services::Logger::Instance()->GetLogFile(logFile, uploadBufferRemaining > 0 ? (uploadBufferRemaining / 1024) : 0);
                pReportWriter->WriteText(logFile.toUtf8());
                
                crashData.mReportText = pReportWriter->GetReportText();
                ORIGIN_LOG_EVENT << "Encoded report length: " << Base64EncodedSize(crashData.mReportText.length()) << " bytes";

#if defined(ORIGIN_MAC)            
                // Disable any further exception handling.  This is particularly important on Mac since the
                // Mach ports used to capture exceptions are inherited by any child processes.
                pHandler->SetEnabled( false );
            
//              // Handle the rest of the crash in a forked copy.
//              // The crashed app is unstable.  It's also unstable to do anything in a forked() process, but
//              // at this point we have little choice, and it's more stable than running in the crashed process.
//              // Forking and exiting the parent also ensures we do not leave this process in "limbo", i.e., an unkillable
//              // dead process.
//              if ( fork() )
//              {
//                  // Exit the parent process.
//                  // Hard exit because destructors and things like shutdownMiddleware() will hang forever.
//                  _exit(0);
//              }            
#endif

                // Send crash report telemetry event
                QByteArray reportPreference;
                switch(crashData.mOptOutMode)
                {
                    case Always:
                        reportPreference = "always";
                        break;
                    case Never:
                        reportPreference = "never";
                        break;
                    case AskMe:
                    default:
                        reportPreference = "askme";
                        break;
                }
                
                // Send crash report telemetry event
                bool shutdownCrash = (Origin::Services::PlatformService::originState() == Origin::Services::PlatformService::ShutDown);
                GetTelemetryInterface()->Metric_ERROR_CRASH(crashData.mCategoryIdString.constData(), reportPreference.constData(), shutdownCrash);
                OriginTelemetry::release(); //Shutdown telemetry so it will send the crash event.
                
#if defined(ORIGIN_PC)    
                OutputDebugStringA(crashData.mReportText.constData());
#else
                printf("%s", crashData.mReportText.constData());
#endif

                QString tempFileName;
                {
                // Create a temporary file to hold the crash report data.
                QTemporaryFile tempFile;
                tempFile.open();
                tempFile.setAutoRemove(false);

                // Write out the crash report data.
                crashData.writeToDisk(tempFile);
                    tempFileName = tempFile.fileName();
                }
            
                ORIGIN_LOG_EVENT << "Wrote temporary crash data to " << tempFileName;
            
#if defined(ORIGIN_MAC)
                // Because Mach exception handler ports are inherited by child processes,
                // uninstall our crash handler.
                pHandler->SetEnabled( false );
            
                QByteArray tempName = tempFileName.toUtf8();
            
                // Get the path of the current executable.
                char pathname[PROC_PIDPATHINFO_MAXSIZE];
                proc_pidpath( getpid(), pathname, sizeof( pathname ) );
            
                // Prepare to run the crash reporter located in the same directory as Origin.
                QString executablePath( pathname );
                executablePath.truncate( executablePath.lastIndexOf( "/" ) );
                executablePath.append( "/OriginCrashReporter" );
                QByteArray asciiPath = executablePath.toLocal8Bit();
                char* executable = asciiPath.data();
            
                ORIGIN_LOG_EVENT << "Launching crash helper: " << executable;

                // Create a new process to handle the crash.
                int childPid = fork();
            
                // Hard exit if we are the parent.
                if (childPid) _exit(0);
            
                // Otherwise, (if we are the new child process)
                else
                {
                    // close all file descriptors beyond stdin, stdout, and stderrr.
                    struct rlimit lim;
                    getrlimit( RLIMIT_NOFILE, &lim );
                    for ( rlim_t i = 3; i != lim.rlim_cur; ++i ) close( i );
                
                    // Execute the crash helper.
                    char* argv[] = { executable, tempName.data(), ( char* ) 0 };
                    execv(executable, argv);
                }
#elif defined(ORIGIN_PC)
                wchar_t processPath[MAX_PATH];
                wchar_t processDir[MAX_PATH];
                EA::StdC::GetCurrentProcessPath(processPath);
            
                QString executablePath( QString::fromWCharArray( processPath ) );
                executablePath.truncate( executablePath.lastIndexOf( "\\" ) );
                executablePath.append( "\\OriginCrashReporter.exe" );
                
                QStringList arguments;
                arguments << tempFileName;
                
                EA::StdC::GetCurrentProcessDirectory(processDir);
                QString workingDir = QString::fromWCharArray(processDir);

                ORIGIN_LOG_EVENT << "Launching crash helper: " << executablePath;

                QApplication::quit();
                QProcess::startDetached(executablePath, arguments, workingDir);
#endif
            }
        }
    }
}




