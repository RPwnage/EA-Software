///////////////////////////////////////////////////////////////////////////////
// EbisuMetrics.cpp
//
// Defines all Origin client-side metrics
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010-2013 Electronic Arts. All rights reserved.
//
// Telemetry definition reference at:
// http://online.ea.com/documentation/display/games/Origin+v8.6+Telemetry+Hooks
//
///////////////////////////////////////////////////////////////////////////////

#include "EbisuMetrics.h"
#include "EABase/eabase.h"

#pragma pack(push, 1)


///////////////////////////////////////////////////////////////////////////////
//  INSTALL
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  INSTALL:  EBISU
//=============================================================================

//-----------------------------------------------------------------------------
//  INSTALL:  EBISU:  Client installation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_INSTALL\
    TELMEVENTDEF_START(METRIC_APP_INSTALL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "EBSU", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "brws"    /* Default browser    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  INSTALL:  EBISU:  Client uninstallation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_UNINSTALL\
    TELMEVENTDEF_START(METRIC_APP_UNINSTALL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "EBSU", "UNIN")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  INSTALL:  EBISU:  Mac escalation service not authorized
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_MAC_ESCALATION_DENIED\
    TELMEVENTDEF_START(METRIC_APP_MAC_ESCALATION_DENIED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "ESCA", "DENY")\
    TELMEVENTDEF_END()

//=============================================================================
//  INSTALL:  INSTALL THRU EBISU
//=============================================================================

//-----------------------------------------------------------------------------
//  INSTALL:  INSTALL THRU EBISU:  Client is running
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ITE_CLIENT_RUNNING\
    TELMEVENTDEF_START(METRIC_ITE_CLIENT_RUNNING)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "ITEC", "RUNG")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rung"    /* Client is running  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  INSTALL:  INSTALL THRU EBISU:  Successful installation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ITE_CLIENT_SUCCESS\
    TELMEVENTDEF_START(METRIC_ITE_CLIENT_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "ITEC", "SUCC")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  INSTALL:  INSTALL THRU EBISU:  Failed installation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ITE_CLIENT_FAILED\
    TELMEVENTDEF_START(METRIC_ITE_CLIENT_FAILED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "ITEC", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "reas"    /* Reason string      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  INSTALL:  INSTALL THRU EBISU:  Cancelled installation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ITE_CLIENT_CANCELLED\
    TELMEVENTDEF_START(METRIC_ITE_CLIENT_CANCELLED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "ITEC", "CANC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "reas"    /* Reason string      */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  APPLICATION
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  APPLICATION:  SESSION
//=============================================================================

//-----------------------------------------------------------------------------
//  APPLICATION:  SESSION:  Logging
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_SESSION_ID\
    TELMEVENTDEF_START(METRIC_APP_SESSION_ID)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "LGNG")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "sess"    /* session ID for logging         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  SESSION:  Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_START\
    TELMEVENTDEF_START(METRIC_APP_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "STRT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  SESSION:  Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_SETTINGS\
    TELMEVENTDEF_START(METRIC_APP_SETTINGS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "SETT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "admn"    /* OS admin:  1-true, 0-false     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "asrt"    /* auto-start:  1-true, 0-false   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "beta"    /* beta version:  1-true, 0-false */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "bver"    /* Bootstrap version              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  SESSION:  Command Line
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_CMD\
    TELMEVENTDEF_START(METRIC_APP_CMD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "CMDL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "urlx"    /* Command Line Paramters     */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_APP_CONFIG\
    TELMEVENTDEF_START(METRIC_APP_CONFIG)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "CNFG")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ckey"    /* Config item */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "cval"    /* Config value */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ovrd"    /* Is override */ )\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  SESSION:  End
//
//  Note: if this event ever becomes critical, verify that the event is 
//  triggered at the proper location(s) in code. Primarily:
//    1) Exiting when logged in
//    2) Exiting when logged out
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_END\
    TELMEVENTDEF_START(METRIC_APP_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BOOT", "SESS", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* Start time         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "endt"    /* App session length */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Time (seconds)     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  CONNECTION:  Connection Lost
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_CONNECTION_LOST\
    TELMEVENTDEF_START(METRIC_APP_CONNECTION_LOST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "CONN", "LOST")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  CONNECTION:  Premature termination
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_PREMATURE_TERMINATION\
    TELMEVENTDEF_START(METRIC_APP_PREMATURE_TERMINATION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "PREM", "TERM")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  POWER: Power Change
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_POWER_MODE\
    TELMEVENTDEF_START(METRIC_APP_POWER_MODE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "POWR", "MODE")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Time stamp of change */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mode"    /* Power mode           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  MOTD: Message of the Day shown
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_MOTD_SHOW\
    TELMEVENTDEF_START(METRIC_APP_MOTD_SHOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "MOTD", "SHOW")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "urlx"    /* MOTD URL         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "text"    /* Text of the MOTD */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  MOTD: Message of the Day closed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_MOTD_CLOSE\
    TELMEVENTDEF_START(METRIC_APP_MOTD_CLOSE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "MOTD", "CLOS")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  DYNAMIC URLS: Dynamic URL config file loaded
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_DYNAMIC_URL_LOAD\
    TELMEVENTDEF_START(METRIC_APP_DYNAMIC_URL_LOAD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "DURL", "LOAD")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "file"    /* Config file Perforce path (RCS)  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "frev"    /* Config file revision (RCS)       */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ovrd"    /* Is override                      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  ADDITIONAL CLIENT LOGS: Send when we write to Client_Log#.txt
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_ADDITIONAL_CLIENT_LOG\
    TELMEVENTDEF_START(METRIC_APP_ADDITIONAL_CLIENT_LOG)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("APPC", "CLNT", "LOGF")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "file"    /* Client filename being written to */)\
    TELMEVENTDEF_END()

//=============================================================================
//  APPLICATION:  ESCALATION
//=============================================================================

//-----------------------------------------------------------------------------
//  APPLICATION:  ESCALATION:   Escalation Client Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_ESCALATION_ERROR\
    TELMEVENTDEF_START(METRIC_APP_ESCALATION_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ESCA", "CLIN", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ecmd"    /* Escalation cmd */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "etyp"    /* Error type     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"    /* System error   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"    /* Reason string  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "edsc"    /* Error descrip  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  APPLICATION:  ESCALATION:   Escalation Client Success
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_APP_ESCALATION_SUCCESS\
    TELMEVENTDEF_START(METRIC_APP_ESCALATION_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ESCA", "CLIN", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ecmd"    /* Escalation cmd */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"    /* Reason string  */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  USER
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  USER:  LOGIN
//=============================================================================

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_START\
    TELMEVENTDEF_START(METRIC_LOGIN_START)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "type"    /* Type of login            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "lhst"    /* From Local Host            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "orgn"    /* If from Local Host -- the Origin domain*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Success
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_OK\
    TELMEVENTDEF_START(METRIC_LOGIN_OK)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mode"    /* Mode: "online"/"offline" */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "uage"    /* Underage user            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "type"    /* Type of login            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "lhst"    /* From Local Host            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "orgn"    /* If from Local Host -- the Origin domain*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_FAIL\
    TELMEVENTDEF_START(METRIC_LOGIN_FAIL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "svcd"    /* Server error code                            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "desc"    /* Server error description                     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "code"    /* Error string                                 */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "type"    /* Type of login                                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "http"    /* Client version                               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "urlx"    /* URL                                          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "qerr"    /* Qt error code                                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "styp"    /* subtype                                      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* Does the client autostart                    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "lhst"    /* From Local Host                              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "orgn"    /* If from Local Host -- the Origin domain      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "qstr"    /* Qt error string                              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  REMID load
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_COOKIE_LOAD\
    TELMEVENTDEF_START(METRIC_LOGIN_COOKIE_LOAD)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "CKLD")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "name"    /* name of the cookie loaded  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "valu"    /* value of the cookie loaded */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "expi"    /* expiration string         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  REMID save
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_COOKIE_SAVE\
    TELMEVENTDEF_START(METRIC_LOGIN_COOKIE_SAVE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "CKSV")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "name"    /* name of the cookie saved */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "valu"    /* value of the cookie saved */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "expi"    /* expiration string         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "code"    /* error code value          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cntx"   /* # of remid cookies in jar */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGOUT\
    TELMEVENTDEF_START(METRIC_LOGOUT)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* App start time     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "endt"    /* App end time       */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* App session length */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "reas"    /* Reason for logout  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Privacy Settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_PRIVSTRT\
    TELMEVENTDEF_START(METRIC_LOGIN_PRIVSTRT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "PRIV", "STRT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Privacy Settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_PRIVCANC\
    TELMEVENTDEF_START(METRIC_LOGIN_PRIVCANC)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "PRIV", "CANC")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Privacy Settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_PRIVSUCC\
    TELMEVENTDEF_START(METRIC_LOGIN_PRIVSUCC)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "PRIV", "SUCC")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  Unknown
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_UNKNOWN\
    TELMEVENTDEF_START(METRIC_LOGIN_UNKNOWN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "LGIN", "UNKN")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "type"    /* Type               */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  OEM LOGIN SUCCESS
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_OEM\
    TELMEVENTDEF_START(METRIC_LOGIN_OEM)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "LGIN", "OEMX")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "oemx"    /* Value of oemx            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  CLEAR WEB APPLICATION CACHE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEB_APPLICATION_CACHE_CLEAR\
    TELMEVENTDEF_START(METRIC_WEB_APPLICATION_CACHE_CLEAR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "LGIN", "CLRX")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  LOGIN:  DOWNLOAD WEB APPLICATION CACHE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEB_APPLICATION_CACHE_DOWNLOAD\
    TELMEVENTDEF_START(METRIC_WEB_APPLICATION_CACHE_DOWNLOAD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "LGIN", "DWNL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
// USER: LOGIN: Number of favorite games
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_FAVORITES\
    TELMEVENTDEF_START(METRIC_LOGIN_FAVORITES)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "LGIN", "FAVR")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "numb"    /* number of favorites    */)\
    TELMEVENTDEF_END()
//-----------------------------------------------------------------------------
// USER: LOGIN: Number of hidden games
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_HIDDEN\
    TELMEVENTDEF_START(METRIC_LOGIN_HIDDEN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "LGIN", "HIDD")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "numb"    /* number of hidden games   */)\
    TELMEVENTDEF_END()

