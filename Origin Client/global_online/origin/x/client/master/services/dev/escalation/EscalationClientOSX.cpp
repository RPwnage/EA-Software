///////////////////////////////////////////////////////////////////////////////
// EscalationClientOSX.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "EscalationClientOSX.h"
#include "services/platform/PlatformService.h"
#include <QFileInfo>
#include <QDir>

namespace Origin
{
    namespace Escalation
    {
        EscalationClientOSX::EscalationClientOSX(QObject* parent) : IEscalationClient(parent)
        {
        }


        ProcessId EscalationClientOSX::executeProcessInternal(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment)
        {
            ProcessId processId = 0;
            return processId;
        }


        void EscalationClientOSX::createDirectoryFallback(const QString & pDirectory, const QString & pAccessControlList)
        {
            QDir path;
            if (!path.mkpath(pDirectory))
            {
                ORIGIN_LOG_ERROR << "Could't create path, error: " << (int)Services::PlatformService::lastSystemResultCode();
            }
        }


       void EscalationClientOSX::getSystemErrorText(int systemErrorId, QString& errorText)
        {
        }

       void EscalationClientOSX::terminateProcessInternal(qint32 handle)
       {

       }
       void EscalationClientOSX::closeProcessInternal(qint32 processId)
       {

       }
    } // namespace Escalation
} // namespace Origin
