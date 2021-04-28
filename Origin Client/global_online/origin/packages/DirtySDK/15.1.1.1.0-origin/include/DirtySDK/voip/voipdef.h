/*H*************************************************************************************************/
/*!

    \File    voipdef.h

    \Description
        Definitions shared between Voip EE library and Voip IRX module.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2004.    ALL RIGHTS RESERVED.

    \Version    1.0        11/03/03 (JLB) First Version
    \Version    1.5        03/02/04 (JLB) VoIP 2.0 - API changes for future multiple channel support.

*/
/*************************************************************************************************H*/

#ifndef _voipdef_h
#define _voipdef_h

/*!
\Moduledef VoipDef VoipDef
\Modulemember Voip
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **************************************************************************************/

//! current version of VoIP (maj.min.rev.patch)
#define VOIP_VERSION                (0x04000000)

//! suggested VOIP port
#define VOIP_PORT                   (6000)

//! special connection identifiers
#define VOIP_CONNID_NONE            (-1)
#define VOIP_CONNID_ALL             (-2)

//! maximum number of supported connections
#define VOIP_MAXCONNECT             (30)

//! VoIP conn status flags (use VoipConnStatus to query)
#define VOIP_CONN_CONNECTED       (1)     //!< UDP connection was set up OK   
#define VOIP_CONN_BROADCONN       (2)     //!< is connection broadband?
#define VOIP_CONN_ACTIVE          (4)     //!< both sides have headsets & broadband and are exchanging voice traffic
#define VOIP_CONN_STOPPED         (8)     //!< connection attempt stopped

//! VoIP remote status flags (use VoipRemoteUserStatus to query)
#define VOIP_REMOTE_USER_RECVVOICE       (1)     //!< voice packets being recieved   
#define VOIP_REMOTE_USER_HEADSETOK       (2)     //!< the remote user's headset is setup

//! VoIP local status flags (use VoipLocalUserStatus to query)
#define VOIP_LOCAL_USER_MICON            (1)     //!< microphone is on
#define VOIP_LOCAL_USER_HEADSETOK        (2)     //!< a compatible headset is connected and successfully initialized
                                                 //!< on PC: can only be set when both VOIP_LOCAL_USER_INPUTDEVICEOK and VOIP_LOCAL_USER_OUTPUTDEVICEOK are set
#define VOIP_LOCAL_USER_TALKING          (4)     //!< local user is talking (will be always set for codecs without VAD)
#define VOIP_LOCAL_USER_SENDVOICE        (8)     //!< voice packets being sent
#define VOIP_LOCAL_USER_INPUTDEVICEOK    (16)    //!< user selected an input voice device
#define VOIP_LOCAL_USER_OUTPUTDEVICEOK   (32)    //!< user selected an output voice device

//! Invalid shared user index 
#define VOIP_INVALID_SHARED_USER_INDEX (0xFF)

//! maximum number of local users on a single host
#if defined(DIRTYCODE_PS4)
#define VOIP_MAXLOCALUSERS (4)
#define VOIP_SHARED_USER_INDEX VOIP_MAXLOCALUSERS
#define VOIP_MAXLOCALUSERS_EXTENDED (VOIP_MAXLOCALUSERS + 1)
#elif defined(DIRTYCODE_XENON)
#define VOIP_MAXLOCALUSERS (4)
#define VOIP_SHARED_USER_INDEX VOIP_INVALID_SHARED_USER_INDEX
#define VOIP_MAXLOCALUSERS_EXTENDED (VOIP_MAXLOCALUSERS)
#elif defined(DIRTYCODE_XBOXONE)
#define VOIP_MAXLOCALUSERS (16)
#define VOIP_SHARED_USER_INDEX VOIP_MAXLOCALUSERS
#define VOIP_MAXLOCALUSERS_EXTENDED (VOIP_MAXLOCALUSERS + 1)
#else
#define VOIP_MAXLOCALUSERS (1)
#define VOIP_SHARED_USER_INDEX VOIP_INVALID_SHARED_USER_INDEX
#define VOIP_MAXLOCALUSERS_EXTENDED (VOIP_MAXLOCALUSERS)
#endif

#define VOIP_MAX_CONCURRENT_CHANNEL (4)
#define VOIP_MAX_LOW_LEVEL_CONNS    (32)
#define VOIP_MAXLOCALCHANNEL        (64)

//! VoIP invalid flag
#define VOIP_FLAG_INVALID           (0x80000000)

// VoIP shutdown flag
#define VOIP_SHUTDOWN_THREADSTARVE  (NET_SHUTDOWN_THREADSTARVE)  //!< special shutdown mode for PS3 that starves threads, allowing for quick exit to XMB

//! VOIP profiling stat enums
typedef enum VoipProfStatE
{
    VOIP_PROFSTAT_RECORD = 0,       //!< number of cycles last spent recording data
    VOIP_PROFSTAT_COMPRESS,         //!< number of cycles last spent compressing data
    VOIP_PROFSTAT_DECOMPRESS,       //!< number of cycles last spent decompressing data
    VOIP_PROFSTAT_PLAY,             //!< average number of cycles spent playing data

    VOIP_NUMPROFSTATS
} VoipProfStatE;

//! callback event types
typedef enum VoipCbTypeE
{
    VOIP_CBTYPE_HSETEVENT,          //<! a headset has been inserted or removed
    VOIP_CBTYPE_FROMEVENT,          //<! VoipStatus('from') change
    VOIP_CBTYPE_SENDEVENT,          //<! VoipLocal() & VOIP_LOCAL_USER_SENDVOICE change
} VoipCbTypeE;


/*** Macros ***************************************************************************************/

#define VOIP_ConnMask(_iConnID, _bEnable) ((_bEnable) << (_iConnID))

/*** Type Definitions *****************************************************************************/

//! The mode a given channel is used in
typedef enum VoipChanModeE
{
    VOIP_CHANNONE = 0, //<! (provided as a helper and so debuggers show the proper enum value.)
    VOIP_CHANSEND = 1, //<! Indicates a channel is used for sending
    VOIP_CHANRECV = 2, //<! Indicates a channel is used for receiving
    VOIP_CHANSENDRECV = (VOIP_CHANSEND | VOIP_CHANRECV) //<! (also has a helper)
} VoipChanModeE;

//! opaque module ref
typedef struct VoipRefT VoipRefT;

//! event callback function prototype
DIRTYCODE_API typedef void (VoipCallbackT)(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData);

//! speaker data callback (only supported on some platforms)
DIRTYCODE_API typedef void (VoipSpkrCallbackT)(int16_t *pFrameData, int32_t iNumSamples, void *pUserData);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

//@}

#endif // _voipdef_h

