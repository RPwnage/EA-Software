/*H*************************************************************************************/
/*!
    \File localization.cpp

    \Description
         This module runs on Blaze to handle the localization of all dynamically
         created content which cannot be shipped on the game CD.
         See TDD_Blaze_Localization.doc for details.

    \Notes
    None.

    \Copyright
        (c) Electronic Arts. All Rights Reserved.

    \Version 1.0 08/27/2007 (danielcarter)  First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "framework/blaze.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/util/localization.h"
#include "framework/util/locales.h"
#include "framework/controller/controller.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if defined(_WIN32) || defined(WIN32)
    #define snprintf _snprintf
#endif

struct LocPhraseT
{
    char *phrase;
    struct LocPhraseT *nextregion;
};

struct LocLocaleT
{
    char region[3];
    char lang[3];
    uint32_t locality;
    int32_t index;
    struct LocLocaleT *next;
};

struct LocParamArgs
{
    const char** pParams;
    int32_t iCount;
    int32_t iNumParams;
};

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Localization

    The default constructor. Initialize and create new dictionary (derived from LocCreate()).

    \param[in]  pFilename - The filename of the csv file to be read in as the dictionary.
*/
/*************************************************************************************************/
Localization::Localization()
    : mLocale(nullptr),
      mDefaultLocaleIndex(0),
      mPhraseList(BlazeStlAllocator("Localization::mPhraseList"))
{
}

/*************************************************************************************************/
/*!
    \brief ~Localization

    The destructor. Destroy the dictionary created for this class (derived from LocDestroy()).
*/
/*************************************************************************************************/
Localization::~Localization(void)
{
    resetState();
} // Localization deconstructor


/*** Public Functions ******************************************************************/

/*************************************************************************************************/
/*!
    \brief localize

    Initialize and create new dictionary.

    \param[in]  char *pStrLookup   - String to localize.
    \param[in]  uint32_t uLocality - Locality to localize to.

    \return - char *               - The localized string. Original string is returned
                                     if there is an error.
*/
/*************************************************************************************************/
const char *Localization::localize(const char *pStrLookup, const uint32_t uLocality)
{
    LocPhraseT *currPhrase = nullptr;
    int32_t localeIdx = findLocaleIndex(uLocality);
    int32_t phraseLocale = 0;
    PhraseHash::const_iterator iter;

    // Find the string in the hash table
    iter = mPhraseList.find_as(pStrLookup);

    // no match found in hash table
    if (iter == mPhraseList.end())
    {
        return pStrLookup;
    }
    currPhrase = iter->second;

    // Once the phrase is found, scan to the locale index
    while (currPhrase != 0)
    {
        // this is the phrase we need
        if (phraseLocale == localeIdx)
            return currPhrase->phrase;

        // go to the next translation
        currPhrase = currPhrase->nextregion;
        phraseLocale++;
    }

    // Return the original phrase if there are no matches
    return pStrLookup;
} // localize()

/*************************************************************************************************/
/*!
    \brief localize

    Localize string using a va_list to store the parameters. (Derived from LocParam())

        
    \param[in]  char     *pStrLookup - String to localize.
    \param[in]  uint32_t  uLocality  - Locality to localize to.
    \param[in]  char     *pStrBuffer - Buffer for output of localized stFileDataRefring.
    \param[in]  int32_t  *uBufLen    - Length of output buffer.

    \return -   int32_t              - Length of string written to buffer. -1 if there is an error.
*/
/*************************************************************************************************/
int32_t Localization::localize(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer, uint32_t uBufLen, ...)
{
    va_list args;
    int32_t iReturnVal;

    va_start(args, uBufLen);
    iReturnVal = localizeParam(pStrLookup, uLocality, pStrBuffer, uBufLen, &Blaze::Localization::extractVA, &args);
    va_end(args);
    return iReturnVal;
} // localize()

