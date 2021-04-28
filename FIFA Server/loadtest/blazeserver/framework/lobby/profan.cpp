/*H*************************************************************************************************/
/*!
    \File Profan.c

    \Description
        A high performance profanity filter based on construction of a finite state
        machine. Requires significant time (milliseconds) and memory (megabytes)
        to construct, but operates in linear time. The number of words does not impact
        the runtime performance (only construction requirements).

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002. All rights reserved.

    \Version 1.0  11/02/2002 (GWS) Initial version
*/
/*************************************************************************************************H*/


/*** Include files ********************************************************************************/

#include "framework/blazedefines.h"
#include <coreallocator/icoreallocatormacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "platform/platform.h"

#include "profan.h"

/*** Constants ************************************************************************************/

#define PROFANITY_FILTER_VERSION    4000
#define PROFANITY_FILTER_SIGNATURE  0xbada55e5

/* 
 * Defining PROFANITY_FILTER_DEBUG will create two dot files named allow and deny in the working
 * directory. The dot files represent the allow and deny state tables respectively. The definition
 * also enables print out of useful debugging information (ie - allow & deny deref #s, allow & deny
 * depth) while converting input.
 */
//#define PROFANITY_FILTER_DEBUG

#define SPACE_MIN 27         // 1 + (last xlat mapping assigned to a latin alpha character [a-zA-Z])
                             // No character has this mapping; the next valid mapping is SPACE_MIN+1
                             // (Note that the characters 'A'-'Z' have the same mappings as their lowercase equivalents)

#define SPACE_MAX 255        // 1 + (max valid xlat mapping for a UTF-8 character)
                             // The filter will recognize (SPACE_MAX - SPACE_MIN - 1) unique UTF-8 characters in addition to the latin alpha characters [a-zA-Z].
                             // WARNING: Due to the use of uint8_t's to store xlat mappings, this value cannot be increased.

#define MAXLINK SPACE_MAX+1        // max number of links per FSM state
#define STATE_MAX 32768            // max number of FSM states
#define FINAL_MAX 2*STATE_MAX-1    // max total number of FSM state transitions
                                   // WARNING: Due to the use of uint16_t's to store FSM state and link indices, MAXLINK, STATE_MAX, and FINAL_MAX 
                                   // cannot be increased beyond 65535.

#define XLATMAX 65536              // 1 + (max supported hex. encoding for a UTF-8 character)
                                   // WARNING: Due to the use of uint16_t's to store UTF-8 characters internally, this value cannot be increased.
                                   // (Note that this limits the supported character set to UTF-8 characters up to 3 bytes long - as a UTF-8 character 
                                   // is read in, its bytes are stripped of high-order bits and compressed into a uint16_t.)

#define MAX_EQUIV 512              // max number of equivalent characters
#define TEXTLEN_MAX 4096           // max length of text to be filtered (not including the terminator character).
                                   // If the supplied text is too long, only the first TEXTLEN_MAX bytes will be filtered.

#define FLAG_EXLFT  1   // extend word to left
#define FLAG_EXRGT  2   // extend word to right
#define FLAG_ENDOF  4   // on either end of larger word
#define FLAG_SPLFT  8   // there is space to left of word
#define FLAG_SPRGT 16   // there is space to right of word
#define FLAG_FILT  32   // filter the word

/*** Macros ************************************************************************************/

// Get the next UTF-8 encoded character from the given buffer
#define GET_UTF8(pBuf,pLen) ((*(pBuf) & 0x80) == 0) ? (*(pLen) = 1, *(pBuf)) : _GetUTF8(pBuf, pLen)

/*** Variables ************************************************************************************/
#define GTERM_SIZE 128
static const unsigned char g_term[GTERM_SIZE] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00-0f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 10-1f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, // 20-2f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 30-3f
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40-4f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, // 50-5f
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60-6f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, // 70-7f
};

static const unsigned char *g_consonants = (const unsigned char *)
    "bcdfghjklmnpqrstvwxyz"
    ;

static const unsigned char *g_equiv = (const unsigned char *)
    "~equiv:"
    "g9"
    "G6"
    "i!"
    "il"
    "i1"
    "i|"
    "o0"
    "uv"
    "sz"
    "S5"
    "S$"
    "E3"
    ;

static const char *g_default =
{
    "ass\n"
    "ass#*\n"
    "bitch\n"
    "cunt\n"
    "dildo\n"
    "fuck\n"
    "jackass\n"
    "nigger\n"
    "prick\n"
    "shit\n"
    "\0"
};


/*** Type Definitions *****************************************************************************/

typedef struct StateT
{
    uint16_t link[MAXLINK];
} StateT;

typedef struct DerefT
{
    uint16_t next;
    unsigned char flags;
    unsigned char depth;
    int32_t word;
} DerefT;

typedef struct TableT
{
    // state machine table
    int32_t scount;                     // current state table count
    int32_t slimit;                     // maximum state table count
    StateT *state;                  // pointer to state tables

    // final state table
    int32_t dcount;                     // current final states
    int32_t dlimit;                     // maximum number of words (max final states)
    DerefT *deref;                  // pointer to final states

    // character translation
    int32_t xindex;                     // current highest character mapping
    unsigned char xlat[XLATMAX];        // map characters to sequential range (0..XLATMAX-1)

    // map equivilent characters
    struct 
    {
        uint16_t uBase;
        uint16_t uEquiv;
    } equiv[MAX_EQUIV];
    int32_t eindex;

    unsigned char term[MAXLINK];        // word termination table

} TableT;

typedef struct ConvT
{
    uint32_t uLen; // Length of converted string in Unicode chars *including* null terminator
    uint32_t uSrcLen; // Length of original string in bytes, *excluding* null terminator
    const unsigned char *pText; // Source string

    // Sequential mapping values for source characters
    struct
    {
        uint8_t uChDeny; // Mapping for the deny FSM
        uint8_t uChAllow; // Mapping for the allow FSM
        uint8_t uAddDeny;
        uint8_t uAddAllow;
        int8_t iLen; // Number of bytes needed to encode source character
        uint16_t uIdx; // Index to start of source character in pText
    } aXlat[TEXTLEN_MAX+1]; // Includes one extra slot for terminator element
} ConvT;

struct ProfanRef
{
    int32_t iReplPos;                     // offset within list
    int32_t iReplLen;                     // list length (for rollover)
    unsigned char strRepl[256];       // replacement text

    // point to data table
    TableT *pDeny;
    TableT *pAllow;

    char *pWords;
    uint16_t uNumWords;
    int32_t iNextWord;
    int32_t iLastMatchedRule;

    ConvT Conv;
};

/*** Private Functions ****************************************************************************/

/*F********************************************************************************/
/*!
     \Function _GetUTF8
 
     \Description
        Extract the next UTF-8 encoded character from the given buffer.  Note that
        this function does not perform any validation of the input.
  
     \Input pBuf - Buffer to extract the next character from.
     \Input pLen - Number of bytes consumed (-1 for invalid encoding).
 
     \Output uint16_t - Extracted character.
 
     \Version 06/20/2006 (doneill)
*/
/********************************************************************************F*/
static uint16_t _GetUTF8(const unsigned char *pBuf, int32_t *pLen)
{
    if (((pBuf[0] & 0xe0) == 0xc0) && (pBuf[1]))
    {
        *pLen = 2;
        return (((pBuf[0] & 0x1f) << 6) | (pBuf[1] & 0x3f));
    }

    if (((pBuf[0] & 0xf0) == 0xe0) && (pBuf[1] & pBuf[2]))
    {
        *pLen = 3;
        return (((pBuf[0] & 0xf) << 12)
                | ((pBuf[1] & 0x3f) << 6)
                | (pBuf[2] & 0x3f));
    }

    if (((pBuf[0] & 0xf8) == 0xf0) && (pBuf[1] & pBuf[2] & pBuf[3]))
    {
        *pLen = 4;
        return (((pBuf[0] & 0x7) << 18)
                | ((pBuf[1] & 0x3f) << 12)
                | ((pBuf[2] & 0x3f) << 6)
                | (pBuf[3] & 0x3f));
    }

    // UTF-8 5&6 byte codes were deprecated by RFC-3629 to match UTF-16 and are no longer supported.

    *pLen = -1;
    return (pBuf[0]);
}

