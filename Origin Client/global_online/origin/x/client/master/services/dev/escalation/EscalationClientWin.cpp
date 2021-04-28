///////////////////////////////////////////////////////////////////////////////
// EscalationClientWin.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <limits>
#include <QDateTime>

#include <windows.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <Sddl.h> 

#include "EscalationClientWin.h"
#include "ServiceUtilsWin.h"

#pragma comment(lib, "shell32.lib")


namespace Origin
{
    namespace Escalation
    {
        
        QThreadPool EscalationClientWin::sEscalationThreadPool;
        class CloseProcessTask : public QRunnable
        {
        public:
            CloseProcessTask(qint32 handle, QSemaphore& barrier) :
              mWindowHandle((HWND)handle), 
			  mBarrier(barrier),
			  mSystemError(0)
              {
                  setAutoDelete(false);
              }

              void run()
              {
                  mSystemError = ERROR_SUCCESS;

                  qint32 iWin32Result = SendMessageTimeout((HWND)mWindowHandle, WM_CLOSE, 0,0, SMTO_BLOCK, 5000, NULL);

                  if(!iWin32Result)
                  {
                      mSystemError = (int)GetLastError();
                  }

                  mBarrier.release();
              }

              int error()
              {
                  return mSystemError;
              }

        private:
            HWND mWindowHandle;
            int mSystemError;
            QSemaphore& mBarrier;
        };

        class TerminateProcessTask : public QRunnable
        {
        public:
            TerminateProcessTask(qint32 processId, QSemaphore& barrier) :
              mProcessId(processId), 
			  mBarrier(barrier),
			  mSystemError(0)
              {
                  setAutoDelete(false);
              }

              void run()
              {
                  mSystemError = ERROR_SUCCESS;

                  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, mProcessId);
                  BOOL iWin32Result =  TerminateProcess(hProcess, 0xf291);

                  if(!iWin32Result)
                  {
                      mSystemError = (int)GetLastError();
                  }

                  CloseHandle(hProcess);
                  mBarrier.release();
              }

              int error()
              {
                  return mSystemError;
              }

        private:
            qint32 mProcessId;
            int mSystemError;
            QSemaphore& mBarrier;
        };

        class ExecuteProcessTask : public QRunnable
        {
        public:
            ExecuteProcessTask(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment, QSemaphore& barrier) :
                  mProcessPath(pProcessPath), mProcessArguments(pProcessArguments), mProcessDirectory(pProcessDirectory), mEnvironment(environment), mResult(0), mSystemError(0), mBarrier(barrier)
            {
                    setAutoDelete(false);
            }

            void run()
            {
                mResult = 0;
                mSystemError = ERROR_SUCCESS;

                // We use ShellExecuteEx instead of the lower level CreateProcess function.
                // The primary difference is that the ShellExecuteEx function lets us launch
                // files and URLs in addition to processes themselves. In the case of files and
                // URLS the returned process id might be 0, even though the execution succeeded.

                SHELLEXECUTEINFOW shellExecuteInfo;
                memset(&shellExecuteInfo, 0, sizeof(shellExecuteInfo));

                shellExecuteInfo.cbSize       = sizeof(shellExecuteInfo);
                shellExecuteInfo.lpFile       = (LPCWSTR)mProcessPath.utf16();
                shellExecuteInfo.lpParameters = (LPCWSTR)mProcessArguments.utf16();
                shellExecuteInfo.lpDirectory  = (LPCWSTR)mProcessDirectory.utf16();
                shellExecuteInfo.lpVerb       = L"open";
                shellExecuteInfo.hwnd         = NULL;
                shellExecuteInfo.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI; // Indicates that the hProcess value should be set.
                shellExecuteInfo.nShow        = SW_SHOWNORMAL;

                // Parse the environment strings (passed in separated by 0xff chars) and set each variable one by one
                QStringList vars = mEnvironment.split(0xff);
                for(QStringList::iterator it = vars.begin(); it != vars.end(); it++)
                {
                    QString sVarLine = *it;
                    QStringList keyValueArray = sVarLine.split('=');
                    if (keyValueArray.size() == 2)
                    {
                        QString sKey = keyValueArray[0];
                        QString sValue = keyValueArray[1];
                        SetEnvironmentVariable(reinterpret_cast<const wchar_t*>(sKey.utf16()), reinterpret_cast<const wchar_t*>(sValue.utf16()));
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "Couldn't split environment key/value pair from:" << sVarLine;
                    }
                }

                BOOL iWin32Result = ShellExecuteExW(&shellExecuteInfo);

                if(iWin32Result)
                {
                    if(shellExecuteInfo.hProcess)
                        mResult = GetProcessId(shellExecuteInfo.hProcess);
                }
                else
                {
                    mSystemError = (int)GetLastError();
                }

                mBarrier.release();
            }

            ProcessId result()
            {
                return mResult;
            }

            int error()
            {
                return mSystemError;
            }

