/*H*************************************************************************************/
/*!
    \File    tagfield.c

    \Description
        This module provides routines to insert/extract tagged data into a free-form text buffer.
        Several data types are provided to allow the representation to be as natural as possible
        (i.e., the Binary routine could be used for everthing, but it is much more difficult to
        decode externally than the String routine).

    \Copyright
        Copyright (c) Electronic Arts 2000-2004. ALL RIGHTS RESERVED.

    \Version 1.0 01/23/2000 (gschaefer) First version
    \Version 1.1 04/03/2002 (gschaefer) Removed sscanf dependency
    \Version 1.2 01/29/2003 (gschaefer) Changed calling to signed char
    \Version 2.0 12/10/2004 (gschaefer) Redesigned "set" logic to avoid stack use
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"

#include "DirtySDK/util/binary7.h"
// self-include
#include "DirtySDK/util/tagfield.h"

/*** Defines ***************************************************************************/

#define LOBBY_TESTTIME  (0)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// used by functions to determine encoding
#define CHAR_STRINGENC  1       // should strings encode
#define CHAR_TOKENENC   2       // should tokens encode
#define CHAR_STRUCTENC  4       // should structures encode
#define CHAR_DELIMENC   8       // should encode for delim
static const unsigned char char_encode[256] =
{
     0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,   // 00..0f
     0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,   // 10..1f
     0x0e,0x04,0x07,0x04,0x04,0x07,0x04,0x04,0x04,0x04,0x04,0x04,0x0c,0x04,0x04,0x04,   // 20..2f
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x00,   // 30..3f

     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 40..4f
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 50..5f
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // 60..6f
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,   // 70..7f
     
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // 80..8f
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // 90..9f
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // a0..af
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // b0..bf
     
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // c0..cf
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // d0..df
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,   // e0..ef
     0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02    // f0..ff
};

// hex encoding tables
static const unsigned char hex_encode[16] =
    "0123456789abcdef";

// hex decoding tables (128=invalid character)
static const unsigned char hex_decode[256] =
{
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128
};