/*F********************************************************************************/
/*!
     \Function _ProfanXlat
 
     \Description
          Perform all the allow/deny character translations for the given
          source string in preparation for filtering.  Also note the original
          locations of each decoded character to handle multi-byte encodings.
          The source input text should be assigned to pConv->pText before
          calling this function.
  
     \Input pRef - Profanity filter reference.
     \Input pConv - Conversion structure to decode into.
 
     \Output int32_t - Number of characters decoded.
 
     \Version 06/20/2006 (doneill)
*/
/********************************************************************************F*/
static int32_t _ProfanXlat(ProfanRef *pRef, ConvT *pConv)
{
    uint32_t uIdx;
    uint16_t uCh;
    int32_t iChLen;
    const unsigned char *pT = pConv->pText;

    for(uIdx = 0; (uIdx < TEXTLEN_MAX)
            && (*pT != '\0'); uIdx++)
    {
        uCh = GET_UTF8(pT, &iChLen);
        pConv->aXlat[uIdx].uChDeny = pRef->pDeny->xlat[uCh];
        pConv->aXlat[uIdx].uChAllow = pRef->pAllow->xlat[uCh];
        pConv->aXlat[uIdx].uIdx = static_cast<uint16_t>(pT - pConv->pText);
        pConv->aXlat[uIdx].uAddDeny = 0;
        pConv->aXlat[uIdx].uAddAllow = 0;
        if (iChLen < 0)
        {
            pConv->aXlat[uIdx].iLen = 1;
            pT += 1;
        }
        else
        {
            pConv->aXlat[uIdx].iLen = static_cast<int8_t>(iChLen);
            pT += iChLen;
        }
    }
    // always add terminator, we deliberately left a slot for it in aXlat[TEXTLEN_MAX+1]
    pConv->aXlat[uIdx].uChDeny = 0;
    pConv->aXlat[uIdx].uChAllow = 0;
    pConv->aXlat[uIdx].uIdx = static_cast<uint16_t>(pT - pConv->pText);
    pConv->aXlat[uIdx].iLen = 1;
    pConv->aXlat[uIdx].uAddDeny = 0;
    pConv->aXlat[uIdx].uAddAllow = 0;
    uIdx++;
    pConv->uSrcLen = (uint32_t)(pT - pConv->pText);
    pConv->uLen = uIdx; // includes terminating element
    return (uIdx);
}

#ifdef PROFANITY_FILTER_DEBUG
static void _DebugPrintTableLimits(const TableT& table)
{
    printf("xindex (last xlat mapping):\nvalue: %i\nlimit: %i\n\n", table.xindex, SPACE_MAX);
    printf("eindex (number of equivalent character rules):\nvalue: %i\nlimit: %i\n\n", table.eindex, MAX_EQUIV);
    printf("scount (number of FSM states):\nvalue: %i\nlimit: %i\n\n", table.scount, table.slimit);
    printf("dcount (total number of FSM state transitions):\nvalue: %i\nlimit: %i\n\n", table.dcount, table.dlimit);
}
#endif // PROFANITY_FILTER_DEBUG

// allocate a new table
static TableT *_TableAlloc(int32_t slimit, int32_t dlimit)
{
    TableT *pTable;
    int32_t iIndex;

    // allocate table storage
    pTable = (TableT *)BLAZE_ALLOC(sizeof(*pTable));
    memset(pTable, 0, sizeof(*pTable));

    // allocate state list
    pTable->scount = 1;
    pTable->slimit = slimit;
    pTable->state = (StateT *)BLAZE_ALLOC(sizeof(pTable->state[0])*pTable->slimit);
    memset(pTable->state, 0, sizeof(pTable->state[0])*pTable->slimit);

    // allocate final list
    pTable->dcount = 1;
    pTable->dlimit = dlimit;
    pTable->deref = (DerefT *)BLAZE_ALLOC(sizeof(pTable->deref[0])*pTable->dlimit);
    memset(pTable->deref, 0, sizeof(pTable->deref[0])*pTable->dlimit);

    // setup the default character translation table
    for (iIndex = 1; iIndex < (int32_t)(EAArrayCount(pTable->xlat)); ++iIndex)
    {
        pTable->xlat[iIndex] = SPACE_MAX;
    }
    // put in uppercase/lowercase
    for (iIndex = 'A'; iIndex <= 'Z'; ++iIndex)
    {
        pTable->xlat[iIndex+0] = (iIndex & 31);
        pTable->xlat[iIndex+32] = (iIndex & 31);
    }
    // put in other characters
    int32_t iCur = (iIndex & 31);
    for (iIndex = ' ' + 1; iIndex <= '~'; ++iIndex)
    {
        if (pTable->xlat[iIndex] == SPACE_MAX)
        {
            pTable->xlat[iIndex] = static_cast<unsigned char>(++iCur);
        }
    }

    pTable->xindex = iCur;

    // return the table
    return(pTable);
}


// add an equivilence character to the mapping table
static bool _TableEquiv(TableT *pTable, uint16_t uBase, uint16_t uEquiv)
{
    if (pTable->eindex+1 == MAX_EQUIV)
        return false;

    // only map if not already defined
    if (pTable->xlat[uEquiv] == SPACE_MAX)
    {
        // if mapping to itself, add a new symbol to table
        if (uBase == uEquiv)
        {
            if (pTable->xindex+1 == SPACE_MAX)
                return false;

            pTable->xlat[uBase] = static_cast<unsigned char>(pTable->xindex);
            pTable->xindex += 1;
        }
        else
        {
            pTable->xlat[uEquiv] = pTable->xlat[uBase];
        }
    }
    pTable->equiv[pTable->eindex].uBase = uBase;
    pTable->equiv[pTable->eindex].uEquiv = uEquiv;
    pTable->eindex++;

    return true;
}


// duplicate a table (and correct the size)
static TableT *_TableDupl(TableT *pSource)
{
    TableT *pTable;

    // allocate table storage
    pTable = (TableT *)BLAZE_ALLOC(sizeof(*pTable));

    // copy over original
    memcpy(pTable, pSource, sizeof(*pTable));

    // allocate state list
    pTable->slimit = pTable->scount = pSource->scount;
    pTable->state = (StateT *)BLAZE_ALLOC(sizeof(pTable->state[0])*pTable->slimit);
    memcpy(pTable->state, pSource->state, sizeof(pTable->state[0])*pTable->slimit);

    // allocate final list
    pTable->dlimit = pTable->dcount = pSource->dcount;
    pTable->deref = (DerefT *)BLAZE_ALLOC(sizeof(pTable->deref[0])*pTable->dlimit);
    memcpy(pTable->deref, pSource->deref, sizeof(pTable->deref[0])*pTable->dlimit);

    // return the table
    return(pTable);
}


// BLAZE_FREE up a table
static void _TableFree(TableT *pTable)
{
    if (pTable != nullptr)
    {
        BLAZE_FREE(pTable->state);
        BLAZE_FREE(pTable->deref);
        BLAZE_FREE(pTable);
    }
}

