/*H********************************************************************************/
/*!
    \File voipheadsetpc.c

    \Description
        VoIP headset manager.

    \Copyright
        Copyright Electronic Arts 2004-2011.

    \Notes
        LPCM is supported only in loopback mode.

    \Version 1.0 03/30/2004 (jbrookes)  First Version
    \Version 2.0 10/20/2011 (jbrookes)  Major rewrite for cleanup and to fix I/O bugs
    \Version 3.0 07/09/2012 (akirchner) Added functionality to play buffer
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <string.h>
#include <mmsystem.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipconnection.h"
#include "voippacket.h"
#include "voipmixer.h"
#include "voipconduit.h"
#include "DirtySDK/voip/voipcodec.h"
#include "voipheadset.h"

#include "voipdvi.h"
#include "voippcm.h"

/*** Defines **********************************************************************/

// Defines taken from mmddk.h - not including mmddk directly as that would
// add a dependancy on the Windows DDK
#define DRVM_MAPPER                     (0x2000)
#define DRVM_MAPPER_CONSOLEVOICECOM_GET (DRVM_MAPPER + 23)

#define VOIP_HEADSET_SAMPLEWIDTH        (2)                     //!< sample size (16-bit samples)
#define VOIP_HEADSET_FRAMESAMPLES       (160)                   //!< frame size in samples (160 samples = 20ms (8khz) 14.5ms (11.025khz) or 10ms (16khz))
#define VOIP_HEADSET_FRAMESIZE          (VOIP_HEADSET_FRAMESAMPLES*VOIP_HEADSET_SAMPLEWIDTH) //!< frame size in bytes
#define VOIP_HEADSET_NUMWAVEBUFFERS     (5)
#define VOIP_HEADSET_MAXDEVICES         (8)
#define VOIP_HEADSET_WAVEFORMAT         (WAVE_FORMAT_1M16)
#define VOIP_HEADSET_BITRATE            (11025)
#define VOIP_HEADSET_PREBUFFERBLOCKS    (4)

#if defined(_WIN64)
    #define VOIP_HEADSET_WAVE_MAPPER ((uint64_t)-1)
#else
    #define VOIP_HEADSET_WAVE_MAPPER (WAVE_MAPPER)
#endif

//! headset types (input, output)
typedef enum VoipHeadsetDeviceTypeE
{
    VOIP_HEADSET_INPDEVICE,
    VOIP_HEADSET_OUTDEVICE
} VoipHeadsetDeviceTypeE;


/*** Macros ************************************************************************/

/*** Type Definitions **************************************************************/

//! playback data
typedef struct VoipHeadsetWaveDataT
{
    WAVEHDR WaveHdr;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];
} VoipHeadsetWaveDataT;

//! wave caps structure; this is a combination of the in and out caps, which are identical other than dwSupport (output only)
typedef struct VoipHeadsetWaveCapsT
{
    WORD    wMid;                  //!< manufacturer ID
    WORD    wPid;                  //!< product ID
    MMVERSION vDriverVersion;      //!< version of the driver
    CHAR    szPname[MAXPNAMELEN];  //!< product name (NULL terminated string)
    DWORD   dwFormats;             //!< formats supported
    WORD    wChannels;             //!< number of sources supported
    WORD    wReserved1;            //!< packing
    DWORD   dwSupport;             //!< functionality supported by driver
} VoipHeadsetWaveCapsT;

//! device info
typedef struct VoipHeadsetDeviceInfoT
{
    VoipHeadsetDeviceTypeE eDevType;    //!< device type

    HANDLE  hDevice;                    //!< handle to currently open output device, or NULL if no device is open
    int32_t iActiveDevice;              //!< device index of active device
    int32_t iDeviceToOpen;              //!< device index of device we want to open, or -1 for any

    int32_t iCurVolume;                 //!< playback volume (output devices only)
    int32_t iNewVolume;                 //!< new volume to set (if different from iCurVolume)

    VoipHeadsetWaveDataT WaveData[VOIP_HEADSET_NUMWAVEBUFFERS]; //!< audio input/output buffer
    int32_t iCurWaveBuffer;             //!< current input/output buffer
    DWORD   dwCheckFlag[VOIP_HEADSET_NUMWAVEBUFFERS];   //!< flag to check for empty buffer (output only)

    VoipHeadsetWaveCapsT WaveDeviceCaps[VOIP_HEADSET_MAXDEVICES];
    int32_t iNumDevices;                //!< number of enumerated output devices

    uint8_t bActive;                    //!< device is active (recording/playing)
    uint8_t bCloseDevice;               //!< device close requested
    uint8_t bChangeDevice;              //!< device change requested
    uint8_t _pad;
} VoipHeadsetDeviceInfoT;

//! VOIP module state data
struct VoipHeadsetRefT
{
    VoipHeadsetDeviceInfoT MicrInfo;      //!< input device info
    VoipHeadsetDeviceInfoT SpkrInfo;      //!< output device info

    // conduit info
    int32_t iMaxConduits;
    VoipConduitRefT *pConduitRef;

    // mixer state
    VoipMixerRefT *pMixerRef;

    // codec frame size in bytes
    int32_t iCmpFrameSize;
    int32_t bParticipating;             //!< local user is now in "participating" state

    // network data status
    volatile uint8_t bMicOn;            //!< TRUE if the mic is on, else FALSE
    uint8_t uSendSeq;
    uint8_t bLoopback;
    uint8_t _pad;

    // user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;
    VoipHeadsetTextDataCbT *pTextDataCb;
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    // speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;
    #endif

