/*H********************************************************************************/
/*!
    \File voipnarrate.c

    \Description
        Voip narration API wrapping Cloud-based text-to-speech services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Polly.
        Narration requests may be up to 255 characters in length, and overlapping
        requests are queued and processed in order.

    \Notes
        References

        IBM Watson:
            Text-to Speech-API: https://www.ibm.com/watson/developercloud/text-to-speech/api/v1/curl.html

        Microsoft Speech Service:
            Text-to-Speech How-To: https://docs.microsoft.com/en-us/azure/cognitive-services/Speech-Service/how-to-text-to-speech
            REST API Reference: https://docs.microsoft.com/en-us/azure/cognitive-services/speech-service/rest-text-to-speech
            Voice list: https://docs.microsoft.com/en-us/azure/cognitive-services/speech-service/language-support

        Google Text-to-Speech
            Text-to-Speech API: https://cloud.google.com/text-to-speech/docs/reference/rest/

        Amazon Polly
            SynthesizeSpeech API: https://docs.aws.amazon.com/polly/latest/dg/API_SynthesizeSpeech.html
            Amazon Endpoint Names: https://docs.aws.amazon.com/general/latest/gr/rande.html
            VoiceId List: https://docs.aws.amazon.com/polly/latest/dg/API_SynthesizeSpeech.html#polly-SynthesizeSpeech-request-VoiceId

        WAV:
            WAVE file format: https://en.wikipedia.org/wiki/WAV#RIFF_WAVE

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 10/25/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protostream.h"
#include "DirtySDK/util/aws.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "DirtySDK/voip/voipdef.h"

#include "DirtySDK/voip/voipnarrate.h"

/*** Defines **********************************************************************/

//! protostream minimum data amount (for base64 decoding; four is the minimum amount but that produces one and a half samples, so we choose eight, BUT...
#define VOIPNARRATE_MINBUF                      (8)

//! how many milliseconds of audio received should we treat as being empty audio (for metrics)
#define VOIPNARRATE_EMPTY_AUDIO_THRESHOLD_MS    (300)

//! ms-specific bearer token lifetime
#define VOIPNARRATE_MICROSOFT_TOKENLIFE         (9*60*1000)     // token lifetime is ten minutes; recommended replacement interval is nine

//! ms bearer token buffer size
#define VOIPNARRATE_MICROSOFT_TOKENSIZE         (2*1024)

//! ms-specific bearer token url; hostname is replaced with tts url hostname
#define VOIPNARRATE_MICTOSOFT_TOKENURL          "https://%s.api.cognitive.microsoft.com/sts/v1.0/issueToken"

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! http state, used for bearer token acquisition
typedef enum VoipNarrateHttpStateE
{
    VOIPNARRATE_HTTPSTATE_IDLE,
    VOIPNARRATE_HTTPSTATE_REQUEST,
    VOIPNARRATE_HTTPSTATE_RESPONSE,
    VOIPNARRATE_HTTPSTATE_FAILED,
    VOIPNARRATE_HTTPSTATE_DONE
} VoipNarrateHttpStateE;

//! voice parameters
typedef struct VoipNarrateVoiceT
{
    char strName[32];                       //!< name to use in request to service
    char strLocale[7];                      //!< locale of voice
    uint8_t uGender;                        //!< gender of voice (VOIPNARRATE_GENDER_*)
} VoipNarrateVoiceT;

//! module configuration
typedef struct VoipNarrateConfigT
{
    VoipSpeechProviderE eProvider;          //!< configured provider
    char strUrl[256];                       //!< URL for text-to-speech request
    char strKey[128];                       //!< API key required for service authentication
} VoipNarrateConfigT;

//! narration request data
typedef struct VoipNarrateRequestT
{
    struct VoipNarrateRequestT *pNext;
    VoipNarrateGenderE eGender;
    uint32_t uFlags;
    char strText[VOIPNARRATE_INPUT_MAX];
    int8_t iUserIndex;
} VoipNarrateRequestT;

// module state
struct VoipNarrateRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoStreamRefT *pProtoStream;          //!< stream transport module to handle buffered download of audio data
    ProtoHttpRefT *pProtoHttp;              //!< protohttp ref for fetching a bearer token for services that require one

    VoipNarrateVoiceDataCbT *pVoiceDataCb;  //!< user callback used to provide voice data
    void *pUserData;                        //!< user data for user callback
    uint32_t uFlags;                        //!< callback flags

    VoipNarrateRequestT *pRequest;          //!< list of queued requests, if any
    VoipNarrateConfigT Config;              //!< module configuration (provider and credentials)

    char strHead[2048];                     //!< http head for narration request
    char strBody[512];                      //!< http body for narration request
    const char *pBody;                      //!< pointer to start of body (may not match buffer start)

    VoipNarrateHttpStateE eHttpState;       //!< http state for bearer token acquisition
    uint8_t *pToken;                        //!< token data
    int32_t iTokenLen;                      //!< token length
    uint32_t uTokenTick;                    //!< when token was received

    VoipTextToSpeechMetricsT Metrics;       //!< Usage metrics of the narration module
    uint32_t uTtsStartTime;                 //!< time when we sent the request

    uint8_t aVoiceBuf[160*3*2];             //!< base64 decode buffer, sized for one 30ms frame of 16khz 16bit voice audio, also a multiple of three bytes to accomodate base64 4->3 ratio
    int32_t iVoiceOff;                      //!< read offset in buffered voice data
    int32_t iVoiceLen;                      //!< end of buffered voice data
    int32_t iSamplesInPhrase;               //!< total number of samples received for this phrase

    char strLang[8];

    uint8_t bStart;                         //!< TRUE if start of stream download, else FALSE
    uint8_t bActive;                        //!< TRUE if stream is active, else FALSE
    int8_t  iUserIndex;                     //!< index of local user current request is being made for
    int8_t  iVerbose;                       //!< verbose debug level (debug only)
};

/*** Variables ********************************************************************/

//! global config state
static VoipNarrateConfigT _VoipNarrate_Config = { VOIPSPEECH_PROVIDER_NONE, "", "" };

//! Watson voice list
static VoipNarrateVoiceT _Watson_aVoices[] =
{
    { "en-US_AllisonVoice",     "en-US", VOIPNARRATE_GENDER_FEMALE },
    { "en-US_MichaelVoice",     "en-US", VOIPNARRATE_GENDER_MALE   },
    { "pt-BR_IsabelaVoice",     "pt-BR", VOIPNARRATE_GENDER_FEMALE },
    { "en-GB_KateVoice",        "en-GB", VOIPNARRATE_GENDER_FEMALE },
    { "fr-FR_ReneeVoice",       "fr-FR", VOIPNARRATE_GENDER_FEMALE },
    { "de-DE_BirgitVoice",      "de-DE", VOIPNARRATE_GENDER_FEMALE },
    { "de-DE_DeiterVoice",      "de-DE", VOIPNARRATE_GENDER_MALE   },
    { "it-IT_FrancescaVoice",   "it-IT", VOIPNARRATE_GENDER_FEMALE },
    { "ja-JP_EmiVoice",         "ja-JP", VOIPNARRATE_GENDER_FEMALE },
    { "es-ES_EnriqueVoice",     "es-ES", VOIPNARRATE_GENDER_MALE   },
    { "es-ES_LauraVoice",       "es-ES", VOIPNARRATE_GENDER_FEMALE },
    { "es-LA_SofiaVoice",       "es-LA", VOIPNARRATE_GENDER_FEMALE },
    { "es-US_SofiaVoice",       "es-US", VOIPNARRATE_GENDER_FEMALE },
    { "",                       "",      VOIPNARRATE_GENDER_NONE   }
};

