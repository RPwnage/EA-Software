///////////////////////////////////////////////////////////////////////////////
// ServiceUtilsWin.h
//
// Created by Ryan Binns
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef SERVICEUTILSWIN_H
#define SERVICEUTILSWIN_H

#include <limits>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>
#include <windows.h>
#include <Sddl.h>

#define ORIGIN_ESCALATION_SERVICE_NAME L"Origin Client Service"
#define ORIGIN_BEACON_SERVICE_NAME L"Origin Web Helper Service"

#define QS16(x) QString::fromUtf16(x)

// Helpers for the WIN32 API tasks we need to do
namespace Origin
{
    namespace Escalation
    {
        class Auto_HANDLE
        {
        public:
            Auto_HANDLE() : _handle(0) { }
            Auto_HANDLE(HANDLE handle) : _handle(handle) { }

            ~Auto_HANDLE() 
            {
                if (_handle)
                {
                    CloseHandle(_handle);
                }
            }

            operator HANDLE() { return _handle; }

            Auto_HANDLE& operator=(const HANDLE rhs)
            {
                _handle = rhs;
                return *this;
            }

            HANDLE* operator&() { return &_handle; }

            HANDLE release()
            {
                HANDLE tmp = _handle;
                _handle = 0;
                return tmp;
            }

        private:
            HANDLE _handle;
        };

        bool IsWindowsXP();

        bool QueryOriginServiceStatusInternal(QString serviceName, int serviceState, SERVICE_STATUS_PROCESS& status);
        bool QueryOriginServiceStatus(QString serviceName, bool& running);

        bool IsUserInteractive();
        bool RegisterService(QString serviceName, QString executablePath, bool useLocalSystemAccount, bool autoStart = false);
        bool UnRegisterService(QString serviceName);

        bool StartOriginService(QString serviceName, QString args, int timeoutMS = 15000);
        bool StopOriginService(QString serviceName, int timeoutMS = 15000);
        bool QueryOriginServiceConfig(QString serviceName);

        bool UpdateProcessDACL(HANDLE hProcess);
        bool SetTokenPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, bool bEnablePrivilege);

        bool LaunchProcessElevatedFromService(int originPid, QString file, QString args, QString currentDirectory, QString processEnvironment, quint32& pid);
    } // Escalation
} // Origin

#endif // SERVICEUTILSWIN_H