int32_t Localization::localize(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer, uint32_t uBufLen, va_list* args)
{
    int32_t iReturnVal;

#ifdef EA_PLATFORM_WINDOWS
    iReturnVal = localizeParam(pStrLookup, uLocality, pStrBuffer, uBufLen, &Blaze::Localization::extractVA, args);
#else
    va_list tmpargs;
    va_copy(tmpargs, *args);
    iReturnVal = localizeParam(pStrLookup, uLocality, pStrBuffer, uBufLen, &Blaze::Localization::extractVA, tmpargs);
#endif
    return iReturnVal;
} // localize()

/*************************************************************************************************/
/*!
    \brief localizeChar()

    Localize using an array of strings

    \param[in]  char    *pStrLookup - String to localize.
    \param[in]  uint32_t uLocality  - Locality to localize to.
    \param[in]  char    *pStrBuffer - Buffer for output of localized string.
    \param[in]  int32_t *uBufLen    - Length of output buffer.
    \param[in]  char*[]  pArgs      - Array of string pointers
    \param[in]  int32_t  iNumParams - Number of parameters strings
    \param[out] 
        

    \return - int32_t               - Length of string written to buffer. -1 if there is an error.
*/
/*************************************************************************************************/
int32_t Localization::localizeChar(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer, uint32_t uBufLen, const char** pArgs, int32_t iNumParams)
{
    int32_t iReturnVal;
    LocParamArgs locParamArgs;
    locParamArgs.pParams = pArgs;
    locParamArgs.iNumParams = iNumParams;
    locParamArgs.iCount = 0;
    iReturnVal = localizeParam(pStrLookup, uLocality, pStrBuffer, uBufLen,
            &Blaze::Localization::extractArray, &locParamArgs);

    return iReturnVal;

} //localizeChar()

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

// Only prints the list at the provided address. Does not print all.
void Localization::printPhraseList(LocPhraseT *pPhraseList) const
{
    LocPhraseT *temp = pPhraseList;
    BLAZE_TRACE_LOG(Log::SYSTEM, "Phrase list: ");
    while(temp != 0)
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "" << temp->phrase);
        temp = temp->nextregion;
    }
} //printPhraseList()

/*************************************************************************************************/
/*!
    \brief destroyPhraseList

    Destroy a list of phrases.

    \param[in]  LocPhraseT * pPhraseList - Pointer to the list that needs to be deallocated.
*/
/*************************************************************************************************/
void Localization::destroyPhraseList(LocPhraseT *pPhraseList) const
{
    LocPhraseT *prevPtr;
    LocPhraseT *currPtr = pPhraseList;

    while (currPtr != 0)
    {
        prevPtr = currPtr;
        currPtr = currPtr->nextregion;
        delete[] prevPtr->phrase;
        delete prevPtr;
    }
} // destroyPhraseList()

/*************************************************************************************************/
/*!
    \brief findLocaleIndex

    Finds the index of the requested locale. Returns 0 if not found.

    \param[in]  LocLocaleT *pLocales   - Pointer to a locale list.
    \param[in]  uint32_t    uLocality  - Locality to find
        
    \return - int32_t                  - Index of the requested locale. 0 if locale does not exist.
*/
/*************************************************************************************************/
int32_t Localization::findLocaleIndex(const uint32_t uLocality)
{
    LocLocaleT *currLocale = mLocale;
    LocLocaleT *pLangMatch = nullptr;

    while (currLocale != 0)
    {
        if(LocaleTokenGetLanguage(currLocale->locality) == LocaleTokenGetLanguage(uLocality))
        {
            if (pLangMatch == nullptr)
            {
                // Save the first language-only match
                pLangMatch = currLocale;
            }

            if(LocaleTokenGetCountry(currLocale->locality) == LocaleTokenGetCountry(uLocality))
            {
                return currLocale->index;
            }
        }

        currLocale = currLocale->next;
    }

    if (pLangMatch != nullptr)
        return (pLangMatch->index);

    // Locale not found, return default locale (which would be 0 if default locale index is not defined in configs)
    return (mDefaultLocaleIndex);
} // findLocaleIndex()

