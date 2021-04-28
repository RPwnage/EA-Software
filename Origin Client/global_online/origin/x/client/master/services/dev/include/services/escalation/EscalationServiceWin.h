///////////////////////////////////////////////////////////////////////////////
// EscalationServiceWin.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ESCALATIONSERVICEWIN_H
#define ESCALATIONSERVICEWIN_H


#include <limits>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <strsafe.h>

#include <userenv.h>
#include <psapi.h>

#include <Softpub.h>	//for exe signing verification
#include <wincrypt.h>	//for exe signing verification 
#include <wintrust.h>	//for exe signing verification

#include "IEscalationService.h"

#include "services/plugin/PluginAPI.h"

#include "services/escalation/ServiceUtilsWin.h"

namespace Origin
{
    namespace Escalation
    {
        class ORIGIN_PLUGIN_API EscalationServiceWin : public IEscalationService
        {
            friend class    Origin::Escalation::IEscalationService;
            Q_OBJECT
        public:
            explicit EscalationServiceWin(bool runningAsService, QStringList serviceArgs, QObject* parent = 0);
            ~EscalationServiceWin();

        protected:
            CommandError validateCaller(const quint32 clientPid);
            void addToMonitorListInternal(QString response);
            void removeFromMonitorListInternal(QString response);
            // internal utility functions
            static DWORD WINAPI processWatcher(LPVOID data);
            bool verifyEmbeddedSignature(LPCWSTR pwszSourceFile);
            bool validateCert(LPCWSTR szCallingProcessFileName);
            void getCertificateContext(LPCWSTR szFileName,  PCCERT_CONTEXT &pCertContext);

            HANDLE hProcessWatchingThread;
            bool mRunningAsService;
        };
    } // namespace Escalation

} // namespace Origin


#endif // Header include guard
