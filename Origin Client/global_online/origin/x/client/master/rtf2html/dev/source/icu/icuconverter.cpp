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

#include "icuconverter.h"

#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/ucnv.h>
#include <unicode/uenum.h>
#include <unicode/unistr.h>
#include <unicode/translit.h>
#include <unicode/uset.h>
#include <unicode/uclean.h>
#include <unicode/utf16.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "cmemory.h"
#include "cstring.h"
#include "ustrfmt.h"

//#include "unicode/uwmsg.h"
#define USE_BUFFER

U_NAMESPACE_USE

#if U_PLATFORM_USES_ONLY_WIN32_API && !defined(__STRICT_ANSI__)
#include <io.h>
#include <fcntl.h>
#if U_PLATFORM_USES_ONLY_WIN32_API
#define USE_FILENO_BINARY_MODE 1
/* Windows likes to rename Unix-like functions */
#ifndef fileno
#define fileno _fileno
#endif
#ifndef setmode
#define setmode _setmode
#endif
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif
#endif
#endif

#if UWMSG
#ifdef UCONVMSG_LINK
/* below from the README */
#include "unicode/utypes.h"
#include "unicode/udata.h"
U_CFUNC char uconvmsg_dat[];
#endif

#else
//turn everything off related to u_wmsg since having trouble building -- it's just to show error anyways
#define u_wmsg void()
#define u_wmsg_errorName void()
//#define udata_setAppData void()
#define uconvmsg_dat NULL
#endif

#define DEFAULT_BUFSZ   4096
#define UCONVMSG "uconvmsg"

#if UWMSG
static UResourceBundle* gBundle = 0;    /* Bundle containing messages. */
#endif

#if 0 //UNIMPLEMENTED, currently NOT USED
/*
 * Initialize the message bundle so that message strings can be fetched
 * by u_wmsg().
 *
 */

static void initMsg(const char* pname) {
    static int ps = 0;

    if (!ps) {
        char dataPath[2048];        /* XXX Sloppy: should be PATH_MAX. */
        UErrorCode err = U_ZERO_ERROR;

        ps = 1;

        /* Set up our static data - if any */
#if defined(UCONVMSG_LINK) && U_PLATFORM != U_PF_OS390 /* On z/OS, this is failing. */
        udata_setAppData(UCONVMSG, (const void*) uconvmsg_dat, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr, "%s: warning, problem installing our static resource bundle data uconvmsg: %s - trying anyways.\n",
                    pname, u_errorName(err));
            err = U_ZERO_ERROR; /* It may still fail */
        }
#endif

        /* Get messages. */
#if UWMSG
        gBundle = u_wmsg_setPath(UCONVMSG, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr,
                    "%s: warning: couldn't open bundle %s: %s\n",
                    pname, UCONVMSG, u_errorName(err));
#ifdef UCONVMSG_LINK
            fprintf(stderr,
                    "%s: setAppData was called, internal data %s failed to load\n",
                    pname, UCONVMSG);
#endif

            err = U_ZERO_ERROR;
            /* that was try #1, try again with a path */
            uprv_strcpy(dataPath, u_getDataDirectory());
            uprv_strcat(dataPath, U_FILE_SEP_STRING);
            uprv_strcat(dataPath, UCONVMSG);

            gBundle = u_wmsg_setPath(dataPath, &err);
            if (U_FAILURE(err)) {
                fprintf(stderr,
                        "%s: warning: still couldn't open bundle %s: %s\n",
                        pname, dataPath, u_errorName(err));
                fprintf(stderr, "%s: warning: messages will not be displayed\n", pname);
            }
        }
#endif
    }
}
#endif

/* Mapping of callback names to the callbacks passed to the converter
   API. */