// extract symbol with optional hex decoding
static const unsigned char *_TableSymbol(const unsigned char *pParse, uint16_t *pData)
{
    int32_t iHex;
    int32_t iLen;

    // force a default value
    *pData = 0;

    // look for a valid character
    for (; *pParse >= ' '; ++pParse)
    {
        // handle hex special
        if ((pParse[0] == '$') && (pParse[1] > ' ') && (pParse[2] > ' '))
        {
            // parse hex digits into value (limit to 2 digits)
            for (iHex = 256, ++pParse; iHex < 65536; ++pParse)
            {
                if ((*pParse >= '0') && (*pParse <= '9'))
                {
                    iHex = (iHex<<4)|(*pParse&15);
                }
                else if ((*pParse >= 'a') && (*pParse <= 'f'))
                {
                    iHex = (iHex<<4)|(*pParse-'a'+10);
                }
                else if ((*pParse >= 'A') && (*pParse <= 'F'))
                {
                    iHex = (iHex<<4)|(*pParse-'A'+10);
                }
                else
                {
                    break;
                }
            }
            // save value
            *pData = static_cast<uint16_t>(iHex);
            break;
        }
        else if (pParse[0] > ' ')
        {
            // grab literal data
            *pData = GET_UTF8(pParse, &iLen);
            if (iLen < 0 || iLen > 3)
            {
                if (iLen == 4)
                    printf("4-byte UTF-8 character encountered.\n");
                else
                    printf("Non UTF-8 character encountered.\n");
                return nullptr;
            }
            else
                pParse += iLen;
            break;
        }
    }

    // point to next data
    return(pParse);
}

// add a word to the table
static int32_t _TableWord(TableT *pTable, const unsigned char *pWord, int32_t iFlags, int32_t iWord)
{
    uint16_t uState = 0;
    uint16_t uPrevState = 0;
    uint16_t uLink = 0;
    const unsigned char *pScan;

    for (pScan = pWord; pScan[0] != 0; ++pScan)
    {
        uLink = pTable->state[uState].link[(int32_t)pScan[0]];

        // see if there is a state reference
        if (uLink == 0)
        {
            if (pTable->dcount > pTable->dlimit-2)
            {
                return(-2);
            }
            uLink = static_cast<uint16_t>((pTable->dcount)++);
            pTable->state[uState].link[(int32_t)pScan[0]] = uLink;
        }

        // add in new state if this is not last
        if ((pScan[1] != 0) && (pTable->deref[uLink].next == 0))
        {
            if (pTable->scount > pTable->slimit-2)
            {
                return(-1);
            }
            pTable->deref[uLink].next = static_cast<uint16_t>((pTable->scount)++);
        }

        // transition to next state & remember current state
        uPrevState = uState;
        uState = pTable->deref[uLink].next;
    }

    // Check if last character of the rule was # (excluding wildcards)
    uint16_t uCh = 0;
    _TableSymbol((const unsigned char *) "#", &uCh);
    uCh = pTable->xlat[uCh];
    if (*(pScan-1) == uCh)
    {
        // Remove the link for #
        pTable->state[uPrevState].link[uCh] = 0;

        // Create new dref for each consonant (b, c, d, f...)
        const unsigned char *u = g_consonants;
        for (; *u != 0; u++)
        {
            // Get index into link for the consonant
            _TableSymbol(u, &uCh);
            uCh = pTable->xlat[uCh];

            // Create & set the deref accordingly if no deref already exists
            if (pTable->state[uPrevState].link[uCh] == 0)
            {
                if (pTable->dcount < pTable->dlimit)
                {
                    // handle termination information
                    pTable->deref[uLink].depth = (unsigned char)(pScan-pWord);
                    pTable->deref[uLink].flags |= iFlags;
                    pTable->deref[uLink].word = iWord;

                    // Link the new deref in the state
                    pTable->state[uPrevState].link[uCh] = uLink;
                    uLink = static_cast<uint16_t>(pTable->dcount++);
                }
                else
                {
                    return(-2);
                }
            }
        }
        pTable->dcount--;
    }
    else
    {
        // handle termination information
        pTable->deref[uLink].depth = (unsigned char)(pScan-pWord);
        pTable->deref[uLink].flags |= iFlags;
        pTable->deref[uLink].word = iWord;
    }

    return(0);
}

static void _TableRecurse2(TableT *pTable, const TableT *pTable0, int32_t iDeref, int32_t iDeref0)
{
    // pTable is mutable, pTable0 is immutable
    // primary deref is iDeref, secondary deref is iDeref0
    // primary state is iState, secondary state is iState0

    // Continue only if we have valid primary & secondary derefs
    if (iDeref == 0 || iDeref == iDeref0)
    {
        return;
    }

    int32_t iState  = pTable0->deref[iDeref].next;
    int32_t iState0 = pTable0->deref[iDeref0].next;

    // Continue only if we have a valid primary state
    if (iState == 0)
    {
        // Copy deref information if our secondary state is valid and if
        // we have an invalid primary deref in the mutable table
        if (pTable->deref[iDeref].next == 0 && iState0 != 0)
        {
            pTable->deref[iDeref].next = static_cast<uint16_t>(iState0);
        }
        return;
    }

    // Process all links in the primary state
    int iChar;
    for (iChar = 0; iChar < pTable->xindex; ++iChar)
    {
        // Determine the next primary & secondary derefs
        int32_t iNextDeref  = pTable->state[iState].link[iChar];
        int32_t iNextDeref0 = pTable0->state[iState0].link[iChar];
        

        if (iNextDeref == 0 && iNextDeref0 != 0)
        {
            pTable->state[iState].link[iChar] = static_cast<uint16_t>(iNextDeref0);
        }
        else if (iNextDeref != 0 && iNextDeref0 != 0)
        {
            // Step the state machines if the primary and secondary states are valid
            _TableRecurse2(pTable, pTable0, pTable0->state[iState].link[iChar], iNextDeref0);
        }
    }
}

static void _TableRecurse(TableT *pTable, TableT *pTable0, int32_t iDeref)
{
    int32_t iState = pTable0->deref[iDeref].next;

    // Process all links in the state
    int iChar;
    for (iChar = 0; iChar < pTable->xindex; ++iChar)
    {
        int32_t iNextDeref = pTable0->state[iState].link[iChar];
        int32_t iNextState = pTable0->deref[iNextDeref].next;

        // Continue recursing into the structure if our next deref & state are valid
        if (iNextDeref > 0 && iNextState > 0)
        {
            //printf("iDeref=%d;iNextDeref=%d=iState[%d].link[%d]\n", iDeref, iNextDeref, iState, iChar);
            // Copy over links while traversing two state machines
            _TableRecurse2(pTable, pTable0, iNextDeref, 0);

            // Step the state machine
            _TableRecurse(pTable, pTable0, iNextDeref);
        }
    }
}

// handle loopback from partial matches
static void _TableLoopback(TableT *pTable)
{
    int32_t iChar;
    int32_t iState;

    for (iState = 1; iState < pTable->scount; ++iState)
    {
        for (iChar = 0; iChar < pTable->xindex; ++iChar)
        {
            if ((pTable->state[iState].link[iChar] == 0) &&
                (pTable->state[0].link[iChar] != 0))
            {
                pTable->state[iState].link[iChar] = pTable->state[0].link[iChar];
            }
        }
    }
}


// recurse the table and allow letters to repeat
static void _TableRepeat(TableT *pTable, int32_t iState)
{
    int32_t iNext;
    int32_t iLink;
    int32_t iIndex;

    // do each character within state
    for (iIndex = 0; iIndex < pTable->xindex; ++iIndex)
    {
        // get the link/next values
        iLink = pTable->state[iState].link[iIndex];
        iNext = pTable->deref[iLink].next;
        if (iNext != 0)
        {
            // recurse down
            _TableRepeat(pTable, iNext);
            // allow repeat
            if (pTable->state[iNext].link[iIndex] == 0)
            {
                pTable->state[iNext].link[iIndex] = static_cast<uint16_t>(iLink);
            }
        }
    }
}


