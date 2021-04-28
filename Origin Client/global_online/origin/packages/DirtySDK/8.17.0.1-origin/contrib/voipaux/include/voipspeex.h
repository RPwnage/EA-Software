/*H********************************************************************************/
/*!
    \File voipspeex.h

    \Description
        PC Audio Encoder / Decoder using Speex

    \Copyright
        Copyright (c) Electronic Arts 2007. ALL RIGHTS RESERVED.

    \Version 1.0 04/02/2007 (cadam) First version
*/
/********************************************************************************H*/

#ifndef _voipspeex_h
#define _voipspeex_h

/*** Include files ****************************************************************/

#include "platform.h"
#include "voipcodec.h"
#include "voip.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// speex codec definition
extern const VoipCodecDefT VoipSpeex_CodecDef;

/*** Functions ********************************************************************/

#ifdef __cplusplus
}
#endif

#endif // _voipspeex_h