static struct callback_ent {
    const char* name;
    UConverterFromUCallback fromu;
    const void* fromuctxt;
    UConverterToUCallback tou;
    const void* touctxt;
} transcode_callbacks[] = {
    {   "substitute",
        UCNV_FROM_U_CALLBACK_SUBSTITUTE, 0,
        UCNV_TO_U_CALLBACK_SUBSTITUTE, 0
    },
    {   "skip",
        UCNV_FROM_U_CALLBACK_SKIP, 0,
        UCNV_TO_U_CALLBACK_SKIP, 0
    },
    {   "stop",
        UCNV_FROM_U_CALLBACK_STOP, 0,
        UCNV_TO_U_CALLBACK_STOP, 0
    },
    {   "escape",
        UCNV_FROM_U_CALLBACK_ESCAPE, 0,
        UCNV_TO_U_CALLBACK_ESCAPE, 0
    },
    {   "escape-icu",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_ICU,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_ICU
    },
    {   "escape-java",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_JAVA,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_JAVA
    },
    {   "escape-c",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C
    },
    {   "escape-xml",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX
    },
    {   "escape-xml-hex",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX
    },
    {   "escape-xml-dec",
        UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC
    },
    {   "escape-unicode", UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_UNICODE,
        UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_UNICODE
    }
};

#if 0 //UNIMPLEMENTED, currently NOT USED
/* Print converter information. If lookfor is set, only that converter will
   be printed, otherwise all converters will be printed. If canon is non
   zero, tags and aliases for each converter are printed too, in the format
   expected for convrters.txt(5). */

static int printConverters(const char* pname, const char* lookfor,
                           UBool canon)
{
    UErrorCode err = U_ZERO_ERROR;
    int32_t num;
    uint16_t num_stds;
    const char** stds;

    /* If there is a specified name, just handle that now. */

    if (lookfor) {
        if (!canon) {
            printf("%s\n", lookfor);
            return 0;
        } else {
            /*  Because we are printing a canonical name, we need the
                true converter name. We've done that already except for
                the default name (because we want to print the exact
                name one would get when calling ucnv_getDefaultName()
                in non-canon mode). But since we do not know at this
                point if we have the default name or something else, we
                need to normalize again to the canonical converter
                name. */

            const char* truename = ucnv_getAlias(lookfor, 0, &err);
            if (U_SUCCESS(err)) {
                lookfor = truename;
            } else {
                err = U_ZERO_ERROR;
            }
        }
    }

    /* Print converter names. We come here for one of two reasons: we
       are printing all the names (lookfor was null), or we have a
       single converter to print but in canon mode, hence we need to
       get to it in order to print everything. */

    num = ucnv_countAvailable();
    if (num <= 0) {
        initMsg(pname);
        u_wmsg(stderr, "cantGetNames");
        return -1;
    }
    if (lookfor) {
        num = 1;                /* We know where we want to be. */
    }

    num_stds = ucnv_countStandards();
    stds = (const char**) uprv_malloc(num_stds * sizeof(*stds));
    if (!stds) {
        u_wmsg(stderr, "cantGetTag", u_wmsg_errorName(U_MEMORY_ALLOCATION_ERROR));
        return -1;
    } else {
        uint16_t s;

        if (canon) {
            printf("{ ");
        }
        for (s = 0; s < num_stds; ++s) {
            stds[s] = ucnv_getStandard(s, &err);
            if (canon) {
                printf("%s ", stds[s]);
            }
            if (U_FAILURE(err)) {
                u_wmsg(stderr, "cantGetTag", u_wmsg_errorName(err));
                goto error_cleanup;
            }
        }
        if (canon) {
            puts("}");
        }
    }

    for (int32_t i = 0; i < num; i++) {
        const char* name;
        uint16_t num_aliases;

        /* Set the name either to what we are looking for, or
        to the current converter name. */

        if (lookfor) {
            name = lookfor;
        } else {
            name = ucnv_getAvailableName(i);
        }

        /* Get all the aliases associated to the name. */

        err = U_ZERO_ERROR;
        num_aliases = ucnv_countAliases(name, &err);
        if (U_FAILURE(err)) {
            printf("%s", name);

            UnicodeString str(name, "");
            putchar('\t');
            u_wmsg(stderr, "cantGetAliases", str.getTerminatedBuffer(),
                   u_wmsg_errorName(err));
            goto error_cleanup;
        } else {
            uint16_t a, s, t;

            /* Write all the aliases and their tags. */

            for (a = 0; a < num_aliases; ++a) {
                const char* alias = ucnv_getAlias(name, a, &err);

                if (U_FAILURE(err)) {
                    UnicodeString str(name, "");
                    putchar('\t');
                    u_wmsg(stderr, "cantGetAliases", str.getTerminatedBuffer(),
                           u_wmsg_errorName(err));
                    goto error_cleanup;
                }

                /* Print the current alias so that it looks right. */
                printf("%s%s%s", (canon ? (a == 0? "" : "\t" ) : "") ,
                       alias,
                       (canon ? "" : " "));

                /* Look (slowly, linear searching) for a tag. */

                if (canon) {
                    /* -1 to skip the last standard */
                    for (s = t = 0; s < num_stds-1; ++s) {
                        UEnumeration* nameEnum = ucnv_openStandardNames(name, stds[s], &err);
                        if (U_SUCCESS(err)) {
                            /* List the standard tags */
                            const char* standardName;
                            UBool isFirst = TRUE;
                            UErrorCode enumError = U_ZERO_ERROR;
                            while ((standardName = uenum_next(nameEnum, NULL, &enumError))) {
                                /* See if this alias is supported by this standard. */
                                if (!strcmp(standardName, alias)) {
                                    if (!t) {
                                        printf(" {");
                                        t = 1;
                                    }
                                    /* Print a * after the default standard name */
                                    printf(" %s%s", stds[s], (isFirst ? "*" : ""));
                                }
                                isFirst = FALSE;
                            }
                        }
                    }
                    if (t) {
                        printf(" }");
                    }
                }
                /* Terminate this entry. */
                if (canon) {
                    puts("");
                }

                /* Move on. */
            }
            /* Terminate this entry. */
            if (!canon) {
                puts("");
            }
        }
    }

    /* Free temporary data. */

    uprv_free(stds);

    /* Success. */

    return 0;
error_cleanup:
    uprv_free(stds);
    return -1;
}
#endif