// setup the character substitution table
static void _TableSubst(TableT *pTable, const unsigned char *pEquivList)
{
    int32_t iState;
    int32_t iIndex;

    // do remap within each state
    for (iState = 0; iState < pTable->scount; ++iState)
    {
        for(iIndex = 0; iIndex < pTable->eindex; iIndex++)
        {
            int32_t uTo = pTable->xlat[pTable->equiv[iIndex].uEquiv];
            int32_t uFrom = pTable->xlat[pTable->equiv[iIndex].uBase];
            if ((pTable->state[iState].link[uFrom] > 0) && (pTable->state[iState].link[uTo] == 0))
            {
                pTable->state[iState].link[uTo] = pTable->state[iState].link[uFrom];
            }
        }
    }
}


// recurse the table and allow spaces to be ignored
static void _TableSpaces(TableT *pTable)
{
    int32_t iNext;
    int32_t iDepth;
    int32_t iDeref;
    int32_t iSpace;

    // do every deref
    for (iDeref = 0; iDeref < pTable->dcount; ++iDeref)
    {
        iNext = pTable->deref[iDeref].next;
        iDepth = pTable->deref[iDeref].depth;
        if ((iNext != 0) && (iDepth == 0))
        {
            // do space repeat for every non-terminate state
            for (iSpace = SPACE_MIN; iSpace <= SPACE_MAX; ++iSpace)
            {
                if (pTable->state[iNext].link[iSpace] == 0)
                {
                    pTable->state[iNext].link[iSpace] = static_cast<uint16_t>(iDeref);
                }
            }
        }
    }
}

uint16_t _ProfanCheck2(const ProfanRef *pRef, uint16_t uDeref, bool bDeny, const char *pWord, uint16_t uIdx, uint16_t uCnt, bool *bSeen)
{
    TableT *pTable = bDeny ? pRef->pDeny: pRef->pAllow;
    // Check if we matched a rule
    if (pTable->deref[uDeref].depth > 0)
    {
        // First time finding a substring for current rule
        if (uCnt == 0)
        {
            printf("Substrings of %s\n", pWord);
        }

        // Don't print rule if we saw the same rule already
        int32_t iIdx = pTable->deref[uDeref].word;
        if (bSeen[iIdx])
        {
            return uCnt;
        }
        bSeen[iIdx] = true;

        printf("    %s\n", &pRef->pWords[iIdx]);
        ++uCnt;
        return uCnt;
    }

    if (pWord[uIdx] == '\0')
    {
        return uCnt;
    }
    
    uint16_t uState = pTable->deref[uDeref].next;
    uint16_t uCh = bDeny ? pRef->Conv.aXlat[uIdx].uChDeny : pRef->Conv.aXlat[uIdx].uChAllow;
    
    // Handle repeated characters in the substring rule.
    if (uState != 0)
    {
        uint16_t uCh2 = bDeny ? pRef->Conv.aXlat[uIdx-1].uChDeny : pRef->Conv.aXlat[uIdx-1].uChAllow;
        uint16_t uDeref2 = pTable->state[uState].link[uCh2];
        if (uDeref2 != uDeref)
        {
            uCnt = _ProfanCheck2(pRef, uDeref2, bDeny, pWord, uIdx, uCnt, bSeen);
        }
    }
    
    // Continue parsing the word only if we have a valid deref
    uint16_t uNextDeref = pTable->state[uState].link[uCh];

    if (uNextDeref == 0)
    {
        return uCnt;
    }

    return _ProfanCheck2(pRef, uNextDeref, bDeny, pWord, uIdx+1, uCnt, bSeen);
}

void ProfanCheck(ProfanRef *pRef)
{
    if (pRef == nullptr)
    {
        return;
    }

    uint16_t uIdx = 0, uCur = 0, uCnt = 0;
    char s[256];
    char *pWords = pRef->pWords;
    const char *pBeg, *pEnd;
    bool bDeny = true;
    bool bSeen[STATE_MAX];

    // Iterate through all the rules
    while (uCur < pRef->uNumWords)
    {
        // Copy word
        pBeg = pWords;

        // Find the start of the word
        for (; *pBeg != '\0' && (*pBeg <= ' ' || *pBeg == '*'); ++pBeg)
            ;

        uIdx = static_cast<uint16_t>(pBeg - pWords);

        if (*pBeg == '\0')
        {
            pWords += uIdx + 1;
            uCur++;
            continue;
        }

        // Find the end of the word
        pEnd = pBeg;
        while (*pEnd != '\0' && *pEnd >= ' ' && *pEnd != '.' && *pEnd != '*' && *pEnd != '^' && *pEnd != '#')
        {
            s[uIdx++] = *pEnd++;
        }
        s[uIdx] = '\0';

        // Find the actual end of the rule
        for (; *pEnd != '\0' && (*pEnd <= ' ' || *pEnd == '*' || *pEnd == '^' || *pEnd == '#'); ++pEnd)
        {
            if (*pEnd == '.')
            {
                bDeny = false;
            }
        }

        if (s[0] == '\0')
        {
            pWords += (pEnd - pWords) + 1;
            uCur++;
            continue;
        }

        memset(bSeen, 0, sizeof(bSeen));

        // Translate the rule
        pRef->Conv.pText = (const unsigned char *) s;
        _ProfanXlat(pRef, &pRef->Conv);

        // Start from uIdx = 1 because we want to find all rules that
        // are substrings of this rule. No need to start from uIdx = 0
        // because then we will only match the current rule.
        for (uIdx = 1; s[uIdx] != '\0'; ++uIdx)
        {
            uCnt = _ProfanCheck2(pRef, 0, bDeny, pWords, uIdx, uCnt, bSeen);
        }

        // Increment for next iteration
        uCnt = 0;
        pWords += uIdx + 1;
        uCur++;
    }
}

// Parse the input word list into the given table
static int32_t _ProfanParse(ProfanRef *pRef, TableT *pTree, const char *pList, int32_t bDeny)
{
    int32_t flags;
    int32_t count = 0;
    unsigned char word[256];
    const unsigned char *s, *t;
    uint16_t uCh;
    unsigned char *u;
    unsigned char uLink;
    const char *pWord;
    int32_t iWord;
    TableT *pNew;

    // extract the word list
    for (s = (const unsigned char *)pList; *s != 0;)
    {
        flags = 0;
        pWord = (const char *)s;

        // skip leading whitespace
        if (*s <= ' ')
        {
            ++s;
            continue;
        }

        // skip comment or control lines
        if ((*s == '#') || (*s == '~'))
        {
            while (*s >= ' ')
                ++s;
            continue;
        }

        // check for leading wildcard
        if (*s == '*')
        {
            flags |= FLAG_EXLFT;
            ++s;
        }

        // locate end of word
        for (t = s; (*s >= ' ') && (*s != '*') && (*s != '.') && (*s != '^'); ++s)
            ;

        // ignore if too long
        if (s-t > (int32_t)sizeof(word)-1)
            continue;

        // copy the word
        for (u = word; t < s; u++)
        {
            t = _TableSymbol(t, &uCh);
            if (t == nullptr)
                return (-1);

            uLink = pTree->xlat[uCh];
            if (uLink == SPACE_MAX)
            {
                if (pTree->xindex+1 >= SPACE_MAX)
                    return (-1);

                pTree->xlat[uCh] = static_cast<unsigned char>(pTree->xindex);
                pTree->xindex += 1;
            }
            *u = pTree->xlat[uCh];
        }
        *u = 0;

        // check for trailing wildcard
        if (*s == '*')
        {
            flags |= FLAG_EXRGT;
            ++s;
        }

        // check for endof marker
        if (*s == '^')
        {
            flags |= FLAG_ENDOF;
            ++s;
        }

        if (bDeny)
        {
            // skip exemption words in this pass
            if (*s == '.')
            {
                ++s;
                continue;
            }
        }
        else
        {
            // make sure valid word indicator present
            if (*s != '.')
            {
                continue;
            }
            ++s;
        }

        iWord = pRef->iNextWord;
        strncpy(&pRef->pWords[pRef->iNextWord], pWord, ((const char *)s) - pWord);
        pRef->iNextWord += static_cast<int32_t>(((const char *)s) - pWord);
        pRef->pWords[pRef->iNextWord++] = '\0';
        pRef->uNumWords++;

        // add to table
        if (_TableWord(pTree, word, flags, iWord) < 0)
        {
            return (-1);
        }
        ++count;
    }

    // release any old table
    if (bDeny)
        _TableFree(pRef->pDeny);
    else
        _TableFree(pRef->pAllow);

    // create a optimal sized table
    pNew = _TableDupl(pTree);
    if (bDeny)
    {
        // do repeated character processing in new table
        _TableRepeat(pNew, 0);
        // handle character substitution
        _TableSubst(pNew, nullptr);
    }
    // recurse base table and use to add substrings
    _TableRecurse(pNew, pTree, 0);

    // do single character loopback
    _TableLoopback(pNew);
    if (bDeny)
    {
        // ignore spaces in words
        _TableSpaces(pNew);
    }
    // BLAZE_FREE original base table
    _TableFree(pTree);

    if (bDeny)
        pRef->pDeny = pNew;
    else
        pRef->pAllow = pNew;

    return (count);
}