/*************************************************************************************************/
/*!
    \brief addLocale
    
    Add a locale to the list of locales.

    \param[in]  char   *pWord   - String containing the locale to add.
*/
/*************************************************************************************************/
void Localization::addLocale(const char *pWord)
{
    int32_t localeIdx = 0;
    LocLocaleT *prevLocale = nullptr;
    LocLocaleT *currLocale = mLocale;

    // Scan to the end of the locale list
    while (currLocale != nullptr)
    {
        localeIdx++;
        prevLocale = currLocale;
        currLocale = currLocale->next;
    }

    currLocale = BLAZE_NEW LocLocaleT;

    if (mLocale == nullptr || prevLocale == nullptr)
        mLocale = currLocale;
    else
        prevLocale->next = currLocale;

    memset(currLocale, 0, sizeof(*currLocale));
    currLocale->index = localeIdx;
    currLocale->lang[0] = pWord[0];
    currLocale->lang[1] = pWord[1];
    currLocale->lang[2] = 0;
    currLocale->region[0] = pWord[3];
    currLocale->region[1] = pWord[4];
    currLocale->region[2] = 0;
    currLocale->locality = LocaleTokenCreate(LocaleTokenGetShortFromString(currLocale->lang),
        LocaleTokenGetShortFromString(currLocale->region));
    currLocale->next = 0;
} // addLocale()

/*************************************************************************************************/
/*!
    \brief addPhrase

    Add a phrase to the phrase dictionary.

    \param[in]  LocPhraseT *pLocPhrase - Pointer to the localization dictionary.
    \param[in]  char       *pWord      - String containing the phrase to add.
    \param[in]  int32_t     iWordSize  - Size of the localized string.
    \param[in]  int32_t     iNewLine   - 1 if this is the first entry for a phrase. 0 otherwise.
    
    \return -    Returns the beginning of the list which contains the added phrase. 
*/
/*************************************************************************************************/
LocPhraseT *Localization::addPhrase(LocPhraseT *pLocPhrase, const char *pWord, int32_t iWordSize, int32_t iNewLine) const
{
    int32_t strPtr;
    LocPhraseT *prevPhrase = nullptr;
    LocPhraseT *currPhrase;
    LocPhraseT *startPhrase;

    currPhrase = pLocPhrase;
    startPhrase = pLocPhrase;

    // Find scan to the end of the current phrase entry to add string for the new locale
    while (currPhrase != nullptr)
    {
        prevPhrase = currPhrase;
        currPhrase = currPhrase->nextregion;
    }

    currPhrase = BLAZE_NEW LocPhraseT;

    if (pLocPhrase == nullptr || prevPhrase == nullptr || iNewLine == 1)
    {
        startPhrase = currPhrase;

        // skip any preceding non-ASCII characters (e.g. byte order mark) from any key (i.e. beginning list entry)
        while ((*pWord & 0x7F) != *pWord && iWordSize > 0)
        {
            ++pWord;
            --iWordSize;
        }
    }
    else
    {
        prevPhrase->nextregion = currPhrase;
    }

    memset(currPhrase, 0, sizeof(*currPhrase));
    currPhrase->phrase = BLAZE_NEW_ARRAY(char, iWordSize+1);
    for (strPtr = 0; strPtr < iWordSize; strPtr++)
        currPhrase->phrase[strPtr] = pWord[strPtr];
    currPhrase->phrase[iWordSize] = 0;
    currPhrase->nextregion = 0;

    //return the beginning of the list
    return startPhrase;

} // addPhrase()