/* Print all available transliterators. If canon is non zero, print
   one transliterator per line. */

#if 0 //UNIMPLEMENTED, currently NOT USED
static int printTransliterators(UBool canon)
{
#if UCONFIG_NO_TRANSLITERATION
    printf("no transliterators available because of UCONFIG_NO_TRANSLITERATION, see uconfig.h\n");
    return 1;
#else
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration* ids = utrans_openIDs(&status);
    int32_t i, numtrans = uenum_count(ids, &status);

    char sepchar = canon ? '\n' : ' ';

    for (i = 0; U_SUCCESS(status)&& (i < numtrans); ++i) {
        int32_t len;
        const char* nextTrans = uenum_next(ids, &len, &status);

        printf("%s", nextTrans);
        if (i < numtrans - 1) {
            putchar(sepchar);
        }
    }

    uenum_close(ids);

    /* Add a terminating newline if needed. */

    if (sepchar != '\n') {
        putchar('\n');
    }

    /* Success. */

    return 0;
#endif
}
#endif

enum {
    uSP = 0x20,         // space
    uCR = 0xd,          // carriage return
    uLF = 0xa,          // line feed
    uNL = 0x85,         // newline
    uLS = 0x2028,       // line separator
    uPS = 0x2029,       // paragraph separator
    uSig = 0xfeff       // signature/BOM character
};

static inline int32_t
getChunkLimit(const UnicodeString& prev, const UnicodeString& s) {
    // find one of
    // CR, LF, CRLF, NL, LS, PS
    // for paragraph ends (see UAX #13/Unicode 4)
    // and include it in the chunk
    // all of these characters are on the BMP
    // do not include FF or VT in case they are part of a paragraph
    // (important for bidi contexts)
    static const UChar paraEnds[] = {
        0xd, 0xa, 0x85, 0x2028, 0x2029
    };
    enum {
        iCR, iLF, iNL, iLS, iPS, iCount
    };

    // first, see if there is a CRLF split between prev and s
    if (prev.endsWith(paraEnds + iCR, 1)) {
        if (s.startsWith(paraEnds + iLF, 1)) {
            return 1; // split CRLF, include the LF
        } else if (!s.isEmpty()) {
            return 0; // complete the last chunk
        } else {
            return -1; // wait for actual further contents to arrive
        }
    }

    const UChar* u = s.getBuffer(), *limit = u + s.length();
    UChar c;

    while (u < limit) {
        c = *u++;
        if (
            ((c < uSP) && (c == uCR || c == uLF)) ||
            (c == uNL) ||
            ((c & uLS) == uLS)
        ) {
            if (c == uCR) {
                // check for CRLF
                if (u == limit) {
                    return -1; // LF may be in the next chunk
                } else if (*u == uLF) {
                    ++u; // include the LF in this chunk
                }
            }
            return (int32_t)(u - s.getBuffer());
        }
    }

    return -1; // continue collecting the chunk
}