//! Amazon voice list
static VoipNarrateVoiceT _Amazon_aVoices[] =
{
    { "Joanna",     "en-US", VOIPNARRATE_GENDER_FEMALE },
    { "Joey",       "en-US", VOIPNARRATE_GENDER_MALE },
    { "Zeina",      "ar-AR", VOIPNARRATE_GENDER_FEMALE },
    { "Zhiyu",      "cmn-CN", VOIPNARRATE_GENDER_FEMALE },
    { "Naja",       "da-DK", VOIPNARRATE_GENDER_FEMALE },
    { "Mads",       "da-DK", VOIPNARRATE_GENDER_MALE },
    { "Lotte",      "nl-NL", VOIPNARRATE_GENDER_FEMALE },
    { "Ruben",      "nl-NL", VOIPNARRATE_GENDER_MALE },
    { "Nicole",     "en-AU", VOIPNARRATE_GENDER_FEMALE },
    { "Russell",    "en-AU", VOIPNARRATE_GENDER_MALE },
    { "Emma",       "en-GB", VOIPNARRATE_GENDER_FEMALE },
    { "Brian",      "en-GB", VOIPNARRATE_GENDER_MALE },
    { "Aditi",      "en-IN", VOIPNARRATE_GENDER_FEMALE }, // bilingual (english and hindi)
    { "Raveena",    "en-IN", VOIPNARRATE_GENDER_FEMALE },
    //{ "Geraint", "en-GB-WLS", VOIPNARRATE_GENDER_MALE }, // todo
    { "Celine",     "fr-FR", VOIPNARRATE_GENDER_FEMALE },
    { "Mathieu",    "fr-FR", VOIPNARRATE_GENDER_MALE },
    { "Chantal",    "fr-CA", VOIPNARRATE_GENDER_FEMALE },
    { "Marlene",    "de-DE", VOIPNARRATE_GENDER_FEMALE },
    { "Hans",       "de-DE", VOIPNARRATE_GENDER_MALE },
    { "Aditi",      "hi-IN", VOIPNARRATE_GENDER_FEMALE }, // bilingual (english and hindi)
    { "Dora",       "is-IS", VOIPNARRATE_GENDER_FEMALE },
    { "Karl",       "is-IS", VOIPNARRATE_GENDER_MALE },
    { "Bianca",     "it-IT", VOIPNARRATE_GENDER_FEMALE },
    { "Giorgio",    "it-IT", VOIPNARRATE_GENDER_MALE },
    { "Mizuki",     "ja-JP", VOIPNARRATE_GENDER_FEMALE },
    { "Takumi",     "ja-JP", VOIPNARRATE_GENDER_MALE },
    { "Seoyeon",    "ko-KR", VOIPNARRATE_GENDER_FEMALE },
    { "Liv",        "nb-NO", VOIPNARRATE_GENDER_FEMALE },
    { "Ewa",        "pl-PL", VOIPNARRATE_GENDER_FEMALE },
    { "Jan",        "pl-PL", VOIPNARRATE_GENDER_MALE },
    { "Vitoria",    "pt-BR", VOIPNARRATE_GENDER_FEMALE },
    { "Ricardo",    "pt-BR", VOIPNARRATE_GENDER_MALE },
    { "Ines",       "pt-PT", VOIPNARRATE_GENDER_FEMALE },
    { "Cristiano",  "pt-PT", VOIPNARRATE_GENDER_MALE },
    { "Carmen",     "ro-RO", VOIPNARRATE_GENDER_FEMALE },
    { "Tatyana",    "ru-RU", VOIPNARRATE_GENDER_FEMALE },
    { "Maxim",      "ru-RU", VOIPNARRATE_GENDER_MALE },
    { "Lucia",      "es-ES", VOIPNARRATE_GENDER_FEMALE },
    { "Enrique",    "es-ES", VOIPNARRATE_GENDER_MALE },
    { "Mia",        "es-MX", VOIPNARRATE_GENDER_FEMALE },
    { "Penelope",   "es-US", VOIPNARRATE_GENDER_FEMALE },
    { "Miguel",     "es-US", VOIPNARRATE_GENDER_MALE },
    { "Astrid",     "sv-SE", VOIPNARRATE_GENDER_FEMALE },
    { "Filiz",      "tr-TR", VOIPNARRATE_GENDER_FEMALE },
    { "Gwyneth",    "cy-GB", VOIPNARRATE_GENDER_FEMALE },
    { "",           "",      VOIPNARRATE_GENDER_NONE   }
};

