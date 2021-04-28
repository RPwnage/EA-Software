///////////////////////////////////////////////////////////////////////////////
// version.h
//
// Defines version numbers and strings for this app
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Middleware
///////////////////////////////////////////////////////////////////////////////

#ifndef EALS_VERSION_H
#define EALS_VERSION_H


#ifndef _BUILD_H_
#define _BUILD_H_

#define _RUPTURE_SERVICE_PRODUCTION	

#define _RUPTURE_BUILD_TEST

#endif

#define EALS_VERSION_BUILD_TIME "12/11/2015 11:33:50 "

#define EALS_COMPANY_NAME_STR "Electronic Arts"

#define EALS_PRODUCT_NAME_STR "EADM"

#define EALS_APPLICATION_NAME_STR "EADM.exe"

#if defined(EA_DEBUG) || defined(_DEBUG)
    #define EALS_BUILD_STR "Debug"
#else
    #define EALS_BUILD_STR "Release"
#endif

#define EALS_REV_NUMBER 10257

#define EALS_VERSION      10,0,0,10257
#define EALS_VERSION_STR "10,0,0,10257"
#define EALS_VERSION_P_DELIMITED "10.0.0.10257"

#define EALS_PRODUCT_VERSION      10,0,0,10257
#define EALS_PRODUCT_VERSION_STR "10,0,0,10257"

#ifdef _RUPTURE_BUILD_TEST
    #define EALS_VERSION_STRING_TEXT "1.2.0 (rev. 10257) (test)"
#else
    #define EALS_VERSION_STRING_TEXT "1.2.0 (rev. 10257)"
#endif

#endif // Header include guard

///////////////////////////////////////////////////////////////////////////////
// UI
///////////////////////////////////////////////////////////////////////////////

#ifndef EBISU_VERSION_H
#define EBISU_VERSION_H

#define EBISU_VERSION_BUILD_TIME "12/11/2015 11:33:50 "

#define EBISU_COMPANY_NAME_STR "Electronic Arts"

#define EBISU_PRODUCT_NAME_STR "Origin"

#define EBISU_APPLICATION_NAME_STR "Origin.exe"

#if defined(EA_DEBUG) || defined(_DEBUG)
    #define EBISU_BUILD_STR "Debug"
#else
    #define EBISU_BUILD_STR "Release"
#endif

#define EBISU_REV_NUMBER 10257

#define EBISU_CHANGELIST_STR "0"

#define EBISU_VERSION      10,0,0,10257
#define EBISU_VERSION_STR "10,0,0,10257"

#define EBISU_PRODUCT_VERSION      10,0,0,10257
#define EBISU_PRODUCT_VERSION_STR "10,0,0,10257"
#define EBISU_PRODUCT_VERSION_P_DELIMITED "10.0.0.10257"

#if defined(_BETA)
    #define EBISU_VERSION_BETA 1
#else
    #define EBISU_VERSION_BETA 0
#endif

#define EBISU_UPDATE_SYSTEM_VERSION_PC 3
#define EBISU_UPDATE_SYSTEM_VERSION_PC_STR "3"

#endif

///////////////////////////////////////////////////////////////////////////////
// Plugin Versions
///////////////////////////////////////////////////////////////////////////////

#ifndef PLUGIN_VERSION_H
#define PLUGIN_VERSION_H

#define PLUGIN_VERSION      1,0,1,10257
#define PLUGIN_VERSION_STR "1,0,1,10257"
#define PLUGIN_VERSION_P_DELIMITED "1.0.1.10257"

#endif

///////////////////////////////////////////////////////////////////////////////
// Escalation Service
///////////////////////////////////////////////////////////////////////////////

#ifndef ESCALATIONSERVICE_VERSION_H
#define ESCALATIONSERVICE_VERSION_H

#define ESCALATIONSERVICE_VERSION_BUILD_TIME "12/11/2015 11:33:50 "

#define ESCALATIONSERVICE_COMPANY_NAME_STR "Electronic Arts"

#define ESCALATIONSERVICE_PRODUCT_NAME_STR "OriginClientService"

#define ESCALATIONSERVICE_APPLICATION_NAME_STR "OriginClientService.exe"

#if defined(EA_DEBUG) || defined(_DEBUG)
    #define ESCALATIONSERVICE_BUILD_STR "Debug"
#else
    #define ESCALATIONSERVICE_BUILD_STR "Release"
#endif

#define ESCALATIONSERVICE_REV_NUMBER 10257

#define ESCALATIONSERVICE_VERSION      10,0,0,10257
#define ESCALATIONSERVICE_VERSION_STR "10,0,0,10257"
#define ESCALATIONSERVICE_VERSION_P_DELIMITED "10.0.0.10257"

#define ESCALATIONSERVICE_PRODUCT_VERSION      10,0,0,10257
#define ESCALATIONSERVICE_PRODUCT_VERSION_STR "10,0,0,10257"

#endif

///////////////////////////////////////////////////////////////////////////////
// EA Core Server
///////////////////////////////////////////////////////////////////////////////

#ifndef EACORESERVER_VERSION_H
#define EACORESERVER_VERSION_H

#define EACORESERVER_VERSION_BUILD_TIME "12/11/2015 11:33:50 "

#define EACORESERVER_COMPANY_NAME_STR "Electronic Arts"

#define EACORESERVER_PRODUCT_NAME_STR "EA Core Server Application"

#define EACORESERVER_APPLICATION_NAME_STR "EACoreServer.exe"

#if defined(EA_DEBUG) || defined(_DEBUG)
    #define EACORESERVER_BUILD_STR "Debug"
#else
    #define EACORESERVER_BUILD_STR "Release"
#endif

#define EACORESERVER_REV_NUMBER 10257

#define EACORESERVER_VERSION      10,0,0,10257
#define EACORESERVER_VERSION_STR "10,0,0,10257"
#define EACORESERVER_VERSION_P_DELIMITED "10.0.0.10257"

#define EACORESERVER_PRODUCT_VERSION      10,0,0,10257
#define EACORESERVER_PRODUCT_VERSION_STR "10,0,0,10257"

#endif