enum {
    CNV_NO_FEFF,    // cannot convert the U+FEFF Unicode signature character (BOM)
    CNV_WITH_FEFF,  // can convert the U+FEFF signature character
    CNV_ADDS_FEFF   // automatically adds/detects the U+FEFF signature character
};

static inline UChar
nibbleToHex(uint8_t n) {
    n &= 0xf;
    return
        n <= 9 ?
        (UChar)(0x30 + n) :
        (UChar)((0x61 - 10) + n);
}

// check the converter's Unicode signature properties;
// the fromUnicode side of the converter must be in its initial state
// and will be reset again if it was used
static int32_t
cnvSigType(UConverter* cnv) {
    UErrorCode err;
    int32_t result;

    // test if the output charset can convert U+FEFF
    USet* set = uset_open(1, 0);
    err = U_ZERO_ERROR;
    ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &err);
    if (U_SUCCESS(err) && uset_contains(set, uSig)) {
        result = CNV_WITH_FEFF;
    } else {
        result = CNV_NO_FEFF; // an error occurred or U+FEFF cannot be converted
    }
    uset_close(set);

    if (result == CNV_WITH_FEFF) {
        // test if the output charset emits a signature anyway
        const UChar a[1] = { 0x61 }; // "a"
        const UChar* in;

        char buffer[20];
        char* out;

        in = a;
        out = buffer;
        err = U_ZERO_ERROR;
        ucnv_fromUnicode(cnv,
                         &out, buffer + sizeof(buffer),
                         &in, a + 1,
                         NULL, TRUE, &err);
        ucnv_resetFromUnicode(cnv);

        if (NULL != ucnv_detectUnicodeSignature(buffer, (int32_t)(out - buffer), NULL, &err) &&
                U_SUCCESS(err)
           ) {
            result = CNV_ADDS_FEFF;
        }
    }

    return result;
}

ICUConverter::ICUConverter()
    : mConvfrom(NULL)
    , mConvto(NULL)
    , buf(NULL)
    , outbuf(NULL)
    , fromoffsets(NULL)
    , bufsz(0)
    , signature(0)
{
    SetEncodeFrom(ucnv_getDefaultName());
    SetEncodeTo(UCNV_TO_DEFAULT, false);
    //assume it's Windows ASCII 1252
    mCodePage = 1252;
}

ICUConverter::~ICUConverter()
{
    CloseEncodeFrom();
    CloseEncodeTo();

    if (buf)
    {
        delete [] buf;
        buf = NULL;
    }
    if (fromoffsets)
    {
        delete [] fromoffsets;
        fromoffsets = NULL;
    }
}

void ICUConverter::setBuffer(char* srcBuf, size_t bufferSize)
{
    bufsz = bufferSize;

    buf = new char[2 * bufsz];
    outbuf = buf + bufsz;

    // +1 for an added U+FEFF in the intermediate Unicode buffer
    fromoffsets = new int32_t[bufsz + 1];
    memcpy (buf, srcBuf, bufsz);
}

void ICUConverter::CloseEncodeFrom()
{
    if (mConvfrom)
    {
        ucnv_close(mConvfrom);
        mConvfrom = NULL;
    }
}

void ICUConverter::CloseEncodeTo()
{
    if (mConvto)
    {
        ucnv_close(mConvto);
        mConvfrom = NULL;
    }
}

