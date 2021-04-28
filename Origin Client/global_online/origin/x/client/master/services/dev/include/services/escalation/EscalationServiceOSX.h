///////////////////////////////////////////////////////////////////////////////
// EscalationServiceOSX.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ESCALATIONSERVICEOSX_H
#define ESCALATIONSERVICEOSX_H


#include "IEscalationService.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
 
    namespace Escalation
    {
        class ORIGIN_PLUGIN_API EscalationServiceOSX : public IEscalationService
        {
            friend class    Origin::Escalation::IEscalationService;
            Q_OBJECT
        public:
            explicit EscalationServiceOSX(QObject* parent = 0);
            explicit EscalationServiceOSX(bool useManualConnections, QObject* parent = 0);
            ~EscalationServiceOSX();

        protected:
            CommandError validateCaller(const quint32 clientPid);
            void addToMonitorListInternal(QString response);
            void removeFromMonitorListInternal(QString response);
        };

    } // namespace Escalation

} // namespace Origin


#endif // Header include guard