        private:
            QString mProcessPath;
            QString mProcessArguments;
            QString mProcessDirectory;
            QString mEnvironment;
            ProcessId mResult;
            int mSystemError;
            QSemaphore& mBarrier;
        };

        class CreateDirectoryTask : public QRunnable
        {
        public:
            CreateDirectoryTask(const QString & pDirectory, const QString & pAccessControlList, QSemaphore& barrier) :
              mDirectory(pDirectory), mAccessControlList(pAccessControlList), mSystemError(0), mBarrier(barrier)
            {
                setAutoDelete(false);
            }

            void run()
            {
                mSystemError = ERROR_SUCCESS;

                if(!mAccessControlList.isEmpty())
                {
                    SECURITY_ATTRIBUTES sa;
                    memset(&sa, 0, sizeof(sa));

                    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
                    sa.bInheritHandle = FALSE;  

                    ConvertStringSecurityDescriptorToSecurityDescriptor(mAccessControlList.utf16(), SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);

                    int iWin32Result = ::SHCreateDirectoryEx(NULL, mDirectory.utf16(), &sa);

                    if(sa.lpSecurityDescriptor) // This should always be true.
                        LocalFree(sa.lpSecurityDescriptor);

                    if(iWin32Result==ERROR_SUCCESS || iWin32Result==ERROR_ALREADY_EXISTS)
                    {
                    }
                    else
                    {
                        mSystemError = iWin32Result;
                    }
                }
                else
                {
                    int iWin32Result = SHCreateDirectoryEx(NULL, mDirectory.utf16(), NULL);

                    if(iWin32Result==ERROR_SUCCESS || iWin32Result==ERROR_ALREADY_EXISTS)
                    {
                    }
                    else
                    {
                        mSystemError = iWin32Result;
                    }
                }

                mBarrier.release();
            }

            int error()
            {
                return mSystemError;
            }
        private:
            QString mDirectory;
            QString mAccessControlList;
            ProcessId mResult;
            int mSystemError;
            QSemaphore& mBarrier;
        };

        EscalationClientWin::EscalationClientWin(QObject* parent) : IEscalationClient(parent)
        {
        }


        ProcessId EscalationClientWin::executeProcessInternal(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment)
        {
            QSemaphore taskBarrier(1);
            taskBarrier.acquire();
            ExecuteProcessTask task(pProcessPath, pProcessArguments, pProcessDirectory, environment, taskBarrier);
            sEscalationThreadPool.start(&task);

            // Wait for the task to complete before continuing on this thread, it will release the semaphore when it finishes
            taskBarrier.acquire();

            if(task.error() != ERROR_SUCCESS)
            {
                mError       = kCommandErrorProcessExecutionFailure;
                mSystemError = task.error();
            }

            return task.result();
        }

        void EscalationClientWin::terminateProcessInternal(qint32 handle)
        {
            QSemaphore taskBarrier(1);
            taskBarrier.acquire();
            TerminateProcessTask task(handle, taskBarrier);
            sEscalationThreadPool.start(&task);

            // Wait for the task to complete before continuing on this thread, it will release the semaphore when it finishes
            taskBarrier.acquire();

            if(task.error() != ERROR_SUCCESS)
            {
                mError = kCommandErrorProcessExecutionFailure;
                mSystemError = task.error();
            }
        }

        void EscalationClientWin::closeProcessInternal(qint32 processId)
        {
            QSemaphore taskBarrier(1);
            taskBarrier.acquire();
            CloseProcessTask task(processId, taskBarrier);
            sEscalationThreadPool.start(&task);

            // Wait for the task to complete before continuing on this thread, it will release the semaphore when it finishes
            taskBarrier.acquire();

            if(task.error() != ERROR_SUCCESS)
            {
                mError = kCommandErrorProcessExecutionFailure;
                mSystemError = task.error();
            }
        }

        void EscalationClientWin::createDirectoryFallback(const QString & pDirectory, const QString & pAccessControlList)
        {
            QSemaphore taskBarrier(1);
            taskBarrier.acquire();
            CreateDirectoryTask task(pDirectory, pAccessControlList, taskBarrier);
            sEscalationThreadPool.start(&task);

            // Wait for the task to complete before continuing on this thread, it will release the semaphore when it finishes
            taskBarrier.acquire();

            if(task.error() != ERROR_SUCCESS)
            {
                mError = kCommandErrorProcessExecutionFailure;
                mSystemError = task.error();
            }
        }

       void EscalationClientWin::getSystemErrorText(int systemErrorId, QString& errorText)
        {
            wchar_t* pTemp    = NULL;
            DWORD    dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL, systemErrorId, LANG_NEUTRAL, (wchar_t*)&pTemp, 0, NULL);
            if(dwResult && pTemp)
            {
                // Remove any trailing newline characters.
                while(!errorText.isEmpty() && ((errorText[errorText.size() - 1] == '\r') || (errorText[errorText.size() - 1] == '\n')))
                    errorText.resize(errorText.size() - 1);
            }

            if(pTemp)
                LocalFree((HLOCAL)pTemp);
        }
    } // namespace Escalation
} // namespace Origin