// convert to uppercase
static const unsigned char to_upper[256] =
{
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
     96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

// encode characters for flags
static const char flag_encode[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ0123-";

// decode values for flags
static const char flag_decode[256] =
{
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   31,   -1,   -1,
       27,   28,   29,   30,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
       16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   -1,   -1,   -1,   -1,   -1,
        0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
       16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

// field divider character
static char g_divider = '\n';       // divider between fields
static int32_t g_divafter = 1;          // how many to increment for "after" termination

/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _ParseNumber

    \Description
        Convert a number from string to integer form.

    \Input *pData       - pointer to data
    \Input *pValue      - pointer to integer to return
    \Input  iDigits     - maximum number of digits to parse

    \Output const char * - pointer to byte after numeric data

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
static const char *_ParseNumber(const char *pData, uint32_t *pValue, int32_t iDigits)
{
    // IF no digit limit
    if ( iDigits <= 0 )
    {
        // Set sufficiently high to emulate no limit
        iDigits = 256;
    }

    for (*pValue = 0; (*pData >= '0') && (*pData <= '9') && (iDigits > 0); ++pData, iDigits--)
    {
        *pValue = (*pValue * 10) + (*pData & 15);
    }
    return(pData);
}

/*F*************************************************************************************************/
/*!
    \Function _TestTime

    \Description
        Test the time functions against library equvilents.

    \Output int32_t - failure count

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
#if LOBBY_TESTTIME
static int32_t _TestTime(void)
{
    int32_t count = 0;
    uint32_t epoch;
    struct tm *t1, *t2, tmp;

    // run random time values through conversion
    for (epoch = 0; epoch < 365*86400*50; epoch += 40000) {
        // do both library and us
        t1 = gmtime(&epoch);
        t2 = ds_secstotime(&tmp, epoch);
        // confirm correctness
        if ((t1->tm_year != t2->tm_year) || (t1->tm_yday != t2->tm_yday) ||
            (t1->tm_mon != t2->tm_mon)   || (t1->tm_mday != t2->tm_mday) ||
            (t1->tm_hour != t2->tm_hour) || (t1->tm_min != t2->tm_min) ||
            (t1->tm_sec != t2->tm_sec)) {
            printf("failure %d: conv %08x: t1=%d.%d.%d %d:%d:%d t2=%d.%d.%d %d:%d:%d\n",
                count, (int32_t)epoch,
                t1->tm_year+1900, t1->tm_mon+1, t1->tm_mday, t1->tm_hour, t1->tm_min, t1->tm_sec,
                t2->tm_year+1900, t2->tm_mon+1, t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec);
            ++count;
        }
    }

    // return failure count
    return(count);
}
#endif

/*F*************************************************************************************************/
/*!
    \Function _TagFieldSetupAppend

    \Description
        Append a new field to an existing record. This is a replacement of TagFieldInsert.

    \Input *pRecord - data record
    \Input iReclen  - record length
    \Input *pName   - field to add
    \Input iMinData - minimum data needed to add field

    \Output int32_t     - NULL=wont fit else pointer to append point

    \Version 12/12/2004 (gschaefer)
*/
/*************************************************************************************************F*/
static unsigned char *_TagFieldSetupAppend(unsigned char *pRecord, int32_t iReclen, const char *pName, int32_t iMinData)
{
    int32_t iNameLen;
    int32_t iInclEqu = 1;
    int32_t iSeparate;
    unsigned char *pData;
    unsigned char *pFind = pRecord;
    unsigned char *pFindCmp;
    unsigned char *pFindDiv;
    const unsigned char *pNameCmp;

    // make sure record is valid
    if (pRecord == NULL)
    {
        return(NULL);
    } 

    // dont allow insert into invalid record
    if (pRecord[0] == '=')
    {
        return(NULL);
    }

    // handle raw replacement case
    if (pName == NULL)
    {
        *pRecord = 0;
        pName = "";
        iInclEqu = 0;
        iNameLen = 0;
    }

    // calc the name length
    for (iNameLen = 0; pName[iNameLen] != 0; ++iNameLen)
    {
        // advance start into record since we look for divider
        if (*pFind != 0)
        {
            ++pFind;
        }
    }

    // do the search
    for (; *pFind != 0; ++pFind)
    {
        // a = used to locate the fields
        if (*pFind == '=')
        {
            // if it is preceded by whitespace, it is a bulk marker
            // inserting to a record with a bulk marker is not permitted
            if (pFind[-1] <= ' ')
            {
                return(NULL);
            }
            // make sure name is preceded with whitespace
            pFindCmp = pFind-iNameLen;
            if ((pFindCmp != pRecord) && (pFindCmp[-1] > ' '))
            {
                continue;
            }
            // compare the names (final char must mismatch)
            for (pNameCmp = (const unsigned char *)pName; to_upper[*pFindCmp] == to_upper[*pNameCmp]; ++pFindCmp, ++pNameCmp)
                ;
            if (*pNameCmp == 0)
            {
                break;
            }
        }
    }

    // see if we need to shift the remaining record contents
    if (*pFind != 0)
    {
        // find the end of the field
        for (pFindDiv = NULL, pData = pFind; *pData >= ' '; ++pData)
        {
            if (*pData == ' ')
            {
                pFindDiv = pData;
            }
            if ((*pData == '=') && (pFindDiv != NULL))
            {
                pData = pFindDiv;
                break;
            }
        }

        // include the separators
        while ((*pData < ' ') && (*pData != 0))
            ++pData;

        // delete the existing field
        for (pFind -= iNameLen; (*pFind = *pData++) != 0; ++pFind)
            ;
    }

    // see if we need to add a separator
    iSeparate = ((pFind != pRecord) && (pFind[-1] >= ' ') && (pFind[-1] != g_divider) ? 1 : 0);

    // see if minimum record length is available
    if ((pRecord+iReclen)-pFind < iSeparate+iNameLen+1+iMinData+g_divafter+1)
    {
        return(NULL);
    }

    // append a separator if needed
    if (iSeparate)
    {
        *pFind++ = g_divider;
    }

    // copy the name
    while (*pName != 0)
    {
        *pFind++ = *pName++;
    }

    // if pName was non-null, include the =
    if (iInclEqu)
    {
        *pFind++ = '=';
    }
    *pFind = 0;

    // return the append point
    return(pFind);
}

/*F*************************************************************************************************/
/*!
    \Function _TagFieldSetupCancel

    \Description
        Cancel an in-progress "set" attempt. Will remove any partial data.

    \Input *pRecord - data record
    \Input *pAppend - where append was taking place

    \Output int32_t - always -1 to allow return(_TagFieldSetupCancel)

    \Version 12/12/2004 (gschaefer)
*/
/*************************************************************************************************F*/
static int32_t _TagFieldSetupCancel(unsigned char *pRecord, unsigned char *pAppend)
{
    // dont scan past start of record
    while (pAppend != pRecord)
    {
        // look for a control character (record separator)
        if (*--pAppend < ' ')
        {
            // dont delete newline separator since it belonds to preceding item
            if (*pAppend == '\n')
            {
                ++pAppend;
            }
            break;
        }
    }
    
    // warn that the operation failed due to buffer overflow
    NetPrintf(("tagfield: tagfield buffer overflow appending record %s\n", pAppend));
    
    *pAppend = 0;

    // return failure indicator
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _TagFieldSetupTerm

    \Description
        Add a terminator to a tagfield record. This is actually implemented as a macro
        for performance reasons.

    \Input *pRecord - data record
    \Input *pAppend - current append point / end of buffer
    \Input *pName   - name

    \Output int32_t - always 0 to allow return(_TagFieldSetupTerm)

    \Version 12/12/2004 (gschaefer)
*/
/*************************************************************************************************F*/
#ifdef __linux__
inline
#endif
static int32_t _TagFieldSetupTerm(char *pRecord, char *pAppend, const char *pName)
{
    if (g_divafter && (pName != NULL))
    {
        *pAppend++ = g_divider;
    }
    *pAppend = 0;
    return((int32_t)(pAppend-pRecord));
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function TagFieldDivider

    \Description
        Set the default field divider character.

    \Input iDivider - new divider character

    \Output char    - previous divider character

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
char TagFieldDivider(char iDivider)
{
    char iPrevious = g_divider;
    g_divider = iDivider;
    g_divafter = (g_divider == '\n' ? 1 : 0);
    return(iPrevious);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldFormat

    \Description
        Convert a tag buffer to use specified marker char (also converts bulk data to
        use strictly newlines).

    \Input *_record - data record
    \Input marker   - new marker

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
void TagFieldFormat(char *_record, unsigned char marker)
{
    unsigned char *src, *dst;
    unsigned char *record = (unsigned char *)_record;

    // convert all newlines to tabs
    // (also remove redundant newline chars)
    for (src = dst = record; *src != 0;)
    {
        // see if this is a field break
        if (*src < ' ')
        {
            *dst++ = marker;
            do
            {
                ++src;
            }
            while ((*src > 0) && (*src < ' '));
            // check for bulk data marker
            if ((src[0] == '=') && (src[1] < ' '))
                break;
        }
        else
        {
            *dst++ = *src++;
        }
    }

    // setup final marker
    if ((dst != record) && (dst[-1] == marker))
    {
        // if using non-newlines and no bulk data, then omit terminator
        dst[-1] = ((marker != '\n') && (*src == 0) ? 0 : '\n');
    }

    // reformat bulk area (always use newlines)
    while (*src != 0)
    {
        if (*src < ' ')
        {
            *dst++ = '\n';
            marker = *src++;
            while ((*src > 0) && (*src < ' ') && (*src != marker))
                ++src;
        }
        else
        {
            *dst++ = *src++;
        }
    }

    // terminate
    *dst = 0;
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldDelete

    \Description
        Delete a field from the record.

    \Input *_record     - data record
    \Input *name        - field name

    \Output int32_t     - negative=no such field, zero=no error

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldDelete(char *_record, const char *name)
{
    unsigned char *data;
    unsigned char *head;
    unsigned char *tail;
    unsigned char *last;
    unsigned char *record = (unsigned char *)_record;

    // find existing
    data = (unsigned char *) TagFieldFind((char *)record, name);
    if (data == NULL)
        return(-1);

    // locate the start of the field
    for (head = data; (head != record) && (head[-1] > ' '); --head)
        ;

    // find the field end
    for (tail = last = data; (*tail >= ' ') && (*tail != '='); ++tail)
    {
        if (*tail == ' ')
        {
            last = tail;
        }
    }
    // if we didn't hit a = then we must have hit a field terminator
    if (*tail != '=')
    {
        last = tail;
    }

    // skip to start of next field
    while ((*last > 0) && (*last <= ' '))
    {
        ++last;
    }

    // shift back remaining data
    while (*last != 0)
    {
        *head++ = *last++;
    }

    // remove any trailing whitespace (from deleting the last record)
    while ((head != record) && (head[-1] <= ' '))
        --head;

    // terminate the record
    *head = 0;

    // deleted
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldRename

    \Description
        Rename a field within the record.

    \Input *_record     - data record
    \Input reclen       - record length
    \Input *oldname     - old name
    \Input *newname     - new name

    \Output int32_t     - negative=no such field, zero=no error

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldRename(char *_record, int32_t reclen, const char *oldname, const char *newname)
{
    int32_t diff;
    unsigned char *data;
    unsigned char *head;
    unsigned char *tail;
    unsigned char *record = (unsigned char *)_record;

    // find existing field
    data = (unsigned char *) TagFieldFind((char *)record, oldname);
    if (data == NULL)
        return(-1);

    // if it has a space divider, skip past it
    if (data[-1] == ' ')
        --data;

    // skip the = token
    --data;

    // locate start of field
    for (head = data; (head != record) && (head[-1] > ' '); --head)
        ;

    // calculate the difference in name length
    diff = (int32_t)strlen(newname) - (int32_t)(data-head);

    // if new name is shorter, than shrink the data
    if (diff < 0)
    {
        while (*data != 0)
        {
            data[diff] = data[0];
            data++;
        }
        data[diff] = 0;
    }

    // if new name is longer, insert data
    if (diff > 0)
    {
        // figure out the length
        for (tail = data; *tail != 0; ++tail)
            ;

        // see if there is room to extend name length
        if (tail-record >= reclen-diff)
        {
            // failed due to length issues
            return(-1);
        }

        // insert characters to bring to proper length
        for (tail++; tail >= data; --tail)
        {
            tail[diff] = tail[0];
        }
    }

    // copy over the new name
    while (*newname != 0)
    {
        *head++ = *newname++;
    }

    // return success
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldDupl

    \Description
        Duplicate an entire record.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pData    - data source

    \Output int32_t  - number of bytes copied

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldDupl(char *pRecord, int32_t iReclen, const char *pData)
{
    int32_t iLimit;

    // copy over the string
    for (iLimit = iReclen; (iLimit > 1) && (*pData != 0); --iLimit)
    {
        *pRecord++ = *pData++;
    }

    // terminate if there is room
    if (iLimit > 0)
    {
        *pRecord = 0;
    }

    // return number of bytes copied
    return(iReclen-iLimit);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldMerge

    \Description
        Merge two records together (does not merge bulk data). As of 12/14/2004 rewrite,
        it no longer returns a merged field count (just that is merged something).

    \Input *_pRecord - data record
    \Input  iReclen  - record length
    \Input *_pMerge  - source record to merge from

    \Output int32_t  - negative=out of space, zero=nothing merged, positive=one or more fields merged

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldMerge(char *_pRecord, int32_t iReclen, const char *_pMerge)
{
    unsigned char *pRecord = (unsigned char *)_pRecord;
    unsigned char *pLimit = pRecord+iReclen-1;
    const unsigned char *pMerge = (const unsigned char *)_pMerge;
    unsigned char *pSrc;
    unsigned char *pDst;
    unsigned char *pName;

    // make sure some data in merge field
    if ((pMerge == NULL) || (*pMerge == 0))
    {
        return(0);
    }

    // shift the main record, deleting any duplicate items
    for (pSrc = pDst = pName = pRecord; *pSrc != 0;)
    {
        // record potential name start
        if (*pSrc <= ' ')
        {
            *pDst++ = *pSrc++;
            pName = pSrc;
            continue;
        }

        // if not a field separator
        if (*pSrc != '=')
        {
            *pDst++ = *pSrc++;
            continue;
        }

        // check for bulk area
        if (pSrc == pName)
        {
            *pDst = 0;
            break;
        }

        // make sure field not in merge
        *pSrc = 0;
        if (TagFieldFind((const char *)pMerge, (const char *)pName) == NULL)
        {
            *pDst++ = *pSrc++ = '=';
            continue;
        }

        // new destination of start of name
        pDst -= pSrc-pName;

        // skip to next tag
        for (++pSrc; (*pSrc != 0) && (*pSrc != '='); ++pSrc)
            ;
        // if not last tag,
        if (*pSrc != 0)
        {
            while (pSrc[-1] > ' ')
                --pSrc;
        }

        // now pointing to new name start
        pName = pSrc;
    }

    // remove final field separator if present and add new one
    while ((pDst != pRecord) && (pDst[-1] <= ' '))
    {
        --pDst;
    }

    // if not enough to start merge, terminate and return
    if (pDst >= pLimit-1)
    {
        // there must have been a terminator there -- put it back
        *pDst = 0;
        // make sure there is room for separator (might not have been one before)
        if (pDst+1 < pRecord+iReclen)
        {
            (void)_TagFieldSetupTerm((char *)pRecord, (char *)pDst, "");
        }
        return(-1);
    }

    // add divider if not empty
    if (pDst != pRecord)
    {
        *pDst++ = g_divider;
    }

    // copy over new record
    while  ((*pMerge != 0) && (pDst < pLimit))
    {
        *pDst++ = *pMerge++;
    }

    // if we ran out of space
    if (*pMerge != 0)
    {
        return(_TagFieldSetupCancel(pRecord, pDst));
    }

    // since we merged a valid record, don't mess with dividers (should already be there)
    *pDst = 0;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldFind

    \Description
        Search the buffer and locate the named field. Now optimized to only use = as field
        separator (old version allowed equals and colen).

    \Input *_pRecord        - data record
    \Input *_pName          - field name

    \Output const char *    - pointer to data start, or NULL if no such field exists

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
const char *TagFieldFind(const char *_pRecord, const char *_pName)
{
    int32_t iNameLen;
    unsigned char uMatch = 0;
    const unsigned char *pCmp1;
    const unsigned char *pCmp2;
    const unsigned char *pName = (const unsigned char *)_pName;
    const unsigned char *pRecord = (const unsigned char *)_pRecord;

    // deal with null data
    if ((pRecord == NULL) || (pName == NULL) || (*pName == 0))
    {
        return(NULL);
    }

    // special case first compare (do it forward -- all others are backwards)
    for (pCmp1 = pName; *pCmp1 != 0; ++pCmp1)
    {
        // bail if we run out of data
        if (*pRecord == 0)
        {
            return(NULL);
        }
        // store the match bits
        uMatch |= (to_upper[*pRecord++] ^ to_upper[*pCmp1]);
    }
    iNameLen = (int32_t)(pCmp1-pName);

    // see if we got initial match
    if ((uMatch == 0) && (*pRecord == '='))
    {
        return((const char *)pRecord+1);
    }

    if (*pRecord == 0)
        return(NULL);

    // find data match (use backwards compare from now on)
    for (++pRecord; *pRecord != 0; ++pRecord)
    {
        if (*pRecord == '=')
        {
            // done if we hit bulk identifier
            if (pRecord[-1] <= ' ')
            {
                break;
            }

            // point to name start in record
            pCmp1 = pRecord-iNameLen;
            // within this part of loop, we are assured of being at least one byte into
            // record which makes following compare always legal
            if (pCmp1[-1] > ' ')
            {
                continue;
            }

            // do fast compare based just on character values
            // (can do this because name terminator will mismatch = sign
            for (pCmp2 = pName; to_upper[*pCmp1] == to_upper[*pCmp2]; ++pCmp1, ++pCmp2)
                ;

            // if we got to end of name then they match
            // 4/1/2005: changed from (*pCmp2 == 0) to (pCmp1 == pRecord) because
            // gcc generates better code -- pRecord is already in a register
            if (pCmp1 == pRecord)
            {
                return((const char *)pRecord+1);
            }
        }
    }

    // no match
    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldFind2

    \Description
        Search the buffer and locate the two-part named field (the names are catenated together).

    \Input *pRecord         - data record
    \Input *pName1          - field name
    \Input *pName2          - field name 2

    \Output const char *    - pointer to data start, or NULL if no such field exists

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
const char *TagFieldFind2(const char *pRecord, const char *pName1, const char *pName2)
{
    char strName[256];
    const char *pFind = NULL;

    // handle NULL cases
    if (pName1 == NULL)
    {
        pFind = TagFieldFind(pRecord, pName2);
    }
    else if (pName2 == NULL)
    {
        pFind = TagFieldFind(pRecord, pName1);
    }
    else
    {
        ds_strnzcpy(strName, pName1, sizeof(strName));
        ds_strnzcat(strName, pName2, sizeof(strName));
        pFind = TagFieldFind(pRecord, strName);
    }

    // return the data
    return(pFind);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldFindIdx

    \Description
        Search the buffer and locate the indexed named field (the tag to find
        is a concatenation of "pName" and the provided index.

    \Input *pRecord         - data record
    \Input *pName           - field name
    \Input  iIdx            - index

    \Output const char *    - pointer to data start, or NULL if no such field exists

    \Version 03/23/04 (doneill)
*/
/*************************************************************************************************F*/
const char *TagFieldFindIdx(const char *pRecord, const char *pName, int32_t iIdx)
{
    char strTag[256];

    // build the tag
    ds_snzprintf(strTag, sizeof(strTag), "%s%d", pName, iIdx);
    return(TagFieldFind(pRecord, strTag));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetRaw

    \Description
        Set a raw field (must come from TagFieldGetRaw or TagFieldFind). If the field is
        encoded badly, it can mess up the tagfield record so don't use data not generated
        by tagfield functions.

    \Input *_pRecord    - data record
    \Input iReclen      - record length
    \Input *pName       - field name
    \Input *_pData      - source record to copy from

    \Output int32_t     - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetRaw(char *_pRecord, int32_t iReclen, const char *pName, const char *_pData)
{
    unsigned char uTerm;
    unsigned char *pAppend;
    unsigned char *pRecord = (unsigned char *)_pRecord;
    const unsigned char *pData = (const unsigned char *)_pData;

    // make sure data is valid
    if (pData == NULL)
    {
        TagFieldDelete((char *)pRecord, pName);
        return(0);
    }

    // get the append point for max-size date format
    pAppend = _TagFieldSetupAppend(pRecord, iReclen, pName, 0);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // copy into record
    uTerm = ' '+1;
    while (*pData >= uTerm)
    {
        if (pAppend >= pRecord+iReclen-2)
        {
            return(_TagFieldSetupCancel(pRecord, pAppend));
        }
        // tricky way to set compare value -- see comment in GetRaw for details
        uTerm ^= (*pData == '"');
        *pAppend++ = *pData++;
    }

    // terminate the field
    return(_TagFieldSetupTerm((char *)pRecord, (char *)pAppend, pName));
}

/*F***************************************************************************/
/*!
    \Function TagFieldGetRaw

    \Description
        Get the raw, undecoded value of a field returned as a string.

    \Input *pData   - data pointer
    \Input *pBuffer - buffer to store field data in
    \Input  iBuflen - length of pBuffer
    \Input *pDefval - default value

    \Output int32_t - field value or pDefval copied to target buffer, returns string length

    \Version 12/13/2004 (gschaefer)
*/
/***************************************************************************F*/
int32_t TagFieldGetRaw(const char *pData, char *pBuffer, int32_t iBuflen, const char *pDefval)
{
    int32_t iLen;
    unsigned char uTerm;
    const unsigned char *pPtr;

    // make sure data is valid
    if (pData == NULL)
    {
        // return default value
        if (pDefval == NULL)
        {
            return(-1);
        }

        // copy over the string
        for (iLen = 1; (iLen < iBuflen) && (*pDefval != 0); ++iLen)
        {
            *pBuffer++ = *pDefval++;
        }
        *pBuffer = 0;
        return(iLen-1);
    }

    // copy over the string
    pPtr = (const unsigned char *)pData;
    uTerm = ' '+1;
    for (iLen = 1; (iLen < iBuflen) && (*pPtr >= uTerm); ++iLen)
    {
        // this code updated the terminator compare: since a C compare
        // results in a value of 0/1, it uses xor to toggle the compare
        // value between ' ' and ' '+1 (doing it this way is very fast
        // since the generated code has no branches)
        uTerm ^= (*pPtr == '"');
        *pBuffer++ = *pPtr++;
    }
    *pBuffer = 0;
    return(iLen-1);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetNumber

    \Description
        Set a numeric field.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input  iValue   - number

    \Output int32_t  - negative=record too small, zero=no error

    \Version 12/12/2004 (gschaefer) Rewrite
*/
/*************************************************************************************************F*/
int32_t TagFieldSetNumber(char *pRecord, int32_t iReclen, const char *pName, int32_t iValue)
{
    int32_t iLength;
    uint32_t uValue;
    unsigned char *pData;
    unsigned char *pAppend;
    unsigned char strData[12];

    // save the sign
    uValue = iValue;

    // special code to both handle negative and special case 0-9 encoding
    if (iValue < 10)
    {
        // deal with negative
        if (iValue < 0)
        {
            uValue = -iValue;
        }
        else
        {
            // must be 0-9 -- encode single digit special
            pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, 1);
            if (pAppend == NULL)
            {
                return(-1);
            }
            *pAppend++ = '0'+iValue;
            return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
        }
    }

    // since length is difficult to calculate and formatting requires shift, just
    // format into local buffer and then shift into final buffer
    pData = strData+sizeof(strData);
    *--pData = 0;

    // insert the digits
    do
    {
        *--pData = '0'+(uValue % 10);
        uValue /= 10;
    }
    while (uValue != 0);

    // insert sign if needed
    if (iValue < 0)
    {
        *--pData = '-';
    }

    // figure out length and setup for data append
    iLength = (int32_t)(strData+sizeof(strData)-pData-1);
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, iLength);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // raw copy of the data
    while (*pData != 0)
    {
        *pAppend++ = *pData++;
    }
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetNumber64

    \Description
        Set a 64-bit numeric field.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input  iValue   - number

    \Output int32_t  - negative=record too small, zero=no error

    \Version 02/04/2005 (gschaefer) First version
*/
/*************************************************************************************************F*/
int32_t TagFieldSetNumber64(char *pRecord, int32_t iReclen, const char *pName, int64_t iValue)
{
    int32_t iLength;
    int32_t iNeg = 0;
    unsigned char *pData;
    unsigned char *pAppend;
    unsigned char strData[20];

    // if its a small number, use 32-bit decimal encoder
    if ((iValue > -10000) && (iValue < 10000))
    {
        return(TagFieldSetNumber(pRecord, iReclen, pName, (int32_t)iValue));
    }

    // since length is difficult to calculate and formatting requires shift, just
    // format into local buffer and then shift into final buffer
    pData = strData+sizeof(strData);
    *--pData = 0;

    // save the sign
    if (iValue < 0)
    {
        iValue = -iValue;
        iNeg = 1;
    }

    // insert the digits
    for (; iValue > 0; iValue >>= 4)
    {
        *--pData = hex_encode[iValue&15];
    }

    // handle zero case
    if (*pData == 0)
    {
        *--pData = '0';
    }

    // insert the hex indicator
    *--pData = '$';

    // insert sign if needed
    if (iNeg)
    {
        *--pData = '-';
    }

    // figure out length and setup for data append
    iLength = (int32_t)(strData+sizeof(strData)-pData-1);
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, iLength);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // raw copy of the data
    for (; *pData != 0; ++pData)
    {
        *pAppend++ = *pData;
    }
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetNumber

    \Description
        Get a numeric field.

    \Input *pData    - data pointer
    \Input  iDefval  - default number

    \Output int32_t  - number from field, or default number if field does not exist

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetNumber(const char *pData, int32_t iDefval)
{
    int32_t iSign = 1;
    uint32_t uDigit;
    uint32_t uValue;
    uint32_t uBase = 10;

    // return the value
    if (pData == NULL)
    {
        return(iDefval);
    }

    // check the prefix char
    if (*pData == '-')
    {
        ++pData;
        iSign = -1;
    }
    else if (*pData == '$')
    {
        ++pData;
        uBase = 16;
    }
    else if (*pData == '+')
    {
        ++pData;
    }

    // parse the number
    for (uValue = 0; (uDigit = hex_decode[(unsigned)*pData]) < uBase; ++pData)
    {
        uValue = (uValue * uBase) + uDigit;
    }

    // return value with sign included
    return(iSign*uValue);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetNumber64

    \Description
        Get a 64-bit numeric field.

    \Input *pData    - data pointer
    \Input  iDefval  - default number

    \Output int64_t  - number from field, or default number if field does not exist

    \Version 02/04/2005 (gschaefer)
*/
/*************************************************************************************************F*/
int64_t TagFieldGetNumber64(const char *pData, int64_t iDefval)
{
    int32_t iSign;
    uint32_t uDigit;
    uint64_t uValue;
    uint32_t uBase = 10;

    // return the value
    if (pData == NULL)
    {
        return(iDefval);
    }

    // check the sign
    iSign = 1;
    if (*pData == '+')
    {
        ++pData;
    }
    else if (*pData == '-')
    {
        ++pData;
        iSign = -1;
    }

    // allow for base-16
    if (*pData == '$')
    {
        ++pData;
        uBase = 16;
    }

    // parse the number
    for (uValue = 0; (uDigit = hex_decode[(unsigned)*pData]) < uBase; ++pData)
    {
        uValue = (uValue * uBase) + uDigit;
    }

    // return value with sign included
    uValue *= iSign;
    return(uValue);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetFlags

    \Description
        Set a flags field.

    \Input *pRecord  - data record
    \Input iReclen   - record length
    \Input *pName    - field name
    \Input iValue    - flags value

    \Output int32_t  - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetFlags(char *pRecord, int32_t iReclen, const char *pName, int32_t iValue)
{
    const char *pFlags;
    unsigned char *pAppend;
    unsigned char *pLimit = (unsigned char *)pRecord+iReclen-1-g_divafter;

    // setup the append point
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, 0);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // add in flags
    for (pFlags = flag_encode; (iValue != 0) && (*pFlags != 0); iValue >>=1, ++pFlags)
    {
        if (iValue & 1)
        {
            // handle buffer overflow
            if (pAppend >= pLimit)
            {
                return(_TagFieldSetupCancel((unsigned char *)pRecord, pAppend));
            }
            *pAppend++ = *pFlags;
        }
    }

    // terminate buffer
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetFlags

    \Description
        Get a flags field.

    \Input *pData    - data record
    \Input  iDefault - default flags

    \Output int32_t  - flags from field, or default flags if field does not exist

    \Version 12/12/2004 (gschaefer) Rewrite
*/
/*************************************************************************************************F*/
int32_t TagFieldGetFlags(const char *pData, int32_t iDefault)
{
    int32_t iShift;
    uint32_t uFlags;

    // handle default value
    if (pData == NULL)
    {
        return(iDefault);
    }

    // parse flags
    for (uFlags = 0; (iShift = flag_decode[(unsigned)*pData]) >= 0; ++pData)
    {
        uFlags |= 1<<iShift;
    }

    // return the flags
    return((int32_t)uFlags);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetAddress

    \Description
        Set an ip-address field.

    \Input *pRecord - data record
    \Input  iReclen - record length
    \Input *pName   - field name
    \Input  uAddr   - ip address

    \Output int32_t - negative=record too small, zero=no error

    \Version 12/10/2004 (gschaefer) Rewrite
*/
/*************************************************************************************************F*/
int32_t TagFieldSetAddress(char *pRecord, int32_t iReclen, const char *pName, uint32_t uAddr)
{
    int32_t iIndex;
    unsigned char uValue;
    unsigned char *pData;
    unsigned char *pAppend;
    unsigned char Segment[4];
    unsigned char strAddr[16];

    // save the individual address bytes
    Segment[3] = (unsigned char)uAddr;
    uAddr >>= 8;
    Segment[2] = (unsigned char)uAddr;
    uAddr >>= 8;
    Segment[1] = (unsigned char)uAddr;
    uAddr >>= 8;
    Segment[0] = (unsigned char)uAddr;

    // point to staging buffer
    pData = strAddr;

    // do the segments
    for (iIndex = 0; iIndex < 4; ++iIndex)
    {
        uValue = Segment[iIndex];

        // insert the period
        if (iIndex > 0)
        {
            *pData++ = '.';
        }

        // see if there are leading digits
        if (uValue > 9)
        {
            // see if there are two leading digits
            if (uValue > 99)
            {
                // put in the 100s digit
                *pData++ = '0'+(uValue/100);
                uValue %= 100;
            }
            // put in the 10s digit
            *pData++ = '0'+(uValue/10);
            uValue %= 10;
        }
        // always put in the 1s digit
        *pData++ = '0'+(uValue/1);
    }
    *pData = 0;

    // figure out length and setup for data append
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, (int32_t)(pData-strAddr));
    if (pAppend == NULL)
    {
        return(-1);
    }

    // raw copy of the data
    for (pData = strAddr; *pData != 0; ++pData)
    {
        *pAppend++ = *pData;
    }
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetAddress

    \Description
        Get an ip address field.

    \Input *pData       - data record
    \Input iDefval      - default address

    \Output uint32_t    - address from field, or default address if field does not exist

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
uint32_t TagFieldGetAddress(const char *pData, uint32_t iDefval)
{
    uint32_t uAddr = 0;

    // make sure data is valid
    if (pData == NULL)
    {
        return(iDefval);
    }

    // parse the value
    for (;; ++pData)
    {
        // check for digit
        if ((*pData >= '0') && (*pData <= '9'))
        {
            uAddr = (uAddr & 0xffffff00) | (((uAddr&255)*10)+(*pData&15));
        }
        else if (*pData == '.')
        {
            uAddr <<= 8;
        }
        else
        {
            break;
        }
    }

    // return the address
    return(uAddr);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetToken

    \Description
        Set a token field.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input  iToken   - token to set

    \Output int32_t  - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer) Rewrite
*/
/*************************************************************************************************F*/
int32_t TagFieldSetToken(char *pRecord, int32_t iReclen, const char *pName, int32_t iToken)
{
    int32_t iIndex;
    unsigned char uByte;
    unsigned char strToken[16];
    unsigned char *pData;
    unsigned char *pAppend;
    uint32_t uToken = iToken;

    // format backwards
    pData = strToken+sizeof(strToken);
    *--pData = 0;

    for (iIndex = 0; iIndex < 4; ++iIndex)
    {
        uByte = (unsigned char)uToken;
        uToken >>= 8;

        if (char_encode[uByte] & CHAR_TOKENENC)
        {
            *--pData = hex_encode[uByte&15];
            *--pData = hex_encode[uByte>>4];
            *--pData = '%';
        }
        else
        {
            *--pData = uByte;
        }
    }

    // get append point for 4 byte buffer
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, (int32_t)(strToken+sizeof(strToken)-pData-1));
    if (pAppend == NULL)
    {
        return(-1);
    }

    // copy over and terminate
    while (*pData != 0)
    {
        *pAppend++ = *pData++;
    }
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetToken

    \Description
        Get a token.

    \Input *_pData     - data record
    \Input  iDefault   - default token

    \Output int32_t    - token from field, or default token if field does not exist

    \Version 12/13/2004 (gschaefer) Rewrite
*/
/*************************************************************************************************F*/
int32_t TagFieldGetToken(const char *_pData, int32_t iDefault)
{
    int32_t iIndex;
    unsigned char uByte;
    uint32_t uToken = 0;
    const unsigned char *pData = (const unsigned char *)_pData;

    // handle default case
    if ((pData == NULL) || (*pData <= ' '))
    {
        return(iDefault);
    }

    // grab 4 bytes
    for (iIndex = 0; iIndex < 4; ++iIndex)
    {
        uByte = *pData++;
        // pad with spaces if out of data
        if (uByte < ' ')
        {
            --pData;
            uByte = ' ';
        }
        // handle % encoding
        if (uByte == '%')
        {
            uByte = hex_decode[*pData++]<<4;
            uByte |= hex_decode[*pData++];
        }
        uToken = (uToken << 8) | uByte;
    }

    // return the token
    return((int32_t)uToken);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetString

    \Description
        Set a string field.

    \Input *_pRecord    - data record
    \Input  iReclen     - record length
    \Input *pName       - field name
    \Input *_pValue     - text string

    \Output int32_t     - negative = record too small, zero=no error

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetString(char *_pRecord, int32_t iReclen, const char *pName, const char *_pValue)
{
    int32_t iRemain;
    unsigned char *pAppend;
    const unsigned char *pSpace;
    unsigned char *pRecord = (unsigned char *)_pRecord;
    const unsigned char *pValue = (const unsigned char *)_pValue;

    // if string == NULL, then delete existing and return
    if (pValue == NULL)
    {
        TagFieldDelete((char *)pRecord, pName);
        return(0);
    }

    // get the append point
    pAppend = _TagFieldSetupAppend(pRecord, iReclen, pName, 2);
    if (pAppend == NULL)
    {
        return(-1);
    }
    iRemain = (int32_t)(pRecord+iReclen-1-pAppend);

    // see if data contains spaces or commas
    for (pSpace = pValue; *pSpace != 0; ++pSpace)
    {
        if ((*pSpace == ' ') || (*pSpace == ','))
        {
            // quote the string
            *pAppend++ = '"';
            iRemain -= 2;
            break;
        }
    }

    // add the string
    for (; (*pValue != 0) && (iRemain > 0); ++pValue)
    {
        // escape character if needed
        if (char_encode[*pValue] & CHAR_STRINGENC)
        {
            if (iRemain >= 3)
            {
                *pAppend++ = '%';
                *pAppend++ = hex_encode[*pValue>>4];
                *pAppend++ = hex_encode[*pValue&15];
            }
            iRemain -= 3;
        }
        else
        {
            *pAppend++ = *pValue;
            iRemain -= 1;
        }
    }

    // if there are spaces, add trailing quote (storage already accounted for)
    if ((*pSpace == ' ') || (*pSpace == ','))
    {
        *pAppend++ = '"';
    }

    // handle overflow
    if (iRemain < g_divafter)
    {
        return(_TagFieldSetupCancel(pRecord, pAppend));
    }

    // terminate the record
    return(_TagFieldSetupTerm((char *)pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetString

    \Description
        Get string field contents.

    \Input *data        - data record
    \Input *buffer      - string target
    \Input buflen       - length of target buffer
    \Input *defval      - default value

    \Output int32_t     - field value or defval copied to target buffer, returns string length

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetString(const char *data, char *buffer, int32_t buflen, const char *defval)
{
    int32_t len;
    unsigned char term;
    const unsigned char *ptr;

    // make sure data is valid
    if (data == NULL)
    {
        // return default value
        if (defval == NULL)
            return(-1);
        // see if they just want length
        if (buffer == NULL)
            return((int32_t)strlen(defval));
        // copy over the string
        for (len = 1; (len < buflen) && (*defval != 0); ++len)
            *buffer++ = *defval++;
        *buffer = 0;
        return(len-1);
    }

    // see if there is a terminator
    if (*data == '"')
        term = *data++;
    else
        term = ' ';

    // see if they want to test string length
    if (buffer == NULL)
    {
        len = 0;
        for (ptr = (const unsigned char *)data; (*ptr != term) && (*ptr >= ' '); ++ptr)
        {
            if ((ptr[0] == '%') && (ptr[1] >= ' ') && (ptr[2] >= ' '))
                ptr += 2;
            len += 1;
        }
        return(len);
    }

    // make sure buffer is valid
    if (buflen < 1)
        return(-1);

    // length=1 to make room for terminator
    len = 1;
    ptr = (const unsigned char *)data;
    // convert the string
    while ((len < buflen) && (*ptr != term) && (*ptr >= ' '))
    {
        // handle escape sequences
        if ((ptr[0] == '%') && (ptr[1] == '%'))
        {
            *buffer++ = '%';
            ptr += 2;
            ++len;
        }
        else if ((ptr[0] == '%') && (ptr[1] >= ' ') && (ptr[2] >= ' '))
        {
            *buffer++ = (hex_decode[(unsigned)ptr[1]]<<4) | hex_decode[(unsigned)ptr[2]];
            ptr += 3;
            ++len;
        }
        else
        {
            *buffer++ = *ptr++;
            ++len;
        }
    }

    // terminate the buffer
    *buffer = 0;

    // return the length (dont count terminator)
    return(len-1);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetBinary

    \Description
        Set a binary field.

    \Input *pRecord     - data record
    \Input  iReclen     - record length
    \Input *pName       - field name
    \Input *_pValue     - start of binary data
    \Input  iVallen     - binary data length

    \Output int32_t     - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetBinary(char *pRecord, int32_t iReclen, const char *pName, const void *_pValue, int32_t iVallen)
{
    unsigned char *pAppend;
    const unsigned char *pValue = _pValue;

    // make sure length is valid
    if (iVallen < 0)
    {
        return(-1);
    }

    // get the append point
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, iVallen*2+1);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // mark as binary
    *pAppend++ = '$';

    // add the binary data
    for (; iVallen > 0; --iVallen, ++pValue)
    {
        *pAppend++ = hex_encode[*pValue>>4];
        *pAppend++ = hex_encode[*pValue&15];
    }

    // terminate the field
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetBinary7

    \Description
        Set a binary field using more efficient 7-bits-per-byte encoding.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input *_pValue  - start of binary data
    \Input  iVallen  - binary data length

    \Output int32_t  - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetBinary7(char *pRecord, int32_t iReclen, const char *pName, const void *_pValue, int32_t iVallen)
{
    unsigned char *pAppend;
    const unsigned char *pValue = _pValue;

    // make sure length is valid
    if (iVallen < 0)
    {
        return(-1);
    }

    // get the append point
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, (iVallen*8+6)/7+1);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // mark as binary7
    *pAppend++ = '^';

    // encode the data
    pAppend = (unsigned char *)Binary7Encode(pAppend, iReclen, pValue, iVallen, FALSE);

    // terminate the field
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetBinary

    \Description
        Get binary field contents.

    \Input *_pData   - data record
    \Input *_pBuffer - string target
    \Input  iBuflen  - length of target buffer

    \Output int32_t      - field value copied to target buffer, returns data length

    \Notes
        This function does not support a default value.

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetBinary(const char *_pData, void *_pBuffer, int32_t iBuflen)
{
    const unsigned char *pBinary;
    const unsigned char *pData = (const unsigned char *)_pData;
    unsigned char *pBuffer = (unsigned char *)_pBuffer;
    unsigned char *pBufend = pBuffer+iBuflen;

    // make sure data is valid
    if ((pData == NULL) || ((pData[0] != '$') && (pData[0] != '^')))
    {
        return(-1);
    }

    // see if they just want the length
    if (pBuffer == NULL)
    {
        if (*pData == '$')
        {
            // calc length wih 4-bit encoding
            for (pBinary = pData+1; (pBinary[0] >= '0') && (pBinary[1] >= '0'); pBinary += 2)
                ;
            return((int32_t)((pBinary-pData-1)/2));
        }
        else
        {
            // calc length with 7-bit encoding
            for (pBinary = pData+1; *pBinary >= 0x80; ++pBinary)
                ;
            return((int32_t)((pBinary-pData-1)*7/8));
        }
    }

    // make sure buffer has a valid length
    if (iBuflen < 1)
    {
        return(-1);
    }

    if (*pData == '$')
    {
        // copy over 4-bit encoded data
        for (pBinary = pData+1; (pBinary[0] >= '0') && (pBinary[1] >= '0') && (pBuffer < pBufend); pBinary += 2)
        {
            *pBuffer++ = (hex_decode[pBinary[0]]<<4) | hex_decode[pBinary[1]];
        }
    }
    else
    {
        pBuffer += ((Binary7Decode(pBuffer, iBuflen, pData+1)-pData-1)*7/8);
    }

    // return the amount of data extracted
    return(iBuflen-(int32_t)(pBufend-pBuffer));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetStructure

    \Description
        Save an entire structure as a single field.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input *pStruct_ - binary structure data
    \Input  iLength  - length of binary data (-1=use pattern length)
    \Input *pPattern - structure pattern

    \Output int32_t  - number of fields changed

    \Notes
        \n Structure pattern is a string that may consist of the following characters:
        \n
        \n  #: Field name (# name = count)
        \n  a: alignment skip ('count' bytes)
        \n  b: byte/character
        \n  w: word (four bytes)
        \n  l: int32_t (eight bytes)
        \n  s: string ('count' bytes)

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetStructure(char *pRecord, int32_t iReclen, const char *pName, const void *pStruct_, int32_t iLength, const char *pPattern)
{
    int32_t iShift;
    int32_t iWidth;
    int32_t iCount;
    uint32_t uValue = 0;
    unsigned char *pMarker;
    unsigned char *pAppend;
    const unsigned char *pStruct = pStruct_;
    const unsigned char *pLength = (iLength < 0 ? pStruct+65536 : pStruct+iLength);
    const unsigned char *pLimit = (unsigned char *)pRecord+iReclen-2;

    // get the append point
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, 0);
    if (pAppend == NULL)
    {
        return(-1);
    }

    // now add the data according to the pattern
    while (*pPattern != 0)
    {
        // make sure we dont read beyond end of structure
        if (pStruct >= pLength)
        {
            break;
        }
        // skip field name in #name= format
        if (*pPattern == '#')
        {
            for (++pPattern; (*pPattern != 0) && (*pPattern++ != '=');)
                ;
        }
        // calc count for commands that use it
        for (iCount = 0; (*pPattern >= '0') && (*pPattern <= '9'); ++pPattern)
        {
            iCount = (iCount * 10) + (*pPattern & 15);
        }

        // handle alignment skip
        if (*pPattern == 'a')
        {
            pStruct += (iCount ? iCount : 1);
        }

        // reset data length (expressed in nibbles)
        iWidth = 0;
        // handle byte/character
        if (*pPattern == 'b')
        {
            uValue = *pStruct;
            pStruct += 1;
            iWidth = 2;
        }
        // handle word
        if (*pPattern == 'w')
        {
            uValue = *((const uint16_t *)pStruct);
            pStruct += 2;
            iWidth = 4;
        }
        // handle int32_t
        if (*pPattern == 'l')
        {
            uValue = *((const uint32_t *)pStruct);
            pStruct += 4;
            iWidth = 8;
        }
        // insert binary data into stream if needed
        if (iWidth > 0)
        {
            // make sure there is room for next unit
            if (pAppend+iWidth > pLimit)
            {
                return(_TagFieldSetupCancel((unsigned char *)pRecord, pAppend));
            }

            // remember current append point so we can tell if comma is required
            pMarker = pAppend;

            // see if negative encoding will shorten data
            if (uValue > 0xffff0000)
            {
                uValue = (uint32_t)-((int32_t)uValue);
                *pAppend++ = '-';
            }
            // figure the initial shift point
            for (iShift = 28; (iShift >= 0) && (((uValue>>iShift)&15) == 0); iShift -= 4)
                ;
            // extract the nibbles from high to low
            for (; iShift >= 0; iShift -= 4)
            {
                *pAppend++ = hex_encode[(uValue>>iShift)&15];
            }

            // if number of chars stored < expected width, use a comma
            if (pAppend < pMarker+iWidth)
            {
                *pAppend++ = ',';
            }
        }

        // handle string
        if ((*pPattern == 's') && (iCount > 0))
        {
            for (iWidth = 0; (iWidth < iCount) && (*pStruct != 0); ++iWidth, ++pStruct)
            {
                // make sure room for max sized unit
                if (pAppend+3 > pLimit)
                {
                    return(_TagFieldSetupCancel((unsigned char *)pRecord, pAppend));
                }

                if (char_encode[*pStruct] & CHAR_STRUCTENC)
                {
                    *pAppend++ = '%';
                    *pAppend++ = hex_encode[*pStruct>>4];
                    *pAppend++ = hex_encode[*pStruct&15];
                }
                else
                {
                    *pAppend++ = *pStruct;
                }
            }
            pStruct += (iCount-iWidth);

            // add divider if room available
            if (pAppend+1 > pLimit)
            {
                return(_TagFieldSetupCancel((unsigned char *)pRecord, pAppend));
            }
            *pAppend++ = ',';
        }

        // advance the pattern
        if (pPattern[1] != '*')
        {
            ++pPattern;
        }
    }

    // truncate all trailing empty fields
    while (pAppend[-1] == ',')
    {
        --pAppend;
    }

    // terminate the field
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetStructure

    \Description
        Get packed structure field contents.

    \Input *_pData   - data record
    \Input *_pBuf    - struct target
    \Input iLen      - length of target buffer
    \Input *pPat     - structure pattern

    \Output int32_t  - struct copied to target buffer, returns struct length

    \Notes
        This function does not support a default value.  See TagFieldSetStructure() for a description
        of the pattern string.

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetStructure(const char *_pData, void *_pBuf, int32_t iLen, const char *pPat)
{
    char iNeg;
    int32_t iSiz, iCnt;
    char strTmp[1024];
    uint32_t uHex;
    uint32_t uRaw;
    const unsigned char *pData = (const unsigned char *)_pData;
    unsigned char *pBuf = _pBuf;
    unsigned char *pEnd = (iLen < 0 ? pBuf+65535 : pBuf+iLen);
    int32_t iDecodeLen;

    // make sure data is valid
    if (pData == NULL)
        pData = (const unsigned char *)"";
    if (pBuf == NULL)
        pBuf = _pBuf = (unsigned char *)strTmp;

    // now extract the data according to the pattern
    while (*pPat != 0)
    {
        // if there is no more input data
        if (*pData <= ' ')
            break;

        // calc count for commands that use it
        for (iCnt = 0; (*pPat >= '0') && (*pPat <= '9'); ++pPat)
            iCnt = (iCnt * 10) + (*pPat & 15);
        // reset data length (expressed in nibbles)
        iSiz = 0;

        // handle int32_t
        if (*pPat == 'l')
        {
            iSiz = 8;
        }
        // handle byte
        else if (*pPat == 'b')
        {
            iSiz = 2;
        }
        // handle word
        else if (*pPat == 'w')
        {
            iSiz = 4;
        }
        // handle string
        else if ((*pPat == 's') && (iCnt > 0))
        {
            // copy over the string
            for (iSiz = 0; ((*pData >= '0') || (*pData == '%')) && (iSiz+1 < iCnt); ++iSiz)
            {
                // do prefix decoding if required
                if (*pData == '%')
                {
                    pBuf[iSiz] = (hex_decode[(unsigned)pData[1]]<<4) | hex_decode[(unsigned)pData[2]];
                    pData += 3;
                }
                else
                {
                    pBuf[iSiz] = *pData++;
                }
            }
            // terminate the string and advance the buffer
            while (iSiz < iCnt)
            {
                pBuf[iSiz++] = 0;
            }
            // advance the buffer
            pBuf += iSiz;
            // skip the divider
            if (*pData == ',')
            {
                ++pData;
            }
            iSiz = 0;
        }
        // handle alignment skip
        else if (*pPat == 'a')
        {
            pBuf += (iCnt ? iCnt : 1);
        }

        // see if we need to extract data
        if (iSiz > 0)
        {
            // see if its negative
            iNeg = *pData;
            if (iNeg == '-')
                ++pData;

            // do the hex extract
            for (uRaw = 0; (iSiz > 0) && (uHex = hex_decode[(unsigned)*pData]) < 16; --iSiz, ++pData)
                uRaw = (uRaw << 4) | uHex;

            // skip the divider
            if ((iSiz > 0) && (*pData == ','))
                ++pData;

            // do negative conversion
            if (iNeg == '-')
                uRaw = (uint32_t)-((int32_t)uRaw);

            // save a int32_t
            if (*pPat == 'l')
            {
                *((uint32_t *)pBuf) = (uint32_t)uRaw;
                pBuf += 4;
            }
            // save a byte
            else if (*pPat == 'b')
            {
                *pBuf = (unsigned char)uRaw;
                pBuf += 1;
            }
            // save a word
            else if (*pPat == 'w')
            {
                *((uint16_t *)pBuf) = (uint16_t)uRaw;
                pBuf += 2;
            }
        }

        // check for buffer end
        if (pBuf >= pEnd)
            break;

        // advance the patter
        ++pPat;
        if (*pPat == '*')
            --pPat;
    }

    iDecodeLen = (int32_t)(pBuf - (unsigned char *)_pBuf);

    // Zero out what is left of the output buffer
    if (iDecodeLen < iLen)
        memset(pBuf, 0, iLen - iDecodeLen);

    // return the length
    return(iDecodeLen);
}


/*F*************************************************************************************************/
/*!
    \Function TagFieldSetEpoch

    \Description
        Set a date from epoch time using std C time.h lib.

    \Input *pRecord - data record
    \Input iReclen  - record length
    \Input *pName   - field name
    \Input uEpoch   - date in psuedo epoch format

    \Output int32_t - negative=record too small, zero=no error

    \Version 12/13/2004 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetEpoch(char *pRecord, int32_t iReclen, const char *pName, uint32_t uEpoch)
{
    int32_t iLength;
    char strDate[20];
    struct tm *tm, tm2;
    unsigned char *pAppend;

    // see if they want to use current time
    if (uEpoch == 0)
    {
        uEpoch = (uint32_t)ds_timeinsecs();
    }

    // convert to calendar time
    tm = ds_secstotime(&tm2, uEpoch);
    if (tm == NULL)
    {
        return(-1);
    }

    // setup in private buffer so we can determine the length
    iLength = ds_snzprintf(strDate, sizeof(strDate), "%d.%d.%d-%d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1,
        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    // get the append point for max-size date format
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, iLength);
    if (pAppend == NULL)
    {
        return(-1);
    }
    strcpy((char *)pAppend, strDate);
    pAppend += iLength;

    // terminate the field
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetEpoch

    \Description
        Get date in epoch time.

    \Input *pData       - data record
    \Input  uDefval     - default value

    \Output uint32_t    - date in psuedo epoch time, or default value if record does not exist

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
uint32_t TagFieldGetEpoch(const char *pData, uint32_t uDefval)
{
    struct tm tm;
    unsigned char uHex;
    uint32_t uDecimal;
    uint32_t uEpoch = 0;

    // make sure its valid
    if (pData == NULL)
    {
    }
    // handle binary format
    else if (pData[0] == '$')
    {
        for (++pData; (uHex = hex_decode[(unsigned)*pData]) < 16; ++pData)
        {
            uEpoch = (uEpoch << 4) | uHex;
        }
    }
    // handle decimal format
    else if ((pData[0] >= '0') && (pData[0] <= '9') && (_ParseNumber(pData, &uDecimal, 0)[0] <= ' '))
    {
        uEpoch = uDecimal;
    }
    // handle text/decimal format
    else if ((pData[0] >= '0') && (pData[0] <= '9'))
    {
        // extract the data
        memset(&tm, 0, sizeof(tm));
        tm.tm_isdst = -1;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_year, 4);
        if (((*pData < '0') || (*pData > '9')) && (*pData != '\0'))
            ++pData;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_mon, 2);
        if (((*pData < '0') || (*pData > '9')) && (*pData != '\0'))
            ++pData;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_mday, 2);
        if (((*pData < '0') || (*pData > '9')) && (*pData != '\0'))
            ++pData;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_hour, 2);
        if (((*pData < '0') || (*pData > '9')) && (*pData != '\0'))
            ++pData;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_min, 2);
        if (((*pData < '0') || (*pData > '9')) && (*pData != '\0'))
            ++pData;
        pData = _ParseNumber(pData, (uint32_t *)&tm.tm_sec, 2);

        // validate the fields
        if ((tm.tm_year < 1970) || (tm.tm_year > 2107) || (tm.tm_mon < 1) || (tm.tm_mon > 12) || (tm.tm_mday < 1) || (tm.tm_mday > 31))
            tm.tm_year = 0;
        if ((tm.tm_hour < 0) || (tm.tm_hour > 23) || (tm.tm_min < 0) || (tm.tm_min > 59) || (tm.tm_sec < 0) || (tm.tm_sec > 61))
            tm.tm_year = 0;

        // return epoch time
        if (tm.tm_year != 0)
        {
            tm.tm_mon -= 1;
            tm.tm_year -= 1900;
            uEpoch = (uint32_t)ds_timetosecs(&tm);
        }
    }

    // handle default case
    if (uEpoch == 0)
    {
        uEpoch = (uDefval == 0) ? (uint32_t)ds_timeinsecs() : uDefval;
    }

    return(uEpoch);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldSetDate

    \Description
        Set a date field from discrete date components.

    \Input *record  - data record
    \Input reclen   - record length
    \Input *name    - field name
    \Input year     - year component
    \Input month    - month component
    \Input day      - day component
    \Input hour     - hour component
    \Input minute   - minute component
    \Input second   - second component
    \Input gmt      - GMT offset in seconds (zero=GMT time)

    \Output int32_t - negative=record too small, zero=no error

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldSetDate(char *record, int32_t reclen, const char *name, int32_t year, int32_t month,
                    int32_t day, int32_t hour, int32_t minute, int32_t second, int32_t gmt)
{
    struct tm tm;
    uint32_t epoch;

    // convert to epoch
    memset(&tm, 0, sizeof(tm));
    tm.tm_isdst = -1;
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

    // convert to epoch
    epoch = (uint32_t)ds_timetosecs(&tm);
    if (epoch == 0)
        return(-1);

    // set the epoch
    if (epoch != 0)
        epoch -= gmt;
    return(TagFieldSetEpoch(record, reclen, name, epoch));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetDate

    \Description
        Get date.

    \Input *data    - data record
    \Input *pyear   - (out) year component
    \Input *pmonth  - (out) month component
    \Input *pday    - (out) day component
    \Input *phour   - (out) hour component
    \Input *pminute - (out) minute component
    \Input *psecond - (out) second component
    \Input gmt      - GMT offset in seconds (zero=GMT time)

    \Output int32_t - date in psuedo epoch time

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetDate(const char *data, int32_t *pyear, int32_t *pmonth, int32_t *pday, int32_t *phour, int32_t *pminute, int32_t *psecond, int32_t gmt)
{
    struct tm *tm, tm2;
    uint32_t epoch;

    // get the epoch time
    epoch = TagFieldGetEpoch(data, 1);
    if (epoch <= 1)
        return(-1);

    // convert to calendar time
    tm = ds_secstotime(&tm2, epoch+gmt);
    if (tm == NULL)
        return(-1);

    // return the results
    if (pyear != NULL)
        *pyear = tm->tm_year+1900;
    if (pmonth != NULL)
        *pmonth = tm->tm_mon+1;
    if (pday != NULL)
        *pday = tm->tm_mday;
    if (phour != NULL)
        *phour = tm->tm_hour;
    if (pminute != NULL)
        *pminute = tm->tm_min;
    if (psecond != NULL)
        *psecond = tm->tm_sec;
    return(0);
}

/*F***************************************************************************/
/*!
    \Function TagFieldSetFloat

    \Description
        Set a floating point field.

    \Input *pRecord  - data record
    \Input  iReclen  - record length
    \Input *pName    - field name
    \Input  fValue   - number

    \Output int32_t  - negative=record too small, zero=no error

    \Version 12/12/2004 (gschaefer)
*/
/***************************************************************************F*/
int32_t TagFieldSetFloat(char *pRecord, int32_t iReclen, const char *pName, float fValue)
{
    float fSign = fValue;
    uint32_t uInteger;
    uint32_t uFraction;
    unsigned char *pAppend;
    unsigned char strFormat[64];
    unsigned char *pData1 = strFormat+sizeof(strFormat)/2;
    unsigned char *pData2 = strFormat+sizeof(strFormat)/2;

    // deal with negative
    if (fValue < 0.0f)
    {
        fValue = -fValue;
    }

    // format the integer portion
    for (uInteger = (uint32_t)fValue; uInteger > 0; uInteger /= 10)
    {
        *--pData1 = '0'+(uInteger % 10);
    }

    // handle zero case
    if (pData1 == pData2)
    {
        *--pData1 = '0';
    }

    // insert sign
    if (fSign < 0.0f)
    {
        *--pData1 = '-';
    }

    // add up to 4 decimal digits
    *pData2++ = '.';
    uFraction = (uint32_t)(fValue * 10000.0f);
    uFraction %= 10000;

    // use do loop to add at least one decimal digit
    do
    {
        *pData2++ = '0'+(uFraction/1000)%10;
        uFraction = (uFraction*10)%10000;
    }
    while (uFraction != 0);

    // get the append point for max-size date format
    pAppend = _TagFieldSetupAppend((unsigned char *)pRecord, iReclen, pName, (int32_t)(pData2-pData1));
    if (pAppend == NULL)
    {
        return(-1);
    }

    // copy over data and terminate
    ds_memcpy(pAppend, pData1, (int32_t)(pData2-pData1));
    pAppend += pData2-pData1;
    return(_TagFieldSetupTerm(pRecord, (char *)pAppend, pName));
}

/*F***************************************************************************/
/*!
    \Function TagFieldGetFloat

    \Description
        Get a floating point field.

    \Input *pData       - data pointer
    \Input  fDefval     - default number

    \Output float       - float from field, or default number if field does not exist

    \Version 12/12/2004 (gschaefer)
*/
/***************************************************************************F*/
float TagFieldGetFloat(const char *pData, float fDefval)
{
    float fSign = 1.0f;
    int32_t iInteger = 0;
    int32_t iFraction = 0;
    int32_t iDivisor = 1;

    // return the default value
    if (pData == NULL)
    {
        return(fDefval);
    }

    // parse the sign
    if (*pData == '+')
    {
        ++pData;
    }
    else if (*pData == '-')
    {
        ++pData;
        fSign = -1.0f;
    }

    // parse the integer
    while ((*pData >= '0') && (*pData <= '9'))
    {
        iInteger = (iInteger * 10) + (*pData++ & 15);
    }

    // check for decimal point
    if (*pData == '.')
    {
        ++pData;

        // parse fraction and increase divisor
        while ((*pData >= '0') && (*pData <= '9'))
        {
            iFraction = (iFraction * 10) + (*pData++ & 15);
            iDivisor *= 10;
        }
    }

    // return the result
    return(fSign*(((float)iInteger)+((float)iFraction)/((float)iDivisor)));
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldPrintf

    \Description
        Create a tagfield record using printf style descriptor.

    \Input *_pRecord    - data record
    \Input  iLength     - record length
    \Input *pFormat     - format string
    \Input  ...         - variable argument list

    \Output int32_t     - field length (including terminator)

    \Notes
        \n Supported tokens include:
        \n
        \n  s: string
        \n  d: decimal
        \n  a: address
        \n  f: flags
        \n  e: epoch
        \n  r: record
        \n  #: replace part of tag name with number
        \n  ,: allow multiple items in delim situation

    \Version 04/03/2002 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldPrintf(char *_pRecord, int32_t iLength, const char *pFormat, ...)
{
    int32_t iCount;
    va_list args;
    unsigned char uCh;
    int32_t iInitial = iLength;
    const char *pInput = "";
    unsigned char *pRecord = (unsigned char *)_pRecord;

    // format the output
    va_start(args, pFormat);

    // see if they want to append to existing string
    if (*pFormat == '~')
    {
        ++pFormat;
        // find the append point
        while ((*pRecord != 0) && (iLength > 1))
        {
            ++pRecord;
            --iLength;
        }
        // add in a divider if needed
        if ((iLength > 1) && (iLength < iInitial) && (pRecord[-1] >= ' '))
        {
            *pRecord++ = g_divider;
            iLength -= 1;
        }
    }
    // see if they adding one more item to field
    else if (*pFormat == ',')
    {
        // find the append point
        while ((*pRecord != 0) && (iLength > 1))
        {
            ++pRecord;
            --iLength;
        }
        // see if we need to remove extra terminator
        while ((iLength < iInitial) && (pRecord[-1] <= ' '))
        {
            --pRecord;
            ++iLength;
        }
        // skip comma if this is first assignment
        if ((pRecord == (unsigned char *)_pRecord)
                || ((iLength < iInitial) && (pRecord[-1] == '=')))
        {
            ++pFormat;
        }
    }

    // walk the format list
    while (((uCh = *pFormat++) != 0) && (iLength > 1))
    {
        // gobble non-formatting chars
        if (uCh <= ' ')
        {
            *pRecord++ = g_divider;
            iLength -= 1;

            // gobble any extra spaces
            while ((*pFormat != 0) && (*pFormat <= ' '))
            {
                ++pFormat;
            }
        }
        // handle tag number replacement
        else if (uCh == '#')
        {
            int32_t iNum = va_arg(args, int32_t);
            iCount = ds_snzprintf((char *)pRecord, iLength, "%d", iNum);
            pRecord += iCount;
            iLength -= iCount;
        }
        // copy over non-formatting chars
        else if (uCh != '%')
        {
            // other characters are inserted normally
            *pRecord++ = uCh;
            iLength -= 1;
        }
        // handle formatting token
        else if (*pFormat != 0)
        {
            iCount = 0;
            uCh = *pFormat++;

            // make set point look like empty buffer
            *pRecord = 0;

            // handle string
            if (uCh == 's')
            {
                const char *pStr = va_arg(args, const char *);
                iCount = TagFieldSetString((char *)pRecord, iLength, NULL, pStr);
            }
            // handle decimal
            else if (uCh == 'd')
            {
                int32_t iNum = va_arg(args, int32_t);
                iCount = TagFieldSetNumber((char *)pRecord, iLength, NULL, iNum);
            }
            // handle token
            else if (uCh == 't')
            {
                uint32_t uToken = va_arg(args, uint32_t);
                iCount = TagFieldSetToken((char *)pRecord, iLength, NULL, uToken);
            }
            // handle address
            else if (uCh == 'a')
            {
                uint32_t uAddr = va_arg(args, uint32_t);
                iCount = TagFieldSetAddress((char *)pRecord, iLength, NULL, uAddr);
            }
            // handle flags
            else if (uCh == 'f')
            {
                int32_t flags = va_arg(args, int32_t);
                iCount = TagFieldSetFlags((char *)pRecord, iLength, NULL, flags);
            }
            // handle epoch
            else if (uCh == 'e')
            {
                uint32_t epoch = va_arg(args, uint32_t);
                iCount = TagFieldSetEpoch((char *)pRecord, iLength, NULL, epoch);
            }
            // set input source
            else if (uCh == 'i')
            {
                pInput = va_arg(args, const char *);
            }
            // handle external item copy
            else if (uCh == 'x')
            {
                const char *pFind = va_arg(args, const char *);
                pFind = TagFieldFind(pInput, pFind);
                iCount = TagFieldSetRaw((char *)pRecord, iLength, NULL, pFind);
            }
            // handle record copy
            else if (uCh == 'r')
            {
                const char *pStr = va_arg(args, const char *);
                while ((*pStr != 0) && (iLength > 0))
                {
                    *pRecord++ = *pStr++;
                    --iLength;
                }
            }

            // adjust the buffer
            if (iCount > 0)
            {
                pRecord += iCount;
                iLength -= iCount;
            }
        }
    }

    va_end(args);

    // see if we need final divider
    if ((g_divider == '\n') && (iInitial != iLength) && (iLength > 1))
    {
        *pRecord++ = g_divider;
        iLength -= 1;
    }

    // include terminator
    if (iLength > 0)
        *pRecord = 0;

    // return length (including terminator)
    return(iInitial - iLength);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldGetDelim

    \Description
        Get item from delimeter separated list.

    \Input *data    - data record
    \Input *buffer  - string target
    \Input buflen   - length of target buffer
    \Input *defval  - default value
    \Input index    - index of item within list
    \Input delim    - list delimiter

    \Output int32_t - field value or defval copied to target buffer, returns string length

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldGetDelim(const char *data, char *buffer, int32_t buflen, const char *defval, int32_t index, int32_t delim)
{
    int32_t len;
    unsigned char term;
    unsigned char stop;
    const unsigned char *ptr;

    // locate the item
    if (data != NULL)
    {
        // see if we need to skip to a subsequent item
        for (; index > 0; --index)
        {
            unsigned char c;

            // setup terminator
            if (*data == '"')
            {
                stop = ' ';
                term = *data++;
            }
            else
            {
                stop = ' '+1;
                term = delim;
            }

            // skip to the next item
            while (c = *data, (c != term) && (c >= stop))
            {
                ++data;
            }

            // gobble closing quote
            if ((*data == '"') && (*data == term))
            {
                ++data;
            }

            // gobble the delim
            if (*data == delim)
            {
                ++data;
            }
            else
            {
                data = NULL;
                break;
            }
        }
    }

    // make sure data is valid
    if (data == NULL)
    {
        // return default value
        if (defval == NULL)
            return(-1);
        // see if they just want length
        if (buffer == NULL)
            return((int32_t)strlen(defval));
        // copy over the string
        for (len = 1; (len < buflen) && (*defval != 0); ++len)
            *buffer++ = *defval++;
        *buffer = 0;
        return(len-1);
    }

    // setup terminator
    if (*data == '"')
    {
        stop = ' ';
        term = *data++;
    }
    else
    {
        stop = ' '+1;
        term = delim;
    }

    // see if they want to test string length
    if (buffer == NULL)
    {
        len = 0;
        for (ptr = (const unsigned char *)data; (*ptr != term) && (*ptr >= stop); ++ptr)
        {
            if ((ptr[0] == '%') && (ptr[1] >= ' ') && (ptr[2] >= ' '))
                ptr += 2;
            len += 1;
        }
        return(len);
    }

    // make sure buffer is valid
    if (buflen < 1)
        return(-1);

    // length=1 to make room for terminator
    len = 1;
    ptr = (const unsigned char *)data;
    // convert the string
    while ((len < buflen) && (*ptr != term) && (*ptr >= stop))
    {
        // handle escape sequences
        if ((ptr[0] == '%') && (ptr[1] == '%'))
        {
            *buffer++ = '%';
            ptr += 2;
            ++len;
        }
        else if ((ptr[0] == '%') && (ptr[1] >= ' ') && (ptr[2] >= ' '))
        {
            *buffer++ = (hex_decode[(unsigned)ptr[1]]<<4) | hex_decode[(unsigned)ptr[2]];
            ptr += 3;
            ++len;
        }
        else
        {
            *buffer++ = *ptr++;
            ++len;
        }
    }

    // terminate the buffer
    *buffer = 0;

    // return the length (dont count terminator)
    return(len-1);
}

/*F*************************************************************************************************/
/*!
    \Function TagFieldFirst

    \Description
        Extract the first field name in record.

    \Input *_record - data record
    \Input *nameptr - name field return pointer
    \Input namelen  - max length

    \Output int32_t - zero=empty record, positive=name length

    \Version 01/23/2000 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t TagFieldFirst(const char *_record, char *nameptr, int32_t namelen)
{
    int32_t len = 0;
    const unsigned char *equ;
    const unsigned char *beg;
    const unsigned char *record = (const unsigned char *)_record;

    // default return buffer to empty string
    if (namelen > 0)
        nameptr[0] = 0;

    // make sure record is valid
    if ((record == NULL) || (*record == 0))
        return(0);

    // find first field
    for (equ = record; *equ != 0; ++equ)
    {
        // look for the assignment
        if (*equ != '=')
            continue;
        // if either is followed by a newline, that marks end of tags
        if ((equ[1] < ' ') && (equ[-1] <= ' '))
            break;
        // find the start of the name field
        for (beg = equ; (beg != record) && (beg[-1] > ' '); --beg)
            ;
        // copy over the name
        while ((beg != equ) && (len+1 < namelen))
        {
            nameptr[len++] = *beg++;
        }
        nameptr[len] = 0;
        break;
    }

    // return name length
    return(len);
}

/*F***************************************************************************/
/*!
    \Function TagFieldFindNext

    \Description
        Find next tag field record.

    \Input *pRecord                 - Data record
    \Input *pName                   - Output buffer to place name of tag in
    \Input  iNameSize               - Size of pName

    \Output const unsigned char *   - pointer to start of next record
*/
/***************************************************************************F*/
const char *TagFieldFindNext(const char *pRecord, char *pName, int32_t iNameSize)
{
    const unsigned char *pRec = (const unsigned char *)pRecord;
    const unsigned char *pEq;

    if ((pRec == NULL) || (pRec[0] == '\0'))
        return(NULL);

    // IF pointing at a quoted string
    if (pRec[0] == '"')
    {
        // Skip until matching closing quote found.  This helps deal with cases when
        // parsing hand-coded tag fields with nested = characters in strings.
        pRec++;
        while ((pRec[0] != '\0') && (pRec[0] != '"'))
            pRec++;
    }

    // Look for = indicating next tag
    while ((pRec[0] != '\0') && (pRec[0] != '='))
        pRec++;

    // No more records available
    if (pRec[0] == '\0')
        return(NULL);

    pEq = pRec;

    // Walk back and populate the tag name
    while (((const char *)pRec != pRecord) && (pRec[0] > ' '))
        pRec--;

    // Don't include separator character if present
    if (*pRec <= ' ')
        pRec++;

    if ((iNameSize > 0) && (pName != NULL))
    {
        // Copy tag name into provided buffer
        while ((pRec < pEq) && (iNameSize > 1))
        {
            pName[0] = pRec[0];
            pName++;
            pRec++;
            iNameSize--;
        }
        pName[0] = '\0';
    }

    // return pointer to data
    return((const char *)(pEq+1));
}

/*F********************************************************************************/
/*!
     \Function TagFieldGetStructureOffsets

     \Description
        Populate pPtrs with memory offsets to the individual fields of a structure
        as defined by the given pattern.

     \Input pPat        - Pattern to build offsets for
     \Input pPtrs       - Array to build offsets into
     \Input iNumPtrs    - Size of pPtrs array

     \Output int32_t    - Number of pointers populated.

     \Version 05/06/2005 (doneill)
*/
/********************************************************************************F*/
int32_t TagFieldGetStructureOffsets(const char *pPat, TagFieldStructPtrT *pPtrs, int32_t iNumPtrs)
{
    int32_t iCnt;
    int32_t iPtr = 0;
    int32_t iOffset = 0;

    // WHILE there is still pattern left and we still have space in the ptrs array
    while ((*pPat != 0) && (iPtr < iNumPtrs))
    {
        // calc count for commands that use it
        for (iCnt = 0; (*pPat >= '0') && (*pPat <= '9'); ++pPat)
            iCnt = (iCnt * 10) + (*pPat & 15);

        // handle int32_t
        if (*pPat == 'l')
        {
            pPtrs[iPtr].eType = TAGFIELD_PATTERN_INT32;
            pPtrs[iPtr].iOffset = iOffset;
            iOffset += 4;
            iPtr++;
        }
        // handle byte
        else if (*pPat == 'b')
        {
            pPtrs[iPtr].eType = TAGFIELD_PATTERN_INT8;
            pPtrs[iPtr].iOffset = iOffset;
            iOffset += 1;
            iPtr++;
        }
        // handle word
        else if (*pPat == 'w')
        {
            pPtrs[iPtr].eType = TAGFIELD_PATTERN_INT16;
            pPtrs[iPtr].iOffset = iOffset;
            iOffset += 2;
            iPtr++;
        }
        // handle string
        else if ((*pPat == 's') && (iCnt > 0))
        {
            pPtrs[iPtr].eType = TAGFIELD_PATTERN_STR;
            pPtrs[iPtr].iOffset = iOffset;
            iOffset += iCnt;
            iPtr++;
        }
        // handle alignment skip
        else if (*pPat == 'a')
        {
            iOffset += iCnt ? iCnt : 1;
        }

        // advance the pattern
        ++pPat;
        if (*pPat == '*')
            --pPat;
    }

    return(iPtr);
}
