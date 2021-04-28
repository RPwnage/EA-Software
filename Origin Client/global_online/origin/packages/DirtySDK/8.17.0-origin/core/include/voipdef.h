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

/*** Include files ********************************************************************************/

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

//! VoIP remote status flags (use VoipRemote to query)
#define VOIP_REMOTE_CONNECTED       (1)     //!< UDP connection was set up OK
#define VOIP_REMOTE_RECVVOICE       (2)     //!< voice packets being recieved
#define VOIP_REMOTE_BROADCONN       (4)     //!< is connection broadband?
#define VOIP_REMOTE_ACTIVE          (8)     //!< both sides have headsets & broadband and are exchanging voice traffic
#define VOIP_REMOTE_HEADSETOK       (16)    //!< the remote user's headset is setup
#define VOIP_REMOTE_DISCONNECTED    (128)   //!< remote side has disconnected

//! VoIP local status flags (use VoipLocal to query)
#define VOIP_LOCAL_MICON            (1)     //!< microphone is on
#define VOIP_LOCAL_BROADBAND        (2)     //!< active interface is broadband
#define VOIP_LOCAL_HEADSETOK        (4)     //!< a compatible headset is connected and successfully initialized
                                            //!< on PC: can only be set when both VOIP_LOCAL_INPUTDEVICEOK and VOIP_LOCAL_OUTPUTDEVICEOK are set
#define VOIP_LOCAL_TALKING          (8)     //!< local user is talking (will be always set for codecs without VAD)
#define VOIP_LOCAL_SENDVOICE        (16)    //!< voice packets being sent
#if DIRTYCODE_PLATFORM == DIRTYCODE_PC
#define VOIP_LOCAL_INPUTDEVICEOK    (32)    //!< user selected an input voice device
#define VOIP_LOCAL_OUTPUTDEVICEOK   (64)    //!< user selected an output voice device
#endif

#if DIRTYCODE_PLATFORM != DIRTYCODE_XENON
//! maximum number of local users on a single host
#define VOIP_MAXLOCALUSERS (1)
#else
//! maximum number of local users on a single host
#define VOIP_MAXLOCALUSERS (4)
#endif

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
    VOIP_CBTYPE_SENDEVENT,          //<! VoipLocal() & VOIP_LOCAL_SENDVOICE change
    VOIP_CBTYPE_CHANEVENT           //<! Notification of remote channel selection change
} VoipCbTypeE;


/*** Macros ***************************************************************************************/

#define VOIP_ConnMask(_iConnID, _bEnable) ((_bEnable) << (_iConnID))

/*** Type Definitions *****************************************************************************/

//! opaque module ref
typedef struct VoipRefT VoipRefT;

//! event callback function prototype
typedef void (VoipCallbackT)(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData);

//! speaker data callback (only supported on some platforms)
typedef void (VoipSpkrCallbackT)(int16_t *pFrameData, int32_t iNumSamples, void *pUserData);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#endif // _voipdef_h

