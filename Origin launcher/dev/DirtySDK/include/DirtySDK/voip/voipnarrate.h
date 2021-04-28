/*H********************************************************************************/
/*!
    \File voipnarrate.h

    \Description
        Voip narration API wrapping Cloud-based text-to-speech services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Polly.
        Narration requests may be up to 255 characters in length, and overlapping
        requests are queued in order.

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 10/25/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipnarrate_h
#define _voipnarrate_h

/*!
\Moduledef VoipNarrate VoipNarrate
\Modulemember Voip
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"

/*** Defines **********************************************************************/

#define VOIPNARRATE_INPUT_MAX       (256)       //!< maximum length of narration text, null termination included

#define VOIPNARRATE_STREAM_START    (-1)        //!< sent in iSize callback param on start of stream 
#define VOIPNARRATE_STREAM_END      (-2)        //!< sent in iSize callback param on end of stream

// narration callback flags
#define VOIPNARRATE_CBFLAG_NONE     (0)         //!< empty callback flag
#define VOIPNARRATE_CBFLAG_LOCAL    (1)         //!< true if narration is to be played back locally

#define VOIPNARRATE_SAMPLERATE      (16000)     //!< audio sample rate used by narration, in hz

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// narration providers - these are aliases for VOIPSPEECH definitions for compatibility with previous versions
typedef enum VoipNarrateProviderE
{
    VOIPNARRATE_PROVIDER_NONE = VOIPSPEECH_PROVIDER_NONE,
    VOIPNARRATE_PROVIDER_IBMWATSON = VOIPSPEECH_PROVIDER_IBMWATSON,     //!< IBM Watson
    VOIPNARRATE_PROVIDER_MICROSOFT = VOIPSPEECH_PROVIDER_MICROSOFT,     //!< Microsoft Speech Services
    VOIPNARRATE_PROVIDER_GOOGLE = VOIPSPEECH_PROVIDER_GOOGLE,           //!< Google Speech
    VOIPNARRATE_PROVIDER_AMAZON = VOIPSPEECH_PROVIDER_AMAZON,           //!< Amazon Polly
    VOIPNARRATE_NUMPROVIDERS = VOIPSPEECH_NUMPROVIDERS
} VoipNarrateProviderE;

//! possible voice genders
typedef enum VoipNarrateGenderE
{
    VOIPNARRATE_GENDER_NONE,
    VOIPNARRATE_GENDER_FEMALE,
    VOIPNARRATE_GENDER_MALE,
    VOIPNARRATE_GENDER_NEUTRAL,
    VOIPNARRATE_NUMGENDERS
} VoipNarrateGenderE;

//! opaque module state
typedef struct VoipNarrateRefT VoipNarrateRefT;

/*!
    \Callback VoipNarrateVoiceDataCbT

    \Description
        Called when the inputed text gets turned into voice data to be sent

    \Input *pVoipNarrate    - module state
    \Input *pSamples        - voice data
    \Input iSize            - size of the sample data in bytes, or VOIPNARRATE_STREAM_* at stream start/end
    \Input *pUserData       - userdata provided to the callback

    \Output int32_t         - number of bytes consumed
*/ 
typedef int32_t (VoipNarrateVoiceDataCbT)(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, const int16_t *pSamples, int32_t iSize, uint32_t uFlags, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API VoipNarrateRefT *VoipNarrateCreate(VoipNarrateVoiceDataCbT *pVoiceDataCb, void *pUserData);

// configure the module
DIRTYCODE_API void VoipNarrateConfig(VoipSpeechProviderE eProvider, const char *pUrl, const char *pKey);

// destroy the module
DIRTYCODE_API void VoipNarrateDestroy(VoipNarrateRefT *pVoipNarrate);

// input text to be synthesized into speech
DIRTYCODE_API int32_t VoipNarrateInput(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, uint32_t uFlags, const char *pText);

// get module status
DIRTYCODE_API int32_t VoipNarrateStatus(VoipNarrateRefT *pVoipNarrate, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize);

// set control options
DIRTYCODE_API int32_t VoipNarrateControl(VoipNarrateRefT *pVoipNarrate, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update narration module
DIRTYCODE_API void VoipNarrateUpdate(VoipNarrateRefT *pVoipNarrate);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voipnarrate_h

