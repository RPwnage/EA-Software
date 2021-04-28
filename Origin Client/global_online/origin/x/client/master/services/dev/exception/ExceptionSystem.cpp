/////////////////////////////////////////////////////////////////////////////
// ExceptionSystem.cpp
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ExceptionSystem.h"
#include "services/platform/PlatformService.h"
#include "services/exception/ExceptionHandler.h"
#include "services/exception/ExceptionReportWriter.h"
#include "services/debug/DebugService.h"

#include <EACallstack/EACallstack.h>
#include <EAIO/EAIniFile.h>

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            /////////////////////////////////////////////////////////////////////////
            // ExceptionSystem
            /////////////////////////////////////////////////////////////////////////
            ExceptionSystem::ExceptionSystem() :
                mExceptionHandler(NULL),
                mExceptionHandlerClient(NULL),
                mExceptionReportWriter(NULL)
            {
            }
            
            ExceptionSystem::~ExceptionSystem()
            {
                EA::Callstack::ShutdownCallstack();
            
                if(mExceptionHandler)
                {
                    mExceptionHandler->RegisterClient(mExceptionHandlerClient, false);
                    delete mExceptionHandler;
                    mExceptionHandler = NULL;
                }

                if(mExceptionHandlerClient)
                {
                    delete mExceptionHandlerClient;
                    mExceptionHandlerClient = NULL;
                }

                if(mExceptionReportWriter)
                {
                    delete mExceptionReportWriter;
                    mExceptionReportWriter = NULL;
                }
            }   

            void ExceptionSystem::start()
            {
                SetupEACallstack();
                SetupExceptionHandler();
            }
        
            void ExceptionSystem::SetupEACallstack()
            {
                EA::Callstack::InitCallstack();
                EA::Callstack::SetAddressRepCache(&mAddressRepCache);
            }

            void ExceptionSystem::SetupExceptionHandler()
            {
                // From the eacore.ini file
                // ; If present and non-zero then a handled exceptions (app crashes) are written to report files on disk. 
                // ; Otherwise they are only written to the BugSentry server.
                // ExceptionReportFileGeneration = 1
                // ExceptionMinidumpGeneration = 1
                std::wstring openFrom(Origin::Services::PlatformService::eacoreIniFilename().toStdWString());
                EA::IO::IniFile iniFile(openFrom.c_str());

                int reportFileWrite   = 0;
                int minidumpFileWrite = 1;

                iniFile.ReadEntryFormatted(L"Origin", L"ExceptionReportFileGeneration", L"%d", &reportFileWrite);
                iniFile.ReadEntryFormatted(L"Origin", L"ExceptionMinidumpGeneration", L"%d", &minidumpFileWrite);
                
                mExceptionHandler = new Services::Exception::ExceptionHandler(minidumpFileWrite != 0);

#ifdef ORIGIN_MAC
                mExceptionHandler->SetMode(ExceptionHandler::kModeVectored);
#endif
                
                mExceptionReportWriter = new Services::Exception::ExceptionReportWriter(reportFileWrite != 0);
                mExceptionHandlerClient = new Services::Exception::ExceptionHandlerClient();

                mExceptionHandler->RegisterClient(mExceptionHandlerClient, true);
                mExceptionHandler->SetAction(EA::Debug::ExceptionHandler::kActionTerminate);
                
                if(EA::Debug::ExceptionHandler::IsDebuggerPresent())
                    mExceptionHandler->SetEnabled(false);//Set it to true to test under a debugger.
                else
                    mExceptionHandler->SetEnabled(true);

                mExceptionHandler->SetReportWriter(mExceptionReportWriter);
            }
            
            bool ExceptionSystem::enabled() const
            {
                if (mExceptionHandler)
                    return mExceptionHandler->IsEnabled();
                
                return false;
            }
            
            void ExceptionSystem::setEnabled(bool enabled)
            {
                if (mExceptionHandler)
                    mExceptionHandler->SetEnabled(enabled);
            }
        }
    }
}