int ICUConverter::SetEncodeFrom (const char* fromcpage)
{
    UErrorCode err = U_ZERO_ERROR;

    //in case previous one wasn't closed
    CloseEncodeFrom();

    mConvfrom = ucnv_open(fromcpage, &err);
    if (U_FAILURE(err)) {
#if 0 //UNIMPLEMENTED, currently NOT USED
        UnicodeString str(fromcpage, "");
        initMsg(pname);
        u_wmsg(stderr, "cantOpenFromCodeset", str.getTerminatedBuffer(),
               u_wmsg_errorName(err));
#endif
        return -1;
        //goto error_exit;
    }

    UConverterToUCallback toucallback = UCNV_TO_U_CALLBACK_STOP;
    const void* touctxt = 0;
    ucnv_setToUCallBack(mConvfrom, toucallback, touctxt, 0, 0, &err);
    if (U_FAILURE(err)) {
#if 0 //UNIMPLEMENTED, currently NOT USED
        initMsg(pname);
        u_wmsg(stderr, "cantSetCallback", u_wmsg_errorName(err));
#endif
        return -2;
        //goto error_exit;
    }

    return 0;
}

int ICUConverter::SetEncodeTo (const char* tocpage, bool fallback)
{
    UErrorCode err = U_ZERO_ERROR;

    //in case previous one wasn't closed
    CloseEncodeTo();
    mConvto = ucnv_open(tocpage, &err);
    if (U_FAILURE(err)) {
#if 0 //UNIMPLEMENTED, currently NOT USED
        UnicodeString str(tocpage, "");
        initMsg(pname);
        u_wmsg(stderr, "cantOpenToCodeset", str.getTerminatedBuffer(),
               u_wmsg_errorName(err));
#endif
        return -3;
        //goto error_exit;
    }

    UConverterFromUCallback fromucallback = UCNV_FROM_U_CALLBACK_STOP;
    const void* fromuctxt = 0;
    ucnv_setFromUCallBack(mConvto, fromucallback, fromuctxt, 0, 0, &err);
    if (U_FAILURE(err)) {
#if 0 //UNIMPLEMENTED, currently NOT USED
        initMsg(pname);
        u_wmsg(stderr, "cantSetCallback", u_wmsg_errorName(err));
#endif
        return -4;
        //goto error_exit;
    }
    ucnv_setFallback(mConvto, fallback);
    return 0;
}

int ICUConverter::CodePage()
{
    return mCodePage;
}

const char* ICUConverter::CodePageName()
{
    return mCodePageName.data();
}

void ICUConverter::SetEncoding (int codePage)
{
    char cpName [256];


    mCodePage = codePage;

    switch (codePage)
    {
        case 437: // PC 437
        case 850: // OEM
        case 874: // Thai
        case 932: // Shift JIS
        case 936: // GB2312
        case 949: // Hangul
        case 950: // Big5
        case 1250: // Eastern European
        case 1251: // Russian
        case 1252:  // ANSI
        case 1253: // Greek
        case 1254: // Turkish
        case 1255: // Hebrew
        case 1256: // Arabic
        case 1257: // Baltic
        case 1258: // Vietnamese
            sprintf (cpName, "cp%d", codePage);
            break;

        case 10000: // Mac Roman
            sprintf (cpName, "%s", "macroman");
            break;

        case 10006: // Mac Greek
            sprintf (cpName, "%s", "macgr");
            break;

        case 10007: // Mac Russian
            sprintf (cpName, "%s", "mac-cyrillic");
            break;

        case 10081: // Mac Turkish
            sprintf (cpName, "%s", "mactr");
            break;

        case 0:
        default:
            mCodePage = 0;
            sprintf (cpName, "%s", ucnv_getDefaultName());
            break;
    }

    SetEncodeFrom (cpName);
    mCodePageName.clear();
    mCodePageName.append(cpName);
}