//! Microsoft voice list
static VoipNarrateVoiceT _Microsoft_aVoices[] =
{
    { "en-US-JessaRUS",        "en-US", VOIPNARRATE_GENDER_FEMALE },
    { "en-US-BenjaminRUS",     "en-US", VOIPNARRATE_GENDER_MALE },
    { "ar-EG-Hoda",            "ar-EG", VOIPNARRATE_GENDER_FEMALE },
    { "ar-SA-Naaf",            "ar-SA", VOIPNARRATE_GENDER_MALE },
    { "bg-BG-Ivan",            "bg-BG", VOIPNARRATE_GENDER_MALE },
    { "ca-ES-HerenaRUS",       "ca-ES", VOIPNARRATE_GENDER_FEMALE },
    { "cs-CZ-Jakub",           "cs-CZ", VOIPNARRATE_GENDER_MALE },
    { "da-DK-HelleRUS",        "da-DK", VOIPNARRATE_GENDER_FEMALE },
    { "de-AT-Michael",         "de-AT", VOIPNARRATE_GENDER_MALE },
    { "de-CH-Karsten",         "de-CH", VOIPNARRATE_GENDER_MALE },
    { "de-DE-Hedda",           "de-DE", VOIPNARRATE_GENDER_FEMALE },
    { "de-DE-Stefan-Apollo",   "de-DE", VOIPNARRATE_GENDER_MALE },
    { "el-GR-Stefanos",        "el-GR", VOIPNARRATE_GENDER_MALE },
    { "en-AU-Catherine",       "en-AU", VOIPNARRATE_GENDER_FEMALE },
    { "en-CA-Linda",           "en-CA", VOIPNARRATE_GENDER_FEMALE },
    { "en-GB-Susan-Apollo",    "en-GB", VOIPNARRATE_GENDER_FEMALE },
    { "en-GB-George-Apollo",   "en-GB", VOIPNARRATE_GENDER_MALE },
    { "en-IE-Sean",            "en-IN", VOIPNARRATE_GENDER_MALE },
    { "en-IN-Heera-Apollo",    "en-IN", VOIPNARRATE_GENDER_FEMALE },
    { "en-IN-Ravi-Apollo",     "en-IN", VOIPNARRATE_GENDER_MALE },
    { "es-ES-Laura-Apollo",    "es-ES", VOIPNARRATE_GENDER_FEMALE },
    { "es-ES-Pablo-Apollo",    "es-ES", VOIPNARRATE_GENDER_MALE },
    { "es-MX-HildaRUS",        "es-MX", VOIPNARRATE_GENDER_FEMALE },
    { "es-MX-Raul-Apollo",     "es-MX", VOIPNARRATE_GENDER_MALE },
    { "fi-FI-HeidiRUS",        "fi-FI", VOIPNARRATE_GENDER_FEMALE },
    { "fr-CA-Caroline",        "fr-CA", VOIPNARRATE_GENDER_FEMALE },
    { "fr-CH-Guillaume",       "fr-CH", VOIPNARRATE_GENDER_MALE },
    { "fr-FR-Julie-Apollo",    "fr-FR", VOIPNARRATE_GENDER_FEMALE },
    { "fr-FR-Paul-Apollo",     "fr-FR", VOIPNARRATE_GENDER_MALE },
    { "he-IL-Asaf",            "he-IL", VOIPNARRATE_GENDER_MALE },
    { "he-IN-Kalpana-Apollo",  "hi-IN", VOIPNARRATE_GENDER_FEMALE },
    { "hi-IN-Hemant",          "hi-IN", VOIPNARRATE_GENDER_MALE },
    { "hr-HR-Matej",           "hr-HR", VOIPNARRATE_GENDER_MALE },
    { "hu-HU-Szabolcs",        "hu-HU", VOIPNARRATE_GENDER_MALE },
    { "id-ID-Andika",          "id-ID", VOIPNARRATE_GENDER_MALE },
    { "it-IT-LuciaRUS",        "it-IT", VOIPNARRATE_GENDER_FEMALE },
    { "it-IT-Cosimo-Apollo",   "it-IT", VOIPNARRATE_GENDER_MALE },
    { "ja-JP-Ayumi-Apollo",    "ja-JP", VOIPNARRATE_GENDER_FEMALE },
    { "ja-JP-Ichiro-Apollo",   "ja-JP", VOIPNARRATE_GENDER_MALE },
    { "ko-KR-HeamiRUS",        "ko-KR", VOIPNARRATE_GENDER_FEMALE },
    { "ms-MY-Rizwan",          "ms-MY", VOIPNARRATE_GENDER_MALE },
    { "nb-NO-HuldaRUS",        "nb-NO", VOIPNARRATE_GENDER_FEMALE },
    { "nl-NL-HannaRUS",        "nl-NL", VOIPNARRATE_GENDER_FEMALE },
    { "pl-PL-PaulinaRUS",      "pl-PL", VOIPNARRATE_GENDER_FEMALE },
    { "pt-BR-HeloisaRUS",      "pt-BR", VOIPNARRATE_GENDER_FEMALE },
    { "pt-BR-Daniel-Apollo",   "pt-BR", VOIPNARRATE_GENDER_MALE },
    { "pt-PT-HeliaRUS",        "pt-PT", VOIPNARRATE_GENDER_FEMALE },
    { "ro-RO-Andrei",          "ro-RO", VOIPNARRATE_GENDER_MALE },
    { "ru-RU-Irina-Apollo",    "ru-RU", VOIPNARRATE_GENDER_FEMALE },
    { "ru-RU-Pavel-Apollo",    "ru-RU", VOIPNARRATE_GENDER_MALE },
    { "sk-SK-Filip",           "sk-SK", VOIPNARRATE_GENDER_MALE },
    { "sl-SL-sl-SL-Lado",      "sl-SL", VOIPNARRATE_GENDER_MALE },
    { "sv-SE-HedvigRUS",       "sv-SE", VOIPNARRATE_GENDER_FEMALE },
    { "ta-IN-Valluvar",        "ta-IN", VOIPNARRATE_GENDER_MALE },
    { "te-IN-Chitra",          "ta-IN", VOIPNARRATE_GENDER_FEMALE },
    { "th-TH-Pattara",         "th-TH", VOIPNARRATE_GENDER_MALE },
    { "tr-TR-SedaRUS",         "tr-TR", VOIPNARRATE_GENDER_FEMALE },
    { "vi-VN-An",              "vi-VN", VOIPNARRATE_GENDER_MALE },
    { "zh-CN-Yaoyao-Apollo",   "zh-CN", VOIPNARRATE_GENDER_FEMALE },
    { "zh-CN-Kangkang-Apollo", "zh-CN", VOIPNARRATE_GENDER_MALE },
    { "zh-HK-Tracy-Apollo",    "zh-HK", VOIPNARRATE_GENDER_FEMALE },
    { "zh-HK-Danny-Apollo",    "zh-HK", VOIPNARRATE_GENDER_MALE },
    { "zh-TW-Yating-Apollo",   "zh-TW", VOIPNARRATE_GENDER_FEMALE },
    { "zh-TW-Zhiwei-Apollo",   "zh-TW", VOIPNARRATE_GENDER_MALE },
    { "",                      "",      VOIPNARRATE_GENDER_NONE }
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipNarrateCustomHeaderCb

    \Description
        Custom header callback used to sign AWS requests

    \Input *pState          - http module state
    \Input *pHeader         - pointer to http header buffer
    \Input uHeaderSize      - size of http header buffer
    \Input *pUserRef        - voipnarrate ref

    \Output
        int32_t             - output header length

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateCustomHeaderCb(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    VoipNarrateRefT *pVoipNarrate = (VoipNarrateRefT *)pUserRef;
    int32_t iHdrLen = (int32_t)strlen(pHeader);
    
    // if amazon and we have room, sign the request
    if ((pVoipNarrate->Config.eProvider != VOIPSPEECH_PROVIDER_AMAZON) || (uHeaderSize < (unsigned)iHdrLen))
    {
        return(iHdrLen);
    }

    // sign the request and return the updated size
    return(AWSSignSigV4(pHeader, uHeaderSize, pVoipNarrate->pBody, pVoipNarrate->Config.strKey, "polly", NULL));
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateSkipWavHeader

    \Description
        Return offset past WAV header in input data

    \Input *pData           - pointer to wav header
    \Input iDataLen         - length of data

    \Output
        int32_t             - offset past WAV header, or zero

    \Version 11/06/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateSkipWavHeader(const uint8_t *pData, int32_t iDataLen)
{
    int32_t iOffset = 0, iChkLen;
    uint8_t bFoundData;

    // validate and skip RIFF/WAVE header
    if ((iDataLen < 12) || ds_strnicmp((const char *)pData, "RIFF", 4) || ds_strnicmp((const char *)pData+8, "WAVE", 4))
    {
        return(0);
    }
    iOffset += 12;

    // process chunks
    for (bFoundData = FALSE; iOffset < (iDataLen+12); iOffset += iChkLen+8)
    {
        // get chunk length
        iChkLen  = pData[iOffset+4];
        iChkLen |= pData[iOffset+5]<<8;
        iChkLen |= pData[iOffset+6]<<16;
        iChkLen |= pData[iOffset+7]<<24;

        // look for data chunk
        if (!ds_strnicmp((const char *)pData+iOffset, "data", 4))
        {
            bFoundData = TRUE;
            iOffset += 8;
            break;
        }
    }
    return(bFoundData ? iOffset : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBasicAuth

    \Description
        Encode Basic HTTP authorization header as per https://tools.ietf.org/html/rfc7617

    \Input *pBuffer     - [out] output buffer for encoded base64 string
    \Input iBufSize     - size of output buffer
    \Input *pUser       - user identifer
    \Input *pPass       - user password

    \Output
        const char *    - pointer to output buffer

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateBasicAuth(char *pBuffer, int32_t iBufSize, const char *pUser, const char *pPass)
{
    char strAuth[128];
    ds_snzprintf(strAuth, sizeof(strAuth), "%s:%s", pUser, pPass);
    Base64Encode2(strAuth, (int32_t)strlen(strAuth), pBuffer, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBase64Decode

    \Description
        Decode Base64-encoded voice data

    \Input *pOutput         - [out] buffer to hold decoded voice data
    \Input *pOutSize        - [in/out] output buffer length, size of output data
    \Input *pInput          - base64-encoded input data
    \Input iInpSize         - input buffer length

    \Output
        int32_t             - negative=failure, else input bytes consumed

    \Version 10/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateBase64Decode(char *pOutput, int32_t *pOutSize, const char *pInput, int32_t iInpSize)
{
    static const char _strJson[] = "\"audioContent\":";
    const char *pInput2, *pInpEnd = pInput + iInpSize;
    int32_t iInpOff = 0;

    // if we have the beginning of json envelope, skip it
    if ((pInput2 = strstr(pInput, _strJson)) != NULL)
    {
        // skip json header
        pInput2 += sizeof(_strJson);
        // skip to base64 data
        for (; (*pInput2 != '"') && (pInput2 < pInpEnd); pInput2 += 1)
            ;
        if (*pInput2 != '"')
        {
            return(-1);
        }
        // skip quote
        pInput2 += 1;
        // remember to consume this in addition to base64 data
        iInpOff = pInput2 - pInput;
        pInput = pInput2;
    }
    // if we have end of json envelope, trim it
    if ((pInput2 = strchr(pInput, '"')) != NULL)
    {
        // handle end of data
        if (pInput2 == pInput)
        {
            iInpOff = iInpSize;
        }
        iInpSize = pInput2-pInput;
    }

    // constrain input size to what will fit in output buffer
    if (iInpSize > Base64EncodedSize(*pOutSize))
    {
        iInpSize = Base64EncodedSize(*pOutSize);
    }

    // make sure input size is a multiple of four to produce an integral number of output bytes
    iInpSize &= ~0x03;

    // base64 decode and save output size
    *pOutSize = Base64Decode3(pInput, iInpSize, pOutput, *pOutSize);
    // return number of bytes of input consumed
    return(iInpSize+iInpOff);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateMatchVoice

    \Description
        Get the voice best matching the input language and gender requested from
        the specified voice table.

    \Input *pVoipNarrate    - pointer to module state
    \Input *pVoiceList      - pointer to list
    \Input *pLocale         - desired locale of voice (may just be language)
    \Input eGender          - desired gender of voice

    \Output
        VoipNarrateVoiceT * - selected voice

    \Version 10/17/2019 (jbrookes)
*/
/********************************************************************************F*/
static const VoipNarrateVoiceT *_VoipNarrateMatchVoice(VoipNarrateRefT *pVoipNarrate, const VoipNarrateVoiceT *pVoiceList, const char *pLocale, VoipNarrateGenderE eGender)
{
    int32_t iEntry, iLocLen;
    // scan list for a match
    for (iEntry = 0, iLocLen = (int32_t)strlen(pLocale); pVoiceList[iEntry].uGender != VOIPNARRATE_GENDER_NONE; iEntry += 1)
    {
        if (!strncmp(pVoiceList[iEntry].strLocale, pLocale, iLocLen) && ((pVoiceList[iEntry].uGender == eGender) || (eGender == VOIPNARRATE_GENDER_NONE)))
        {
            break;
        }
    }
    // return result to caller
    return ((pVoiceList[iEntry].uGender != VOIPNARRATE_GENDER_NONE) ? &pVoiceList[iEntry] : NULL);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateGetVoice

    \Description
        Get the voice best matching the input language and gender requested from
        the specified voice list.

    \Input *pVoipNarrate    - pointer to module state
    \Input *pVoiceList      - pointer to list
    \Input *pLocale         - desired locale of voice
    \Input eGender          - desired gender of voice

    \Output
        const char *        - selected voice identifier

    \Version 10/17/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateGetVoice(VoipNarrateRefT *pVoipNarrate, const VoipNarrateVoiceT *pVoiceList, const char *pLocale, VoipNarrateGenderE eGender)
{
    const VoipNarrateVoiceT *pVoice;
    char strLang[3];
    // first pass - scan for exact match
    if ((pVoice = _VoipNarrateMatchVoice(pVoipNarrate, pVoiceList, pLocale, eGender)) != NULL)
    {
        return(pVoice->strName);
    }
    // second pass - scan for locale match with any gender
    if ((pVoice = _VoipNarrateMatchVoice(pVoipNarrate, pVoiceList, pLocale, VOIPNARRATE_GENDER_NONE)) != NULL)
    {
        return(pVoice->strName);
    }
    // generate language-only string (discard country portion of locale)
    ds_strnzcpy(strLang, pLocale, sizeof(strLang));
    // third pass - scan for language match and exact gender
    if ((pVoice = _VoipNarrateMatchVoice(pVoipNarrate, pVoiceList, strLang, eGender)) != NULL)
    {
        return(pVoice->strName);
    }
    // fourth pass - scan for language match with any gender
    if ((pVoice = _VoipNarrateMatchVoice(pVoipNarrate, pVoiceList, strLang, VOIPNARRATE_GENDER_NONE)) != NULL)
    {
        return(pVoice->strName);
    }
    // last - return default (en-US)
    pVoice = (eGender == VOIPNARRATE_GENDER_MALE) ? &pVoiceList[1] : &pVoiceList[0];
    return(pVoice->strName);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateRequestAdd

    \Description
        Queue request for later sending

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice narration
    \Input uFlags           - callback flags
    \Input *pText           - text to be converted

    \Output
        int32_t             - negative=failure, else success

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateRequestAdd(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, uint32_t uFlags, const char *pText)
{
    VoipNarrateRequestT *pRequest;

    // allocate and clear the request
    if ((pRequest = DirtyMemAlloc(sizeof(*pRequest), VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarrate: could not allocate request\n"));
        pVoipNarrate->Metrics.uErrorCount += 1;
        return(-1);
    }
    ds_memclr(pRequest, sizeof(*pRequest));

    // copy the request data
    ds_strnzcpy(pRequest->strText, pText, sizeof(pRequest->strText));
    pRequest->iUserIndex = iUserIndex;
    pRequest->eGender = eGender;
    pRequest->uFlags = uFlags;

    // add to queue
    pRequest->pNext = pVoipNarrate->pRequest;
    pVoipNarrate->pRequest = pRequest;

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateRequestGet

    \Description
        Get queued request

    \Input *pVoipNarrate    - pointer to module state
    \Input *pRequest        - [out] storage for request (may be null)

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipNarrateRequestGet(VoipNarrateRefT *pVoipNarrate, VoipNarrateRequestT *pRequest)
{
    VoipNarrateRequestT **ppRequest;
    // get oldest request (we add to head, so get from tail)
    for (ppRequest = &pVoipNarrate->pRequest; (*ppRequest)->pNext != NULL; ppRequest = &((*ppRequest)->pNext))
        ;
    // copy request
    if (pRequest != NULL)
    {
        ds_memcpy_s(pRequest, sizeof(*pRequest), *ppRequest, sizeof(**ppRequest));
    }
    // free request
    DirtyMemFree(*ppRequest, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
    // remove from list
    *ppRequest = NULL;
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBearerTokenValid

    \Description
        Returns if bearer token is valid (or not required)

    \Input *pVoipNarrate    - pointer to module state
    \Input uCurTick         - current tick count

    \Output
        int32_t             - TRUE if bearer token is valid or not required, else FALSE

    \Version 11/02/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateBearerTokenValid(VoipNarrateRefT *pVoipNarrate, uint32_t uCurTick)
{
    // bearer token only required for microsoft
    if (pVoipNarrate->Config.eProvider != VOIPSPEECH_PROVIDER_MICROSOFT)
    {
        return(TRUE);
    }
    // if we have a token and it's not expired
    return((pVoipNarrate->pToken != NULL) && (NetTickDiff(uCurTick, pVoipNarrate->uTokenTick) < VOIPNARRATE_MICROSOFT_TOKENLIFE));
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBearerTokenRegionGet

    \Description
        Get region for bearer token url from tts url

    \Input *pVoipNarrate    - pointer to module state
    \Input *pRegion         - [out] buffer for region
    \Input iRegionSize      - size of region buffer

    \Output
        char *              - pointer to region string, or empty string on error

    \Version 11/02/2019 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateBearerTokenRegionGet(VoipNarrateRefT *pVoipNarrate, char *pRegion, int32_t iRegionSize)
{
    char strKind[6], *pStr;
    int32_t iPort, iSecure;
    // extract hostname from tts request url
    ProtoHttpUrlParse(pVoipNarrate->Config.strUrl, strKind, sizeof(strKind), pRegion, iRegionSize, &iPort, &iSecure);
    // we want just the region, so truncate at the first period
    if ((pStr = strchr(pRegion, '.')) != NULL)
    {
        *pStr = '\0';
    }
    else
    {
        *pRegion = '\0';
    }
    return(pRegion);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBearerTokenUpdate

    \Description
        Update bearer token processing (microsoft only)

    \Input *pVoipNarrate    - pointer to module state

    \Version 11/02/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipNarrateBearerTokenUpdate(VoipNarrateRefT *pVoipNarrate)
{
    int32_t iResult = 0;
    uint32_t uCurTick = NetTick();

    // only get a bearer token if provider is microsoft and we have a request pending
    if ((pVoipNarrate->Config.eProvider != VOIPSPEECH_PROVIDER_MICROSOFT) || (pVoipNarrate->pRequest == NULL))
    {
        return;
    }

    // idle processing; get token if we have no token or the token we have is expired
    if ((pVoipNarrate->eHttpState == VOIPNARRATE_HTTPSTATE_IDLE) && !_VoipNarrateBearerTokenValid(pVoipNarrate, uCurTick))
    {
        if (pVoipNarrate->pToken == NULL)
        {
            pVoipNarrate->pToken = (uint8_t *)DirtyMemAlloc(VOIPNARRATE_MICROSOFT_TOKENSIZE, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
            pVoipNarrate->iTokenLen = 0;
        }
        if (pVoipNarrate->pToken != NULL)
        {
            pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_REQUEST;
        }
    }

    // request processing... make the request
    if (pVoipNarrate->eHttpState == VOIPNARRATE_HTTPSTATE_REQUEST)
    {
        if (pVoipNarrate->pProtoHttp == NULL)
        {
            pVoipNarrate->pProtoHttp = ProtoHttpCreate(0);
        }
        if (pVoipNarrate->pProtoHttp != NULL)
        {
            char strHead[1024], strUrl[128], strRegion[32];
            int32_t iOffset=0;

            /* format request url; we construct this with the bearer token url and substitute the configured regional server
               as per https://docs.microsoft.com/en-us/azure/cognitive-services/speech-service/rest-text-to-speech */
            ds_snzprintf(strUrl, sizeof(strUrl), VOIPNARRATE_MICTOSOFT_TOKENURL, _VoipNarrateBearerTokenRegionGet(pVoipNarrate, strRegion, sizeof(strRegion)));

            // format request header
            iOffset += ds_snzprintf(strHead+iOffset, sizeof(strHead)-iOffset, "Content-Type: application/x-www-form-urlencoded\r\n");
            iOffset += ds_snzprintf(strHead+iOffset, sizeof(strHead)-iOffset, "Ocp-Apim-Subscription-Key: %s\r\n", pVoipNarrate->Config.strKey);
            ProtoHttpControl(pVoipNarrate->pProtoHttp, 'apnd', 0, 0, strHead);
            ProtoHttpControl(pVoipNarrate->pProtoHttp, 'spam', 3, 0, NULL);
            // make the request
            if (ProtoHttpPost(pVoipNarrate->pProtoHttp, strUrl, NULL, 0, FALSE) == 0)
            {
                pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_RESPONSE;
            }
        }
    }

    // response processing... wait for the response
    if (pVoipNarrate->eHttpState == VOIPNARRATE_HTTPSTATE_RESPONSE)
    {
        // update http state
        ProtoHttpUpdate(pVoipNarrate->pProtoHttp);
        // check receive state
        if ((iResult = ProtoHttpRecvAll(pVoipNarrate->pProtoHttp, (char *)pVoipNarrate->pToken, VOIPNARRATE_MICROSOFT_TOKENSIZE)) > 0)
        {
            int32_t iCode;
            if ((iCode = ProtoHttpStatus(pVoipNarrate->pProtoHttp, 'code', NULL, 0)) == PROTOHTTP_RESPONSE_OK)
            {
                pVoipNarrate->iTokenLen = iResult;
                pVoipNarrate->uTokenTick = uCurTick;
                NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: received bearer token (%d bytes, tick=%u)\n", pVoipNarrate->iTokenLen, pVoipNarrate->uTokenTick));
                pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_DONE;
            }
            else
            {
                NetPrintf(("voipnarrate: bearer token http request failed; result=%d\n%s\n", iCode, pVoipNarrate->pToken));
                pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_FAILED;
            }
        }
        else if ((iResult < 0) && (iResult != PROTOHTTP_RECVWAIT))
        {
            NetPrintf(("voipnarrate: bearer token request failed; err=%d\n", iResult));
            pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_FAILED;
        }
    }

    // failure processing... deal with a bearer token request failure
    if (pVoipNarrate->eHttpState == VOIPNARRATE_HTTPSTATE_FAILED)
    {
        // if we failed, pull a request off the queue so we don't keep failing; this limits us to one bearer token failure per request
        _VoipNarrateRequestGet(pVoipNarrate, NULL);
        // track failure in metrics
        pVoipNarrate->Metrics.uEventCount += 1;
        pVoipNarrate->Metrics.uErrorCount += 1;
        // move to done state
        pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_DONE;
    }

    // done with response... cleanup
    if (pVoipNarrate->eHttpState == VOIPNARRATE_HTTPSTATE_DONE)
    {
        pVoipNarrate->eHttpState = VOIPNARRATE_HTTPSTATE_IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadWatson

    \Description
        Format connection header for IBM Watson Speech service
        Ref: https://console.bluemix.net/docs/services/text-to-speech/http.html#usingHTTP

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer
    \Input eGender          - preferred gender for voice narration

    \Output
        int32_t             - negative=failure, else success

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadWatson(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen, VoipNarrateGenderE eGender)
{
    char strAuth[128];
    int32_t iOffset=0;

    // encode Basic authorization string with string apikey:<key>
    _VoipNarrateBasicAuth(strAuth, sizeof(strAuth), "apikey", pVoipNarrate->Config.strKey);

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Accept: audio/wav; rate=%d\r\n", VOIPNARRATE_SAMPLERATE);
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Authorization: Basic %s\r\n", strAuth);

    // format url with voice based on preferred gender and language
    ds_snzprintf(pUrl, iUrlLen, "%s?voice=%s", pVoipNarrate->Config.strUrl, _VoipNarrateGetVoice(pVoipNarrate, _Watson_aVoices, pVoipNarrate->strLang, eGender));
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyWatson

    \Description
        Format request body for IBM Watson Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

        \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyWatson(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, const char *pText)
{
    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);
    JsonAddStr(pBody, "text", pText);
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadMicrosoft

    \Description
        Format connection header for Microsoft Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadMicrosoft(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    int32_t iOffset=0;

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/ssml+xml\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "X-Microsoft-OutputFormat: raw-%dkhz-16bit-mono-pcm\r\n", VOIPNARRATE_SAMPLERATE/1000);
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Authorization: Bearer %s\r\n", pVoipNarrate->pToken);

    // copy url
    ds_strnzcpy(pUrl, pVoipNarrate->Config.strUrl, iUrlLen);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyMicrosoft

    \Description
        Format request body for Microsoft Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyMicrosoft(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    int32_t iOffset=0;
    // format request body
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "<speak version='1.0' xml:lang='%s'>", pVoipNarrate->strLang);
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "<voice xml:lang='%s' xml:gender='%s' name='%s'>%s</voice>",
        pVoipNarrate->strLang, (eGender == VOIPNARRATE_GENDER_FEMALE) ? "Female" : "Male",
        _VoipNarrateGetVoice(pVoipNarrate, _Microsoft_aVoices, pVoipNarrate->strLang, eGender),
        pText);
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "</speak>");
    // return pointer to body
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadGoogle

    \Description
        Format connection header for Google Text to Speech

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadGoogle(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    // format request header
    *pHead = '\0';
    // format request url
    ds_snzprintf(pUrl, iUrlLen, "%s?key=%s", pVoipNarrate->Config.strUrl, pVoipNarrate->Config.strKey);
    // return url
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyGoogle

    \Description
        Format request body for Google text-to-speech request

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Notes
        Ref: https://cloud.google.com/text-to-speech/docs/reference/rest/

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static char * _VoipNarrateFormatBodyGoogle(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    static const char *_strGender[VOIPNARRATE_NUMGENDERS] = { "SSML_VOICE_GENDER_UNSPECIFIED", "FEMALE", "MALE", "NEUTRAL" };

    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);

    JsonObjectStart(pBody, "input");
     JsonAddStr(pBody, "text", pText);
    JsonObjectEnd(pBody);

    JsonObjectStart(pBody, "voice");
     /* unlike other providers where we need to choose a specific voice based on language/gender, we just tell google what the language/gender are
        and let the service pick a voice for us */
     JsonAddStr(pBody, "languageCode", pVoipNarrate->strLang);
     JsonAddStr(pBody, "ssmlGender", _strGender[eGender]);
    JsonObjectEnd(pBody);

    JsonObjectStart(pBody, "audioConfig");
     JsonAddStr(pBody, "audioEncoding", "LINEAR16");
     JsonAddInt(pBody, "sampleRateHertz", VOIPNARRATE_SAMPLERATE);
    JsonObjectEnd(pBody);

    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadAmazon

    \Description
        Format connection header for Amazon Polly

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 11/21/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadAmazon(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    int32_t iOffset=0;
    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Accept: audio/wav; rate=%d\r\n", VOIPNARRATE_SAMPLERATE);
    // copy url
    ds_strnzcpy(pUrl, pVoipNarrate->Config.strUrl, iUrlLen);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyAmazon

    \Description
        Format request body for Amazon Polly

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 12/21/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyAmazon(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);
    JsonAddStr(pBody, "OutputFormat", "pcm");
    JsonAddStr(pBody, "Text", pText);
    JsonAddStr(pBody, "LanguageCode", pVoipNarrate->strLang);  // used for bilingual voices
    JsonAddStr(pBody, "VoiceId", _VoipNarrateGetVoice(pVoipNarrate, _Amazon_aVoices, pVoipNarrate->strLang, eGender));
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStreamCallbackGoogle

    \Description
        Decode Base64-encoded voice data

    \Input *pVoipNarrate    - pointer to module state
    \Input eStatus          - ProtoStream status
    \Input *pData           - base64-encoded input data
    \Input iDataSize        - input buffer length

    \Output
        int32_t             - negative=failure, else number of input bytes consumed

    \Version 10/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStreamCallbackGoogle(VoipNarrateRefT *pVoipNarrate, ProtoStreamStatusE eStatus, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iDataRead, iDataDecoded;

    // submit any base64-decoded data we have first
    if (pVoipNarrate->iVoiceLen > 0)
    {
        // if start of stream, see if we need to skip WAV header
        if (pVoipNarrate->bStart)
        {
            pVoipNarrate->iVoiceOff = _VoipNarrateSkipWavHeader(pVoipNarrate->aVoiceBuf, pVoipNarrate->iVoiceLen);
            pVoipNarrate->iVoiceLen -= pVoipNarrate->iVoiceOff;
            pVoipNarrate->bStart = FALSE;
        }
        // pass data to user
        iDataRead = pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)(pVoipNarrate->aVoiceBuf+pVoipNarrate->iVoiceOff), pVoipNarrate->iVoiceLen, pVoipNarrate->uFlags, pVoipNarrate->pUserData);
        // mark data as read
        pVoipNarrate->iVoiceOff += iDataRead;
        pVoipNarrate->iVoiceLen -= iDataRead;
    }
    // if we don't have data to decode, or we still have decoded voice data that hasn't been consumed yet, don't decode more
    if ((pVoipNarrate->iVoiceLen > 0) || (iDataSize <= 0))
    {
        return(0);
    }
    pVoipNarrate->iVoiceOff = 0;

    // base64-decode voice data 
    if ((iDataRead = _VoipNarrateBase64Decode((char *)pVoipNarrate->aVoiceBuf, (iDataDecoded = sizeof(pVoipNarrate->aVoiceBuf), &iDataDecoded), (const char *)pData, iDataSize)) >= 0)
    {
        pVoipNarrate->iVoiceLen = iDataDecoded;
        pVoipNarrate->iSamplesInPhrase += iDataDecoded;
        pData = pVoipNarrate->aVoiceBuf;
    }
    else
    {
        NetPrintf(("voipnarrate: error; could not base64 decode data\n"));
        NetPrintMem(pData, iDataSize, "base64 data");
        pVoipNarrate->Metrics.uErrorCount += 1;
    }
    return(iDataRead);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStreamCallback

    \Description
        Receive streamed voice data and submit it to callback

    \Input *pProtoStream    - ProtoStream module state
    \Input eStatus          - ProtoStream status
    \Input *pData           - base64-encoded input data
    \Input iDataSize        - input buffer length
    \Input *pUserData       - callback user data (VoipNarrate module ref)

    \Output
        int32_t             - negative=failure, else number of input bytes consumed

    \Version 10/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStreamCallback(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus, const uint8_t *pData, int32_t iDataSize, void *pUserData)
{
    VoipNarrateRefT *pVoipNarrate = (VoipNarrateRefT *)pUserData;
    int32_t iDataRead, iResult;
    char strError[256] = "";

    // handle start callback notification
    if (eStatus == PROTOSTREAM_STATUS_BEGIN)
    {
        pVoipNarrate->iSamplesInPhrase = 0;
        pVoipNarrate->Metrics.uDelay += NetTickDiff(NetTick(), pVoipNarrate->uTtsStartTime);
        pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, VOIPNARRATE_STREAM_START, pVoipNarrate->uFlags, pVoipNarrate->pUserData);
    }
    // handle end callback notification
    if (eStatus == PROTOSTREAM_STATUS_DONE)
    {
        // save metrics
        int32_t iPhraseDuration = ((pVoipNarrate->iSamplesInPhrase * 1000) / VOIPNARRATE_SAMPLERATE);
        pVoipNarrate->Metrics.uDurationMsRecv += iPhraseDuration;
        if (iPhraseDuration < VOIPNARRATE_EMPTY_AUDIO_THRESHOLD_MS)
        {
            pVoipNarrate->Metrics.uEmptyResultCount += 1;
        }
        
        // signal end of stream
        pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, VOIPNARRATE_STREAM_END, pVoipNarrate->uFlags, pVoipNarrate->pUserData);
        pVoipNarrate->bActive = FALSE;

        // check for a completion result that is not successful, and log error response (if any) to debug output
        if ((iResult = ProtoStreamStatus(pProtoStream, 'code', NULL, 0)) != PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            ProtoStreamStatus(pProtoStream, 'serr', strError, sizeof(strError));
            NetPrintf(("voipnarrate: stream failed with http result %d:\n%s\n", iResult, strError));
            pVoipNarrate->Metrics.uErrorCount += 1;
        }
    }

    // read data and pass it to callback, processing as necessary
    for (iDataRead = 0, iResult = 1; (iResult > 0) && (iDataSize > 0); )
    {
        if (pVoipNarrate->Config.eProvider != VOIPSPEECH_PROVIDER_GOOGLE)
        {
            // if start of stream, see if we need to skip WAV header
            if ((pVoipNarrate->bStart) && (iDataSize > 0))
            {
                iDataRead = _VoipNarrateSkipWavHeader(pData, iDataSize);
                pData += iDataRead;
                iDataSize -= iDataRead;
                pVoipNarrate->bStart = FALSE;
            }
            iResult = pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, iDataSize, pVoipNarrate->uFlags, pVoipNarrate->pUserData);
            pVoipNarrate->iSamplesInPhrase += iResult;
        }
        else
        {
            // google-specific processing to deal with base64 encoded audio
            iResult = _VoipNarrateStreamCallbackGoogle(pVoipNarrate, eStatus, pData, iDataSize);
        }

        iDataRead += iResult;
        iDataSize -= iResult;
        pData += iResult;
    }
    return(iDataRead);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStart

    \Description
        Receive streamed voice data and submit it to callback

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice for narration
    \Input uFlags           - callback flags
    \Input *pText           - pointer to text request

    \Output
        int32_t             - ProtoStream request result

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStart(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, uint32_t uFlags, const char *pText)
{
    const char *pUrl, *pReq;
    char strUrl[256];
    int32_t iResult;

    // format header/url and request body
    if (pVoipNarrate->Config.eProvider == VOIPSPEECH_PROVIDER_MICROSOFT)
    {
        pUrl = _VoipNarrateFormatHeadMicrosoft(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyMicrosoft(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPSPEECH_PROVIDER_GOOGLE)
    {
        pUrl = _VoipNarrateFormatHeadGoogle(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyGoogle(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPSPEECH_PROVIDER_IBMWATSON)
    {
        pUrl = _VoipNarrateFormatHeadWatson(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead), eGender);
        pReq = _VoipNarrateFormatBodyWatson(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPSPEECH_PROVIDER_AMAZON)
    {
        pUrl = _VoipNarrateFormatHeadAmazon(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyAmazon(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else
    {
        NetPrintf(("voipnarrate: undefined provider\n"));
        return(-1);
    }
    NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: request url: %s\n", pUrl));
    NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: request head\n%s\n", pVoipNarrate->strHead));
    NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: request body\n%s\n", pReq));
    pVoipNarrate->pBody = pReq;
    pVoipNarrate->uFlags = uFlags;

    // set request header
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'apnd', 0, 0, pVoipNarrate->strHead);

    pVoipNarrate->Metrics.uEventCount += 1;
    pVoipNarrate->Metrics.uCharCountSent += (uint32_t)strlen(pText);
    pVoipNarrate->uTtsStartTime = NetTick();

    // make the request
    if ((iResult = ProtoStreamOpen2(pVoipNarrate->pProtoStream, pUrl, pReq, PROTOSTREAM_FREQ_ONCE)) >= 0)
    {
        // mark as stream start and active
        pVoipNarrate->bStart = pVoipNarrate->bActive = TRUE;
    }
    else
    {
        NetPrintf(("voipnarrate: failed to open stream\n"));
        pVoipNarrate->Metrics.uErrorCount += 1;
    }

    // return to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateConfig

    \Description
        Configure the VoipNarrate module

    \Input *pVoipNarrate    - pointer to module state
    \Input *pConfig         - module configuration to set

    \Output
        uint32_t            - TRUE if configured successfully

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipNarrateConfig(VoipNarrateRefT *pVoipNarrate, VoipNarrateConfigT *pConfig)
{
    uint8_t uRet = TRUE;
    NetCritEnter(NULL);
    if (pConfig->eProvider != VOIPSPEECH_PROVIDER_NONE)
    {
        ds_memcpy_s(&pVoipNarrate->Config, sizeof(pVoipNarrate->Config), pConfig, sizeof(*pConfig));
    }
    else
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 0, "voipnarrate: narration disabled\n"));
        ds_memclr(&pVoipNarrate->Config, sizeof(pVoipNarrate->Config));
        uRet = FALSE;
    }
    NetCritLeave(NULL);
    return(uRet);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipNarrateCreate

    \Description
        Create the narration module

    \Input *pVoiceDataCb        - callback used to provide voice data
    \Input *pUserData           - callback user data

    \Output
        VoipNarrateRefT *       - new module state, or NULL

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
VoipNarrateRefT *VoipNarrateCreate(VoipNarrateVoiceDataCbT *pVoiceDataCb, void *pUserData)
{
    VoipNarrateRefT *pVoipNarrate;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // validate callback
    if (pVoiceDataCb == NULL)
    {
        NetPrintf(("voipnarrate: could not create module with null callback\n"));
        return(NULL);
    }

    // allocate and init module state
    if ((pVoipNarrate = DirtyMemAlloc(sizeof(*pVoipNarrate), VOIPNARRATE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarrate: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipNarrate, sizeof(*pVoipNarrate));
    pVoipNarrate->iMemGroup = iMemGroup;
    pVoipNarrate->pMemGroupUserData = pMemGroupUserData;
    pVoipNarrate->pVoiceDataCb = pVoiceDataCb;
    pVoipNarrate->pUserData = pUserData;
    pVoipNarrate->iVerbose = 1;

    // set default language
    ds_strnzcpy(pVoipNarrate->strLang, "en-US", sizeof(pVoipNarrate->strLang));

    // allocate streaming module with a buffer to hold up to 1s of 16khz 16bit streaming audio
    if ((pVoipNarrate->pProtoStream = ProtoStreamCreate(16*2*1024)) == NULL)
    {
        VoipNarrateDestroy(pVoipNarrate);
        return(NULL);
    }
    // set protostream callback with a 20ms call rate
    ProtoStreamSetCallback(pVoipNarrate->pProtoStream, 20, _VoipNarrateStreamCallback, pVoipNarrate);
    // set protostream minimum data amount (for base64 decoding; four is the minimum amount but that produces one and a half samples, so we choose eight)
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'minb', 8, 0, NULL);
    // set protostream debug level
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'spam', 1, 0, NULL);
    // set keepalive
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'keep', 1, 0, NULL);
    // set protostream http custom header callback, used to sign AWS requests
    ProtoStreamSetHttpCallback(pVoipNarrate->pProtoStream, _VoipNarrateCustomHeaderCb, NULL, pVoipNarrate);

    // configure for particular provider
    if (!_VoipNarrateConfig(pVoipNarrate, &_VoipNarrate_Config))
    {
        NetPrintf(("voipnarrate: could not configure for provider\n"));
        VoipNarrateDestroy(pVoipNarrate);
        return(NULL);
    }

    // return ref to caller
    return(pVoipNarrate);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateConfig

    \Description
        Set global state to configure the VoipNarrate modules

    \Input eProvider        - VOIPSPEECH_PROVIDER_* (VOIPSPEECH_PROVIDER_NONE to disable)
    \Input *pUrl            - pointer to url to use for tts requests
    \Input *pKey            - pointer to authentication key to use for tts requests

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateConfig(VoipSpeechProviderE eProvider, const char *pUrl, const char *pKey)
{
    NetCritEnter(NULL);
    _VoipNarrate_Config.eProvider = eProvider;
    ds_strnzcpy(_VoipNarrate_Config.strUrl, pUrl, sizeof(_VoipNarrate_Config.strUrl));
    ds_strnzcpy(_VoipNarrate_Config.strKey, pKey, sizeof(_VoipNarrate_Config.strKey));
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateDestroy

    \Description
        Destroy the VoipNarrate module

    \Input *pVoipNarrate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateDestroy(VoipNarrateRefT *pVoipNarrate)
{
    // destroy protostream module, if allocated
    if (pVoipNarrate->pProtoStream != NULL)
    {
        ProtoStreamDestroy(pVoipNarrate->pProtoStream);
    }
    // release any queued requests
    while (pVoipNarrate->pRequest != NULL)
    {
        _VoipNarrateRequestGet(pVoipNarrate, NULL);
    }
    // dispose of module memory
    DirtyMemFree(pVoipNarrate, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateInput

    \Description
        Input text to be convert to speech

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice narration
    \Input uFlags           - callback flags
    \Input *pText           - text to be converted

    \Output
        int32_t             - zero=success, otherwise=failure

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateInput(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, uint32_t uFlags, const char *pText)
{
    // make sure a provider is configured
    if (pVoipNarrate->Config.eProvider == VOIPSPEECH_PROVIDER_NONE)
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 0, "voipnarrate: no provider configured\n"));
        return(-1);
    }
    // handle if there is already narration ongoing
    if (pVoipNarrate->bActive || !_VoipNarrateBearerTokenValid(pVoipNarrate, NetTick()))
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: queueing request '%s'\n", pText));
        return(_VoipNarrateRequestAdd(pVoipNarrate, iUserIndex, eGender, uFlags, pText));
    }
    // if ready, start the request
    return(_VoipNarrateStart(pVoipNarrate, iUserIndex, eGender, uFlags, pText));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateStatus

    \Description
        Get module status.

    \Input *pVoipNarrate    - pointer to module state
    \Input iStatus          - status selector
    \Input iValue           - selector specific
    \Input *pBuffer         - selector specific
    \Input iBufSize         - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        Other status codes are passed down to the stream transport handler.

    \verbatim
        'ttsm' - get the VoipTextToSpeechMetricsT via pBuffer
    \endverbatim

    \Version 11/15/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateStatus(VoipNarrateRefT *pVoipNarrate, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    if (iStatus == 'ttsm')
    {
        if ((pBuffer != NULL) && (iBufSize >= (int32_t)sizeof(VoipTextToSpeechMetricsT)))
        {
            ds_memcpy_s(pBuffer, iBufSize, &pVoipNarrate->Metrics, sizeof(VoipTextToSpeechMetricsT));
            return(0);
        }
        return(-1);
    }
    return(ProtoStreamStatus(pVoipNarrate->pProtoStream, iStatus, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateControl

    \Description
        Set control options

    \Input *pVoipNarrate    - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'conf' - refresh config
            'ctsm' - clear text to speech metrics in VoipTextToSpeechMetricsT
            'lang' - set language
            'spam' - set verbose debug level (debug only)
        \endverbatim

        Unhandled codes are passed through to the stream transport handler

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateControl(VoipNarrateRefT *pVoipNarrate, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // refresh config
    if (iControl == 'conf')
    {
        return(_VoipNarrateConfig(pVoipNarrate, &_VoipNarrate_Config));
    }
    // clear tts metrics
    if (iControl == 'ctsm')
    {
        ds_memclr(&(pVoipNarrate->Metrics), sizeof(pVoipNarrate->Metrics));
        return(0);
    }
    // set language
    if (iControl == 'lang')
    {
        ds_strnzcpy(pVoipNarrate->strLang, pValue, sizeof(pVoipNarrate->strLang));
        NetPrintf(("voipnarrate: setting language to %s\n", pVoipNarrate->strLang));
        return(0);
    }
    #if DIRTYCODE_LOGGING
    // set verbosity for us and pass through to stream transport handler
    if (iControl == 'spam')
    {
        pVoipNarrate->iVerbose = iValue;
    }
    #endif
    // if not handled, let stream transport handler take a stab at it
    return(ProtoStreamControl(pVoipNarrate->pProtoStream, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateUpdate

    \Description
        Update the narration module

    \Input *pVoipNarrate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateUpdate(VoipNarrateRefT *pVoipNarrate)
{
    // update bearer token processing if required
    _VoipNarrateBearerTokenUpdate(pVoipNarrate);

    // see if we need to start a queued narration request
    if ((pVoipNarrate->pRequest != NULL) && !pVoipNarrate->bActive && _VoipNarrateBearerTokenValid(pVoipNarrate, NetTick()))
    {
        VoipNarrateRequestT Request;
        _VoipNarrateRequestGet(pVoipNarrate, &Request);
        _VoipNarrateStart(pVoipNarrate, Request.iUserIndex, Request.eGender, Request.uFlags, Request.strText);
    }

    // give life to stream module
    ProtoStreamUpdate(pVoipNarrate->pProtoStream);
}