/*************************************************************************************************/
/*!
    \brief extractArray

    Extract the next element in an array for LocParam parameters

    \param[in]  void *pArgs     - Structure containing parameters data


    \return -  char *           - The next parameter in the array
*/
/*************************************************************************************************/
const char *Localization::extractArray(void *pArgs)
{
    // cast the parameters
    struct LocParamArgs *_pLocParamArgs = (struct LocParamArgs *)pArgs;
    
    // get the next string
    const char *pString = _pLocParamArgs->pParams[_pLocParamArgs->iCount];

    // check if there are anymore strings left
    // if not don't increment past the last string
    if(_pLocParamArgs->iCount < (_pLocParamArgs->iNumParams-1))
    {
        _pLocParamArgs->iCount++;
    }

    return pString;
} // extractArray()

/*************************************************************************************************/
/*!
    \brief extractVA
    
    Extract the next argument from a va_list

    \param[in]  void *pArgs     - Pointer to a va_list

    \return - char *            - The next argument in the list
*/
/*************************************************************************************************/
const char * Localization::extractVA(void *pArgs)
{
    va_list *args = (va_list*)pArgs;
    return va_arg(*args, const char *);
} // extractVA()

/*************************************************************************************************/
/*!
    \brief localizeParam

    Localize a string with parameters.

    \param[in]  char     *pStrLookup - String to localize.
    \param[in]  uint32_t  uLocality  - Locality to localize to.
    \param[in]  char     *pStrBuffer - Buffer for output of localized string.
    \param[in]  int32_t   uBufLen    - Length of output buffer.
    \param[in]  void     *pFunc      - Function pointer to get next string function
    \param[in]  void     *pArgs      - Arguments to the get next string function
    \param[out] char     *pStrBuffer - Buffer for output of localized string.

    \return - int32_t                - Length of string written to buffer. -1 if there is an error.
*/
/*************************************************************************************************/
int32_t Localization::localizeParam(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer, uint32_t uBufLen, const char *(*pFunc)(void *), void *pArgs)
{
    LocPhraseT *phrPtr = nullptr;
    int32_t localeIdx = findLocaleIndex(uLocality);
    int32_t numParams = 0;
    const char *strIdx;
    LocPhraseT *strParams = 0;
    LocPhraseT *prevParam = 0;
    LocPhraseT *currParam = 0;
    int32_t count = 0;
    PhraseHash::const_iterator iter;

    // Scan through the string to generate a list of the variable arguments
    for (strIdx = pStrLookup; *strIdx != 0; strIdx++)
    {
        // If we find a parameter tag in the string, read a parameter off the variable argument list
        if ((*strIdx == '<') && (*(strIdx+2) == '>') && (*(strIdx+1) >= '0') && (*(strIdx+1) <= '9'))
        {
            const char *charPtr = (*pFunc)(pArgs);

            // Read the argument into a structure
            if (*charPtr != 0)
            {
                uint32_t strPtr;
                uint32_t uCharPtrLen = static_cast<uint32_t>(strlen(charPtr));
                currParam = BLAZE_NEW LocPhraseT;
                memset(currParam, 0, sizeof(*currParam));
                currParam->phrase = BLAZE_NEW_ARRAY(char8_t, strlen(charPtr)+1);
                for (strPtr = 0; strPtr < uCharPtrLen; strPtr++)
                    currParam->phrase[strPtr] = charPtr[strPtr];
                currParam->phrase[strlen(charPtr)] = 0;

                //use nextregion instead of nextphrase
                //to eliminate it from the struct
                if (strParams == 0)
                    strParams = currParam;
                else if (prevParam != nullptr)
                    prevParam->nextregion = currParam;

                prevParam = currParam;
                numParams++;
            }
            strIdx += 2;
        }
    }

    // Find the phrase list for the string 
    iter = mPhraseList.find_as(pStrLookup);

    // no match found in hash table
    if (iter != mPhraseList.end())
    {
        phrPtr = iter->second;

        //find the localized string
        while (phrPtr != 0)
        {
            // this is the correct locale index
            if (count == localeIdx)
                break;
            phrPtr = phrPtr->nextregion;
            count++;
        }
    }

    // If the a localized version of the phrase isn't found, return original
    const char* phrase;
    if (phrPtr == 0)
        phrase = pStrLookup;
    else
        phrase = phrPtr->phrase;

    char *workingStr = pStrBuffer;
    *workingStr = '\0'; // always nullptr terminate
    for (strIdx = phrase; *strIdx != 0; strIdx++)
    {
        int32_t charsWritten;

        // Scan through the string until hitting a parameter tag
        if ((*strIdx == '<') && (*(strIdx+2) == '>') && (*(strIdx+1) >= '0') && (*(strIdx+1) <= '9'))
        {
            currParam = strParams;

            // Retrieve the parameter string from the created parameter structure
            count = 0;
            while (currParam != 0)
            {
                if (count == (*(strIdx+1)-'0'))
                    break;
                else
                    currParam = currParam->nextregion;

                count++;
            }

            // If the parameter is found, insert it in output string, else leave the tag placeholder
            if (currParam != 0)
                charsWritten = blaze_snzprintf(workingStr, uBufLen-(workingStr-pStrBuffer), "%s", currParam->phrase);
            else
                charsWritten = blaze_snzprintf(workingStr, uBufLen-(workingStr-pStrBuffer), "<%c>", *(strIdx+1));

            // If the write succeeded, increment number of chars written, else exit the localization
            if (charsWritten > 0)
                workingStr += charsWritten;
            else
            {
                destroyPhraseList(strParams);
                return -1;
            }
            strIdx += 2;
            continue;
        }

        // A tag was not found, so append character to the output string
        charsWritten = blaze_snzprintf(workingStr, uBufLen-(workingStr-pStrBuffer), "%c", *strIdx);
        if (charsWritten > 0)
            workingStr += charsWritten;
        else
        {
            destroyPhraseList(strParams);
            return -1;
        }
    }
    destroyPhraseList(strParams);
    // This cast moves int64 to int32. Will this cause problems?
    return static_cast<int32_t>(workingStr-pStrBuffer);
} // localizeParam()