// Convert buffer, input buffer = buf, output = outbuf
UBool ICUConverter::convertBuffer(size_t* outlen,
                                  std::vector<char>* outVector)
{
    UBool ret = TRUE;
    UErrorCode err = U_ZERO_ERROR;
    UBool flush;
    const char* cbufp, *prevbufp;
    char* bufp;

    uint32_t infoffset = 0, outfoffset = 0;   /* Where we are in the file, for error reporting. */

    const UChar* unibuf, *unibufbp;
    UChar* unibufp;

    size_t rd; //, wr;

#if !UCONFIG_NO_TRANSLITERATION
//    Transliterator *t = 0;      // Transliterator acting on Unicode data.
    UnicodeString chunk;        // One chunk of the text being collected for transformation.
#endif
    UnicodeString u;            // String to do the transliteration.
    int32_t ulen;

    // use conversion offsets for error messages
    // unless a transliterator is used -
    // a text transformation will reorder characters in unpredictable ways
    UBool useOffsets = TRUE;

    UBool willexit, fromSawEndOfBytes, toSawEndOfUnicode;
    int8_t sig;

    // OK, we can convert now.
    sig = signature;
    rd = bufsz;

    do {
        willexit = FALSE;

        /*
                // input file offset at the beginning of the next buffer
                infoffset += rd;

                rd = fread(buf, 1, bufsz, infile);
                if (ferror(infile) != 0) {
                    UnicodeString str(strerror(errno));
                    initMsg(pname);
                    u_wmsg(stderr, "cantRead", str.getTerminatedBuffer());
                    goto error_exit;
                }
        */
        // Convert the read buffer into the new encoding via Unicode.
        // After the call 'unibufp' will be placed behind the last
        // character that was converted in the 'unibuf'.
        // Also the 'cbufp' is positioned behind the last converted
        // character.
        // At the last conversion in the file, flush should be set to
        // true so that we get all characters converted.
        //
        // The converter must be flushed at the end of conversion so
        // that characters on hold also will be written.

        cbufp = buf;
        flush = (UBool)(rd != bufsz);

        // convert until the input is consumed
        do {
            // remember the start of the current byte-to-Unicode conversion
            prevbufp = cbufp;

            unibuf = unibufp = u.getBuffer((int32_t)bufsz);

            // Use bufsz instead of u.getCapacity() for the targetLimit
            // so that we don't overflow fromoffsets[].
            ucnv_toUnicode(mConvfrom, &unibufp, unibuf + bufsz, &cbufp,
                           buf + rd, useOffsets ? fromoffsets : NULL, flush, &err);

            ulen = (int32_t)(unibufp - unibuf);
            u.releaseBuffer(U_SUCCESS(err) ? ulen : 0);

            // fromSawEndOfBytes indicates that ucnv_toUnicode() is done
            // converting all of the input bytes.
            // It works like this because ucnv_toUnicode() returns only under the
            // following conditions:
            // - an error occurred during conversion (an error code is set)
            // - the target buffer is filled (the error code indicates an overflow)
            // - the source is consumed
            // That is, if the error code does not indicate a failure,
            // not even an overflow, then the source must be consumed entirely.
            fromSawEndOfBytes = (UBool)U_SUCCESS(err);

            if (err == U_BUFFER_OVERFLOW_ERROR) {
                err = U_ZERO_ERROR;
            } else if (U_FAILURE(err)) {
                char errorBytes[32]; //pos[32],
                int8_t i, errorLength; //length,

                UErrorCode localError = U_ZERO_ERROR;
                errorLength = (int8_t)sizeof(errorBytes);
                ucnv_getInvalidChars(mConvfrom, errorBytes, &errorLength, &localError);
                if (U_FAILURE(localError) || errorLength == 0) {
                    errorLength = 1;
                }

                // print the input file offset of the start of the error bytes:
                // input file offset of the current byte buffer +
                // length of the just consumed bytes -
                // length of the error bytes
#if TODO_ERROR
                length =
                    (int8_t)sprintf(pos, "%d",
                                    (int)(infoffset + (cbufp - buf) - errorLength));
#endif

                // output the bytes that caused the error
                UnicodeString str;
                for (i = 0; i < errorLength; ++i) {
                    if (i > 0) {
                        str.append((UChar)uSP);
                    }
                    str.append(nibbleToHex((uint8_t)errorBytes[i] >> 4));
                    str.append(nibbleToHex((uint8_t)errorBytes[i]));
                }

#if TODO_ERROR
                initMsg(pname);
                u_wmsg(stderr, "problemCvtToU",
                       UnicodeString(pos, length, "").getTerminatedBuffer(),
                       str.getTerminatedBuffer(),
                       u_wmsg_errorName(err));
#endif
                willexit = TRUE;
                err = U_ZERO_ERROR; /* reset the error for the rest of the conversion. */
            }

            // Replaced a check for whether the input was consumed by
            // looping until it is; message key "premEndInput" now obsolete.

            if (ulen == 0) {
                continue;
            }

            // remove a U+FEFF Unicode signature character if requested
            if (sig < 0) {
                if (u.charAt(0) == uSig) {
                    u.remove(0, 1);

                    // account for the removed UChar and offset
                    --ulen;

                    if (useOffsets) {
                        // remove an offset from fromoffsets[] as well
                        // to keep the array parallel with the UChars
                        memmove(fromoffsets, fromoffsets + 1, ulen * 4);
                    }

                }
                sig = 0;
            }

#if 0 //UNIMPLEMENTED, currently NOT USED
#if !UCONFIG_NO_TRANSLITERATION
            // Transliterate/transform if needed.

            // For transformation, we use chunking code -
            // collect Unicode input until, for example, an end-of-line,
            // then transform and output-convert that and continue collecting.
            // This makes the transformation result independent of the buffer size
            // while avoiding the slower keyboard mode.
            // The end-of-chunk characters are completely included in the
            // transformed string in case they are to be transformed themselves.
            if (t != NULL) {
                UnicodeString out;
                int32_t chunkLimit;

                do {
                    chunkLimit = getChunkLimit(chunk, u);
                    if (chunkLimit < 0 && flush && fromSawEndOfBytes) {
                        // use all of the rest at the end of the text
                        chunkLimit = u.length();
                    }
                    if (chunkLimit >= 0) {
                        // complete the chunk and transform it
                        chunk.append(u, 0, chunkLimit);
                        u.remove(0, chunkLimit);
                        t->transliterate(chunk);

                        // append the transformation result to the result and empty the chunk
                        out.append(chunk);
                        chunk.remove();
                    } else {
                        // continue collecting the chunk
                        chunk.append(u);
                        break;
                    }
                } while (!u.isEmpty());

                u = out;
                ulen = u.length();
            }
#endif
#endif

            // add a U+FEFF Unicode signature character if requested
            // and possible/necessary
            if (sig > 0) {
                if (u.charAt(0) != uSig && cnvSigType(mConvto) == CNV_WITH_FEFF) {
                    u.insert(0, (UChar)uSig);

                    if (useOffsets) {
                        // insert a pseudo-offset into fromoffsets[] as well
                        // to keep the array parallel with the UChars
                        memmove(fromoffsets + 1, fromoffsets, ulen * 4);
                        fromoffsets[0] = -1;
                    }

                    // account for the additional UChar and offset
                    ++ulen;
                }
                sig = 0;
            }

            // Convert the Unicode buffer into the destination codepage
            // Again 'bufp' will be placed behind the last converted character
            // And 'unibufp' will be placed behind the last converted unicode character
            // At the last conversion flush should be set to true to ensure that
            // all characters left get converted

            unibuf = unibufbp = u.getBuffer();

            do {
                bufp = outbuf;

                // Use fromSawEndOfBytes in addition to the flush flag -
                // it indicates whether the intermediate Unicode string
                // contains the very last UChars for the very last input bytes.
                ucnv_fromUnicode(mConvto, &bufp, outbuf + bufsz,
                                 &unibufbp,
                                 unibuf + ulen,
                                 NULL, (UBool)(flush && fromSawEndOfBytes), &err);

                // toSawEndOfUnicode indicates that ucnv_fromUnicode() is done
                // converting all of the intermediate UChars.
                // See comment for fromSawEndOfBytes.
                toSawEndOfUnicode = (UBool)U_SUCCESS(err);

                if (err == U_BUFFER_OVERFLOW_ERROR) {
                    err = U_ZERO_ERROR;
                } else if (U_FAILURE(err)) {
                    UChar errorUChars[4];
                    const char* errtag;
                    //char pos[32];
                    UChar32 c;
                    int8_t i, errorLength; //length,

                    UErrorCode localError = U_ZERO_ERROR;
                    errorLength = (int8_t)UPRV_LENGTHOF(errorUChars);
                    ucnv_getInvalidUChars(mConvto, errorUChars, &errorLength, &localError);
                    if (U_FAILURE(localError) || errorLength == 0) {
                        // need at least 1 so that we don't access beyond the length of fromoffsets[]
                        errorLength = 1;
                    }

                    int32_t ferroffset;

                    if (useOffsets) {
                        // Unicode buffer offset of the start of the error UChars
                        ferroffset = (int32_t)((unibufbp - unibuf) - errorLength);
                        if (ferroffset < 0) {
                            // approximation - the character started in the previous Unicode buffer
                            ferroffset = 0;
                        }

                        // get the corresponding byte offset out of fromoffsets[]
                        // go back if the offset is not known for some of the UChars
                        int32_t fromoffset;
                        do {
                            fromoffset = fromoffsets[ferroffset];
                        } while (fromoffset < 0 && --ferroffset >= 0);

                        // total input file offset =
                        // input file offset of the current byte buffer +
                        // byte buffer offset of where the current Unicode buffer is converted from +
                        // fromoffsets[Unicode offset]
                        ferroffset = infoffset + (prevbufp - buf) + fromoffset;
                        errtag = "problemCvtFromU";
                    } else {
                        // Do not use fromoffsets if (t != NULL) because the Unicode text may
                        // be different from what the offsets refer to.

                        // output file offset
                        ferroffset = (int32_t)(outfoffset + (bufp - outbuf));
                        errtag = "problemCvtFromUOut";
                    }

#if TODO_ERROR
                    length = (int8_t)sprintf(pos, "%u", (int)ferroffset);
#endif
                    // output the code points that caused the error
                    UnicodeString str;
                    for (i = 0; i < errorLength;) {
                        if (i > 0) {
                            str.append((UChar)uSP);
                        }
                        U16_NEXT(errorUChars, i, errorLength, c);
                        if (c >= 0x100000) {
                            str.append(nibbleToHex((uint8_t)(c >> 20)));
                        }
                        if (c >= 0x10000) {
                            str.append(nibbleToHex((uint8_t)(c >> 16)));
                        }
                        str.append(nibbleToHex((uint8_t)(c >> 12)));
                        str.append(nibbleToHex((uint8_t)(c >> 8)));
                        str.append(nibbleToHex((uint8_t)(c >> 4)));
                        str.append(nibbleToHex((uint8_t)c));
                    }

#if TODO_ERROR
                    initMsg(pname);
                    u_wmsg(stderr, errtag,
                           UnicodeString(pos, length, "").getTerminatedBuffer(),
                           str.getTerminatedBuffer(),
                           u_wmsg_errorName(err));
                    u_wmsg(stderr, "errorUnicode", str.getTerminatedBuffer());
#endif
                    willexit = TRUE;
                    err = U_ZERO_ERROR; /* reset the error for the rest of the conversion. */
                }

                // Replaced a check for whether the intermediate Unicode characters were all consumed by
                // looping until they are; message key "premEnd" now obsolete.
                size_t outbufsz = (size_t) (bufp - outbuf);
                *outlen += outbufsz;
                outVector->insert(outVector->end(), outbuf, outbuf+outbufsz);
                /*
                                // Finally, write the converted buffer to the output file
                                size_t outlen = (size_t) (bufp - outbuf);
                                outfoffset += (int32_t)(wr = fwrite(outbuf, 1, outlen, outfile));
                                if (wr != outlen) {
                                    UnicodeString str(strerror(errno));
                                    initMsg(pname);
                                    u_wmsg(stderr, "cantWrite", str.getTerminatedBuffer());
                                    willexit = TRUE;
                                }
                */
                if (willexit) {
                    goto error_exit;
                }
            } while (!toSawEndOfUnicode);
        } while (!fromSawEndOfBytes);

        rd = 0;
    } while (!flush);           // Stop when we have flushed the
    // converters (this means that it's
    // the end of output)

    goto normal_exit;

error_exit:
    ret = FALSE;

normal_exit:
    // Cleanup.

    /*
    #if !UCONFIG_NO_TRANSLITERATION
        delete t;
    #endif

        if (closeFile) {
            fclose(infile);
        }
    */
    return ret;
}
