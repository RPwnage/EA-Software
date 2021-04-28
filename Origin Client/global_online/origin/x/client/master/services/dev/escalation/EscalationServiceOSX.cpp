///////////////////////////////////////////////////////////////////////////////
// EscalationServiceOSX.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// References:
//
// http://www.kinook.com/blog/?p=10
//
///////////////////////////////////////////////////////////////////////////////

#include "EscalationServiceOSX.h"
#include "CommandProcessorOSX.h"


#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

namespace Origin
{

    namespace Escalation
    {

        EscalationServiceOSX::EscalationServiceOSX(QObject* parent) : IEscalationService(parent)
        {
        }

        EscalationServiceOSX::EscalationServiceOSX(bool useManualConnections, QObject* parent) : IEscalationService(useManualConnections, parent)
        {
        }

        EscalationServiceOSX::~EscalationServiceOSX()
        {
        }

        CommandError EscalationServiceOSX::validateCaller(const quint32 clientPid)
        {
			Q_UNUSED(clientPid);
            return kCommandErrorNone;
        }

        void EscalationServiceOSX::addToMonitorListInternal(QString response)
        {

        }

        void EscalationServiceOSX::removeFromMonitorListInternal(QString response)
        {

        }

    } // namespace Escalation


} // namespace Origin