void Localization::validateConfig(const char8_t* localizationData, uint32_t locStringMaxLength, ConfigureValidationErrors& validationErrors)
{
    if (localizationData[0] == '\0')
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Localization].validateConfig(): no localization information available.");
    }
}

bool Localization::configure(const char8_t* localizationData, const char8_t* defaultLocale)
{
    // Create the dictionary
    const char *s;
    int32_t firstline = 1;
    int32_t isNewLine = 1;
    LocPhraseT *pLocPhrase = nullptr;

    if (localizationData[0] == '\0')
    {
        return false;
    }

    char8_t *word = BLAZE_NEW_ARRAY(char8_t, gController->getFrameworkConfigTdf().getLocStringMaxLength());

    // Clear any previous state
    resetState();

    // Parse the localization contents
    for (s = localizationData; *s != 0;)
    {
        const char *t;
        char *u;
        int32_t quoted = 0;
        int32_t wordSize = 0;

        // Skip leading whitespace, control chars etc (ASCII/Unicode based encoding assumed here)
        if ((*s >= '\0') && (*s < ' '))
        {
            if (*s == '\n')
            {
                firstline = 0;
                isNewLine = 1;
            }
            ++s;
            continue;
        }

        // Check for leading quotes
        if (*s == '"')
        {
            quoted = 1;
            ++s;
        }

        // Locate end of word
        for (t = s;;++s)
        {
            // Stop reading a CSV entry on any ending conditions
            if (((quoted == 0) && (*s >= '\0') && (*s < ' ')) ||
                ((quoted == 0) && (*s == ',')) ||
                ((quoted == 1) && (*s == '"')))
            {
                // Accept two double quotes following each other
                if ((quoted == 1) && (*s == '"') && (*(s+1) == '"'))
                    ++s;
                else
                    break;
            }
        }

        // Ignore if too int32_t
        if (s-t > gController->getFrameworkConfigTdf().getLocStringMaxLength() -1)
            continue;

        // copy over the word
        for (u = word; t != s;)
        {
            // Convert two 'double quotes' to a single 'double quote'
            if ((quoted == 1) && (*t == '"') && (*(t+1) == '"'))
                t++;

            // Basic escape character support: 
            bool noEscape = true;
            if (*t == '\\')
            {
                noEscape = false;
                switch (*(t + 1))
                {
                case 't':   *u++ = '\t';    break;
                case 'n':   *u++ = '\n';    break;
                case 'r':   *u++ = '\r';    break;
                default:   noEscape = true; break;
                }

                if (noEscape == false)
                    t += 2;
            }
            
            if (noEscape == true)
            {
                *u++ = *t++;
            }
            wordSize++;
        }
        *u = 0;

        // Add localization regions to table
        if (firstline == 1)
        {
            addLocale(word);
        }
        else
        // Add phrases to the table
        {
            //if this is a new line and not the first line
            if(isNewLine && pLocPhrase != 0)
            {
                // add the phrase list to the hash table
                addPhraseToMap(pLocPhrase);
                pLocPhrase = nullptr;
            }

            // add the phrase to the phrase list
            pLocPhrase = addPhrase(pLocPhrase, word, wordSize, isNewLine);
            isNewLine = 0;
        }

        // check for trailing quote
        if ((quoted == 1) && (*s == '"'))
            ++s;

        // check for trailing comma
        if (*s == ',')
            ++s;
    }

    //if this is a new line and not the first line
    if(pLocPhrase != 0)
    {
        // add the phrase list to the hash table
        // mPhraseList[key] = value;
        addPhraseToMap(pLocPhrase);
    }

    // set our default locale index according to configs
    mDefaultLocaleIndex = 0; // reset default locale in case this is a reconfigure attempt
    const char8_t* lang = defaultLocale;
    const char8_t* country = nullptr;
    if (strlen(defaultLocale) > 2)
    {
        country = lang + 2;
    }
    uint32_t defaultLocaleIndex = LocaleTokenCreateFromStrings(lang, country);
    mDefaultLocaleIndex = findLocaleIndex(defaultLocaleIndex);
    if (mDefaultLocaleIndex == 0)
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[Localization].configure(): default localization is not set because \"" << defaultLocale << "\" does not match any locales specified in localization.csv");
    }

    delete[] word;

    return true;
}

