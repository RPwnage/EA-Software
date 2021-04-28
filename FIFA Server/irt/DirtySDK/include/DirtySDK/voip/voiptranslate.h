/*H********************************************************************************/
/*!
    \File voiptranslate.h

    \Description
        Voip translation API wrapping Cloud-based text translation services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Polly.
        Translation requests may be up to 255 characters in length, and overlapping
        requests are queued in order.

    \Copyright
        Copyright 2019 Electronic Arts

    \Version 01/28/2019 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voiptranslate_h
#define _voiptranslate_h

/*!
\Moduledef VoipTranslate VoipTranslate
\Modulemember Voip
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"

/*** Defines **********************************************************************/

#define VOIPTRANSLATE_MAX    (256)   //!< maximum length of translation text, null termination included (not in characters!)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// translation providers - these are aliases of VOIPSPEECH definitions for compatibility with previous versions
typedef enum VoipTranslateProviderE
{
    VOIPTRANSLATE_PROVIDER_NONE = VOIPSPEECH_PROVIDER_NONE,
    VOIPTRANSLATE_PROVIDER_IBMWATSON = VOIPSPEECH_PROVIDER_IBMWATSON,     //!< IBM Watson
    VOIPTRANSLATE_PROVIDER_MICROSOFT = VOIPSPEECH_PROVIDER_MICROSOFT,     //!< Microsoft Speech Services
    VOIPTRANSLATE_PROVIDER_GOOGLE = VOIPSPEECH_PROVIDER_GOOGLE,           //!< Google Speech
    VOIPTRANSLATE_PROVIDER_AMAZON = VOIPSPEECH_PROVIDER_AMAZON,           //!< Amazon Polly
    VOIPTRANSLATE_NUMPROVIDERS = VOIPSPEECH_NUMPROVIDERS
} VoipTranslateProviderE;

//! opaque module state
typedef struct VoipTranslateRefT VoipTranslateRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API VoipTranslateRefT *VoipTranslateCreate(void);

// configure the module
DIRTYCODE_API void VoipTranslateConfig(VoipSpeechProviderE eProvider, const char *pUrl, const char *pKey);

// destroy the module
DIRTYCODE_API void VoipTranslateDestroy(VoipTranslateRefT *pVoipTranslate);

// input text to be synthesized into speech
DIRTYCODE_API int32_t VoipTranslateInput(VoipTranslateRefT *pVoipTranslate, int32_t iUserIndex, const char *pSrcLang, const char *pDstLang, const char *pText, void *pUserData);

// get a translation
DIRTYCODE_API int32_t VoipTranslateGet(VoipTranslateRefT *pVoipTranslate, char *pBuffer, int32_t iBufLen, void **ppUserData);

// get module status
DIRTYCODE_API int32_t VoipTranslateStatus(VoipTranslateRefT *pVoipTranslate, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize);

// set control options
DIRTYCODE_API int32_t VoipTranslateControl(VoipTranslateRefT *pVoipTranslate, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update narration module
DIRTYCODE_API void VoipTranslateUpdate(VoipTranslateRefT *pVoipTranslate);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voiptranslate_h

