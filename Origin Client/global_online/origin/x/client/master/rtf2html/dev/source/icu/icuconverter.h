/*****************************************************************************
*
*   Copyright (C) 1999-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************/

/*
 * uconv(1): an iconv(1)-like converter using ICU.
 *
 * Original code by Jonas Utterstr&#x00F6;m <jonas.utterstrom@vittran.norrnod.se>
 * contributed in 1999.
 *
 * Conversion to the C conversion API and many improvements by
 * Yves Arrouye <yves@realnames.com>, current maintainer.
 *
 * Markus Scherer maintainer from 2003.
 * See source code repository history for changes.
 */
#ifndef __ICUCONVERTER_H
#define __ICUCONVERTER_H

//#include <unicode/utypes.h>
//#include <unicode/putil.h>
#include <unicode/ucnv.h>
//#include <unicode/uenum.h>
//#include <unicode/unistr.h>
//#include <unicode/translit.h>
//#include <unicode/uset.h>
//#include <unicode/uclean.h>
//#include <unicode/utf16.h>

//#include <stdio.h>
//#include <errno.h>
//#include <string.h>
//#include <stdlib.h>
#include <vector>

//#include "cmemory.h"
//#include "cstring.h"
//#include "ustrfmt.h"

//#include "unicode/uwmsg.h"
#define USE_BUFFER

U_NAMESPACE_USE

#define UCNV_TO_DEFAULT "utf-16"        //set it as the default convert TO

class ICUConverter
{
    public:
        ICUConverter();
        ~ICUConverter();

        void setBuffer(char* srcBuf, size_t bufferSize);

        UBool convertBuffer(size_t* outlen,
                            std::vector<char>* outVector);

        int SetEncodeFrom (const char* fromcpage);
        int SetEncodeTo (const char* tocpage, bool fallback);

        void CloseEncodeFrom ();
        void CloseEncodeTo ();

        int CodePage();
        void SetEncoding (int codePage);
        const char* CodePageName();

    private:
        UConverter* mConvfrom;
        UConverter* mConvto;

        char* buf, *outbuf;
        int32_t* fromoffsets;

        size_t bufsz;
        int8_t signature; // add (1) or remove (-1) a U+FEFF Unicode signature character

        int mCodePage;
        std::string mCodePageName;
};
#endif //__ICUCONVERTER_H
