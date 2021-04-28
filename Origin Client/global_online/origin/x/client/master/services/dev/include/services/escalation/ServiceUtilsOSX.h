///////////////////////////////////////////////////////////////////////////////
// ServiceUtilsOSX.h
//
// Created by Ryan Binns
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef SERVICEUTILSOSX_H
#define SERVICEUTILSOSX_H

#define ORIGIN_BEACON_SERVICE_NAME "com.ea.origin.WebHelper"

// Helpers for the Cocoa API tasks we need to do
namespace Origin
{
    namespace Escalation
    {
    
        bool InstallService(const char* serviceName);
        bool UninstallService(const char* serviceName);
        
    } // Escalation
} // Origin

#endif // SERVICEUTILSOSX_H