/*!*********************************************************************************/
/*! \brief Inserts new phrase list of translations to internal map.
        If one already exists for the phrase, clean replace it.
    \param[in] pLocPhrase - contains the name of phrase to add.
************************************************************************************/
void Localization::addPhraseToMap(LocPhraseT *pLocPhrase)
{
    // check to see if there is already an object mapped to that phrase
    PhraseHash::iterator itr = mPhraseList.find_as(pLocPhrase->phrase);
    if(itr != mPhraseList.end())
    {
        LocPhraseT* existingElement = itr->second;
        if (existingElement != nullptr)
        {
            destroyPhraseList(existingElement);
        }
    }

    // having cleared the appropriate space, insert our phrase
    mPhraseList[pLocPhrase->phrase] = pLocPhrase;
}

void Localization::resetState()
{
    LocLocaleT *prevLocale;
    LocLocaleT *currLocale = mLocale;
    LocPhraseT *destroyPhrase = nullptr;
    PhraseHash::const_iterator iter = mPhraseList.begin();

    // Free mLocale memory
    while (currLocale != 0)
    {
        prevLocale = currLocale;
        currLocale = currLocale->next;
        delete prevLocale;
    }
    mLocale = nullptr;

    // Start at the beginning of the hash table
    while (iter != mPhraseList.end())
    {
        // Identify the location of the string to be destroyed.
        destroyPhrase = iter->second;

        // continue enumerating through the phrase lists
        while(destroyPhrase != nullptr)
        {
            // free this phrase list
            destroyPhraseList(destroyPhrase);

            // get the next phrase list in the hash table
            if (++iter == mPhraseList.end())
                destroyPhrase = nullptr;
            else
                destroyPhrase = iter->second;
        }
    }

    mPhraseList.clear();
}

} // Blaze