//=============================================================================
//  USER:  NUX FLOW
//=============================================================================

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_START\
    TELMEVENTDEF_START(METRIC_NUX_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "step"    /* Step: What page the user is on */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* Does the client autostart*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "free"   /* free game installation flow: 1-true,0-false */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  Cancel
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_CANCEL\
    TELMEVENTDEF_START(METRIC_NUX_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "CANC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "step"    /* Step: What page the user is on */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_END\
    TELMEVENTDEF_START(METRIC_NUX_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "step"    /* Step: What page the user is on */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  Profile
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_PROFILE\
    TELMEVENTDEF_START(METRIC_NUX_PROFILE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "PROF")\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "firs"    /* First Name                    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "last"    /* Last Name                     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "wind"    /* Avatar Window                 */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "gall"    /* Avatar Gallery                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "upld"    /* Avatar Upload                 */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_EMAILDUP\
    TELMEVENTDEF_START(METRIC_NUX_EMAILDUP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "EMAL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  OEM REGISTRATION SUCCESS
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_OEM\
    TELMEVENTDEF_START(METRIC_NUX_OEM)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("USER", "REGI", "OEMX")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "oemx"    /* Value of oemx              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  NUX FLOW:  Error (to be added in 9.2)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NUX_ERROR\
    TELMEVENTDEF_START(METRIC_NUX_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "NUXF", "ERRO")\
    TELMEVENTDEF_END()


//=============================================================================
//  USER:  PASSWORD
//=============================================================================

//-----------------------------------------------------------------------------
//  USER:  PASSWORD:  Sent password reset request
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PASSWORD_SENT\
    TELMEVENTDEF_START(METRIC_PASSWORD_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "PASS", "SENT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  PASSWORD:  Reset password
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PASSWORD_RESET\
    TELMEVENTDEF_START(METRIC_PASSWORD_RESET)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "PASS", "RSET")\
    TELMEVENTDEF_END()

//=============================================================================
//  USER:  CAPTCHA
//=============================================================================

//-----------------------------------------------------------------------------
//  USER:  CAPTCHA:  Captcha flow started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CAPTCHA_START\
    TELMEVENTDEF_START(METRIC_CAPTCHA_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "CAPT", "STRT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  USER:  CAPTCHA:  Captcha attempt failed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CAPTCHA_FAIL\
    TELMEVENTDEF_START(METRIC_CAPTCHA_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "CAPT", "FAIL")\
    TELMEVENTDEF_END()

//=============================================================================
//  USER:  AUTHENTICATION
//=============================================================================

//-----------------------------------------------------------------------------
//  USER:  AUTHENTICATION:  Authentication lost (403 returned)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_AUTHENTICATION_LOST\
    TELMEVENTDEF_START(METRIC_AUTHENTICATION_LOST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "AUTH", "LOST")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "qnet"    /* QNetwork error   */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* Http status      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "link"    /* URL              */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  SETTINGS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SETTINGS:  General settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SETTINGS\
    TELMEVENTDEF_START(METRIC_SETTINGS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "USER", "INFO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lang"    /* Client language     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "alog"    /* Auto login          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "apat"    /* Auto patch          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "asrt"    /* Auto start          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "aclu"    /* Auto client update  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "beta"    /* Beta opt-in         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "clod"    /* Cloud Saves enabled */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "hwoi"    /* Hardware opt-in     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtab"    /* Default Tab         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "igoe"    /* IGO enable settings */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "oslo"    /* OS locale           */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_QUIETMODE_ENABLED\
    TELMEVENTDEF_START(METRIC_QUIETMODE_ENABLED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "USER", "QTMD")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dpro"    /* Disable promos      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "inmu"    /* Ignore non mandatory updates */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sogf"    /* Shutdown on game finish */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  Privacy settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PRIVACY_SETTINGS\
    TELMEVENTDEF_START(METRIC_PRIVACY_SETTINGS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SETT", "USER", "PRIV")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tmoo"    /* Telemetry opt out   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uage"    /* Underage user       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "send"    /* true if sending non critical telemetry */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  Email verification: Starts in the client
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_EMAIL_VERIFICATION_STARTS_CLIENT\
    TELMEVENTDEF_START(METRIC_EMAIL_VERIFICATION_STARTS_CLIENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "EVER", "CLIE")\
    TELMEVENTDEF_END()

    //-----------------------------------------------------------------------------
    //  SETTINGS:  Email verification: Dismissed
    //-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_EMAIL_VERIFICATION_DISMISSED\
    TELMEVENTDEF_START(METRIC_EMAIL_VERIFICATION_DISMISSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "EVER", "DISS")\
    TELMEVENTDEF_END()

    //-----------------------------------------------------------------------------
    //  SETTINGS:  Email verification: Starts in the banner
    //-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_EMAIL_VERIFICATION_STARTS_BANNER\
    TELMEVENTDEF_START(METRIC_EMAIL_VERIFICATION_STARTS_BANNER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "EVER", "BANR")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  Invalid character(non-ASCII, or symbol) in settings download/cache path
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_INVALID_SETTINGS_PATH_CHAR\
    TELMEVENTDEF_START(METRIC_INVALID_SETTINGS_PATH_CHAR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "PATH", "INVA")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "path"    /* Invalid path        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  Setting file failed to sync
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SETTINGS_SYNC_FAILED\
    TELMEVENTDEF_START(METRIC_SETTINGS_SYNC_FAILED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SETT", "SYNC", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "file"    /* Settings file       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "emsg"    /* Reason for failure  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "read"    /* Readable            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "writ"    /* Writable            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "rsys"    /* Read system error   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "wsys"    /* Write system error  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "fwin"    /* Windows file attrs  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "perm"    /* Qt perm flag        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  Setting file fix result
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SETTINGS_FILE_FIX_RESULT\
    TELMEVENTDEF_START(METRIC_SETTINGS_FILE_FIX_RESULT)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SETT", "FILE", "FIXX")\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "succ"    /* File "fixed"        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "read"    /* Read access         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "writ"    /* Write access        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "qfix"    /* Qt "fix" success    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "file"    /* Settings file path  */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  HARDWARE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SETTINGS:  OS
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_OS\
    TELMEVENTDEF_START(METRIC_HW_OS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCSW", "OSYS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "arch"    /* OS architecture (32/64) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "bild"    /* OS build number         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "osys"    /* OS name                 */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ossp"    /* Service pack            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "hypr"    /* Hypervisor              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  CPU
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_CPU\
    TELMEVENTDEF_START(METRIC_HW_CPU)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "CPUX")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "name"    /* CPU name                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cnum"    /* Physical CPU count      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "core"    /* Core count/Physical CPU */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ghzx"    /* CPU speed GHz           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "iset"    /* Instruction set         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "bios"    /* BIOS string             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mbrd"    /* Motherboard string      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "msrl"    /* Motherboard serial      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "bsrl"    /* BIOS serial             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "spro"    /* Surface Pro (temp)      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ooax"    /* OOA Machine hash        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  VIDEO
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_VIDEO\
    TELMEVENTDEF_START(METRIC_HW_VIDEO)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "VIDO")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Total number of cards  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cnum"    /* Video card index       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "card"    /* Video card name        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "bitd"    /* Bit depth              */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "vram"    /* VRAM memory            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "vnid"    /* Video card:  vendor id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dvid"    /* Video card:  device id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "drvv"    /* Video card:  driver version   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  VIDEO CAPTURE (WEBCAM)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_WEBCAM\
    TELMEVENTDEF_START(METRIC_HW_WEBCAM)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "WCAM")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Total number of devices  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "wnum"    /* Capture device index     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "webc"    /* Capture device name      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  PHYSICAL DISPLAY
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_DISPLAY\
    TELMEVENTDEF_START(METRIC_HW_DISPLAY)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "DISP")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Total number of cards  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dnum"    /* Display index          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "wres"    /* X resolution           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "hres"    /* Y resolution           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  VIRTUAL DISPLAY
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_RESOLUTION\
    TELMEVENTDEF_START(METRIC_HW_RESOLUTION)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "ERES")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "wres"    /* X resolution   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "hres"    /* Y resolution   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  MEMORY
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_MEM\
    TELMEVENTDEF_START(METRIC_HW_MEM)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "RAMX")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "size"    /* Amount of RAM (MB) */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  HARD DRIVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_HDD\
    TELMEVENTDEF_START(METRIC_HW_HDD)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "STOR")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Total number of drives            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "driv"    /* Drive number                      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "size"    /* Size (GB)                         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "infc"    /* Interface: IDE                    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "type"    /* Media type: Fixed hard disk media */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"    /* Model name: ST3250820AS           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "hsrl"    /* Serial Number                     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ooax"    /* OOA Machine hash                  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  OPTICAL DRIVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_ODD\
    TELMEVENTDEF_START(METRIC_HW_ODD)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "ODDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Total number of drives                      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "driv"    /* Drive number                                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "type"    /* Type:  "DVD-ROM"                            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"    /* Name:  HL-DT-ST DVD-ROM GDR8164B ATA Device */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SETTINGS:  MICROPHONE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HW_MICROPHONE\
    TELMEVENTDEF_START(METRIC_HW_MICROPHONE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HDWR", "PCHW", "MIKE")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mike"    /* Microphone         */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  GAME INSTALL
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME INSTALL:  Start Install 
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INSTALL_START\
    TELMEVENTDEF_START(METRIC_GAME_INSTALL_START)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "GAME", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "dtyp"    /* Download type  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME INSTALL:  End Install
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INSTALL_SUCCESS\
    TELMEVENTDEF_START(METRIC_GAME_INSTALL_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "GAME", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID                          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type                       */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME INSTALL:  Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INSTALL_ERROR\
    TELMEVENTDEF_START(METRIC_GAME_INSTALL_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "GAME", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID                          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type                       */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "type"    /* Return error type                   */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Return code from install process    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME INSTALL:  Uninstall clicked
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_UNINSTALL_CLICKED\
    TELMEVENTDEF_START(METRIC_GAME_UNINSTALL_CLICKED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("UNIN", "GAME", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME INSTALL:  Uninstall cancelled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_UNINSTALL_CANCEL\
    TELMEVENTDEF_START(METRIC_GAME_UNINSTALL_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("UNIN", "GAME", "CANC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME INSTALL:  Uninstall started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_UNINSTALL_START\
    TELMEVENTDEF_START(METRIC_GAME_UNINSTALL_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("UNIN", "GAME", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()
    
//-----------------------------------------------------------------------------
//  GAME INSTALL:  Zero length update
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_ZERO_LENGTH_UPDATE\
    TELMEVENTDEF_START(METRIC_GAME_ZERO_LENGTH_UPDATE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("INST", "GAME", "NOUP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()
    
///////////////////////////////////////////////////////////////////////////////
//  GAME LAUNCH
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME LAUNCH:  Launch Cancelled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_LAUNCH_CANCELLED\
    TELMEVENTDEF_START(METRIC_GAME_LAUNCH_CANCELLED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GSMR", "LNCH", "CNCL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "reas"    /* Cancelled reason   */)\
    TELMEVENTDEF_END()
    

///////////////////////////////////////////////////////////////////////////////
//  GAME SESSION
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME SESSION:  Start Session
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_SESSION_START\
    TELMEVENTDEF_START(METRIC_GAME_SESSION_START)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("GSMR", "SESS", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U64, "gsid"    /* Game session id    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gsrc"     /* Source Content Id  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nogs"    /* Non-Origin game    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ltyp"    /* Launch Type        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lsrc"    /* Launch source      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "addx"    /* Additional content */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "gmvs"    /* Game version       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uage"    /* Underage user      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "lapy"    /* Last played        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "sbac"    /* Subscription activation */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "sdsi"    /* Start date of subscription instance */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sben"    /* Is subscription entitlement? */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME SESSION:  Unmonitored Session
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_SESSION_UNMONITORED\
    TELMEVENTDEF_START(METRIC_GAME_SESSION_UNMONITORED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("GSMR", "SESS", "UNMO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ltyp"    /* Launch Type        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lsrc"    /* Launch source      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "addx"    /* Additional content */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "gmvs"    /* Game version       */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GAME SESSION:  End Session
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_SESSION_END\
    TELMEVENTDEF_START(METRIC_GAME_SESSION_END)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("GSMR", "SESS", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U64, "gsid"    /* Game session id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Game session length */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nogs"    /* Non-Origin game     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error code          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uage"    /* Underage user       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "lapy"    /* Last played        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "sbac"    /* Subscription activation */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "sdsi"    /* Start date of subscription instance */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sben"    /* Is subscription entitlement? */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  ALT LAUNCHER:  Session Fail
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ALTLAUNCHER_SESSION_FAIL\
    TELMEVENTDEF_START(METRIC_ALTLAUNCHER_SESSION_FAIL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("ALTL", "SESS", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ltyp"    /* Launch Type        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "reas"    /* Reason             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "valu"    /* Launch Result Value*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ALT LAUNCHER: Game Session Fail
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_SESSION_ALTLAUNCHER_FAIL\
    TELMEVENTDEF_START(METRIC_GAME_SESSION_ALTLAUNCHER_FAIL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("GSMR", "ALTL", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "gsrc"    /* Source Content ID  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Target Content ID  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "state"   /* State              */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  DOWNLOAD
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_START\
    TELMEVENTDEF_START(METRIC_DL_START)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "strt"    /* Download start time   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "stst"    /* Download start status */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctty"    /* Content type          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "link"    /* Link URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptch"    /* Auto patch            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tpth"    /* Target path           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "plod"    /* Preload               */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Critical Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_ERROR\
    TELMEVENTDEF_START(METRIC_DL_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctty"    /* Content type          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "link"    /* Link URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ectx"    /* Error context         */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ejob"    /* Job error code        */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"    /* System error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* Error at source file  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "line"    /* Error at source line  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptch"    /* Auto patch            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "syml"    /* Dest symlink detected */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "plod"    /* Preload               */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Critical Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_ERRORBOX\
    TELMEVENTDEF_START(METRIC_DL_ERRORBOX)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "GUIX", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error code            */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "type"    /* Error type            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Immediate Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_IMMEDIATE_ERROR\
    TELMEVENTDEF_START(METRIC_DL_IMMEDIATE_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "IMMD", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "err1"    /* Error string 1        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "err2"    /* Error string 2        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* Error at source file  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "line"    /* Error at source line  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Data Error (Missing, bad CRC, etc)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DATA_ERROR\
    TELMEVENTDEF_START(METRIC_DL_DATA_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DATA", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ecat"    /* Error category        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "edet"    /* Error details         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* Error at source file  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "line"    /* Error at source line  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Populate Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_POPULATE_ERROR\
    TELMEVENTDEF_START(METRIC_DL_POPULATE_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "POPL", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ejob"    /* Job error code        */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"    /* System error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* Error at source file  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "line"    /* Error at source line  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Preallocate Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_ERROR_PREALLOCATE\
    TELMEVENTDEF_START(METRIC_DL_ERROR_PREALLOCATE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "ERR3")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "etyp"    /* Error type: file/dir  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "path"    /* Path of file/dir      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "code"    /* System error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "size"    /* Uncompressed file size*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Error - Vendor IP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_ERROR_VENDOR_IP\
    TELMEVENTDEF_START(METRIC_DL_ERROR_VENDOR_IP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "EVIP")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "link"    /* Link URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "cipr"    /* CDN Vendor IP addr    */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ejob"    /* Job error code        */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"    /* System error code     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Redownload File
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_REDOWNLOAD\
    TELMEVENTDEF_START(METRIC_DL_REDOWNLOAD)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "REDL")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* File                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "erdl"    /* Redownload error code */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"    /* System error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ext1"    /* Extra data 1          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ext2"    /* Extra data 2          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: CRC Failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_CRC_FAILURE\
    TELMEVENTDEF_START(METRIC_DL_CRC_FAILURE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "CRCF")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "file"    /* File                  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "utyp"    /* Unpack type           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "crcf"    /* CRC in the file       */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "crcc"    /* CRC we calculated     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "fdat"    /* File date             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ftim"    /* File time             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Canceled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_CANCEL\
    TELMEVENTDEF_START(METRIC_DL_CANCEL)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "CANC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "strt"    /* Download start time   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "stst"    /* Download start status */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "endt"    /* Download end time     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "enst"    /* Download end status   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "byte"    /* Bytes downloaded      */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "bitr"    /* Bitrate (Kbps)        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctty"    /* Content type          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "link"    /* Link URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptch"    /* Auto patch            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "plod"    /* Preload               */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Success
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_SUCCESS\
    TELMEVENTDEF_START(METRIC_DL_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "strt"    /* Download start time   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "stst"    /* Download start status */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "endt"    /* Download end time     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "enst"    /* Download end status   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "byte"    /* Bytes downloaded      */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "bitr"    /* Bitrate (Kbps)        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctty"    /* Content type          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "link"    /* Link URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptch"    /* Auto patch            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "plod"    /* Preload               */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Session Paused
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_PAUSE\
    TELMEVENTDEF_START(METRIC_DL_PAUSE)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "PAUS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "prsn"    /* Reason for pausing    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctty"    /* Content type          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "dtyp"    /* Download type         */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "byte"    /* Bytes downloaded      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptch"    /* Auto patch            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ndip"    /* Non-DiP to DiP Upgrade*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Content Length
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_CONTENT_LENGTH\
    TELMEVENTDEF_START(METRIC_DL_CONTENT_LENGTH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "CNTL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "link"    /* Reason for pausing        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "rtyp"    /* Request type              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "rslt"    /* Result - success/failure  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "lenb"    /* Start/End Byte            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "totl"    /* Byte Length               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cipr"    /* CDN Vendor IP address     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3.0 Download Prepare
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DIP3_PREPARE\
    TELMEVENTDEF_START(METRIC_DL_DIP3_PREPARE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DIP3", "PREP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"numf"    /* Number of files to patch  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3.0 Patching Summary
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DIP3_SUMMARY\
    TELMEVENTDEF_START(METRIC_DL_DIP3_SUMMARY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DIP3", "SUMM")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"totf"    /* Total changed files       */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"numf"    /* Num of files diff patch   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"numr"    /* Num of files can't patch  */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"adsz"    /* Update added data (can't patch) */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"orsz"    /* DiP 2 Total Update size         */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"dusz"    /* DiP 3 Total Update size         */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"ndsz"    /* DiP 3 Update Non-Patched size   */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"disz"    /* DiP 3 Update Patched size       */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"dsav"    /* DiP 3 Update patched savings    */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"bitr"    /* Diff Calculation Bitrate (bps) */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"ndip"    /* Non-DiP to DiP Upgrade    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3.0 Download Processing Canceled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DIP3_CANCELED\
    TELMEVENTDEF_START(METRIC_DL_DIP3_CANCELED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DIP3", "CNCL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"reat"    /* Reason patching canceled type */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"reac"    /* Reason patching canceled code */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"ndip"    /* Non-DiP to DiP Upgrade    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3.0 Download Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DIP3_ERROR\
    TELMEVENTDEF_START(METRIC_DL_DIP3_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DIP3", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"errt"    /* Error type                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"errc"    /* Error code                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ectx"    /* Error context             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"ndip"    /* Non-DiP to DiP Upgrade    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3.0 Patching Success
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_DIP3_SUCCESS\
    TELMEVENTDEF_START(METRIC_DL_DIP3_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "DIP3", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"ndip"    /* Non-DiP to DiP Upgrade    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DOWNLOAD: Track time from start of download to ready to play state
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_ELAPSEDTIME_TO_READYTOPLAY\
    TELMEVENTDEF_START(METRIC_DL_ELAPSEDTIME_TO_READYTOPLAY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "TIMR", "RDTP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"time"    /* Time it took to reach ready to play state */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iito"    /* Is ito                                    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "lsrc"    /* Is local source                           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "updt"    /* Is update                                 */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "repr"    /* Is repair                                 */)\
    TELMEVENTDEF_END()

//  DOWNLOAD: Optimization Data
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_OPTI_DATA\
    TELMEVENTDEF_START(METRIC_DL_OPTI_DATA)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "OPTD")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "fdrt"    /* final download rate (MB/sec – float) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "airt"    /* average IP download rate (MB/sec – float) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "wkst"    /* worker saturation (float) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "chst"    /* channel saturation (float) */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "avcs"    /* average chunk size (int) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "host"    /* URL                   */)\
    TELMEVENTDEF_ATTR(TYPEID_I16 , "ipus"    /* IPs used (int) */)\
    TELMEVENTDEF_ATTR(TYPEID_I16 , "iper"    /* IPs errors (int) */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "nrqc"    /* number of requests in clusters (int) */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "nsfc"    /* number of single file chunks(int) */)\
    TELMEVENTDEF_END()


//  DOWNLOAD: Optimization Data
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_PROXY_DETECTED\
    TELMEVENTDEF_START(METRIC_DL_PROXY_DETECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "PRXY")\
    TELMEVENTDEF_END()

//  DOWNLOAD: Session Connection stats
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DL_CONNECTION_STATS\
    TELMEVENTDEF_START(METRIC_DL_CONNECTION_STATS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DNDL", "SESS", "STAT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "envt"    /* Environment           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uuid"    /* UUID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "dest"    /* Dest IP               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "link"    /* URL                   */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "byte"   /* Bytes downloaded      */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "time"   /* Time elapsed in ms    */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "used"   /* Chunks downloaded     */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "errc"   /* Error  count          */)\
    TELMEVENTDEF_END()

//  DYNAMIC DOWNLOAD: Game launched
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DYNAMICDOWNLOAD_GAMELAUNCH\
    TELMEVENTDEF_START(METRIC_DYNAMICDOWNLOAD_GAMELAUNCH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DYDL", "GAME", "LAUN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uuid"    /* Flow UUID             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "lsrc"    /* Launch source (hovercard, etc)*/)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "byte"   /* Bytes downloaded      */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "breq"   /* Bytes required        */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "btot"   /* Bytes total           */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  ERROR REPORTING
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Crash
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ERROR_CRASH\
    TELMEVENTDEF_START(METRIC_ERROR_CRASH)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("ERRO", "CRIT", "CRAS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "code"    /* Crash category id      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pref"    /* User report preference */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "shut"    /* Crash on Shutdown      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Crash
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ERROR_DELETE_FILE\
    TELMEVENTDEF_START(METRIC_ERROR_DELETE_FILE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ERRO", "DELT", "FILE")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctxt"    /* Context of delete */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "file"    /* File to delete    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "code"    /* Qt Error code      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Reason (non-critical)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ERROR_NOTIFY\
    TELMEVENTDEF_START(METRIC_ERROR_NOTIFY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ERRO", "CRIT", "NOTF")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "text"    /* Reason string  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "code"    /* Code string    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Rest (non-critical)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ERROR_REST\
    TELMEVENTDEF_START(METRIC_ERROR_REST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ERRO", "CRIT", "REST")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "rest"    /* Rest error       */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "qnet"    /* Qt Reply Error   */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* Http status      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "link"    /* URL              */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "qstr"    /* Qt error string  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  BugSentry
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BUG_REPORT\
    TELMEVENTDEF_START(METRIC_BUG_REPORT)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("BSID", "STR0", "STR1")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "sess"    /* BugSentry session ID */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "code"    /* Crash category id    */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  SOCIAL
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  SOCIAL:  CHAT
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  CHAT:  Session started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHAT_SESSION_START\
    TELMEVENTDEF_START(METRIC_CHAT_SESSION_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHAT", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "sndr"     /* nid of the chat initiator */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHAT:  Session finished
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHAT_SESSION_END\
    TELMEVENTDEF_START(METRIC_CHAT_SESSION_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHAT", "ENDT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  FRIENDS
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  View friend list source
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_VIEWSOURCE\
    TELMEVENTDEF_START(METRIC_FRIEND_VIEWSOURCE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "VSRC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Source of click */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Count
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_COUNT\
    TELMEVENTDEF_START(METRIC_FRIEND_COUNT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "CONT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cont"    /* Friend count   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Import
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_IMPORT\
    TELMEVENTDEF_START(METRIC_FRIEND_IMPORT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "IMPT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation accepted
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_INVITE_ACCEPTED\
    TELMEVENTDEF_START(METRIC_FRIEND_INVITE_ACCEPTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "INRX")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation ignored
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_INVITE_IGNORED\
    TELMEVENTDEF_START(METRIC_FRIEND_INVITE_IGNORED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "INIG")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_INVITE_SENT\
    TELMEVENTDEF_START(METRIC_FRIEND_INVITE_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "INTX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "isrc"    /* Invite source  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Block add request sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_BLOCK_ADD_SENT\
    TELMEVENTDEF_START(METRIC_FRIEND_BLOCK_ADD_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "BKAD")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Block remove request sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_BLOCK_REMOVE_SENT\
    TELMEVENTDEF_START(METRIC_FRIEND_BLOCK_REMOVE_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "BKRM")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Report request sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_REPORT_SENT\
    TELMEVENTDEF_START(METRIC_FRIEND_REPORT_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "RPTX")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Remove request sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_REMOVAL_SENT\
    TELMEVENTDEF_START(METRIC_FRIEND_REMOVAL_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "RMTX")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Roster returned missing names
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FRIEND_USERNAME_MISSING\
    TELMEVENTDEF_START(METRIC_FRIEND_USERNAME_MISSING)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "FRND", "MISS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cont"    /* Friend count   */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  GAME
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation accepted
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INVITE_ACCEPTED\
    TELMEVENTDEF_START(METRIC_GAME_INVITE_ACCEPTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GAME", "INRX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* OfferId of entitlement */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation decline sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INVITE_DECLINE_SENT\
    TELMEVENTDEF_START(METRIC_GAME_INVITE_DECLINE_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GAME", "INDT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation decline received
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INVITE_DECLINE_RECEIVED\
    TELMEVENTDEF_START(METRIC_GAME_INVITE_DECLINE_RECEIVED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GAME", "INDR")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation sent
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAME_INVITE_SENT\
    TELMEVENTDEF_START(METRIC_GAME_INVITE_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GAME", "INTX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* OfferId of entitlement */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  LOGIN
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  LOGIN:  Invisible
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOGIN_INVISIBLE\
    TELMEVENTDEF_START(METRIC_LOGIN_INVISIBLE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "LGIN", "INVS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "invs"    /* Invisible      */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  USER PROFILE
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  USER PROFILE:  View profile
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_USER_PROFILE_VIEW\
    TELMEVENTDEF_START(METRIC_USER_PROFILE_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "PROF", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "scpx"    /* Scope of click */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Source of click */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "view"    /* Destination of click */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  USER PROFILE:  View profile source from inside games
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_USER_PROFILE_VIEWGAMESOURCE\
    TELMEVENTDEF_START(METRIC_USER_PROFILE_VIEWGAMESOURCE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "PROF", "GSRC")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "srcx"    /* Source of click */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  PRESENCE
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  PRESENCE:  Set
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_USER_PRESENCE_SET\
    TELMEVENTDEF_START(METRIC_USER_PRESENCE_SET)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "PSNC", "SETX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "psnc"    /* Presence       */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  GROUP CHAT
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Create MUC Room
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_CREATE_BORN_MUC_ROOM\
    TELMEVENTDEF_START(METRIC_GC_CREATE_BORN_MUC_ROOM)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "CRMR")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Receive MUC Invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_RECEIVE_MUC_INVITE\
    TELMEVENTDEF_START(METRIC_GC_RECEIVE_MUC_INVITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "RCMI")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Decline MUC Invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_DECLINING_MUC_INVITE\
    TELMEVENTDEF_START(METRIC_GC_DECLINING_MUC_INVITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "DCMI")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "reas"    /* Reason         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Accepting MUC Invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_ACCEPTING_MUC_INVITE\
    TELMEVENTDEF_START(METRIC_GC_ACCEPTING_MUC_INVITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "ACMI")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Accepting Close Room Invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_ACCEPTING_CR_INVITE\
    TELMEVENTDEF_START(METRIC_GC_ACCEPTING_CR_INVITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "ACRI")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Initiating MUC Upgrade
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_MUC_UPGRADE_INITIATING\
    TELMEVENTDEF_START(METRIC_GC_MUC_UPGRADE_INITIATING)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "IMUP")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ltem"    /* Last Thread Empty flag  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Auto-accepting MUC Upgrade
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_MUC_UPGRADE_AUTO_ACCEPT\
    TELMEVENTDEF_START(METRIC_GC_MUC_UPGRADE_AUTO_ACCEPT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "AAMU")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Exited MUC room
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GC_MUC_EXIT\
    TELMEVENTDEF_START(METRIC_GC_MUC_EXIT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "GRCT", "EXIT")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "dura"    /* Duration in seconds */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "msgs"    /* Messages sent */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "part"    /* Maximum participants */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  9.5 CHATROOM TELEMETRY
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Enter room failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_ENTER_ROOM_FAIL\
    TELMEVENTDEF_START(METRIC_CHATROOM_ENTER_ROOM_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","RERR")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "node"    /* Xmpp Node */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "room"    /* Room Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error Code */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Create New Group
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_CREATE_GROUP\
    TELMEVENTDEF_START(METRIC_CHATROOM_CREATE_GROUP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","NWGP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Leave Group
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_LEAVE_GROUP\
    TELMEVENTDEF_START(METRIC_CHATROOM_LEAVE_GROUP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","LVGP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Delete Group
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_DELETE_GROUP\
    TELMEVENTDEF_START(METRIC_CHATROOM_DELETE_GROUP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","DLGP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Delete Group Failed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_DELETE_GROUP_FAILED\
    TELMEVENTDEF_START(METRIC_CHATROOM_DELETE_GROUP_FAILED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "DLGF")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Room Deactivated
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_DEACTIVATED\
    TELMEVENTDEF_START(METRIC_CHATROOM_DEACTIVATED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "DACT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "rmid"    /* Room Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "daty"   /* Deactivation Type */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "byid"    /* Origin Id of user who caused the deactivation */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Group was deleted
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_GROUP_WAS_DELETED\
    TELMEVENTDEF_START(METRIC_CHATROOM_GROUP_WAS_DELETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","GPWD")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Kicked from Group
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_KICKED_FROM_GROUP\
    TELMEVENTDEF_START(METRIC_CHATROOM_KICKED_FROM_GROUP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","KICK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Group Count at login
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_GROUP_COUNT\
    TELMEVENTDEF_START(METRIC_CHATROOM_GROUP_COUNT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","CONT")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"    /* Group Count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Chat Room Enter
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_ENTER_ROOM\
    TELMEVENTDEF_START(METRIC_CHATROOM_ENTER_ROOM)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","ENTR")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "room"    /* Room Id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Chat Room ExitC
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_EXIT_ROOM\
    TELMEVENTDEF_START(METRIC_CHATROOM_EXIT_ROOM)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR","EXIT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "room"     /* Room Id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"     /* Group Id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "iigo"    /* In IGO flag */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  9.5 CHATROOM INVITES
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Sent invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_INVITE_SENT\
    TELMEVENTDEF_START(METRIC_CHATROOM_INVITE_SENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "INVS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group id    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inid"    /* Invitee id    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Received invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_INVITE_RECEIVED\
    TELMEVENTDEF_START(METRIC_CHATROOM_INVITE_RECEIVED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "INVR")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group id    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Declined invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_INVITE_DECLINED\
    TELMEVENTDEF_START(METRIC_CHATROOM_INVITE_DECLINED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "INVD")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group id    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Accepted invite
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CHATROOM_INVITE_ACCEPTED\
    TELMEVENTDEF_START(METRIC_CHATROOM_INVITE_ACCEPTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "CHTR", "INVA")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "gpid"    /* Group id    */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "gpct"   /* Group Count */)\
    TELMEVENTDEF_END()

//=============================================================================
//  SOCIAL:  VOICE CHAT
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  VOICE CHAT:  Voice Channel Rest Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_VC_CHANNEL_ERROR\
    TELMEVENTDEF_START(METRIC_VC_CHANNEL_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "VOIC","ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* Error type */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "mseg"    /* Error Message */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error Code */)\
    TELMEVENTDEF_END()


//=============================================================================
//  SOCIAL:  IGO
//=============================================================================

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser session started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_START\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "STRT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser session ended
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_END\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "ENDT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser tab added
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_ADDTAB\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_ADDTAB)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "ADTB")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser tab closed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_CLOSETAB\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_CLOSETAB)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "CLTB")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser page load started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_PAGELOAD\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_PAGELOAD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "PGLD")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser URL Shortcut entered
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_URLSHORTCUT\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_URLSHORTCUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "USHC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "shct"    /* shortcut        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser miniapp opened
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_BROWSER_MINIAPP\
    TELMEVENTDEF_START(METRIC_IGO_BROWSER_MINIAPP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOB", "MAPP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "gsid"   /* Game session id     */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Overlay session started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_OVERLAY_START\
    TELMEVENTDEF_START(METRIC_IGO_OVERLAY_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOV", "STRT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Overlay session finished
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_OVERLAY_END\
    TELMEVENTDEF_START(METRIC_IGO_OVERLAY_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SOCL", "IGOV", "ENDT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Detect 3rd party DLLs
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_3RDPARTY_DLL\
    TELMEVENTDEF_START(METRIC_IGO_3RDPARTY_DLL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "3RDP", "DETC")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "host"    /* 3rd party DLL in Origin */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "game"    /* 3rd party DLL in game   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "name"    /* DLL name                */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Injection failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_INJECTION_FAIL\
    TELMEVENTDEF_START(METRIC_IGO_INJECTION_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "INJC", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Failure context  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "elev"    /* Is elevated game */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "frtr"    /* Is free trials   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking begin session
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_HOOKING_BEGIN\
    TELMEVENTDEF_START(METRIC_IGO_HOOKING_BEGIN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "HOOK", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctid"    /* ProductId  */)\
    TELMEVENTDEF_ATTR(TYPEID_I64 , "tstp"    /* Timestamp/session id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "ppid"    /* Process pid */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "osid"    /* OS info summary */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "cpus"    /* CPU info summary */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "gpus"    /* GPU info summary */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_HOOKING_FAIL\
    TELMEVENTDEF_START(METRIC_IGO_HOOKING_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "HOOK", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctid"    /* ProductId  */)\
    TELMEVENTDEF_ATTR(TYPEID_I64 , "tstp"    /* Timestamp/session id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "ppid"    /* Process pid */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctxt"    /* Failure context  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "rend"    /* Renderer */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "mesg"    /* Message   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking info
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_HOOKING_INFO\
    TELMEVENTDEF_START(METRIC_IGO_HOOKING_INFO)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "HOOK", "INFO")\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctid"    /* ProductId  */)\
    TELMEVENTDEF_ATTR(TYPEID_I64 , "tstp"    /* Timestamp/session id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "ppid"    /* Process pid */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctxt"    /* Info context  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "rend"    /* Renderer */)\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "mesg"    /* Message   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking end session
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_IGO_HOOKING_END\
    TELMEVENTDEF_START(METRIC_IGO_HOOKING_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("IGOX", "HOOK", "ENDT")\
    TELMEVENTDEF_ATTR(TYPEID_S8  , "ctid"    /* ProductId  */)\
    TELMEVENTDEF_ATTR(TYPEID_I64 , "tstp"    /* Timestamp/session id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32 , "ppid"    /* Process pid */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  NON ORIGN GAMES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  name on Login
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NON_ORIGIN_GAME_ID_LOGIN\
    TELMEVENTDEF_START(METRIC_NON_ORIGIN_GAME_ID_LOGIN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NOGS", "NAME", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"   /* ID of item     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  name on Logout
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NON_ORIGIN_GAME_ID_LOGOUT\
    TELMEVENTDEF_START(METRIC_NON_ORIGIN_GAME_ID_LOGOUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NOGS", "NAME", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"   /* ID of item     */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Count on Login
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NON_ORIGIN_GAME_COUNT_LOGIN\
    TELMEVENTDEF_START(METRIC_NON_ORIGIN_GAME_COUNT_LOGIN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NOGS", "CONT", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "totl"    /* Non Origin game total count */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Count on Logout
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NON_ORIGIN_GAME_COUNT_LOGOUT\
    TELMEVENTDEF_START(METRIC_NON_ORIGIN_GAME_COUNT_LOGOUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NOGS", "CONT", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sttl"    /* Start count    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "entl"    /* End count      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "detl"    /* Delta          */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Add game
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NON_ORIGIN_GAME_ADD\
    TELMEVENTDEF_START(METRIC_NON_ORIGIN_GAME_ADD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NOGS", "OPER", "ADDX")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"   /* ID of item     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Customize boxart start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CUSTOMIZE_BOXART_START\
    TELMEVENTDEF_START(METRIC_CUSTOMIZE_BOXART_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "BOXA", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "isng"   /* is NOG            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Customize boxart apply
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CUSTOMIZE_BOXART_APPLY\
    TELMEVENTDEF_START(METRIC_CUSTOMIZE_BOXART_APPLY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "BOXA", "APLY")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "isng"   /* is NOG            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  NON ORIGN GAMES:  Cusotmize boxart cancel
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CUSTOMIZE_BOXART_CANCEL\
    TELMEVENTDEF_START(METRIC_CUSTOMIZE_BOXART_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "BOXA", "CANC")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "isng"   /* is NOG            */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  SECURITY QUESTIONS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SECURITY QUESTIONS:  Security Question Show
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SECURITY_QUESTION_SHOW\
    TELMEVENTDEF_START(METRIC_SECURITY_QUESTION_SHOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "SECQ", "SHOW")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SECURITY QUESTIONS:  Security Question Set
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SECURITY_QUESTION_SET\
    TELMEVENTDEF_START(METRIC_SECURITY_QUESTION_SET)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "SECQ", "SETX")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SECURITY QUESTIONS:  Security Question Cancel
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SECURITY_QUESTION_CANCEL\
    TELMEVENTDEF_START(METRIC_SECURITY_QUESTION_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("USER", "SECQ", "CANC")\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  ENTITLEMENTS
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  ENTITLEMENTS:  GAMES
//=============================================================================

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  No entitlements then go to store.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENTS_NONE_GOTO_STORE\
    TELMEVENTDEF_START(METRIC_ENTITLEMENTS_NONE_GOTO_STORE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "NOET", "STOR")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "type"    /* 0- Store Home, 1- Free games */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Count on Login
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_GAME_COUNT_LOGIN\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_GAME_COUNT_LOGIN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENUM", "ENTM", "CONT")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "game"    /* Entitlements: games */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nogs"    /* Entitlements:  Non-Origin games*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Count on Logout
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_GAME_COUNT_LOGOUT\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_GAME_COUNT_LOGOUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENUM", "ENTM", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "totl"    /* Total entitlements    */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "stgm"    /* Start games           */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "engm"    /* End games             */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "degm"    /* Delta games           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "favo"    /* Favorite entitlements */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "hide"    /* Hidden entitlements   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nogs"    /* Non-Origin games      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Hide game
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_HIDE\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_HIDE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "HIDE", "SETX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Unhide game
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_UNHIDE\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_UNHIDE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "HIDE", "USET")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Favorite game
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_FAVORITE\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_FAVORITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "FAVO", "SETX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Unfavorite game
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_UNFAVORITE\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_UNFAVORITE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "FAVO", "USET")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Reload/refresh
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_RELOAD\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_RELOAD)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "RELO", "RQST")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Source request */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Free trial error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_FREE_TRIAL_ERROR\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_FREE_TRIAL_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "FRET", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "rest"    /* Rest error     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* Http status    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Dirty bit entitlement updated event from server
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_DIRTYBIT_NOTIFICATION\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_DIRTYBIT_NOTIFICATION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "DBIT", "NOTF")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Client entitlement refresh from dirty bit entitlement updated event from server
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_DIRTYBIT_REFRESH\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_DIRTYBIT_REFRESH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "DBIT", "RFSH")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Client receives dirty bit catalog updated event from server
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "CATL", "PARS")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Client detected a catalog offer was updated
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "CATO", "UPDT")\
    TELMEVENTDEF_ATTR(TYPEID_U64, "btch"    /* catalog batch time */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "offr"    /* offer id */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Client detected a catalog offer was updated
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "CATM", "UPDT")\
    TELMEVENTDEF_ATTR(TYPEID_U64, "btch"    /* catalog batch time */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "mast"    /* master title id */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Entitlement request completed successfully
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_REFRESH_COMPLETED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_REFRESH_COMPLETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "RFSH", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "stme"    /* Server response time */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cent"    /* Number of entitlements returned */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "dlta"    /* Change in number of entitlements */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "loct"    /* Local time UTC */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "devi"    /* Deviation between local time and server time (absolute value) */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  An error occurred during refresh.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_REFRESH_ERROR\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_REFRESH_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "RFSH", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ecod"    /* Rest error code */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "hcod"    /* HTTP error code */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ccod"    /* Client error code */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Entitlement request completed successfully
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "ECRF", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "mstr"    /* Master Title Id of refresh */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "stme"    /* Server response time */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cent"    /* Number of entitlements returned */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  An error occurred during refresh.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "ECRF", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "mstr"    /* Master Title Id of refresh */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ecod"    /* Rest error code */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "hcod"    /* HTTP error code */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "ccod"    /* Client error code */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Ecommerce server refused incremental request and responsed will full response
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_REFRESH_REQUEST_REFUSED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_REFRESH_REQUEST_REFUSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "RFSH", "RFSD")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dlta"    /* Seconds elapsed since last successful entitlement request */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "clnt"    /* Bool indicating that client refused request [1] or server did [0] */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sess"    /* Current client session length */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Offer live build version changed detected: Version Check Started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "OVUC", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "ctid"  /* Offer ID                 */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "iver"  /* Installed Build Version */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Offer live build version changed detected: Version Check Completed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "OVUC", "CMPL")\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "ctid"  /* Offer ID                 */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "iver"  /* Installed Build Version */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "lver"  /* Live Build Version */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Offer live build version changed detected: Version Check Timeout
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT\
    TELMEVENTDEF_START(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ENTM", "OVUC", "TMEO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "ctid"  /* Offer ID                 */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  STORE
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  STORE:  NAVIGATION
//=============================================================================

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION:  Client store url
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_STORE_URL\
    TELMEVENTDEF_START(METRIC_STORE_URL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("STOR", "HTTP", "URLX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "urlx"    /* URL navigated to */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION:  Load status
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_STORE_LOAD_STATUS\
    TELMEVENTDEF_START(METRIC_STORE_LOAD_STATUS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("STOR", "HTTP", "LSTS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,    "urlx"     /* URL navigated to     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,   "time"     /* Time to load page    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,   "succ"     /* Load success/fail    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION:  Client OtH navigation
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_STORE_OTH\
    TELMEVENTDEF_START(METRIC_STORE_OTH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("STOR", "HTTP", "OTHX")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "from"    /* OTH navigated from */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  HOME
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//  HOME:  NAVIGATION
//=============================================================================

//-----------------------------------------------------------------------------
//  HOME:  NAVIGATION:  Client HOME url
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOME_URL\
    TELMEVENTDEF_START(METRIC_HOME_URL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HOME", "HTTP", "URLX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "urlx"    /* URL navigated to */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  PATCH
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PATCH:  Manual patching started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_MANUALPATCH_DOWNLOAD_START\
    TELMEVENTDEF_START(METRIC_MANUALPATCH_DOWNLOAD_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PATC", "DNDL", "MANU")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inst"    /* Locally installed version */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "serv"    /* Server version            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PATCH:  Automatic patching started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_AUTOPATCH_DOWNLOAD_START\
    TELMEVENTDEF_START(METRIC_AUTOPATCH_DOWNLOAD_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PATC", "DNDL", "AUTO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inst"    /* Locally installed version */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "serv"    /* Server version            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PATCH:  Download skip
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_AUTOPATCH_DOWNLOAD_SKIP\
    TELMEVENTDEF_START(METRIC_AUTOPATCH_DOWNLOAD_SKIP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PATC", "DNDL", "SKIP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature untrusted signer
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH\
    TELMEVENTDEF_START(METRIC_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PATC", "DNDL", "DMAT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Entitlement content id  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inst"    /* Installed version  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "serv"    /* Server version  */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  AUTOREPAIR
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  AUTOREPAIR: User accepted task
//-----------------------------------------------------------------------------

#define TELMEVENT_METRIC_AUTOREPAIR_TASK_ACCEPTED\
    TELMEVENTDEF_START(METRIC_AUTOREPAIR_TASK_ACCEPTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ATRP", "DNDL", "ACCP")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  AUTOREPAIR: User declined task
//-----------------------------------------------------------------------------

#define TELMEVENT_METRIC_AUTOREPAIR_TASK_DECLINED\
    TELMEVENTDEF_START(METRIC_AUTOREPAIR_TASK_DECLINED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ATRP", "DNDL", "DECL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  AUTOREPAIR: Content needs repair
//-----------------------------------------------------------------------------

#define TELMEVENT_METRIC_AUTOREPAIR_NEEDS_REPAIR\
    TELMEVENTDEF_START(METRIC_AUTOREPAIR_NEEDS_REPAIR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ATRP", "DNDL", "NDRP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Entitlement content id  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inst"    /* Installed version  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "serv"    /* Server version  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  AUTOREPAIR: Repair started
//-----------------------------------------------------------------------------

#define TELMEVENT_METRIC_AUTOREPAIR_DOWNLOAD_START\
    TELMEVENTDEF_START(METRIC_AUTOREPAIR_DOWNLOAD_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ATRP", "DNDL", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Entitlement content id  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "inst"    /* Installed version  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "serv"    /* Server version  */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  SELF PATCH
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch found
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_FOUND\
    TELMEVENTDEF_START(METRIC_SELFPATCH_FOUND)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "PTCH", "FOUN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch declined
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_DECLINED\
    TELMEVENTDEF_START(METRIC_SELFPATCH_DECLINED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "PTCH", "DECL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch download start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_DL_START\
    TELMEVENTDEF_START(METRIC_SELFPATCH_DL_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "DWNL", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch download finished
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_DL_FINISHED\
    TELMEVENTDEF_START(METRIC_SELFPATCH_DL_FINISHED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "DWNL", "FNSH")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patcher launched
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_LAUNCHED\
    TELMEVENTDEF_START(METRIC_SELFPATCH_LAUNCHED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "PTCH", "LNCH")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patcher offline mode
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_OFFLINE_MODE\
    TELMEVENTDEF_START(METRIC_SELFPATCH_OFFLINE_MODE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "PTCH", "OFMD")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature validation error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_SIGNATURE_VALIDATION_ERROR\
    TELMEVENTDEF_START(METRIC_SELFPATCH_SIGNATURE_VALIDATION_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "SIGN", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature invalid hash
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_SIGNATURE_INVALID_HASH\
    TELMEVENTDEF_START(METRIC_SELFPATCH_SIGNATURE_INVALID_HASH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SPTC", "SIGN", "HASH")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature untrusted signer
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER\
    TELMEVENTDEF_START(METRIC_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PATC", "DNDL", "TRST")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "newv"    /* Patch version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "reqd"    /* Required flag  */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "emrg"    /* Emergency flag */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  NETWORK
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  NETWORK: SSL error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NETWORK_SSL_ERROR\
    TELMEVENTDEF_START(METRIC_NETWORK_SSL_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NETW", "HTTP", "SSLE")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "surl"    /* URL            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "code"    /* Error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "cert"    /* Cert SHA1      */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  CODE REDEMPTION
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  On successful code redemption
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_CODE_REDEEM_SUCCESS\
    TELMEVENTDEF_START(METRIC_ACTIVATION_CODE_REDEEM_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("ACTV", "CODE", "SUCC")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Error with code being redeemed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_CODE_REDEEM_ERROR\
    TELMEVENTDEF_START(METRIC_ACTIVATION_CODE_REDEEM_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("ACTV", "CODE", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "erro"   /* Error code     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Request for the code redemption web pages
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_REDEEM_PAGE_REQUEST\
    TELMEVENTDEF_START(METRIC_ACTIVATION_REDEEM_PAGE_REQUEST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACTV", "REDP", "RQST")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Page loaded successfully
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_REDEEM_PAGE_SUCCESS\
    TELMEVENTDEF_START(METRIC_ACTIVATION_REDEEM_PAGE_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACTV", "REDP", "SUCC")\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Error trying to loading code redemption pages
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_REDEEM_PAGE_ERROR\
    TELMEVENTDEF_START(METRIC_ACTIVATION_REDEEM_PAGE_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACTV", "REDP", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "erro"    /* Start time         */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* HTTP status code   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Requesting FAQ pages
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_FAQ_PAGE_REQUEST\
    TELMEVENTDEF_START(METRIC_ACTIVATION_FAQ_PAGE_REQUEST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACTV", "FAQP", "RQST")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CODE REDEMPTION:  Close code redemption page event
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVATION_WINDOW_CLOSE\
    TELMEVENTDEF_START(METRIC_ACTIVATION_WINDOW_CLOSE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACTV", "REDP", "CLOS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "evnt"    /* Type of event: success or generic close */)\
    TELMEVENTDEF_END()



///////////////////////////////////////////////////////////////////////////////
//  REPAIR INSTALL
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: num files verified replaced
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_FILES_STATS\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_FILES_STATS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "STAT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "frep"    /* Replaced files */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "fver"    /* Verified files */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: requested
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_REQUESTED\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_REQUESTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "RQST")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: verify canceled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_VERIFYCANCELED\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_VERIFYCANCELED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "VCNL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: repair canceled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_REPLACECANCELED\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_REPLACECANCELED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "RCNL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: replacing files
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_REPLACINGFILES\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_REPLACINGFILES)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "RFIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: verify files completed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_VERIFYFILESCOMPLETED\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_VERIFYFILESCOMPLETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "VCMP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  REPAIR INSTALL: repair files completed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_REPAIRINSTALL_REPLACINGFILESCOMPLETED\
    TELMEVENTDEF_START(METRIC_REPAIRINSTALL_REPLACINGFILESCOMPLETED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DNDL", "CACH", "RCMP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  CLOUD SAVE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_WARNING_SPACE_CLOUD_LOW\
    TELMEVENTDEF_START(METRIC_CLOUD_WARNING_SPACE_CLOUD_LOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "WARN", "CLLW")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "clsp"    /* Cloud space    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY\
    TELMEVENTDEF_START(METRIC_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "WARN", "CLMX")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "clsp"    /* Cloud space    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_WARNING_SYNC_CONFLICT\
    TELMEVENTDEF_START(METRIC_CLOUD_WARNING_SYNC_CONFLICT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "WARN", "CSCF")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_ACTION_OBJECT_DELETION\
    TELMEVENTDEF_START(METRIC_CLOUD_ACTION_OBJECT_DELETION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "ACTN", "ODEL")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nmbr"    /* Object deleted */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED\
    TELMEVENTDEF_START(METRIC_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "ACTN", "GNCD")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_MANUAL_RECOVERY\
    TELMEVENTDEF_START(METRIC_CLOUD_MANUAL_RECOVERY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "MANL", "RECO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_AUTOMATIC_SAVE_SUCCESS\
    TELMEVENTDEF_START(METRIC_CLOUD_AUTOMATIC_SAVE_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "AUTO", "SVOK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_AUTOMATIC_RECOVERY_SUCCESS\
    TELMEVENTDEF_START(METRIC_CLOUD_AUTOMATIC_RECOVERY_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "AUTO", "RCOK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_ACTION_SYNC_LOCKING_UNSUCCESFUL\
    TELMEVENTDEF_START(METRIC_CLOUD_ACTION_SYNC_LOCKING_UNSUCCESFUL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "ACTN", "SYLS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_ACTION_PUSH_FAILED\
    TELMEVENTDEF_START(METRIC_CLOUD_ACTION_PUSH_FAILED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "ACTN", "PSHF")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_ACTION_PULL_FAILED\
    TELMEVENTDEF_START(METRIC_CLOUD_ACTION_PULL_FAILED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "ACTN", "PULF")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_MIGRATION_SUCCESS\
    TELMEVENTDEF_START(METRIC_CLOUD_MIGRATION_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "MIGR", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content id         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CLOUD SAVE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CLOUD_PUSH_MONITORING\
    TELMEVENTDEF_START(METRIC_CLOUD_PUSH_MONITORING)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CLOD", "PUSH", "MNTR")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"  /* Content id           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "numf"  /* # of files to upload */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "size"  /* Total upload size    */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  OFM-8399: SUBSCRIPTION user interaction events
///////////////////////////////////////////////////////////////////////////////
#define TELMEVENT_METRIC_SUBSCRIPTION_ENT_UPGRADED\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_ENT_UPGRADED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "ACTN", "UPGR")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* owned offer id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "sbid"    /* subscription offer id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "time"    /* subscription elapsed time */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "succ"    /* was success? */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SUBSCRIPTION_ENT_REMOVE_START\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_ENT_REMOVE_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "REMO", "STAR")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* subscription offer id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "time"    /* subscription elapsed time */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "succ"    /* was success? */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SUBSCRIPTION_ENT_REMOVED\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_ENT_REMOVED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "REMO", "DONE")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* subscription offer id */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "succ"    /* was success? */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SUBSCRIPTION_UPSELL\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_UPSELL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "ACTN", "USELL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sact"    /* is subscription active */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SUBSCRIPTION_FAQ_PAGE_REQUEST\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_FAQ_PAGE_REQUEST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "FAQP", "RQST")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SUBSCRIPTION_ONLINE\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_ONLINE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "OSTA", "ONLN")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lkgt"     /* Last Known Good time (i.e. the last time the user was online) */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "sexp"     /* Subscription Expiration date for the signed-in user */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  MEMORY FOOTPRINT
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  UNLOAD/RELOAD STORE BROWSER MEMORY FOOTPRINT INFO
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FOOTPRINT_INFO_STORE\
    TELMEVENTDEF_START(METRIC_FOOTPRINT_INFO_STORE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("MEMO", "INFO", "STOR")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ulst"    /* Unload store         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rlst"     /* Reload store         */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  SINGLE LOGIN
//  Notes:  Used to debug single login
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SINGLE LOGIN
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SINGLE_LOGIN_USER_LOGGING_IN\
    TELMEVENTDEF_START(METRIC_SINGLE_LOGIN_USER_LOGGING_IN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SING", "USER", "LGIN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* Login type     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SINGLE LOGIN
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SINGLE_LOGIN_USER_LOGGING_OUT\
    TELMEVENTDEF_START(METRIC_SINGLE_LOGIN_USER_LOGGING_OUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SING", "USER", "LGOT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SINGLE LOGIN
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SINGLE_LOGIN_GO_ONLINE_ACTION\
    TELMEVENTDEF_START(METRIC_SINGLE_LOGIN_GO_ONLINE_ACTION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SING", "ACTI", "ONLI")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SINGLE LOGIN
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SINGLE_LOGIN_GO_OFFLINE_ACTION\
    TELMEVENTDEF_START(METRIC_SINGLE_LOGIN_GO_OFFLINE_ACTION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SING", "ACTI", "OFFL")\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  SINGLE SIGN-ON (SSO)
//  Notes:  Used to debug the SSO
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SSO FLOW START
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SSO_FLOW_START\
    TELMEVENTDEF_START(METRIC_SSO_FLOW_START)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SSON", "FLOW", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "code"    /* Auth code (ID2)       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "tokn"    /* Auth token (ID1)      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "type"    /* Login type            */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "user"    /* User currently online */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SSO FLOW RESULT
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SSO_FLOW_RESULT\
    TELMEVENTDEF_START(METRIC_SSO_FLOW_RESULT)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SSON", "FLOW", "RSLT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "rslt"    /* Flow result      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "actn"    /* SSO actoin       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "type"    /* Login type       */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SSO FLOW RESULT
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SSO_FLOW_ERROR\
    TELMEVENTDEF_START(METRIC_SSO_FLOW_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SSON", "FLOW", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"    /* Error reason     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "desc"    /* REST description */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "rest"    /* REST error       */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "http"    /* HTTP error       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer IDs        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  SSO FLOW INFO
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SSO_FLOW_INFO\
    TELMEVENTDEF_START(METRIC_SSO_FLOW_INFO)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SSON", "FLOW", "INFO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "text"    /* Info text        */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  3PDD INSTALL
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_INSTALL_DIALOG_SHOW\
    TELMEVENTDEF_START(METRIC_3PDD_INSTALL_DIALOG_SHOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "INST", "DSHO")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_INSTALL_DIALOG_CANCEL\
    TELMEVENTDEF_START(METRIC_3PDD_INSTALL_DIALOG_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "INST", "DCNL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_INSTALL_TYPE\
    TELMEVENTDEF_START(METRIC_3PDD_INSTALL_TYPE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "INST", "ITYP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* Install type   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_PLAY_DIALOG_SHOW\
    TELMEVENTDEF_START(METRIC_3PDD_PLAY_DIALOG_SHOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "PLAY", "DSHO")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_PLAY_DIALOG_CANCEL\
    TELMEVENTDEF_START(METRIC_3PDD_PLAY_DIALOG_CANCEL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "PLAY", "DCNL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_PLAY_TYPE\
    TELMEVENTDEF_START(METRIC_3PDD_PLAY_TYPE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "PLAY", "PTYP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* Install type   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_INSTALL_COPYCDKEY\
    TELMEVENTDEF_START(METRIC_3PDD_INSTALL_COPYCDKEY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "INST", "CDKY")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  3PDD INSTALL
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_3PDD_PLAY_COPYCDKEY\
    TELMEVENTDEF_START(METRIC_3PDD_PLAY_COPYCDKEY)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("3PDD", "PLAY", "CDKY")\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  FREE TRIALS
///////////////////////////////////////////////////////////////////////////////
#define TELMEVENT_METRIC_FREETRIAL_PURCHASE\
    TELMEVENTDEF_START(METRIC_FREETRIAL_PURCHASE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("FTRL", "GAME", "PURC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "expd"    /* Expired        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Source of click*/)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  CONTENT SALES
///////////////////////////////////////////////////////////////////////////////
#define TELMEVENT_METRIC_CONTENTSALES_PURCHASE\
    TELMEVENTDEF_START(METRIC_CONTENTSALES_PURCHASE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CTSL", "ADDX", "PURC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content/product ID             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ptyp"    /* Product type                   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Description of purchase method */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_CONTENTSALES_VIEW_IN_STORE\
    TELMEVENTDEF_START(METRIC_CONTENTSALES_VIEW_IN_STORE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CTSL", "ADDX", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content/product ID             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ptyp"    /* Product type                   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Description of purchase method */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  HELP
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  HELP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HELP_ORIGIN_HELP\
    TELMEVENTDEF_START(METRIC_HELP_ORIGIN_HELP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HELP", "BTTN", "CLCK")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  HELP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HELP_WHATS_NEW\
    TELMEVENTDEF_START(METRIC_HELP_WHATS_NEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("WHNW", "BTTN", "CLCK")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  HELP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HELP_ORIGIN_FORUM\
    TELMEVENTDEF_START(METRIC_HELP_ORIGIN_FORUM)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("FRUM", "BTTN", "CLCK")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//	HELP: OER launched from help menu.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HELP_ORIGIN_ER\
    TELMEVENTDEF_START(METRIC_HELP_ORIGIN_ER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HOER", "BTTN", "CLCK")\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  ORIGIN ERROR REPORTER
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//	OER: Fired from OER when it lanuches. Must be critical because OER has no login.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_OER_LAUNCHED\
    TELMEVENTDEF_START(METRIC_OER_LAUNCHED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HOER", "SESS", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "clnt"    /* Bool: From client? */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//	OER: OER application had an error. Must be critical because OER has no login.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_OER_ERROR\
    TELMEVENTDEF_START(METRIC_OER_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HOER", "SESS", "ERRO")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//	OER: OER application sent a report. Must be critical because OER has no login.
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_OER_REPORT_SENT\
    TELMEVENTDEF_START(METRIC_OER_REPORT_SENT)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("HOER", "SESS", "SENT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "rept"    /* Report ID */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  PERFORMANCE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PERFORMANCE
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PERFORMANCE_TIMER\
    TELMEVENTDEF_START(METRIC_PERFORMANCE_TIMER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PERF", "PROF", "TIME")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "area"    /* Area of timer                   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Duration in milliseconds        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "xtra"    /* Extra info: # friend or # games */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "xtr2"    /* Extra info 2: cached/uncached   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* Start time in seconds           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ends"    /* End time in seconds             */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PERFORMANCE: Active Client
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACTIVE_CLIENT_TIMER\
    TELMEVENTDEF_START(METRIC_ACTIVE_CLIENT_TIMER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PERF", "PROF", "ACTV")\
    TELMEVENTDEF_ATTR(TYPEID_U64, "actv"    /* Duration in milliseconds of active use   */)\
    TELMEVENTDEF_ATTR(TYPEID_U64, "totl"    /* Duration in milliseconds of total runtime*/)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  GUI
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// GUI: My Games Play Clicked Source
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMES_PLAY_SOURCE\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMES_PLAY_SOURCE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MGPL", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"  /* What kind of action caused the play: hovercard, gametile, contextmenu, gamedetails*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  My Games Details Page Viewed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMESDETAILSPAGEVIEW\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMESDETAILSPAGEVIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MGDP", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  My Games Cloud Storage Tab Viewed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "CLST", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  My Games Manual Link Click
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMES_MANUAL_LINK_CLICK\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMES_MANUAL_LINK_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MANL", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Content ID     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMEVIEWSTATETOGGLE\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMEVIEWSTATETOGGLE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MGVS", "TOGG")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "view"    /* App session length */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  My Games View State on logout/exit (unused)
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMEVIEWSTATEEXIT\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMEVIEWSTATEEXIT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MGVS", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "view"    /* View: 0-"grid", 1-"list" */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  NAVIGATION:  Tab
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_TAB_VIEW\
    TELMEVENTDEF_START(METRIC_GUI_TAB_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "TABS", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ntab"    /* New tab string */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "init"    /* Client startup */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  NAVIGATION:  Application Settings
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_SETTINGS_VIEW\
    TELMEVENTDEF_START(METRIC_GUI_SETTINGS_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "SETT", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "type"    /* Type: 0-Application, 1-Account */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GUI:  Update clicked from game details
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_DETAILS_UPDATE_CLICKED\
    TELMEVENTDEF_START(METRIC_GUI_DETAILS_UPDATE_CLICKED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "UPDT", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
// GUI: My Games HTTP Request ERROR
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GUI_MYGAMES_HTTP_ERROR\
    TELMEVENTDEF_START(METRIC_GUI_MYGAMES_HTTP_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GUIX", "MGRQ", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "qnet"    /* QNetworkReply error  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* HTTP status code     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "cach"    /* Cache control        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "link"    /* Error URL            */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  GREY MARKET
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GREY MARKET: Language Selection
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GREYMARKET_LANGUAGE_SELECTION\
    TELMEVENTDEF_START(METRIC_GREYMARKET_LANGUAGE_SELECTION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GRMK", "LANG", "SELC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lang"    /* Language selection  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "list"    /* Available languages */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mfst"    /* Manifest languages  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GREY MARKET: Language Selection for Bypass Content
//-----------------------------------------------------------------------------

#define TELMEVENT_METRIC_GREYMARKET_LANGUAGE_SELECTION_BYPASS\
    TELMEVENTDEF_START(METRIC_GREYMARKET_LANGUAGE_SELECTION_BYPASS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GRMK", "LANG", "SLBY")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lang"    /* Language selection  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "list"    /* Available languages */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mfst"    /* Manifest languages  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GREY MARKET: Bypass filter
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GREYMARKET_BYPASS_FILTER\
    TELMEVENTDEF_START(METRIC_GREYMARKET_BYPASS_FILTER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GRMK", "LANG", "BYPS")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "list"    /* Available languages */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mfst"    /* Manifest languages  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GREY MARKET: Language Selection Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GREYMARKET_LANGUAGE_SELECTION_ERROR\
    TELMEVENTDEF_START(METRIC_GREYMARKET_LANGUAGE_SELECTION_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GRMK", "LANG", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "list"    /* Available languages */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mfest"   /* Manifest languages  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  GREY MARKET: Available Languages List
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GREYMARKET_LANGUAGE_LIST\
    TELMEVENTDEF_START(METRIC_GREYMARKET_LANGUAGE_LIST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GRMK", "LANG", "LIST")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Content ID          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "list"    /* Available languages */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "from"    /* Origin of request   */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_NETPROMOTER_RESULT\
    TELMEVENTDEF_START(METRIC_NETPROMOTER_RESULT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NPRM", "ACTN", "RATE")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "resp"    /* 0 for no response - 1-10 feedback value */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptfm"    /* platform used (pc, mac, etc...)         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "xtra"    /* feedback text                           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lang"    /* language used (from system locale)      */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "cont"    /* can contact user about this issue       */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_NETPROMOTER_GAME_RESULT\
    TELMEVENTDEF_START(METRIC_NETPROMOTER_GAME_RESULT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("NPRM", "GAME", "RATE")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ctid"    /* Offer Id for game */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "resp"    /* 0 for no response - 1-10 feedback value */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "ptfm"    /* platform used (pc, mac, etc...)         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "xtra"    /* feedback text                           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "lang"    /* language used (from system locale)      */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "cont"    /* can contact user about this issue       */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_JUMPLIST_ACTION\
    TELMEVENTDEF_START(METRIC_JUMPLIST_ACTION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("JLST", "ACTN", "EXEC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "type"    /* task selected from jumplist (e.g. "My Origin", "RecentGame"               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "isrc"    /* where user selected task from (e.g. "jumplist", "startmenu", "systemtray" */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "xtra"    /* future use (e.g. product ID of recent game selected)                      */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  ORIGIN DEVELOPER MODE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DEVELOPER MODE: Activation Successful
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DEVMODE_ACTIVATE_SUCCESS\
    TELMEVENTDEF_START(METRIC_DEVMODE_ACTIVATE_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DEVM", "ACTV", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tlvs"    /* Tool version          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "plat"    /* Platform              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DEVELOPER MODE: Activation Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DEVMODE_ACTIVATE_ERROR\
    TELMEVENTDEF_START(METRIC_DEVMODE_ACTIVATE_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DEVM", "ACTV", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tlvs"    /* Tool version          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "plat"    /* Platform              */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "errc"    /* Error code            */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DEVELOPER MODE: Deactivation Successful
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DEVMODE_DEACTIVATE_SUCCESS\
    TELMEVENTDEF_START(METRIC_DEVMODE_DEACTIVATE_SUCCESS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DEVM", "DACT", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tlvs"    /* Tool version          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "plat"    /* Platform              */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DEVELOPER MODE: Deactivation Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DEVMODE_DEACTIVATE_ERROR\
    TELMEVENTDEF_START(METRIC_DEVMODE_DEACTIVATE_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("DEVM", "DACT", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "tlvs"    /* Tool version          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "plat"    /* Platform              */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "errc"    /* Error code            */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  SONAR
///////////////////////////////////////////////////////////////////////////////

#define TELMEVENT_METRIC_SONAR_MESSAGE\
    TELMEVENTDEF_START(METRIC_SONAR_MESSAGE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "MESS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scat"    /* Sonar Category            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "smsg"    /* Sonar Message             */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SONAR_STAT\
    TELMEVENTDEF_START(METRIC_SONAR_STAT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "STAT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scat"    /* Stat category             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "snam"    /* Stat name                 */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "sval"    /* Stat value                */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SONAR_ERROR\
    TELMEVENTDEF_START(METRIC_SONAR_ERROR)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scat"    /* Sonar Category            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "smsg"    /* Sonar Message             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "serr"    /* Sonar Error               */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SONAR_CLIENT_CONNECTED\
    TELMEVENTDEF_START(METRIC_SONAR_CLIENT_CONNECTED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "CCON")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "chid"    /* Channel id                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "idev"    /* Input device name         */)\
    TELMEVENTDEF_ATTR(TYPEID_I8,  "iagc"    /* Input auto-gain setting   */)\
    TELMEVENTDEF_ATTR(TYPEID_I16, "igai"    /* Input gain setting        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "odev"    /* Output device name        */)\
    TELMEVENTDEF_ATTR(TYPEID_I16, "ogai"    /* Output gain setting       */)\
	TELMEVENTDEF_ATTR(TYPEID_U32, "vsip"    /* Voice server IP address   */)\
	TELMEVENTDEF_ATTR(TYPEID_S8,  "capm"    /* Capture mode              */)\
    TELMEVENTDEF_ATTR(TYPEID_I16, "ithd"    /* Input threshold           */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SONAR_CLIENT_DISCONNECTED\
    TELMEVENTDEF_START(METRIC_SONAR_CLIENT_DISCONNECTED)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "CDIS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "chid"    /* Channel id                */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"    /* Disconnect reason         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "rsds"    /* Disconnect reason desc    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "msen"    /* Messages sent             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mrec"    /* Messages received         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mlos"    /* Messages lost             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "moos"    /* Messages out of sequence  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "bcid"    /* Bad cids                  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mtrn"    /* Messages truncated        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "afcl"    /* Audio frames clipped      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "skse"    /* Socket send error         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "skre"    /* Socket receive error      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rpmn"    /* Receive-playback latency mean  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rpmx"    /* Receive-playback latency max   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ppmn"    /* Playback-playback latency mean */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ppmx"    /* Playback-playback latency max  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jamn"    /* Audio packet reception mean    */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jamx"    /* Audio packet reception max     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jpmn"    /* Audio packet playback mean     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jpmx"    /* Audio packet playback max      */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "pbur"    /* PlaybackUnderrun             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "pbof"    /* PlaybackOverflow             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "pbdl"    /* PlaybackDeviceLost           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dncn"    /* DropNotConnected             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "drsf"    /* DropReadServerFrameCounter   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "drtn"    /* DropReadTakeNumber           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dtst"    /* DropTakeStopped              */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "drci"    /* DropReadCid                  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "dnpl"    /* DropNullPayload              */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "drcf"    /* DropReadClientFrameCounter   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "jbur"    /* JitterbufferUnderrun         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nlos"    /* Network packets lost         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "njtr"    /* Network jitter               */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "npng"    /* Network ping                 */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nrcv"    /* Network packets received     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nqua"    /* Network quality              */)\
	TELMEVENTDEF_ATTR(TYPEID_U32, "vsip"    /* Voice server IP address      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "capm"    /* Capture mode                 */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jbon"    /* Jitter buffer occupancy mean */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "jbox"    /* Jitter buffer occupancy max  */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_SONAR_CLIENT_STATS\
    TELMEVENTDEF_START(METRIC_SONAR_CLIENT_STATS)\
    TELMEVENTDEF_CRITICAL(true)\
    TELMEVENTDEF_MGS("SONR", "MTRC", "CSTT")\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "vers"    /* Client version               */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "chid"    /* Channel id                   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nlmi"    /* Network latency min          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nlma"    /* Network latency max          */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "nlme"    /* Network latency mean         */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "bsnt"    /* Bytes sent                   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "brec"    /* Bytes received               */)\
	TELMEVENTDEF_ATTR(TYPEID_U32, "mlst"    /* Messages lost                */)\
	TELMEVENTDEF_ATTR(TYPEID_U32, "moos"    /* Messages out of sequence     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mjmi"    /* Messages rec'd jitter min    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mjma"    /* Messages rec'd jitter max    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "mjme"    /* Messages rec'd jitter mean   */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "colp"    /* CPU overall load %           */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  PROMO MANAGER
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PROMOMANAGER_START\
    TELMEVENTDEF_START(METRIC_PROMOMANAGER_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "DLGX", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer ID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pcon"    /* Primary context           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scon"    /* Secondary context         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PROMOMANAGER_END\
    TELMEVENTDEF_START(METRIC_PROMOMANAGER_END)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "DLGX", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Duration dialog (seconds) */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "strt"    /* Start time in seconds     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "ends"    /* End time in seconds       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer ID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pcon"    /* Primary context           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scon"    /* Secondary context         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  User manually opened dialog
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PROMOMANAGER_MANUAL_OPEN\
    TELMEVENTDEF_START(METRIC_PROMOMANAGER_MANUAL_OPEN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "MANU", "OPEN")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer ID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pcon"    /* Primary context           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scon"    /* Secondary context         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  Result of manually opened dialog
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PROMOMANAGER_MANUAL_RESULT\
    TELMEVENTDEF_START(METRIC_PROMOMANAGER_MANUAL_RESULT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "MANU", "RSLT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rslt"    /* Result of promo lookup */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer ID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pcon"    /* Primary context           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scon"    /* Secondary context         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  "View in store" button clicked from "no promo" window
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PROMOMANAGER_MANUAL_VIEW_IN_STORE\
    TELMEVENTDEF_START(METRIC_PROMOMANAGER_MANUAL_VIEW_IN_STORE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "MANU", "VIST")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Offer ID                  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "pcon"    /* Primary context           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "scon"    /* Secondary context         */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  PROMO MANAGER:  On The House banner was loaded in client
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_OTH_BANNER_LOADED\
    TELMEVENTDEF_START(METRIC_OTH_BANNER_LOADED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PRMO", "OTHX", "BANN")\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  ORIGIN SDK
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SDK CONNECT:  SDK Version
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SDK_VERSION\
    TELMEVENTDEF_START(METRIC_SDK_VERSION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("OSDK", "CONN", "VERS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "sdkv"    /* SDK version    */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  SDK:  Client requested entitlements without filtering
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SDK_UNFILTERED_ENTITLEMENTS_REQUEST\
    TELMEVENTDEF_START(METRIC_SDK_UNFILTERED_ENTITLEMENTS_REQUEST)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("OSDK", "ENTM", "UFRQ")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* Game           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "sdkv"    /* SDK version    */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  WEB WIDGETS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  WEB WIDGETS:  Download Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEBWIDGET_DL_START\
    TELMEVENTDEF_START(METRIC_WEBWIDGET_DL_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("WDGT", "DNDL", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "etag"    /* Server etag version  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  WEB WIDGETS:  Download End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEBWIDGET_DL_SUCCESS\
    TELMEVENTDEF_START(METRIC_WEBWIDGET_DL_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("WDGT", "DNDL", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "etag"    /* Server etag version  */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "time"    /* Duration in seconds  */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  WEB WIDGETS:  Download Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEBWIDGET_DL_ERROR\
    TELMEVENTDEF_START(METRIC_WEBWIDGET_DL_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("WDGT", "DNDL", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "etag"    /* Server etag version  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error code           */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  WEB WIDGETS:  Version Used
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_WEBWIDGET_LOADED\
    TELMEVENTDEF_START(METRIC_WEBWIDGET_LOADED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("WDGT", "LOAD", "VERS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "name"    /* Widget name    */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cver"    /* Widget version */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  BROADCAST
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  BROADCAST:  Stream Start
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BROADCAST_STREAM_START\
    TELMEVENTDEF_START(METRIC_BROADCAST_STREAM_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BROD", "STRM", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "chid"    /* channel id     */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"stid"    /* stream id      */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "fsdk"    /* from sdk      */)\
    TELMEVENTDEF_END()


//-----------------------------------------------------------------------------
//  BROADCAST:  Stream End
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BROADCAST_STREAM_STOP\
    TELMEVENTDEF_START(METRIC_BROADCAST_STREAM_STOP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BROD", "STRM", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "chid"    /* channel id            */)\
    TELMEVENTDEF_ATTR(TYPEID_U64,"stid"    /* stream id             */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"mivw"    /* min viewers           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"mavw"    /* max viewers           */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,"time"    /* broadcast duration    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  BROADCAST:  Account Linked
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BROADCAST_ACCOUNT_LINKED\
    TELMEVENTDEF_START(METRIC_BROADCAST_ACCOUNT_LINKED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BROD", "ACCT", "LNKD")\
    TELMEVENTDEF_ATTR(TYPEID_U16, "fsdk"    /* from sdk            */)\
    TELMEVENTDEF_END()
//-----------------------------------------------------------------------------
//  BROADCAST:  Stream Error
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BROADCAST_STREAM_ERROR\
    TELMEVENTDEF_START(METRIC_BROADCAST_STREAM_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BROD", "STRM", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* Error code     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "code"    /* Error type     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  BROADCAST:  Stream Joined
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_BROADCAST_JOINED\
    TELMEVENTDEF_START(METRIC_BROADCAST_JOINED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("BROD", "STRM", "JOIN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srcx"    /* Source         */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  ACHIEVEMENTS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Post Success
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_ACH_POST_SUCCESS\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_ACH_POST_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "ACPS", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "aset"    /* achievement set id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "acid"    /* achievement id     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Achievement Post Fail
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_ACH_POST_FAIL\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_ACH_POST_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "ACPS", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* content id         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "aset"   /* achievement set id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "acid"   /* achievement id     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"   /* http status code   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"   /* reason             */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Wincode Post Fail
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_WC_POST_FAIL\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_WC_POST_FAIL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "WCPS", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* content id         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "aset"   /* achievement set id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "wcid"   /* wincode            */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"   /* http status code   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"   /* reason             */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Page view
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_WIDGET_PAGE_VIEW\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_WIDGET_PAGE_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "WDGT", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "name"   /* page name        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* product ID       */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "igov"   /* IGO view         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Purchase click
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_WIDGET_PURCHASE_CLICK\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_WIDGET_PURCHASE_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "WDGT", "PRCH")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* product ID       */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Achievement Achieved XP/RP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_ACHIEVED\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_ACHIEVED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "UPDT", "ACXR")\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "achd"   /* Achieved          */)\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "xpnt"   /* Experience Points */)\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "rpnt"   /* Reward Points     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Achievement Achieved per Game XP/RP
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_ACHIEVED_PER_GAME\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_ACHIEVED_PER_GAME)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "GAME", "ACXR")\
    TELMEVENTDEF_ATTR(TYPEID_S8,   "ctid"   /* product ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,   "aset"   /* Achievement Set Id */)\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "achd"   /* Achieved           */)\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "xpnt"   /* Experience Points  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32,  "rpnt"   /* Reward Points      */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Grant Detected
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_GRANT_DETECTED\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_GRANT_DETECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "GRNT", "DETC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* product ID         */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "aset"   /* Achievement Set Id */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "acid"   /* achievement id     */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Share achievement show
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "SHAR", "SHOW")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "srcx"    /* Source         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Share achievement dismissed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "SHAR", "DISS")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "srcx"    /* Source         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Comparison View
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ACHIEVEMENT_COMPARISON_VIEW\
    TELMEVENTDEF_START(METRIC_ACHIEVEMENT_COMPARISON_VIEW)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ACHV", "COMP", "VIEW")\
    TELMEVENTDEF_ATTR(TYPEID_S8 ,  "ctid"    /* Content ID     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "owns"    /* User owns game */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  DIRTY BITS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DIRTY BITS Network connected
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DIRTYBITS_NETWORK_CONNECTED\
    TELMEVENTDEF_START(METRIC_DIRTYBITS_NETWORK_CONNECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DIRT", "NETW", "CONN")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DIRTY BITS Network disconnected
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DIRTYBITS_NETWORK_DISCONNECTED\
    TELMEVENTDEF_START(METRIC_DIRTYBITS_NETWORK_DISCONNECTED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DIRT", "NETW", "DISC")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "reas"    /* Reason         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DIRTY BITS Message counter
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DIRTYBITS_MESSAGE_COUNTER\
    TELMEVENTDEF_START(METRIC_DIRTYBITS_MESSAGE_COUNTER)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DIRT", "MESG", "CNTR")\
    TELMEVENTDEF_ATTR(TYPEID_S8,"totl"     /*total            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"emal"     /*email            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"orid"     /*origin id        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"pass"     /* password        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"dblc"     /*double connection*/)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"achi"     /*achievements     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"avat"     /*avatar           */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"entl"     /*entitlements     */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"catl"     /*catalog updates  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"unkn"     /*unknown          */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,"dupl"     /*duplicates         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DIRTY BITS Retries capped
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DIRTYBITS_RETRIES_CAPPED\
    TELMEVENTDEF_START(METRIC_DIRTYBITS_RETRIES_CAPPED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DIRT", "RETR", "CAPD")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "tout"    /* Timeout         */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  DIRTY BITS Sudden disconnection
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION\
    TELMEVENTDEF_START(METRIC_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("DIRT", "NETW", "SDSC")\
    TELMEVENTDEF_ATTR(TYPEID_I64,  "tcon"    /* Time connected         */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  GAME PROPERTIES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME PROPERTIES: apply changes
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAMEPROPERTIES_APPLY_CHANGES\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_APPLY_CHANGES)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "APLY", "CHNG")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "isng"   /* is NOG            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* product ID        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "path"   /* install path      */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "args"   /* command-line args */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "digo"   /* disable IGO       */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_GAMEPROPERTIES_LANG_CHANGES\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_LANG_CHANGES)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "LANG", "CHNG")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"   /* product ID        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "lang"   /* language */)\
    TELMEVENTDEF_END()

#define TELMEVENT_METRIC_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "DOWL", "EXTR")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "mast"   /* master title ID        */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,  "enab"   /* enabled */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "cntx"   /* context */)\
    TELMEVENTDEF_END()


//------------------------------------------------------------------------------
// GAME PROPERTIES: updates available on logout
//------------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "UPAV", "ENDS")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cont"   /* number of games requiring update at logout       */)\
    TELMEVENTDEF_END()


//------------------------------------------------------------------------------
// GAME PROPERTIES: add on description expanded
//------------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAMEPROPERTIES_ADDON_DESCR_EXPANDED\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_ADDON_DESCR_EXPANDED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "ADDO", "EXPD")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"   /* the id of the addon */)\
    TELMEVENTDEF_END()

//------------------------------------------------------------------------------
// GAME PROPERTIES: add on buy clicked
//------------------------------------------------------------------------------
#define TELMEVENT_METRIC_GAMEPROPERTIES_ADDON_BUY_CLICK\
    TELMEVENTDEF_START(METRIC_GAMEPROPERTIES_ADDON_BUY_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("GMPR", "ADDO", "BUYC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"   /* the offer id of the addon */)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  ORIGIN DEVELOPER TOOL (ODT)
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ODT:  Launch
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_LAUNCH\
    TELMEVENTDEF_START(METRIC_ODT_LAUNCH)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "TOOL", "LNCH")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Apply Changes
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_APPLY_CHANGES\
    TELMEVENTDEF_START(METRIC_ODT_APPLY_CHANGES)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "APLY", "CHNG")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Modify an override
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_OVERRIDE_MODIFIED\
    TELMEVENTDEF_START(METRIC_ODT_OVERRIDE_MODIFIED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "OVRD", "MDFY")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "sect"    /* section of the modified setting */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "stng"    /* name of the modified setting */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "valu"    /* new setting value */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Press a button
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_BUTTON_PRESSED\
    TELMEVENTDEF_START(METRIC_ODT_BUTTON_PRESSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "BUTN", "PRES")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "name"    /* name of the pressed button */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  NAVIGATION:  Open url in browser
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_NAVIGATE_URL\
    TELMEVENTDEF_START(METRIC_ODT_NAVIGATE_URL)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "HTTP", "URLX")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "urlx"    /* URL navigated to */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Restart the client
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_RESTART_CLIENT\
    TELMEVENTDEF_START(METRIC_ODT_RESTART_CLIENT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "RSTR", "CLNT")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Telemetry viewer was opened
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_TELEMETRY_VIEWER_OPENED\
    TELMEVENTDEF_START(METRIC_ODT_TELEMETRY_VIEWER_OPENED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "TELM", "OPEN")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  ODT:  Telemetry viewer was closed
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_ODT_TELEMETRY_VIEWER_CLOSED\
    TELMEVENTDEF_START(METRIC_ODT_TELEMETRY_VIEWER_CLOSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("ODTL", "TELM", "CLOS")\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  LOCALHOST
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  LOCALHOST:  SSO Started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOCALHOST_AUTH_SSO_START\
    TELMEVENTDEF_START(METRIC_LOCALHOST_AUTH_SSO_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("LHST", "AUTH", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "type"    /* type - authcode or authtoken */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "orgn"    /* origin - url of domain that we are sso-ing from */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "uuid"    /* uuid of request*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  LOCALHOST:  Localhost started
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOCALHOST_SERVER_START\
    TELMEVENTDEF_START(METRIC_LOCALHOST_SERVER_START)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("LHST", "SRVR", "STRT")\
    TELMEVENTDEF_ATTR(TYPEID_U32,   "port"     /* port we started the server on    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32,   "succ"     /* server start success (1)/fail(0)    */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  LOCALHOST:  Localhost stopped
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOCALHOST_SERVER_STOP\
    TELMEVENTDEF_START(METRIC_LOCALHOST_SERVER_STOP)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("LHST", "SRVR", "STOP")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  LOCALHOST:  Localhost disabled
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOCALHOST_SERVER_DISABLED\
    TELMEVENTDEF_START(METRIC_LOCALHOST_SERVER_DISABLED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("LHST", "SRVR", "DSBL")\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  LOCALHOST:  Localhost service security failure
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_LOCALHOST_SECURITY_FAILURE\
    TELMEVENTDEF_START(METRIC_LOCALHOST_SECURITY_FAILURE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("LHST", "SSCR", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "reas"    /* failure reason */)\
    TELMEVENTDEF_END()


////////////////////////////////////////////////////////////////////////////////
// HOVERCARD
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// HOVERCARD: BUY NOW CLICK
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_BUYNOW_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_BUYNOW_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "BUYN", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// HOVERCARD: BUY Click
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_BUY_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_BUY_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "BUYH", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()
//----------------------------------------------------------------------------
// HOVERCARD: Download click
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_DOWNLOAD_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_DOWNLOAD_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "DWLD", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()
//----------------------------------------------------------------------------
// HOVERCARD: Details Click
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_DETAILS_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_DETAILS_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "DETL", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()
//----------------------------------------------------------------------------
// HOVERCARD: Achievements CLICK
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_ACHIEVEMENTS_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_ACHIEVEMENTS_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "ACHV", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// HOVERCARD: UPDATE CLICK
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_HOVERCARD_UPDATE_CLICK\
    TELMEVENTDEF_START(METRIC_HOVERCARD_UPDATE_CLICK)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("HVCD", "UPDT", "CLCK")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* the game that initiated this click*/)\
    TELMEVENTDEF_END()


////////////////////////////////////////////////////////////////////////////////
// PINNED WIDGETS
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// PINNED WIDGETS: STATE CHANGED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PINNED_WIDGETS_STATE_CHANGED\
    TELMEVENTDEF_START(METRIC_PINNED_WIDGETS_STATE_CHANGED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PINW", "STAT", "CHND")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id		*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "wiid"    /* widget id			*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "pist"    /* pinnedState		*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "fsdk"    /* from sdk			*/)\
    TELMEVENTDEF_END()


//----------------------------------------------------------------------------
// PINNED WIDGETS: HOTKEY TOGGLED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PINNED_WIDGETS_HOTKEY_TOGGLED\
    TELMEVENTDEF_START(METRIC_PINNED_WIDGETS_HOTKEY_TOGGLED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PINW", "STAT", "TOGL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id		*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "visb"    /* visible			*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "fsdk"    /* from sdk			*/)\
    TELMEVENTDEF_END()


//----------------------------------------------------------------------------
// PINNED WIDGETS: STATE CHANGED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PINNED_WIDGETS_COUNT\
    TELMEVENTDEF_START(METRIC_PINNED_WIDGETS_COUNT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PINW", "STAT", "CONT")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* content id		            */)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "numb"    /* max. pinned widgets			*/)\
    TELMEVENTDEF_ATTR(TYPEID_U16, "fsdk"    /* from sdk			            */)\
    TELMEVENTDEF_END()


//----------------------------------------------------------------------------
// PLUGIN: LOAD SUCCESSFUL
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PLUGIN_LOAD_SUCCESS\
    TELMEVENTDEF_START(METRIC_PLUGIN_LOAD_SUCCESS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PLGN", "LOAD", "SUCC")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* product id		                            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "plvr"    /* Plugin version (read from binary)             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ccvr"    /* Compatible client version (read from binary)  */)\
    TELMEVENTDEF_END()


//----------------------------------------------------------------------------
// PLUGIN: LOAD FAILED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PLUGIN_LOAD_FAILED\
    TELMEVENTDEF_START(METRIC_PLUGIN_LOAD_FAILED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PLGN", "LOAD", "FAIL")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* product id		                            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "plvr"    /* Plugin version (read from binary)             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ccvr"    /* Compatible client version (read from binary)  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "erro"   /* Error code                                    */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "esys"   /* System error code (if any)                    */)\
    TELMEVENTDEF_END()


//----------------------------------------------------------------------------
// PLUGIN: PLUG-IN OPERATION
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_PLUGIN_OPERATION\
    TELMEVENTDEF_START(METRIC_PLUGIN_OPERATION)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("PLGN", "OPER", "OPER")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* product id		                            */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "plvr"    /* Plugin version (read from binary)             */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ccvr"    /* Compatible client version (read from binary)  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "oper"    /* plug-in operation                             */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"   /* Operation exit code (0 = success)             */)\
    TELMEVENTDEF_END()

////////////////////////////////////////////////////////////////////////////////
// DOWNLOAD QUEUED
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE ENTITLEMENT COUNT
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_ENTITLMENT_COUNT\
    TELMEVENTDEF_START(METRIC_QUEUE_ENTITLMENT_COUNT)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "STAT", "CONT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "numb"    /* number of queue entitlements     */)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE WINDOW OPENED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_WINDOW_OPENED\
    TELMEVENTDEF_START(METRIC_QUEUE_WINDOW_OPENED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "WIND", "OPEN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* Context that the window as opened */)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE ORDER CHANGED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_ORDER_CHANGED\
    TELMEVENTDEF_START(METRIC_QUEUE_ORDER_CHANGED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "ORDR", "CHNG")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "olhd"    /* Old head product id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "nwhd"    /* Old head product id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context that the order was changed   */)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: ITEM REMOVED
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_ITEM_REMOVED\
    TELMEVENTDEF_START(METRIC_QUEUE_ITEM_REMOVED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "ITEM", "REMO")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* offer id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "wshd"    /* Was head of the queue  */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context event happened   */)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE CALLOUT SHOWN
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_CALLOUT_SHOWN\
    TELMEVENTDEF_START(METRIC_QUEUE_CALLOUT_SHOWN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "CALO", "SHOW")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* product id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "cntx"    /* context event happened   */)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE HEAD BUSY WANRING SHOWN
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_HEAD_BUSY_WARNING_SHOWN\
    TELMEVENTDEF_START(METRIC_QUEUE_HEAD_BUSY_WARNING_SHOWN)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "BUSY", "WARN")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "head"    /* head product id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "hdop"    /* head operation   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "ctid"    /* blocked entitlement product id   */)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "beop"    /* blocked entitlement operation   */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
// DOWNLOAD QUEUE: HTTP Request ERROR
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_QUEUE_GUI_HTTP_ERROR\
    TELMEVENTDEF_START(METRIC_QUEUE_GUI_HTTP_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("QUEU", "MGRQ", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_I32, "qnet"    /* QNetworkReply error  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* HTTP status code     */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "cach"    /* Cache control        */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "link"    /* Error URL            */)\
    TELMEVENTDEF_END()

////////////////////////////////////////////////////////////////////////////////
// FEATURE CALLOUTS
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// FEATURE CALLOUTS: Callout dismissed
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FEATURE_CALLOUT_DISMISSED\
    TELMEVENTDEF_START(METRIC_FEATURE_CALLOUT_DISMISSED)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("FEAT", "CALL", "DISM")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srs"    /* series name		*/)\
    TELMEVENTDEF_ATTR(TYPEID_S8, "objn"    /* object name			*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rslt"    /* result		*/)\
    TELMEVENTDEF_END()

//----------------------------------------------------------------------------
// FEATURE CALLOUTS: series complete
//----------------------------------------------------------------------------
#define TELMEVENT_METRIC_FEATURE_CALLOUT_SERIES_COMPLETE\
    TELMEVENTDEF_START(METRIC_FEATURE_CALLOUT_SERIES_COMPLETE)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("FEAT", "CALL", "COMP")\
    TELMEVENTDEF_ATTR(TYPEID_S8, "srs"    /* series name		*/)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "rslt"    /* result		*/)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CATALOG DEFINITIONS:  Client encountered an error while looking up a catalog definition
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CATALOG_DEFINTION_LOOKUP_ERROR\
    TELMEVENTDEF_START(METRIC_CATALOG_DEFINTION_LOOKUP_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CATD", "LKUP", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Product ID           */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "qnet"    /* QNetworkReply error  */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "http"    /* HTTP status code     */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "pubo"    /* Public offer?        */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CATALOG DEFINITIONS:  Client encountered an error while parsing up a catalog definition
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CATALOG_DEFINTION_PARSE_ERROR\
    TELMEVENTDEF_START(METRIC_CATALOG_DEFINTION_PARSE_ERROR)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CATD", "PARS", "ERRO")\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctid"    /* Product ID       */)\
    TELMEVENTDEF_ATTR(TYPEID_S8,  "ctxt"    /* Error context    */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cach"    /* Data from cache? */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "pubo"    /* Public offer?        */)\
    TELMEVENTDEF_ATTR(TYPEID_I32, "code"    /* Error code       */)\
    TELMEVENTDEF_END()

//-----------------------------------------------------------------------------
//  CATALOG DEFINITIONS:  CDN lookup stats for last time period
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_CATALOG_DEFINTION_CDN_LOOKUP_STATS\
    TELMEVENTDEF_START(METRIC_CATALOG_DEFINTION_CDN_LOOKUP_STATS)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("CATD", "CDNL", "STAT")\
    TELMEVENTDEF_ATTR(TYPEID_U32, "succ"    /* Number of successful lookups */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "errc"    /* Number of failed lookups */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "cnfc"    /* Number of confidential lookups */)\
    TELMEVENTDEF_ATTR(TYPEID_U32, "uncc"    /* Number of lookups resulting in 304 Not Modified */)\
    TELMEVENTDEF_END()


///////////////////////////////////////////////////////////////////////////////
//  SUBSCRIPTION TRIAL
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  Subscription Info
//-----------------------------------------------------------------------------
#define TELMEVENT_METRIC_SUBSCRIPTION_INFO\
    TELMEVENTDEF_START(METRIC_SUBSCRIPTION_INFO)\
    TELMEVENTDEF_CRITICAL(false)\
    TELMEVENTDEF_MGS("SUBS", "INFO", "MACH")\
    TELMEVENTDEF_ATTR(TYPEID_I32 , "Trial"    /* Is this a trial subscription */)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "SbId"     /* The subscription Id.*/)\
    TELMEVENTDEF_ATTR(TYPEID_S8 , "mach"     /* The Machine Id.*/)\
    TELMEVENTDEF_END()

///////////////////////////////////////////////////////////////////////////////
//  MACRO EXPANSION
//  Generates const TelemetryDataFormatTypes METRIC_GUI_MYGAMEVIEWSTATEEXIT[] = 
//      DEFINE_EXTERNAL_METRIC_ATTRIBUTES(
//          TYPEID_S8,  /* view: 0-"grid", 1-"list" */
//          TYPEID_S8   /* client version number */);
///////////////////////////////////////////////////////////////////////////////

#define TELMEVENTDEF_START(name)\
    const TelemetryDataFormatTypes name[] = { TYPEID_U32, TYPEID_U32,
#define TELMEVENTDEF_CRITICAL(critical)
#define TELMEVENTDEF_MGS(m, g, s)
#define TELMEVENTDEF_ATTR(attrtype, attrname)\
    attrtype,
#define TELMEVENTDEF_END()\
    TYPEID_END };

#define TELEMETRYMETRICDEF(metric) TELMEVENT_ ## metric
    TELEMETRYMETRICDEFS
#undef TELEMETRYMETRICDEF

#undef TELMEVENTDEF_START
#undef TELMEVENTDEF_CRITICAL
#undef TELMEVENTDEF_MGS
#undef TELMEVENTDEF_ATTR
#undef TELMEVENTDEF_END


///////////////////////////////////////////////////////////////////////////////
//  MACRO EXPANSION
//  Generates const char8_t* GOS_METRIC_GUI_MYGAMEVIEWSTATEEXIT[] = 
//      {"GUIX", "MGVS", "ENDS", "view", 0};
///////////////////////////////////////////////////////////////////////////////

#define TELMEVENTDEF_START(name)\
    const char8_t* GOS_ ## name[] = {
#define TELMEVENTDEF_CRITICAL(critical)
#define TELMEVENTDEF_MGS(m, g, s)\
    m, g, s,
#define TELMEVENTDEF_ATTR(attrtype, attrname)\
    attrname,
#define TELMEVENTDEF_END()\
    0 };

#define TELEMETRYMETRICDEF(metric) TELMEVENT_ ## metric
    TELEMETRYMETRICDEFS
#undef TELEMETRYMETRICDEF

#undef TELMEVENTDEF_START
#undef TELMEVENTDEF_CRITICAL
#undef TELMEVENTDEF_MGS
#undef TELMEVENTDEF_ATTR
#undef TELMEVENTDEF_END


///////////////////////////////////////////////////////////////////////////////
//  MACRO EXPANSION
//  Generates const bool TelemetryCritical[] = { true, false, ... };
///////////////////////////////////////////////////////////////////////////////

#define TELMEVENTDEF_START(name)
#define TELMEVENTDEF_CRITICAL(critical) critical,
#define TELMEVENTDEF_MGS(m, g, s)
#define TELMEVENTDEF_ATTR(attrtype, attrname)
#define TELMEVENTDEF_END()

const bool TelemetryCritical[] = 
{
    #define TELEMETRYMETRICDEF(metric)  TELMEVENT_ ## metric
        TELEMETRYMETRICDEFS
    #undef TELEMETRYMETRICDEF
};

#undef TELMEVENTDEF_START
#undef TELMEVENTDEF_CRITICAL
#undef TELMEVENTDEF_MGS
#undef TELMEVENTDEF_ATTR
#undef TELMEVENTDEF_END


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

const char8_t **METRIC_LIST[] = 
{ 
    #define TELEMETRYMETRICDEF(metric) GOS_ ## metric,
        TELEMETRYMETRICDEFS
    #undef TELEMETRYMETRICDEF

    0 
};

#pragma pack(pop)