static int32_t _SaveTable(TableT *pTable, int32_t iFd)
{
    int32_t iVal;
    int16_t iVal16;
    uint32_t uIdx;
    int32_t iState;
    int32_t iDeref;
    StateT *pState;
    DerefT *pDeref;

    // Write the state and deref table sizes
    iVal = htonl(pTable->scount);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    iVal = htonl(pTable->dcount);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);

    // Write character translations
    iVal = htonl(pTable->xindex);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    iVal = 0;
    for(uIdx = 0; uIdx < EAArrayCount(pTable->xlat); uIdx++)
    {
        if (pTable->xlat[uIdx] != SPACE_MAX)
            iVal++;
    }
    iVal = htonl(iVal);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    for(uIdx = 0; uIdx < EAArrayCount(pTable->xlat); uIdx++)
    {
        if (pTable->xlat[uIdx] != SPACE_MAX)
        {
            iVal16 = htons(static_cast<u_short>(uIdx));
            if (write(iFd, &iVal16, sizeof(iVal16)) != sizeof(iVal16))
                return (-1);
            if (write(iFd, &pTable->xlat[uIdx], sizeof(pTable->xlat[uIdx])) != sizeof(pTable->xlat[uIdx]))
                return (-1);
        }
    }
    iVal = htonl(sizeof(pTable->term));
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    if (write(iFd, pTable->term, sizeof(pTable->term)) != sizeof(pTable->term))
        return (-1);

    // Write the state table
    for(iState = 0; iState < pTable->scount; iState++)
    {
        pState = &pTable->state[iState];
        for(uIdx = 0; uIdx < (uint32_t)pTable->xindex; uIdx++)
        {
            iVal16 = htons(pState->link[uIdx]);
            if (write(iFd, &iVal16, sizeof(iVal16)) != sizeof(iVal16))
                return (-1);
        }
    }

    // Write the deref table
    for(iDeref = 0; iDeref < pTable->dcount; iDeref++)
    {
        pDeref = &pTable->deref[iDeref];
        iVal16 = htons(pDeref->next);
        if (write(iFd, &iVal16, sizeof(iVal16)) != sizeof(iVal16))
            return (-1);
        if (write(iFd, &pDeref->flags, sizeof(pDeref->flags)) != sizeof(pDeref->flags))
            return (-1);
        if (write(iFd, &pDeref->depth, sizeof(pDeref->depth)) != sizeof(pDeref->depth))
            return (-1);
        iVal = htonl(pDeref->word);
        if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
            return (-1);
    }
    return (0);
}

static TableT *_LoadTable(int32_t iFd)
{
    int32_t iVal;
    int16_t iVal16;
    int32_t iIdx;
    int32_t iState;
    int32_t iDeref;
    StateT *pState;
    DerefT *pDeref;
    uint16_t uXlatIdx;
    unsigned char uCh;

    int32_t iSCount;
    int32_t iDCount;
    TableT *pTable;

    // Read the state and deref table sizes
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (nullptr);
    iSCount = ntohl(iVal);
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (nullptr);
    iDCount = ntohl(iVal);
    
    pTable = _TableAlloc(iSCount, iDCount);
    pTable->scount = iSCount;
    pTable->dcount = iDCount;

    // Read character translations
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
    {
        _TableFree(pTable);
        return (nullptr);
    }
    pTable->xindex = ntohl(iVal);
    if (pTable->xindex >= SPACE_MAX)
    {
        _TableFree(pTable);
        return (nullptr);
    }
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
    {
        _TableFree(pTable);
        return (nullptr);
    }
    iVal = ntohl(iVal);
    memset(pTable->xlat, SPACE_MAX, sizeof(pTable->xlat));
    for(iIdx = 0; iIdx < iVal; iIdx++)
    {
        if (read(iFd, &uXlatIdx, sizeof(uXlatIdx)) != sizeof(uXlatIdx))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        uXlatIdx = ntohs(uXlatIdx);
        if (read(iFd, &uCh, sizeof(uCh)) != sizeof(uCh))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        pTable->xlat[uXlatIdx] = uCh;
    }

    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
    {
        _TableFree(pTable);
        return (nullptr);
    }
    iVal = ntohl(iVal);
    if (read(iFd, pTable->term, iVal) != iVal)
    {
        _TableFree(pTable);
        return (nullptr);
    }

    // Read the state table
    for(iState = 0; iState < pTable->scount; iState++)
    {
        pState = &pTable->state[iState];
        for(iIdx = 0; iIdx < pTable->xindex; iIdx++)
        {
            if (read(iFd, &iVal16, sizeof(iVal16)) != sizeof(iVal16))
            {
                _TableFree(pTable);
                return (nullptr);
            }
            pState->link[iIdx] = ntohs(iVal16);
        }
    }

    // Read the deref table
    for(iDeref = 0; iDeref < pTable->dcount; iDeref++)
    {
        pDeref = &pTable->deref[iDeref];
        if (read(iFd, &iVal16, sizeof(iVal16)) != sizeof(iVal16))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        pDeref->next = ntohs(iVal16);

        if (read(iFd, &pDeref->flags, sizeof(pDeref->flags)) != sizeof(pDeref->flags))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        if (read(iFd, &pDeref->depth, sizeof(pDeref->depth)) != sizeof(pDeref->depth))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        {
            _TableFree(pTable);
            return (nullptr);
        }
        pDeref->word = ntohl(iVal);
    }

    return (pTable);
}


/////////////////////////////////////////////////////////////////////////////////////


// create new profanity filter
ProfanRef *ProfanCreate()
{
    ProfanRef *pState;

    // allocate local state
    pState = (ProfanRef *)BLAZE_ALLOC(sizeof(*pState));
    memset(pState, 0, sizeof(*pState));

    // setup default replacement
    ProfanSubst(pState, "*");

    // return state to caller
    return(pState);
}


// destroy the profanity filter
void ProfanDestroy(ProfanRef *pState)
{
    if (pState == nullptr)
        return;

    // clear the table
    _TableFree(pState->pDeny);
    _TableFree(pState->pAllow);
    if(pState->pWords != nullptr)
        BLAZE_FREE(pState->pWords);
    // kill module state
    BLAZE_FREE(pState);
}


// let user set their own substiution table
void ProfanSubst(ProfanRef *pState, const char *pReplList)
{
    if (pState == nullptr || pReplList == nullptr)
        return;

    int32_t iIndex;
    const unsigned char *pRepl = (const unsigned char *)pReplList;

    // copy over the table
    for (iIndex = 0; (*pRepl >= ' ') && (iIndex < (int32_t)sizeof(pState->strRepl)-1); ++pRepl)
    {
        if (*pRepl != ' ')
        {
            pState->strRepl[iIndex++] = *pRepl;
        }
    }

    // save length
    pState->iReplPos = 0;
    pState->iReplLen = iIndex;
}