    // critical section for guarding access to device close/change flags
    NetCritT DevChangeCrit;

    // player
    int32_t iPlayerActive;
    int16_t *pPlayerBuffer;
    uint32_t uPlayerBufferFrameCurrent;
    uint32_t uPlayerBufferFrames;
    uint32_t uPlayerFirstTime;
    int32_t iPlayerFirstUse;
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveGetNumDevs

    \Description
        Wraps wave(In|Out)GetNumDevs()

    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        UINT            - wave(In|Out)GetNumDevs() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static UINT _VoipHeadsetWaveGetNumDevs(VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInGetNumDevs());
    }
    else
    {
        return(waveOutGetNumDevs());
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveGetDevCaps

    \Description
        Wraps wave(In|Out)GetDevCaps()

    \Input uDeviceID    - device to get capabilities of
    \Input *pWaveCaps   - wave capabilities (note this structure does double-duty for input and output)
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)GetNumDevs() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveGetDevCaps(UINT_PTR uDeviceID, VoipHeadsetWaveCapsT *pWaveCaps, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInGetDevCaps(uDeviceID, (LPWAVEINCAPSA)pWaveCaps, sizeof(WAVEINCAPSA)));
    }
    else
    {
        return(waveOutGetDevCaps(uDeviceID, (LPWAVEOUTCAPSA)pWaveCaps, sizeof(WAVEOUTCAPSA)));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveOpen

    \Description
        Wraps wave(In|Out)Open()

    \Input *pHandle     - [out] storage for handle to opened device
    \Input uDeviceID    - device to open
    \Input *pWaveFormat - wave format we want
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Open() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveOpen(HANDLE *pHandle, UINT uDeviceID, LPCWAVEFORMATEX pWaveFormat, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInOpen((LPHWAVEIN)pHandle, uDeviceID, pWaveFormat, 0, 0, 0));
    }
    else
    {
        return(waveOutOpen((LPHWAVEOUT)pHandle, uDeviceID, pWaveFormat, 0, 0, 0));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveClose

    \Description
        Wraps wave(In|Out)Close()

    \Input hHandle      - handle to device to close
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Close() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveClose(HANDLE hHandle, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInClose((HWAVEIN)hHandle));
    }
    else
    {
        return(waveOutClose((HWAVEOUT)hHandle));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWavePrepareHeader

    \Description
        Wraps wave(In|Out)PrepareHeader()

    \Input hHandle      - handle to device to prepare wave header for
    \Input pWaveHeader  - wave header to prepare
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)PrepareHeader() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWavePrepareHeader(HANDLE hHandle, LPWAVEHDR pWaveHeader, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInPrepareHeader((HWAVEIN)hHandle, pWaveHeader, sizeof(WAVEHDR)));
    }
    else
    {
        return(waveOutPrepareHeader((HWAVEOUT)hHandle, pWaveHeader, sizeof(WAVEHDR)));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveUnprepareHeader

    \Description
        Wraps wave(In|Out)UnprepareHeader()

    \Input hHandle      - handle to device to prepare wave header for
    \Input pWaveHeader  - wave header to unprepare
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)UnprepareHeader() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveUnprepareHeader(HANDLE hHandle, LPWAVEHDR pWaveHeader, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInUnprepareHeader((HWAVEIN)hHandle, pWaveHeader, sizeof(WAVEHDR)));
    }
    else
    {
        return(waveOutUnprepareHeader((HWAVEOUT)hHandle, pWaveHeader, sizeof(WAVEHDR)));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveReset

    \Description
        Wraps wave(In|Out)Reset()

    \Input hHandle      - handle to device to prepare wave header for
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Reset() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveReset(HANDLE hHandle, VoipHeadsetDeviceTypeE eDevType)
{
    if (eDevType == VOIP_HEADSET_INPDEVICE)
    {
        return(waveInReset((HWAVEIN)hHandle));
    }
    else
    {
        return(waveOutReset((HWAVEOUT)hHandle));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPrepareWaveHeaders

    \Description
        Prepare wave headers for use.

    \Input *pDeviceInfo - device info
    \Input iNumBuffers - number of buffers in array
    \Input hDevice     - handle to wave device

    \Output
        int32_t         - negative=failure, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetPrepareWaveHeaders(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iNumBuffers, HANDLE hDevice)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        // set up the wave header
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.lpData = (LPSTR)pDeviceInfo->WaveData[iPlayBuf].FrameData;
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwBufferLength = sizeof(pDeviceInfo->WaveData[iPlayBuf].FrameData);
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwFlags = 0L;
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwLoops = 0L;

        // prepare the header
        if ((hResult = _VoipHeadsetWavePrepareHeader(hDevice, &pDeviceInfo->WaveData[iPlayBuf].WaveHdr, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
        {
            NetPrintf(("voipheadsetpc: error %d preparing %s buffer %d\n", hResult,
                pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iPlayBuf));
            return(-1);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUnprepareWaveHeaders

    \Description
        Unprepare wave headers, prior to releasing them.

    \Input *pDeviceInfo - device info
    \Input iNumBuffers - number of buffers in array
    \Input hDevice     - handle to wave device

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetUnprepareWaveHeaders(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iNumBuffers, HANDLE hDevice)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        if ((hResult = _VoipHeadsetWaveUnprepareHeader(hDevice, &pDeviceInfo->WaveData[iPlayBuf].WaveHdr, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
        {
            NetPrintf(("voipheadsetpc: error %d unpreparing %s buffer %d\n", hResult,
                pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iPlayBuf));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetOpenDevice

    \Description
        Open a wave device.

    \Input *pDeviceInfo - device state
    \Input iDevice      - handle to wave device (may be WAVE_MAPPER)
    \Input iNumBuffers  - number of input buffers

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetOpenDevice(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iDevice, int32_t iNumBuffers)
{
    WAVEFORMATEX WaveFormatEx = { WAVE_FORMAT_PCM, 1, VOIP_HEADSET_BITRATE, VOIP_HEADSET_BITRATE*2, 2, 16, 0 };
    HANDLE hDevice = NULL;
    HRESULT hResult;

    // open device
    if ((hResult = _VoipHeadsetWaveOpen(&hDevice, iDevice, &WaveFormatEx, pDeviceInfo->eDevType)) != S_OK)
    {
        NetPrintf(("voipheadsetpc: error %d opening input device\n", hResult));
        return(-1);
    }

    // set up input wave data
    if (_VoipHeadsetPrepareWaveHeaders(pDeviceInfo, iNumBuffers, hDevice) >= 0)
    {
        // set volume to -1 so it will be reset
        pDeviceInfo->iCurVolume = -1;

        // set active device
        pDeviceInfo->iActiveDevice = iDevice;

        if (pDeviceInfo->eDevType == VOIP_HEADSET_OUTDEVICE)
        {
            int32_t iBuffer;

            // mark as playing
            pDeviceInfo->bActive = TRUE;

            // set up check flag (we have to check for WHDR_PREPARED until we've written into the buffer)
            for (iBuffer = 0; iBuffer < VOIP_HEADSET_NUMWAVEBUFFERS; iBuffer += 1)
            {
                pDeviceInfo->dwCheckFlag[iBuffer] = WHDR_PREPARED;
            }
        }

        NetPrintf(("voipheadsetpc: opened %s device\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output"));
    }
    else
    {
        _VoipHeadsetWaveClose(hDevice, pDeviceInfo->eDevType);
        hDevice = NULL;
    }

    pDeviceInfo->hDevice = hDevice;
    return((hDevice == NULL) ? -2 : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCloseDevice

    \Description
        Close an open device.

    \Input *pHeadset    - module state
    \Input *pDeviceInfo - device info

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetCloseDevice(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    MMRESULT hResult;
    int32_t iDevice;

    // no open device, nothing to do
    if (pDeviceInfo->hDevice == 0)
    {
        return;
    }

    // reset the device
    if ((hResult = _VoipHeadsetWaveReset(pDeviceInfo->hDevice, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
    {
        NetPrintf(("voipheadsetpc: failed to reset %s device (err=%d)\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "waveIn" : "waveOut", hResult));
    }

    // unprepare the wave headers
    _VoipHeadsetUnprepareWaveHeaders(pDeviceInfo, 2, pDeviceInfo->hDevice);

    // close the device
    if ((hResult = _VoipHeadsetWaveClose(pDeviceInfo->hDevice, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
    {
        NetPrintf(("voipheadsetpc: failed to close %s device (err=%d)\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "waveIn" : "waveOut", hResult));
    }

    // reset device state
    NetPrintf(("voipheadsetpc: closed %s device\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output"));

    pDeviceInfo->hDevice = NULL;
    pDeviceInfo->bActive = FALSE;
    iDevice = pDeviceInfo->iActiveDevice;
    pDeviceInfo->iActiveDevice = -1;

    // trigger device inactive callback
    // user index is always 0 because PC does not support MLU
    pHeadset->pStatusCb(0, FALSE, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? VOIP_HEADSET_STATUS_INPUT : VOIP_HEADSET_STATUS_OUTPUT, pHeadset->pCbUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEnumerateDevices

    \Description
        Enumerate devices attached to system, and add those that are compatible
        to device list.

    \Input  *pHeadset   - headset module state
    \Input  *pDeviceInfo - device info

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetEnumerateDevices(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    int32_t iDevice, iNumDevices, iAddedDevices;
    VoipHeadsetWaveCapsT *pDeviceCaps;
    const char *pActiveDeviceName = NULL;
    int32_t iActiveDevice = -1;

    // get total number of output devices
    iNumDevices = _VoipHeadsetWaveGetNumDevs(pDeviceInfo->eDevType);

    // get the active device name, if a device is active
    if (pDeviceInfo->iActiveDevice != -1)
    {
        iActiveDevice = pDeviceInfo->iActiveDevice;
        pActiveDeviceName = pDeviceInfo->WaveDeviceCaps[iActiveDevice].szPname;
    }

    // walk device list
    for (iDevice = 0, iAddedDevices = 0; (iDevice < iNumDevices) && (iAddedDevices < VOIP_HEADSET_MAXDEVICES); iDevice++)
    {
        // get device capabilities
        pDeviceCaps = &pDeviceInfo->WaveDeviceCaps[iAddedDevices];
        _VoipHeadsetWaveGetDevCaps(iDevice, pDeviceCaps, pDeviceInfo->eDevType);

        // output device info
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc: querying %s device %d\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   wMid=%d\n", pDeviceCaps->wMid));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   wPid=%d\n", pDeviceCaps->wPid));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   vDriverVersion=%d.%d\n", (pDeviceCaps->vDriverVersion&0xff00) >> 8, pDeviceCaps->vDriverVersion&0xff));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   szPname=%s\n", pDeviceCaps->szPname));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   dwFormats=0x%08x\n", pDeviceCaps->dwFormats));
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:   wChannels=%d\n", pDeviceCaps->wChannels));

        // is it compatible?
        if (pDeviceCaps->dwFormats & VOIP_HEADSET_WAVEFORMAT)
        {
            // if this device is our active input device make sure to update our iActiveDevice value;
            if ((iActiveDevice != -1) && (strcmp(pActiveDeviceName, pDeviceCaps->szPname) == 0))
            {
                // if the active input device index changed log the change
                if (iActiveDevice != iAddedDevices)
                {
                    pDeviceInfo->iActiveDevice = iAddedDevices;
                    NetPrintf(("voipheadsetpc: active %s device, %s, moved from index %d to %d\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                        pActiveDeviceName, iActiveDevice, iAddedDevices));
                }

                // already matched don't compare names again
                iActiveDevice = -1;
            }

            NetPrintf(("voipheadsetpc: %s device %d is compatible\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
            iAddedDevices++;
        }
        else
        {
            NetPrintf(("voipheadsetpc: %s device %d is not compatible\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
        }

        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc:\n"));
    }

    // set number of devices
    pDeviceInfo->iNumDevices = iAddedDevices;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetStop

    \Description
        Stop recording & playback, and close USB audio device.

    \Notes
        This function is safe to call regardless of device state.

    \Version 11/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetStop(VoipHeadsetRefT *pHeadset)
{
    _VoipHeadsetCloseDevice(pHeadset, &pHeadset->MicrInfo);
    _VoipHeadsetCloseDevice(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEnumerate

    \Description
        Enumerate all connected audio devices.

    \Input *pHeadset   - headset module state

    \Notes
        Although we only support one headset, we will scan up to eight to try and
        locate one that meets our needs for data format and sample rate.

    \Version 07/27/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetEnumerate(VoipHeadsetRefT *pHeadset)
{
    NetPrintf(("voipheadsetpc: enumerating devices\n"));

    // re-enumerating will kill our existing devices, so clean up appropriately.
    _VoipHeadsetStop(pHeadset);

    // enumerate input devices
    _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->MicrInfo);

    // enumerate output devices
    _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetCodec

    \Description
        Sets the specified codec.

    \Input *pHeadset    - pointer to module state
    \Input iCodecIdent  - codec identifier to set

    \Output
        int32_t         - zero=success, negative=failure

    \Version 10/26/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetCodec(VoipHeadsetRefT *pHeadset, int32_t iCodecIdent)
{
    int32_t iResult;

    // pass through codec creation request
    if ((iResult = VoipCodecCreate(iCodecIdent, pHeadset->iMaxConduits)) < 0)
    {
        return(iResult);
    }

    // query codec output size
    pHeadset->iCmpFrameSize = VoipCodecStatus(iCodecIdent, 'fsiz', VOIP_HEADSET_FRAMESAMPLES, NULL, 0);

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessPlay

    \Description
        Process recording of data from the buffer and sending it to the registered
        mic data user callback.

    \Input *pHeadset - pointer to headset state

    \Version 07/09/2012 (akirchner)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessPlay(VoipHeadsetRefT *pHeadset)
{
    VoipMicrPacketT voipMicrPacketT;
    int32_t         iCompBytes;
    uint32_t        uPlayerLastTime;
    uint32_t        uPackets;
    uint32_t        uPacket;

    if (!pHeadset->iPlayerFirstUse)
    {
        pHeadset->iPlayerFirstUse = 1;
        pHeadset->uPlayerFirstTime = NetTick();
    }

    uPlayerLastTime = NetTick();
    uPackets = (uPlayerLastTime - pHeadset->uPlayerFirstTime) / VOIP_THREAD_SLEEP_DURATION;

    for (uPacket = 0; uPacket < uPackets; uPacket++)
    {
        pHeadset->uPlayerBufferFrameCurrent += VOIP_HEADSET_SAMPLEWIDTH;
        pHeadset->uPlayerBufferFrameCurrent = (pHeadset->uPlayerBufferFrameCurrent >= pHeadset->uPlayerBufferFrames)? 0 : pHeadset->uPlayerBufferFrameCurrent;

        // compress the buffer data, and if there is compressed data send it to the appropriate function
        if ((iCompBytes = VoipCodecEncode(voipMicrPacketT.aData, &pHeadset->pPlayerBuffer[VOIP_HEADSET_FRAMESAMPLES * pHeadset->uPlayerBufferFrameCurrent], VOIP_HEADSET_FRAMESAMPLES)) > 0)
        {
            if (pHeadset->bLoopback == FALSE)
            {
                // send to mic data callback
                pHeadset->pMicDataCb(voipMicrPacketT.aData, iCompBytes,  NULL, 0, 0, pHeadset->uSendSeq++, pHeadset->pCbUserData);
            }
            else
            {
                voipMicrPacketT.MicrInfo.uSubPacketSize = iCompBytes;
                voipMicrPacketT.MicrInfo.uSeqn += 1;
                VoipConduitReceiveVoiceData(pHeadset->pConduitRef, (VoipUserT *)"", voipMicrPacketT.aData, iCompBytes);
            }
        }
    }

    pHeadset->uPlayerFirstTime = uPlayerLastTime;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessRecord

    \Description
        Process recording of data from the mic and sending it to the registered
        mic data user callback.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessRecord(VoipHeadsetRefT *pHeadset)
{
    VoipHeadsetDeviceInfoT *pMicrInfo = &pHeadset->MicrInfo;
    VoipMicrPacketT PacketData;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];      //!< current recorded audio frame
    VoipMicrPacketT *pPacket = &PacketData;
    uint8_t *pData = pPacket->aData;
    int32_t iCompBytes;
    HRESULT hResult;

    // only record if we have a recording device open
    if (pMicrInfo->hDevice == NULL || !pHeadset->bParticipating)
    {
        return;
    }

    // read data from the mic
    if (pMicrInfo->bActive == TRUE)
    {
        if (pHeadset->bMicOn == FALSE)
        {
            // tell hardware to stop recording
            if ((hResult = waveInStop(pMicrInfo->hDevice)) == MMSYSERR_NOERROR)
            {
                // mark as not recording
                NetPrintf(("voipheadsetpc: stop recording\n"));
                pMicrInfo->bActive = FALSE;
            }
            else
            {
                NetPrintf(("voipheadsetpc: error %d trying to stop recording\n", hResult));
            }
        }

        // pull in any waiting data
        for ( ; pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].WaveHdr.dwFlags & WHDR_DONE; )
        {
            // copy audio data out of buffer
            ds_memcpy_s(FrameData, sizeof(FrameData), &pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].FrameData, VOIP_HEADSET_FRAMESIZE);

            // compress the input data, and if there is compressed data send it to the appropriate function
            if ((iCompBytes = VoipCodecEncode(pData, (int16_t *)FrameData, VOIP_HEADSET_FRAMESAMPLES)) > 0)
            {
                if (pHeadset->bLoopback == FALSE)
                {
                    // send to mic data callback
                    pHeadset->pMicDataCb(&pPacket->aData, iCompBytes,  NULL, 0, 0, pHeadset->uSendSeq++, pHeadset->pCbUserData);
                }
                else
                {
                    pPacket->MicrInfo.uSubPacketSize = iCompBytes;
                    pPacket->MicrInfo.uSeqn += 1;
                    VoipConduitReceiveVoiceData(pHeadset->pConduitRef, (VoipUserT *)"", pPacket->aData, iCompBytes);
                }
            }

            // re-queue buffer to read more data
            if ((hResult = waveInAddBuffer(pMicrInfo->hDevice, &pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
            {
                if (hResult == MMSYSERR_NODRIVER)
                {
                    NetPrintf(("voipheadsetpc: waveInAddBuffer returned MMSYSERR_NODRIVER. Headset removed? Closing headset\n"));
                    _VoipHeadsetStop(pHeadset);
                    break;
                }
                else if (hResult != WAVERR_STILLPLAYING)
                {
                    NetPrintf(("voipheadsetpc: could not add buffer %d to record queue (err=%d)\n", pMicrInfo->iCurWaveBuffer, hResult));
                }
            }

            // index to next read buffer
            pMicrInfo->iCurWaveBuffer = (pMicrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
        }
    }
    // if not recording and a mic-on request comes in, start recording
    else if (pHeadset->bMicOn == TRUE)
    {
        int32_t iBuffer;

        if ((hResult = waveInStart(pMicrInfo->hDevice)) == MMSYSERR_NOERROR)
        {
            // mark as recording
            NetPrintf(("voipheadsetpc: recording...\n"));
            pMicrInfo->bActive = TRUE;
        }
        else
        {
            NetPrintf(("voipheadsetpc: error %d trying to start recording\n", hResult));
        }

        // add buffers to start recording
        for (iBuffer = 0; iBuffer < VOIP_HEADSET_NUMWAVEBUFFERS; iBuffer += 1)
        {
            if ((hResult = waveInAddBuffer(pMicrInfo->hDevice, &pMicrInfo->WaveData[iBuffer].WaveHdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
            {
                NetPrintf(("voipheadsetpc: waveInAddBuffer for buffer %d failed err=%d\n", iBuffer, hResult));
            }
        }
        pMicrInfo->iCurWaveBuffer = 0;

        // reset compression state
        VoipCodecReset();
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessPlayback

    \Description
        Process playback of data received from the network to the headset earpiece.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessPlayback(VoipHeadsetRefT *pHeadset)
{
    VoipHeadsetDeviceInfoT *pSpkrInfo = &pHeadset->SpkrInfo;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];      //!< current recorded audio frame
    int32_t iSampBytes;
    HRESULT hResult;

    // only play if we have a playback device open
    if (pSpkrInfo->hDevice == NULL || !pHeadset->bParticipating)
    {
        return;
    }

    // write data to the audio output if it's available
    if (pSpkrInfo->bActive == TRUE)
    {
        int32_t iLoop, iNumBufAvail;

        // update play volume, if requested
        if (pSpkrInfo->iCurVolume != pSpkrInfo->iNewVolume)
        {
            int32_t iVolume;

            iVolume = (pSpkrInfo->iNewVolume * 65535) / 100;
            iVolume |= iVolume << 16;

            if ((hResult = waveOutSetVolume(pSpkrInfo->hDevice, iVolume)) == MMSYSERR_NOERROR)
            {
                NetPrintf(("voipheadsetpc: changed play volume to %d\n", pSpkrInfo->iNewVolume));
                pSpkrInfo->iCurVolume = pSpkrInfo->iNewVolume;
            }
            else
            {
                NetPrintf(("voipheadsetpc: error %d trying to change play volume\n", hResult));
            }
        }

        // count number of buffers available to write into
        for (iLoop = 0, iNumBufAvail = 0; iLoop < VOIP_HEADSET_NUMWAVEBUFFERS; iLoop += 1)
        {
            iNumBufAvail += (pSpkrInfo->WaveData[iLoop].WaveHdr.dwFlags & WHDR_DONE) ? 1 : 0;
        }

        // if we have space to write
        for ( ; pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr.dwFlags & pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer]; )
        {
            // decode and mix buffered packet data
            if ((iSampBytes = VoipMixerProcess(pHeadset->pMixerRef, FrameData)) == VOIP_HEADSET_FRAMESIZE)
            {
                // if there's nothing playing, we want to prebuffer with silence
                if (iNumBufAvail == VOIP_HEADSET_NUMWAVEBUFFERS)
                {
                    uint8_t FrameSilenceData[VOIP_HEADSET_FRAMESIZE];
                    int32_t iBlock;

                    ds_memclr(FrameSilenceData, sizeof(FrameSilenceData));
                    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc: prebuffering %d blocks\n", VOIP_HEADSET_PREBUFFERBLOCKS));
                    for (iBlock = 0; iBlock < VOIP_HEADSET_PREBUFFERBLOCKS; iBlock += 1)
                    {
                        ds_memcpy_s(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData, sizeof(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData), FrameSilenceData, sizeof(FrameSilenceData));
                        if ((hResult = waveOutWrite(pSpkrInfo->hDevice, &pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) == MMSYSERR_NOERROR)
                        {
                            pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer] = WHDR_DONE;
                            pSpkrInfo->iCurWaveBuffer = (pSpkrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
                        }
                        else
                        {
                            NetPrintf(("voipheadsetpc: write failed (error=%d)\n", hResult));
                        }
                    }
                    iNumBufAvail = 0;
                }

                // forward data to speaker callback, if callback is specified
                if (pHeadset->pSpkrDataCb != NULL)
                {
                    pHeadset->pSpkrDataCb((int16_t *)FrameData, VOIP_HEADSET_FRAMESAMPLES, pHeadset->pSpkrCbUserData);
                }

                // copy data to prepared wave buffer
                ds_memcpy_s(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData, sizeof(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData), FrameData, VOIP_HEADSET_FRAMESIZE);

                // write out wave buffer
                if ((hResult = waveOutWrite(pSpkrInfo->hDevice, &pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) == MMSYSERR_NOERROR)
                {
                    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetpc: [%2d] wrote %d samples\n", pSpkrInfo->iCurWaveBuffer, VOIP_HEADSET_FRAMESAMPLES));
                    pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer] = WHDR_DONE;
                    pSpkrInfo->iCurWaveBuffer = (pSpkrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
                }
                else
                {
                    if (hResult == MMSYSERR_NODRIVER)
                    {
                        NetPrintf(("voipheadsetpc: returned MMSYSERR_NODRIVER. Headset removed? Closing headset\n"));
                        _VoipHeadsetStop(pHeadset);
                    }
                    else if (hResult == WAVERR_STILLPLAYING)
                    {
                        NetPrintf(("voipheadsetpc: WAVERR_STILLPLAYING\n"));
                    }
                    else
                    {
                        NetPrintf(("voipheadsetpc: write failed (error=%d)\n", hResult));
                    }
                }
            }
            else if (iSampBytes > 0)
            {
                NetPrintf(("voipheadsetpc: error - got %d bytes from mixer when we were expecting %d\n", iSampBytes, VOIP_HEADSET_FRAMESIZE));
            }
            else
            {
                // no data waiting in the mixer, so break out
                break;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessDeviceChange

    \Description
        Process a device change request.

    \Input *pHeadset    - headset state
    \Input *pDeviceInfo - device info

    \Version 10/12/2011 (jbrookes) Split from VoipHeadsetProcess()
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessDeviceChange(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    // early out if no change is requested
    if (!pDeviceInfo->bChangeDevice && !pDeviceInfo->bCloseDevice)
    {
        return;
    }

    // device change requires sole access to headset critical section
    NetCritEnter(&pHeadset->DevChangeCrit);

    // close the device
    _VoipHeadsetCloseDevice(pHeadset, pDeviceInfo);
    pDeviceInfo->bCloseDevice = FALSE;

    // process change input device request
    if (pDeviceInfo->bChangeDevice)
    {
        if (_VoipHeadsetOpenDevice(pDeviceInfo, pDeviceInfo->iDeviceToOpen, VOIP_HEADSET_NUMWAVEBUFFERS) == 0)
        {
            // user index is always 0 because PC does not support MLU
            pHeadset->pStatusCb(0, TRUE, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? VOIP_HEADSET_STATUS_INPUT : VOIP_HEADSET_STATUS_OUTPUT, pHeadset->pCbUserData);
        }

        pDeviceInfo->bChangeDevice = FALSE;
    }

    NetCritLeave(&pHeadset->DevChangeCrit);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxConduits - max number of conduits
    \Input *pMicDataCb  - pointer to user callback to trigger when mic data is ready
    \Input *pTextDataCb - pointer to user callback to trigger when transcribed text is ready
    \Input *pStatusCb   - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData - pointer to user callback data
    \Input iData        - platform-specific - unused for PC

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Version 03/30/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxConduits, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxconduits
    if (iMaxConduits > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetpc: request for %d conduits exceeds max\n", iMaxConduits));
        return(NULL);
    }

    // allocate and clear module state
    if ((pHeadset = DirtyMemAlloc(sizeof(*pHeadset), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pHeadset, sizeof(*pHeadset));

    // allocate mixer
    if ((pHeadset->pMixerRef = VoipMixerCreate(16, VOIP_HEADSET_FRAMESAMPLES)) == NULL)
    {
        NetPrintf(("voipheadsetpc: unable to create mixer\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // allocate conduit manager
    if ((pHeadset->pConduitRef = VoipConduitCreate(iMaxConduits)) == NULL)
    {
        NetPrintf(("voipheadsetpc: unable to allocate conduit manager\n"));
        DirtyMemFree(pHeadset->pMixerRef, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    pHeadset->iMaxConduits = iMaxConduits;

    // set mixer
    VoipConduitSetMixer(pHeadset->pConduitRef, pHeadset->pMixerRef);

    // register codecs
    VoipCodecRegister('dvid', &VoipDVI_CodecDef);
    VoipCodecRegister('lpcm', &VoipPCM_CodecDef);

    // set up to use dvi codec by default
    VoipHeadsetControl(pHeadset, 'cdec', 'dvid', 0, NULL);

    // enable microphone
    VoipHeadsetControl(pHeadset, 'micr', TRUE, 0, NULL);

    // save info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pTextDataCb = pTextDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;

    // no currently active device
    pHeadset->SpkrInfo.eDevType = VOIP_HEADSET_OUTDEVICE;
    pHeadset->SpkrInfo.iActiveDevice = -1;
    pHeadset->SpkrInfo.iDeviceToOpen = WAVE_MAPPER;

    pHeadset->MicrInfo.eDevType = VOIP_HEADSET_INPDEVICE;
    pHeadset->MicrInfo.iActiveDevice = -1;
    pHeadset->MicrInfo.iDeviceToOpen = WAVE_MAPPER;

    // play
    pHeadset->iPlayerActive = 0;

    //$$tmp - set initial volume
    pHeadset->SpkrInfo.iNewVolume = 90;

    // enumerate headsets on startup
    _VoipHeadsetEnumerate(pHeadset);

    // init the critical section
    NetCritInit(&pHeadset->DevChangeCrit, "voipheadsetpc-devchange");

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // stop headsets
    _VoipHeadsetStop(pHeadset);

    // free conduit manager
    VoipConduitDestroy(pHeadset->pConduitRef);

    // free mixer
    VoipMixerDestroy(pHeadset->pMixerRef);

    // free active codec
    VoipCodecDestroy();

    // kill the critical section
    NetCritKill(&pHeadset->DevChangeCrit);

    // dispose of module memory
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUsers - user we're receiving the voice data from
    \Input *pMicrInfo    - micr info from inbound packet
    \Input *pPacketData  - pointer to beginning of data in packet payload
    \Input *pUserData    - VoipHeadsetT ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uMicrPkt;
    const BYTE *pSubPacket;

    // if we're not playing, ignore it
    if (pHeadset->SpkrInfo.bActive == FALSE)
    {
        #if DIRTYCODE_LOGGING
        if ((pMicrInfo->uSeqn % 30) == 0)
        {
            NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc: playback disabled, discarding voice data (seqn=%d)\n", pMicrInfo->uSeqn));
        }
        #endif
        return;
    }

    // validate subpacket size
    if (pMicrInfo->uSubPacketSize != pHeadset->iCmpFrameSize)
    {
        NetPrintf(("voipheadsetpc: discarding voice packet with %d voice bundles and mismatched sub-packet size %d (expecting %d)\n",
            pMicrInfo->uNumSubPackets, pMicrInfo->uSubPacketSize, pHeadset->iCmpFrameSize));
        return;
    }

    // submit voice sub-packets
    for (uMicrPkt = 0; uMicrPkt < pMicrInfo->uNumSubPackets; uMicrPkt++)
    {
        // get pointer to data subpacket
        pSubPacket = (const BYTE *)pPacketData + (uMicrPkt * pMicrInfo->uSubPacketSize);

        // send it to conduit manager
        VoipConduitReceiveVoiceData(pHeadset->pConduitRef, &pRemoteUsers[pMicrInfo->uUserIndex], pSubPacket, pMicrInfo->uSubPacketSize);
    }
}

/*F********************************************************************************/
/*!
    \Function VoipRegisterUserCb

    \Description
        Connectionlist callback to register/unregister a new user with the
        VoipConduit module.

    \Input *pRemoteUser - user to register
    \Input bRegister    - true=register, false=unregister
    \Input *pUserData   - voipheadset module ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    VoipConduitRegisterUser(pHeadset->pConduitRef, pRemoteUser, bRegister);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - process iteration counter

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    if (pHeadset->iPlayerActive)
    {
        // process playing
        _VoipHeadsetProcessPlay(pHeadset);
    }
    else
    {
        // process recording
        _VoipHeadsetProcessRecord(pHeadset);
    }

    // process playback
    _VoipHeadsetProcessPlayback(pHeadset);

    // process device change requests
    _VoipHeadsetProcessDeviceChange(pHeadset, &pHeadset->MicrInfo);
    _VoipHeadsetProcessDeviceChange(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSetVolume

    \Description
        Sets play and record volume.

    \Input *pHeadset    - pointer to headset state
    \Input iPlayVol     - play volume to set
    \Input iRecVol      - record volume to set

    \Notes
        To not set a value, specify it as -1.

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol)
{
    if (iPlayVol != -1)
    {
        pHeadset->SpkrInfo.iNewVolume = iPlayVol;
    }
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetControl

    \Description
        Control function.

    \Input *pHeadset    - headset module state
    \Input iControl      - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

        \verbatim
            'cdec' - create new codec
            'cide' - close voip input device
            'code' - close voip output device
            'edev' - enumerate voip input/output devices
            'idev' - select voip input device
            'loop' - enable/disable loopback
            'micr' - enable/disable recording
            'odev' - select voip output device
            'play' - enable/disable playing
            'svol' - changes speaker volume
        \endverbatim

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'aloc')
    {
        pHeadset->bParticipating = ((iValue2 == 0) ? FALSE : TRUE);

        NetPrintf(("voipheadsetpc: %s participating state\n", pHeadset->bParticipating ? "entering" : "exiting"));

        return(0);
    }
    if (iControl == 'cdec')
    {
        return(_VoipHeadsetSetCodec(pHeadset, iValue));
    }
    if ((iControl == 'cide') || (iControl == 'code'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iControl == 'cide') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        NetCritEnter(&pHeadset->DevChangeCrit);
        pDeviceInfo->bCloseDevice = TRUE;
        NetCritLeave(&pHeadset->DevChangeCrit);
    }
    if (iControl == 'edev')
    {
        _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->MicrInfo);
        _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->SpkrInfo);
        return(0);
    }
    if ((iControl == 'idev') || (iControl == 'odev'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iControl == 'idev') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        if (pDeviceInfo->iActiveDevice != iValue)
        {
            NetPrintf(("voipheadsetpc: '%C' selector used to change %s device from %d to %d\n", iControl, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                pDeviceInfo->iActiveDevice, iValue));
            NetCritEnter(&pHeadset->DevChangeCrit);
            pDeviceInfo->iDeviceToOpen = iValue;
            pDeviceInfo->bChangeDevice = TRUE;
            NetCritLeave(&pHeadset->DevChangeCrit);
        }
        else
        {
            NetPrintf(("voipheadsetpc: '%C' selector ignored because %s device %d is already active\n", iControl, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                pDeviceInfo->iActiveDevice, iValue));
        }
        return(0);
    }
    if (iControl == 'loop')
    {
        pHeadset->bLoopback = iValue;
        NetPrintf(("voipheadsetpc: loopback mode %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    if (iControl == 'micr')
    {
        pHeadset->bMicOn = (uint8_t)iValue;
        NetPrintf(("voipheadsetpc: mic %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        pHeadset->iDebugLevel = iValue;
        NetPrintf(("voipheadsetpc: debuglevel=%d\n", pHeadset->iDebugLevel));
        VoipConduitControl(pHeadset->pConduitRef, 'spam', iValue, pValue);
        return(VoipCodecControl(VOIP_CODEC_ACTIVE, iControl, iValue, 0, NULL));
    }
    #endif
    if (iControl == 'play')
    {
        if (iValue)
        {
            pHeadset->pPlayerBuffer = (int16_t *) pValue;
            pHeadset->uPlayerBufferFrameCurrent = 0;
            pHeadset->uPlayerBufferFrames = iValue / (VOIP_HEADSET_SAMPLEWIDTH * VOIP_HEADSET_FRAMESAMPLES);
            pHeadset->uPlayerFirstTime = 0;

            pHeadset->iPlayerActive = 1;
        }
        else
        {
            pHeadset->iPlayerActive = 0;
        }

        NetPrintf(("voipheadsetpc: play %s\n", ((pHeadset->iPlayerActive) ? "enabled" : "disabled")));

        return(0);
    }
    if (iControl == 'svol')
    {
        VoipHeadsetSetVolume(pHeadset, iValue, 0);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetStatus

    \Description
        Status function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input *pBuf        - buffer pointer
    \Input iBufSize     - buffer size

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

        \verbatim
            'ndev' - zero
            'idev' - get name of input device at index iValue (-1 returns number of input devices)
            'odev' - get name of output device at index iValue (-1 returns number of input devices)
            'idft' - get the default input device index (as specified in control panel)
            'odft' - get the default output device index (as specified in control panel)
            'ruid' - always return TRUE because MLU is not supported on PC
        \endverbatim

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'ndev')
    {
        return(0);
    }
    if ((iSelect == 'idev') || (iSelect == 'odev'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iSelect == 'idev') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        if (iValue == -1)
        {
            return(pDeviceInfo->iNumDevices);
        }
        else if ((iValue >= 0) && (iValue < pDeviceInfo->iNumDevices))
        {
            ds_memcpy(pBuf, pDeviceInfo->WaveDeviceCaps[iValue].szPname, sizeof(pDeviceInfo->WaveDeviceCaps[iValue].szPname));
            return(iValue);
        }
    }
    if (iSelect == 'idft')
    {
        DWORD dwPreferredDevice=0;
        DWORD dwStatusFlags=0;
        DWORD dwRetVal = 0;

        dwRetVal = waveInMessage((HWAVEIN)VOIP_HEADSET_WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&dwPreferredDevice, (DWORD_PTR)&dwStatusFlags);
        return((int32_t)dwPreferredDevice);
    }
    if (iSelect == 'odft')
    {
        DWORD dwPreferredDevice=0;
        DWORD dwStatusFlags=0;
        DWORD dwRetVal = 0;

        dwRetVal = waveOutMessage((HWAVEOUT)VOIP_HEADSET_WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&dwPreferredDevice, (DWORD_PTR)&dwStatusFlags);
        return((int32_t)dwPreferredDevice);
    }
    if (iSelect == 'ruid')
    {
        return (TRUE);
    }
    // unhandled result
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 12/12/2005 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDisplayTranscribedText

    \Description
        Display transcribed text in system UI

    \Input *pHeadset    - headset module state
    \Input *pOriginator - originator of the message
    \Input *pText       - message to be displayed
    \Output
        int32_t         - 0 if success, negative otherwise

    \Version 06/13/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetDisplayTranscribedText(VoipHeadsetRefT *pHeadset, VoipUserT *pOriginator, const wchar_t *pText)
{
    NetPrintf(("voipheadsetpc: VoipHeadsetDisplayTranscribedText() not yet supported on PC\n"));
    return(-1);
}
