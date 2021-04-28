/*****************************************************************************
*
*/
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

//#include "cmemory.h"
//#include "cstring.h"
//#include "ustrfmt.h"

//#include "unicode/uwmsg.h"
#include <assert.h>
#include "icustring.h"

U_NAMESPACE_USE

ICUString::ICUString ()
{
    mUnicodeStr.remove();
}

ICUString::~ICUString()
{
    mUnicodeStr.remove();
}

int ICUString::SetStringFromBuffer (const char* srcBuffer, size_t bufsz, const char* fromcpage)
{
    UErrorCode err = U_ZERO_ERROR;

    UConverter* convfrom = ucnv_open(fromcpage, &err);

    UConverterToUCallback toucallback = UCNV_TO_U_CALLBACK_STOP;
    const void* touctxt = 0;
    ucnv_setToUCallBack(convfrom, toucallback, touctxt, 0, 0, &err);

    const UChar* unibuf; //, *unibufbp;
    UChar* unibufp;

    //to make sure it fits, set capacity to double the size
    unibuf = unibufp = mUnicodeStr.getBuffer((int32_t)(bufsz*2));

    const char* buf = srcBuffer;

    ucnv_toUnicode(convfrom, &unibufp, unibuf + bufsz, &srcBuffer,
                   buf + bufsz, NULL, true, &err);

    if (!U_SUCCESS(err))
    {
        assert (0);
    }

    int32_t ulen = (int32_t)(unibufp - unibuf);
    //reset it to the correct size
    mUnicodeStr.releaseBuffer(U_SUCCESS(err) ? ulen : 0);

    ucnv_close(convfrom);
    return ((int)err);
}