#ifdef PROFANITY_FILTER_DEBUG
static void dumpDotFile(ProfanRef* ref, TableT* table, const char* filename)
{
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "digraph {\n");
    int32_t iChar;
    int32_t iDeref;

    fprintf(fp, "    \"start\" [label=\"start\"];\n");
    for (iChar = 0; iChar < table->xindex; ++iChar)
    {
        char ch = '*';
        for(int i = 0; i < 0xffff; i++)
        {
            if (table->xlat[i] == iChar)
            {
                ch = i;
                break;
            }
        }
        if (ch == 0)
            ch = '*';

        if (table->state[0].link[iChar] != 0)
        {

            fprintf(fp, "    \"start\" -> \"%d\" [label=\"%c\"];\n",
                    table->state[0].link[iChar],
                    ch);
        }
    }

    for(iDeref = 0; iDeref < table->dcount; ++iDeref)
    {
        if (table->deref[iDeref].depth > 0)
        {
            fprintf(fp, "    \"%d\" [label=\"%s\"];\n", iDeref,
                    &ref->pWords[table->deref[iDeref].word]);
        }
        if (table->deref[iDeref].next != 0)
        {
            for (iChar = 0; iChar < table->xindex; ++iChar)
            {
                char ch = '*';
                for(int i = 0; i < 0xffff; i++)
                {
                    if (table->xlat[i] == iChar)
                    {
                        ch = i;
                        break;
                    }
                }
                if (ch == 0)
                    ch = '*';

                if (table->state[table->deref[iDeref].next].link[iChar] != 0)
                {
                    fprintf(fp, "    \"%d\" -> \"%d\" [label=\"%c\"];\n",
                            iDeref,
                            table->state[table->deref[iDeref].next].link[iChar],
                            ch);
                }
            }
        }
    }

    fprintf(fp, "}\n");
    fclose(fp);
}
#endif // PROFANITY_FILTER_DEBUG

// import a profanity filter
int32_t ProfanImport(ProfanRef *ref, const char *list)
{
    int32_t count = 0;
    uint16_t uCh0;
    uint16_t uCh1;
    const unsigned char *s;
    TableT *pTree;
    struct
    {
        uint16_t uCh;
        char bTerm;
    } term[MAX_EQUIV + GTERM_SIZE];
    uint32_t uTermIdx = 0;
    uint32_t uIdx;

    // allow default list
    if (list == nullptr)
    {
        list = g_default;
    }

    if (ref->pWords != nullptr)
        BLAZE_FREE(ref->pWords);
    ref->pWords = (char *)BLAZE_ALLOC(strlen(list));
    ref->iNextWord = 0;

    // look for custom replacement pattern
    s = (const unsigned char *)strstr(list, "~repl:");
    if (s != nullptr)
    {
        ProfanSubst(ref, (const char *)(s+6));
    }

    // create the initial tree
    pTree = _TableAlloc(STATE_MAX, FINAL_MAX);

    // Setup the initial word termination table
    for(uTermIdx = 0; uTermIdx < EAArrayCount(g_term); uTermIdx++)
    {
        term[uTermIdx].uCh = static_cast<uint16_t>(uTermIdx);
        term[uTermIdx].bTerm = g_term[uTermIdx];
    }

    // look for additional equiv tables
    s = (const unsigned char *)strstr(list, "~equiv:");
    if (s == nullptr)
        s = g_equiv;

    // copy over the equiv data
    for (; s != nullptr; s = (const unsigned char *) strstr((char *)s, "~equiv:"))
    {
        for (s += 7; *s >= ' ';)
        {
            if ((s = _TableSymbol(s, &uCh0)) == nullptr || (s = _TableSymbol(s, &uCh1)) == nullptr)
            {
                BLAZE_FREE(ref->pWords);
                ref->pWords = nullptr;
                _TableFree(pTree);
                return (-1);
            }
            if ((uCh0 > ' ') && (uCh1 > ' '))
            {
                if (!_TableEquiv(pTree, uCh0, uCh1))
                {
                    BLAZE_FREE(ref->pWords);
                    ref->pWords = nullptr;
                    _TableFree(pTree);
                    return (-1);
                }

                // Set the equivalent character to have the same termination state
                // as the base character
                term[uTermIdx].uCh = uCh1;
                term[uTermIdx].bTerm = 1;
                for(uIdx = 0; uIdx < uTermIdx; uIdx++)
                {
                    if (term[uIdx].uCh == uCh0)
                    {
                        term[uTermIdx].bTerm = term[uIdx].bTerm;
                        break;
                    }
                }
                uTermIdx++;
            }
        }
    }

    // extract the word list
    // if successful (count is nonnegative), this frees the memory allocated for pTree
    count = _ProfanParse(ref, pTree, list, true);

    if (count < 0)
    {
        BLAZE_FREE(ref->pWords);
        ref->pWords = nullptr;
        _TableFree(pTree);
        return (-1);
    }

#ifdef PROFANITY_FILTER_DEBUG
    printf("Deny tree:\n");
    _DebugPrintTableLimits(*pTree);
#endif // PROFANITY_FILTER_DEBUG

    // Build the termination table
    memset(ref->pDeny->term, 0, sizeof(ref->pDeny->term));
    for(uIdx = 0; uIdx < uTermIdx; uIdx++)
        ref->pDeny->term[ref->pDeny->xlat[term[uIdx].uCh]] = term[uIdx].bTerm;

    // create the allow tree
    pTree = _TableAlloc(STATE_MAX, FINAL_MAX);

    // Setup the initial word termination table
    for(uTermIdx = 0; uTermIdx < EAArrayCount(g_term); uTermIdx++)
    {
        term[uTermIdx].uCh = static_cast<uint16_t>(uTermIdx);
        term[uTermIdx].bTerm = g_term[uTermIdx];
    }

    // parse exemption list
    // if successful (_ProfanParse returns a nonnegative int), this frees the memory allocated for pTree
    if (_ProfanParse(ref, pTree, list, false) < 0)
    {
        BLAZE_FREE(ref->pWords);
        ref->pWords = nullptr;
        _TableFree(pTree);
        return (-1);
    }

#ifdef PROFANITY_FILTER_DEBUG
    printf("Allow tree:\n");
    _DebugPrintTableLimits(*pTree);
#endif // PROFANITY_FILTER_DEBUG

    // Build the termination table
    memset(ref->pAllow->term, 0, sizeof(ref->pAllow->term));
    for(uIdx = 0; uIdx < uTermIdx; uIdx++)
        ref->pAllow->term[ref->pAllow->xlat[term[uIdx].uCh]] = term[uIdx].bTerm;

#ifdef PROFANITY_FILTER_DEBUG
    dumpDotFile(ref, ref->pDeny, "deny.dot");
    dumpDotFile(ref, ref->pAllow, "allow.dot");
#endif // PROFANITY_FILTER_DEBUG

    // return number of words in filter
    return(count);
}

uint8_t _ConvGetChar(ConvT *pConv, int32_t iIdx, bool bDeny)
{
    return bDeny ? pConv->aXlat[iIdx].uChDeny : pConv->aXlat[iIdx].uChAllow;
}

