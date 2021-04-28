/*H********************************************************************************/
/*!
    \File voipcelp.h

    \Description
        Sony CELP Audio Encoder / ADEC decoder

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 10/25/2006 (tdills) First version
*/
/********************************************************************************H*/

#ifndef _voipcelp_h
#define _voipcelp_h

/*** Include files ****************************************************************/

#include "DirtySDK/voip/voipcodec.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// 16khz CELP codec
extern const VoipCodecDefT   VoipCelp_CodecDef;

// 8khz CELP codec
extern const VoipCodecDefT   VoipCelp8_CodecDef;


/*** Functions ********************************************************************/

#endif // _voipcelp_h
