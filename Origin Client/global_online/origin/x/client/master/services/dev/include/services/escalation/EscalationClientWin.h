///////////////////////////////////////////////////////////////////////////////
// EscalationClientWin.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ESCALATIONCLIENTWIN_H
#define ESCALATIONCLIENTWIN_H


#include "IEscalationClient.h"
#include <QThreadPool>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{ 

    namespace Escalation
    { 
        ///////////////////////////////////////////////////////////////////////////////
        // EscalationClient
        ///////////////////////////////////////////////////////////////////////////////

        class ORIGIN_PLUGIN_API EscalationClientWin : public IEscalationClient
        {
            friend class    Origin::Escalation::IEscalationClient;

            Q_OBJECT

        protected:
            explicit EscalationClientWin(QObject* parent = 0);
            void getSystemErrorText(int systemErrorId, QString& errorText);

        protected:
            ProcessId   executeProcessInternal(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment);
            void        createDirectoryFallback(const QString & pDirectory, const QString & pAccessControlList);
            void        terminateProcessInternal(qint32 handle);
            void        closeProcessInternal(qint32 processId);

        private:
            static QThreadPool sEscalationThreadPool;

        };
    } // namespace Escalation

} // namespace Origin


#endif // Header include guard