void _ProfanMapRule(ProfanRef *pState, int32_t iDepth, int32_t iCur, int32_t iLink, int32_t iIdx, int32_t &iBeg, int32_t &iEnd, int32_t &iFlags, bool bDeny)
{
    TableT *pTable = (bDeny) ? pState->pDeny : pState->pAllow;
    // find potential match start
    for (iBeg = iIdx; (iBeg > 0) && (iDepth > 0); --iBeg)
        iDepth -= bDeny ? pState->Conv.aXlat[iBeg-1].uAddDeny : pState->Conv.aXlat[iBeg-1].uAddAllow;

    // see if we need to move forward to avoid repeated character problem
    if ((iBeg > 0) && (!pTable->term[_ConvGetChar(&pState->Conv, iBeg-1, bDeny)]))
    {
        int32_t iBeg2;
        unsigned char uLetter = _ConvGetChar(&pState->Conv, iBeg, bDeny);

        // skip past repeated letter
        for (iBeg2 = iBeg; _ConvGetChar(&pState->Conv, iBeg2+1, bDeny) == uLetter; ++iBeg2)
            ;
        // skip past spaces
        while ((iBeg2 < iIdx) && (pTable->term[_ConvGetChar(&pState->Conv, iBeg2+1, bDeny)]))
            ++iBeg2;
        // if we found the same character again, make the new word start
        if ((iBeg2 < iIdx) && _ConvGetChar(&pState->Conv, iBeg2+1, bDeny) == uLetter)
            iBeg = iBeg2+1;
    }

    // handle final state repeat
    for (iEnd = iIdx; pTable->state[iCur].link[_ConvGetChar(&pState->Conv, iEnd, bDeny)] == iLink; ++iEnd)
        ;

    // get word flags
    iFlags = pTable->deref[iLink].flags;
    if ((iBeg == 0) || (pTable->term[_ConvGetChar(&pState->Conv, iBeg-1, bDeny)]))
    {
        iFlags |= FLAG_SPLFT;
    }
    if (pTable->term[_ConvGetChar(&pState->Conv, iEnd, bDeny)])
    {
        iFlags |= FLAG_SPRGT;
    }

    // check for endof condition
    if (iFlags & FLAG_ENDOF)
    {
        if ((iFlags & (FLAG_SPLFT+FLAG_SPRGT)) == FLAG_SPLFT)
        {
            iFlags |= FLAG_EXRGT;
        }
        if ((iFlags & (FLAG_SPLFT+FLAG_SPRGT)) == FLAG_SPRGT)
        {
            iFlags |= FLAG_EXLFT;
        }
    }

    // extend to left is desired
    if (iFlags & FLAG_EXLFT)
    {
        while ((iBeg > 0) && !pTable->term[_ConvGetChar(&pState->Conv, iBeg-1, bDeny)])
            --iBeg;
    }

    // extend to right if needed
    if (iFlags & FLAG_EXRGT)
    {
        while (!pTable->term[_ConvGetChar(&pState->Conv, iEnd, bDeny)])
            ++iEnd;
    }
}

/*
 * Logic determines if we match a rule completely in the input.
 *
 * Returns true if there is a space to the left or if we have a wildcard 
 * on the left of the rule AND if there is a space to the right or if we 
 * have a wildcard on the right of the rule.
 */
bool _IsCompleteMatch(int32_t iFlags)
{
    return (iFlags & (FLAG_SPLFT + FLAG_EXLFT)) && (iFlags & (FLAG_SPRGT + FLAG_EXRGT));
}

/*
 * Run the profanity filter over the UTF-8 encoded 'pText' and update it
 * in place where necessary to remove/hide any profanity.
 *
 * The profanity filter was written with ASCII in mind and is optimised
 * for that case.  Since 'pText' is actually UTF-8 encoded then right
 * now a *very* simplistic approach is taken to dealing with non-ASCII
 * characters: any 8-bit character is looked up as a ' ' in the translation
 * tables.  For the most part words using two or more byte UTF-8 charactes
 * survive the profanity filtering intact and will display correctly 
 * on the user's screen.  However, it does mean that the user can effectively 
 * bypass the profanity filter by adding an accent to one or more letters in 
 * a profane word.  The next phase is to catch that case by doing
 * UTF-8 -> ASCII conversion i.e 'n' + '~' is still treated as 'n'.
 */
int32_t ProfanConv(ProfanRef *pState, char *p, bool justCheckIsProfane)
{
    int32_t iCurDeny;
    int32_t iCurAllow;
    int32_t iIdx;
    int32_t iValidIdx;
    int32_t iBeg;
    int32_t iBegText;
    int32_t iEnd;
    int32_t iEndText;
    int32_t iCnt = 0;
    int32_t iLinkDeny;
    int32_t iLinkAllow;
    int32_t iValidLink;
    int32_t iDepthDeny;
    int32_t iDepthAllow;
    int32_t iFlags;
    int32_t iXlatLen;
    int32_t iSrcLen;
    unsigned char *pText = (unsigned char*)p;
    unsigned char uChDeny;
    unsigned char uChAllow;
    bool bResetDeny;
    
    // make sure we have a ref
    if (pState == nullptr)
        return(0);

    // handle empty line
    if ((pText == nullptr) || (pText[0] == 0))
        return(0);

    // make sure the state machine is built
    if ((pState->pDeny == nullptr) || (pState->pAllow == nullptr))
        return(0);

    pState->iLastMatchedRule = -1;

    // Translate the first set of input
    pState->Conv.pText = pText;
    _ProfanXlat(pState, &pState->Conv);
    // Number of unicode chars *including* null terminator
    iXlatLen = (int32_t) pState->Conv.uLen;
    // Number of source bytes *not* including null terminator
    iSrcLen = (int32_t) pState->Conv.uSrcLen;

    // parse the line
    for (iIdx = 0, iCurDeny = iCurAllow = 0; iIdx < iXlatLen;)
    {
        bResetDeny = false;
        uChDeny = pState->Conv.aXlat[iIdx].uChDeny;
        uChAllow = pState->Conv.aXlat[iIdx].uChAllow;
        if (uChDeny == 0)
            break;
        // lookup next state
        iLinkDeny = pState->pDeny->state[iCurDeny].link[uChDeny];
        iLinkAllow = pState->pAllow->state[iCurAllow].link[uChAllow];
        iDepthDeny = pState->pDeny->deref[iLinkDeny].depth;
        iDepthAllow = pState->pAllow->deref[iLinkAllow].depth;
        pState->Conv.aXlat[iIdx].uAddDeny =
            ((iCurDeny == pState->pDeny->deref[iLinkDeny].next && pState->pDeny->deref[iLinkDeny].depth == 0) ? 0 : 1);
        pState->Conv.aXlat[iIdx].uAddAllow =
            ((iCurAllow == pState->pAllow->deref[iLinkAllow].next && pState->pAllow->deref[iLinkAllow].depth == 0) ? 0 : 1);
        iIdx++;

#ifdef PROFANITY_FILTER_DEBUG
        printf("iLinkDeny=%d, iLinkAllow=%d, iDepthDeny=%d, iDepthAllow=%d", iLinkDeny, iLinkAllow, iDepthDeny, iDepthAllow);
        printf(", iCurDeny=%d, iCurAllow=%d\n", iCurDeny, iCurAllow);
#endif // PROFANITY_FILTER_DEBUG

        // see if we got a hit
        if (iDepthAllow > 0)
        {
            _ProfanMapRule(pState, iDepthAllow, iCurAllow, iLinkAllow, iIdx, iBeg, iEnd, iFlags, false);
            if (_IsCompleteMatch(iFlags))
            {
                bResetDeny = true;
                // Note the rule used for the substitution
                pState->iLastMatchedRule = pState->pAllow->deref[iLinkAllow].word;                
            }
        }
        else if ((iDepthAllow == 0) && (iDepthDeny > 0))
        {
            _ProfanMapRule(pState, iDepthDeny, iCurDeny, iLinkDeny, iIdx, iBeg, iEnd, iFlags, true);

#ifdef PROFANITY_FILTER_DEBUG
            // show match data
            printf("beg=%d, idx=%d, end=%d, flags=%02x\n", iBeg, iIdx, iEnd, iFlags);
#endif // PROFANITY_FILTER_DEBUG

            // see if we should filter the word
            if (_IsCompleteMatch(iFlags))
            {
                // Continue traversing the allow FSM looking for a valid word
                iValidLink = -1;
                iValidIdx = iIdx;
                iCurAllow = pState->pAllow->deref[iLinkAllow].next;
                while (iCurAllow != 0)
                {
                    uChAllow = pState->Conv.aXlat[iValidIdx].uChAllow;
                    if (uChAllow == 0)
                    {
                        // end of buffer so no valid word found
                        break;
                    }
                    iLinkAllow = pState->pAllow->state[iCurAllow].link[uChAllow];
                    iDepthAllow = pState->pAllow->deref[iLinkAllow].depth;
                    iValidIdx++;

                    if (iDepthAllow > 0)
                    {
                        // next char is a terminator or extend right
                        if ((pState->pAllow->term[pState->Conv.aXlat[iValidIdx].uChAllow])
                                || (pState->pAllow->deref[iLinkAllow].flags & FLAG_EXRGT))
                        {
                            bResetDeny = true;
                            iValidLink = 1;
                            iIdx = iValidIdx;

                            // Note the rule used for the substitution
                            pState->iLastMatchedRule =
                                pState->pAllow->deref[iLinkAllow].word;

                            // Don't look for any more legit rules
                            break;
                        }
                    }

                    iCurAllow = pState->pAllow->deref[iLinkAllow].next;
                }

                // if no valid word was found
                if (iValidLink == -1)
                {
                    // Early out as soon as we find profanity if we're not trying to filter:
                    if (justCheckIsProfane)
                        return 1;


                    // dont remove any leading/trailing spaces
                    while (pState->pDeny->term[pState->Conv.aXlat[iBeg].uChDeny])
                        ++iBeg;
                    while (pState->pDeny->term[pState->Conv.aXlat[iEnd-1].uChDeny])
                        --iEnd;

#ifdef PROFANITY_FILTER_DEBUG
                    printf("subst: beg=%d, end=%d\n", iBeg, iEnd);
#endif // PROFANITY_FILTER_DEBUG


                    // do the substitution
                    iBegText = pState->Conv.aXlat[iBeg].uIdx;
                    iEndText = pState->Conv.aXlat[iEnd].uIdx;

                    // safety guard to ensure we only overwrite characters in the provided input
                    if (iBegText >= 0 && iEndText <= iSrcLen)
                    {
                        while (iBegText < iEndText)
                        {
                            pText[iBegText++] = pState->strRepl[(++(pState->iReplPos)) % pState->iReplLen];
                        }
                    }

                    // process final chars
                    while (iIdx < iEnd)
                    {
                        pState->Conv.aXlat[iIdx].uAddDeny = 0;
                        iIdx++;
                    }

                    // count the change
                    ++iCnt;

                    // Note the rule used for the substitution
                    pState->iLastMatchedRule = pState->pDeny->deref[iLinkDeny].word;

                    // Restart the deny table
                    bResetDeny = true;
                }
            }
        }

        // complete state transition
        if (bResetDeny)
            iCurDeny = 0;
        else
            iCurDeny = pState->pDeny->deref[iLinkDeny].next;
        iCurAllow = pState->pAllow->deref[iLinkAllow].next;
    }

    // conversion complete
    return(iCnt);
}


