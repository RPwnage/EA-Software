/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

/*** Include files *******************************************************************************/
#include "EASTL/string.h"
#include "EASTL/hash_map.h"
#include "eathread/eathread_storage.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{


typedef struct LocPhraseT LocPhraseT;
typedef struct LocLocaleT LocLocaleT;

class Localization
{
    NON_COPYABLE(Localization);

public:
    Localization();
    ~Localization();
    bool configure(const char8_t* localizationData, const char8_t* defaultLocale);
    static void validateConfig(const char8_t* localizationData, uint32_t locStringMaxLength, ConfigureValidationErrors& validationErrors);

    const char *localize(const char *pStrLookup, const uint32_t uLocality);
    int32_t localize(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer,
            uint32_t uBufLen, ...);
    int32_t localize(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer,
            uint32_t uBufLen, va_list* args);
    int32_t localizeChar(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer,
            uint32_t uBufLen, const char** pArgs, int32_t iNumParams);

private:
    LocLocaleT *mLocale;
    uint32_t mDefaultLocaleIndex;

    typedef eastl::hash_map<eastl::string, LocPhraseT*> PhraseHash;
    PhraseHash mPhraseList;

    void printPhraseList(LocPhraseT *pPhraseList) const;
    void addLocale(const char *pWord);
    LocPhraseT *addPhrase(LocPhraseT *pLocPhrase, const char *pWord, int32_t iWordSize,
            int32_t iNewLine) const;
    void addPhraseToMap(LocPhraseT *pLocPhrase);
    void destroyPhraseList(LocPhraseT *pPhraseList) const;
    int32_t findLocaleIndex(const uint32_t uLocality);
    int32_t localizeParam(const char *pStrLookup, const uint32_t uLocality, char *pStrBuffer,
            uint32_t uBufLen, const char *(*pFunc)(void *), void *pArgs);
    void resetState();
    static const char * extractArray(void *pArgs);
    static const char * extractVA(void *pArgs);
};

extern EA_THREAD_LOCAL Localization* gLocalization;

} // Blaze

#endif // LOCALIZATION_H
