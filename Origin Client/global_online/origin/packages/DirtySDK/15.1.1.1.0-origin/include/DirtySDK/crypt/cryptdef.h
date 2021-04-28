/*H*******************************************************************/
/*!
    \File cryptdef.h

    \Description
        Common definitions for Crypt modules

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/19/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

#ifndef _cryptdef_h
#define _cryptdef_h

/*!
    \Moduledef Crypt Crypt

    \Description
        The Crypt module provides routines for encryption and decryption.

        <b>Overview</b>

        Overview TBD
*/

/*!
\Moduledef CryptDef CryptDef
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"

/*** Defines *********************************************************/

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

//! a binary large object, such as a modulus or exponent
typedef struct CryptBinaryObjT
{
    uint8_t *pObjData;
    int32_t iObjSize;
} CryptBinaryObjT;

//@}

#endif // _cryptdef_h