// chop leading/trailing/extra spaces from line
int32_t ProfanChop(ProfanRef *ref, char *line)
{
    if (ref == nullptr || line == nullptr)
        return 0;

    char *s, *t;
    int32_t count = 0;

    // skip leading spaces
    for (s = line; (*s != 0) && isspace(*s & 0xff); ++s)
        ++count;

    // filter remainder of string (no out of range data nor large runs of spaces
    for (t = line; *s != 0; ++s)
    {
        // dont allow more than 3 spaces in a row
        if ((*s == ' ') && (t > line+3) && (t[-1] == ' ') && (t[-2] == ' ') && (t[-3] == ' '))
        {
            ++count;
            continue;
        }
        // copy over the data
        *t++ = *s;
    }

    // remove trailing spaces
    for (; (t != line) && isspace(t[-1] & 0xff); --t)
        ++count;
    *t = 0;

    // return delete count
    return(count);
}

/*F********************************************************************************/
/*!
     \Function ProfanLastMatchedRule
 
     \Description
          Return the rule that was last used during a ProfanConv() call.  This
          is meant for debugging purposes.
  
     \Input pRef - Reference to profanity filter instance.
 
     \Output const char * - nullptr if not found; rule from input filter otherwise.
 
     \Version 02/15/2006 (doneill)
*/
/********************************************************************************F*/
const char *ProfanLastMatchedRule(ProfanRef *pRef)
{
    if (pRef == nullptr || pRef->iLastMatchedRule < 0)
        return (nullptr);

    return (&pRef->pWords[pRef->iLastMatchedRule]);
}

/*F********************************************************************************/
/*!
     \Function ProfanSave
 
     \Description
          Save the FSM and other necessary state to the given file descriptor.
  
     \Input pRef - Profanity filter reference to save
     \Input iFd - File descriptor to write to
 
     \Output int32_t - 0 on success; < 0 on error.
 
     \Version 03/03/2006 (doneill)
*/
/********************************************************************************F*/
int32_t ProfanSave(ProfanRef *pRef, int32_t iFd)
{
    if (pRef == nullptr)
        return (-1);

    int32_t iLen;
    int32_t iVal;

    iVal = htonl(PROFANITY_FILTER_SIGNATURE);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    iVal = htonl(PROFANITY_FILTER_VERSION);
    if (write(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);

    // Write replacement string
    iLen = htonl(pRef->iReplLen);
    if (write(iFd, &iLen, sizeof(iLen)) != sizeof(iLen))
        return (-1);
    if (write(iFd, pRef->strRepl, pRef->iReplLen) != pRef->iReplLen)
        return (-1);

    // Write word list
    iLen = htonl(pRef->iNextWord);
    if (write(iFd, &iLen, sizeof(iLen)) != sizeof(iLen))
        return (-1);
    if (write(iFd, pRef->pWords, pRef->iNextWord) != pRef->iNextWord)
        return (-1);

    // Write state tables
    if (_SaveTable(pRef->pDeny, iFd) < 0)
        return (-1);
    if (_SaveTable(pRef->pAllow, iFd) < 0)
        return (-1);
    return (0);
}

/*F********************************************************************************/
/*!
     \Function ProfanLoad
 
     \Description
          Load the profanity filter state from the given file descriptor.
  
     \Input pRef - Profanity filter reference to load into
     \Input iFd - File descriptor to load from.
 
     \Output int32_t - 0 on success; < 0 on error.
 
     \Version 03/03/2006 (doneill)
*/
/********************************************************************************F*/
int32_t ProfanLoad(ProfanRef *pRef, int32_t iFd)
{
    if (pRef == nullptr)
        return (-10);

    int32_t iVal;

    if (pRef->pWords != nullptr)
        BLAZE_FREE(pRef->pWords);
    _TableFree(pRef->pDeny);
    _TableFree(pRef->pAllow);

    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-1);
    iVal = ntohl(iVal);
    if ((uint32_t)iVal != PROFANITY_FILTER_SIGNATURE)
        return (-1);

    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-2);
    iVal = ntohl(iVal);
    if (iVal != PROFANITY_FILTER_VERSION)
        return (-2);

    // Read replacement string
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-3);
    pRef->iReplPos = 0;
    pRef->iReplLen = ntohl(iVal);
    if (read(iFd, pRef->strRepl, pRef->iReplLen) != pRef->iReplLen)
        return (-3);

    // Read word list
    if (read(iFd, &iVal, sizeof(iVal)) != sizeof(iVal))
        return (-4);
    iVal = ntohl(iVal);
    pRef->pWords = (char *)BLAZE_ALLOC(iVal);
    if (read(iFd, pRef->pWords, iVal) != iVal)
        return (-4);

    // Read state tables
    pRef->pDeny = _LoadTable(iFd);
    if (pRef->pDeny == nullptr)
        return (-5);

    pRef->pAllow = _LoadTable(iFd);
    if (pRef->pAllow == nullptr)
        return (-6);

    return (0);
}
