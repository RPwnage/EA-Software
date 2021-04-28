/*****************************************************************************
*
*/
#ifndef __ICUSTRING_H
#define __ICUSTRING_H

#include <unicode/utypes.h>
//#include <unicode/putil.h>
#include <unicode/ucnv.h>
//#include <unicode/uenum.h>
#include <unicode/unistr.h>
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

class ICUString
{
    public:
        ICUString();
        ~ICUString();

        int SetStringFromBuffer (const char* srcBuffer, size_t bufsz, const char* fromcpage);
        UnicodeString* unicodeStr() {
            return &mUnicodeStr;
        }

    private:
        UnicodeString mUnicodeStr;
};

#endif //__ICUSTRING_H
