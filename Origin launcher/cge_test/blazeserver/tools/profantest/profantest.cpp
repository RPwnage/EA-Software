/*H*******************************************************************/
/*!
    \File profantest.c

    \Description
        A test tool for profan.c

    \Notes
        Lets you load up a filter and check if a word is filtered.
        Much the same as cursit/cursit.cpp except this runs under Unix
        (or Windows if you add it to the project).  This was originaly 
        created for FIFA 2004 hence the date below of the first version.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 08/29/2003 (sbevan) First Version
    \Version 1.1 06/05/2004 (sbevan) Converted to C++ (added a cast :-)
    \Version 2.0 08/08/2004 (sbevan) Now supports more ways to test words
*/
/*******************************************************************H*/

/*** Include files ***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "framework/lobby/filedata.h"
#include "framework/lobby/profan.h"
#include "framework/util/shared/blazestring.h"
namespace profantest
{

class FilterC
{
public:
    FilterC();
    bool Open(const char *pFileName);
    int32_t Filter(char *pText);
    void Check();
    int32_t Chop(char *pText);
    int32_t Conv(char *pText);
    void Close();
    const char *LastMatchedRule();
    ~FilterC();
private:
    ProfanRef* pFilter;
    FileDataRef* pLoader;
    void *pData;
};


FilterC::FilterC()
    : pFilter(0), pLoader(0), pData(0)
{
}


bool FilterC::Open(const char *pFileName)
{
    pFilter = ProfanCreate();
    pLoader = FileDataCreate();
    if (pFilter == 0 || pLoader == 0)
    {
        Close();
        return false;
    }
    pData = FileDataLoad(pLoader, pFileName, 0);
    ProfanImport(pFilter, static_cast<const char *>(pData));
    return true;
}


int32_t FilterC::Filter(char *pText)
{
    Chop(pText);
    return (Conv(pText));
}

int32_t FilterC::Chop(char *pText)
{
    return (ProfanChop(pFilter, pText));
}

int32_t FilterC::Conv(char *pText)
{
    return (ProfanConv(pFilter, pText));
}

void FilterC::Check()
{
    ProfanCheck(pFilter);
}

void FilterC::Close()
{
    if (pFilter != 0)
        ProfanDestroy(pFilter);
    if (pData != 0 && pLoader != 0)
        FileDataFree(pLoader, pData);
    if (pLoader != 0)
        FileDataDestroy(pLoader);
    pFilter = 0;
    pLoader = 0;
    pData = 0;
}
        
const char *FilterC::LastMatchedRule()
{
    return (ProfanLastMatchedRule(pFilter));
}

FilterC::~FilterC()
{
    Close();
}


namespace
{
    void _Usage(const char* pProgramName)
    {
        printf("%s: [ -f<filter-file-name> ] [ -w<word> ] [ -t[<test-file-name>] ] [ -h ] [ <word> ]\n\n", pProgramName);
        printf("  -f<filter-file-name> - use <filter-file-name> instead of default filt-text.txt\n");
        printf(" where\n");
        printf("  -r                   - check the filter file to see what rules are overshadowed\n");
        printf("  -w<word>             - check if <word> is profane\n");
        printf("  -t                   - check filter against filt-test.txt\n");
        printf("  -t<test-file-name>   - check filter against <test-file-name>\n");
        printf("  --                   - print words from stdin that are filtered\n");
        printf("  -+                   - print words from stdin that are not filtered\n");
        printf("  <word>               - check if <word> is profane\n\n");
        printf(" Examples\n");
        printf("  Check if 'knobby' is a profane word :-\n\n");
        printf("    $ profantest knobby\n\n");
        printf("  Check the default filter list filt-text.txt against the default test words in filt-test.txt :-\n\n");
        printf("    $ profantest\n");
    }

    bool _CheckWords(FilterC& Filter, const char *pProgramName, 
                     const char *pTestFileName)
    {
        bool bTestPassed = true;
        FILE *pTestWords;
        char *pEnd;
        uint32_t uLineNumber = 0;
        char strTestWord[80];
        char strExpectedResult[80];
        char strBuffer[80];

        pTestWords = fopen(pTestFileName, "r");
        if (pTestWords == 0)
        {
            fprintf(stderr, "%s: could not open test file %s\n",
                    pProgramName, pTestFileName);
            return EXIT_FAILURE;
        }
        for (;;)
        {
            fgets(strTestWord, sizeof strTestWord, pTestWords);
            if (feof(pTestWords))
                break;
            fgets(strExpectedResult, sizeof strExpectedResult, pTestWords);
            if (feof(pTestWords))
            {
                fprintf(stderr, "%s:%u missing the test result for %s",
                        pTestFileName, uLineNumber, strTestWord);
                bTestPassed = false;
                break;
            }
            uLineNumber += 2;
            pEnd = strrchr(strTestWord, '\n');
            if (pEnd != 0)
                *pEnd = '\0';
            pEnd = strrchr(strExpectedResult, '\n');
            if (pEnd != 0)
                *pEnd = '\0';
            blaze_strnzcpy(strBuffer, strTestWord, sizeof strBuffer);
            Filter.Filter(strBuffer);
            if (strcmp(strBuffer, strExpectedResult) != 0)
            {
                printf("%s:%u fail '%s' => '%s', should be '%s' (used rule '%s')\n", 
                       pTestFileName, uLineNumber, strTestWord,
                       strBuffer, strExpectedResult, Filter.LastMatchedRule());
                bTestPassed = false;
            }
        }
        fclose(pTestWords);
        if (bTestPassed)
        {
            printf("All tests passed!\n");
        }
        return bTestPassed;
    }
}


/*** Public functions ************************************************/

int profantest_main(int argc, char **argv)
{
    const char *pProgramName = argv[0];
    const char *pFilterFileName = "filt-text.txt";
    const char *pTestFileName = "filt-test.txt";
    char *pWord = 0;
    bool bCheckTestFile = true;
    bool bCheckRuleFile = false;
    enum { CHECK_NORMAL, CHECK_FILTERED, CHECK_ACCEPTED } eCheck;
    char strIn[256];
    char strOut[256];

    eCheck = CHECK_NORMAL;

    while (++argv, --argc != 0)
    {
        if (argv[0][0] != '-')
        {
            if (argc == 1)
            {
                pWord = &argv[0][0];
                bCheckTestFile = false;
            }
            else
            {
                fprintf(stderr, "%s: unknown option %s\n", pProgramName, argv[0]);
                return EXIT_FAILURE;
            }
        }
        else
        {
            switch (argv[0][1])
            {
            case 'r':
                bCheckRuleFile = true;
                bCheckTestFile = false;
                break;
            case 'f':
                pFilterFileName = &argv[0][2];
                break;
            case 'w':
                pWord = &argv[0][2];
                bCheckTestFile = false;
                break;
            case 't':
                if (argv[0][2] != '\0')
                    pTestFileName = &argv[0][2];
                break;
            case 'h':
                _Usage(pProgramName);
                return EXIT_SUCCESS;
            case '-':
                eCheck = CHECK_FILTERED;
                break;
            case '+':
                eCheck = CHECK_ACCEPTED;
                break;
            case '\0':
                fprintf(stderr, "%s: missing option\n", pProgramName);
                _Usage(pProgramName);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "%s: unknown argument '-%c'\n", 
                        pProgramName, argv[0][1]);
                _Usage(pProgramName);
                return EXIT_FAILURE;
            }
        }
    }
    FilterC Filter;
    if (!Filter.Open(pFilterFileName))
    {
        fprintf(stderr, "%s: could not open filter file %s\n", pProgramName,
                pFilterFileName);
        return EXIT_FAILURE;
    }
    if (eCheck != CHECK_NORMAL)
    {
        // Read from stdin
        while (!feof(stdin) && (fgets(strIn, sizeof(strIn), stdin) != 0))
        {
            Filter.Chop(strIn);
            strcpy(strOut, strIn);

            Filter.Conv(strOut);
            if (eCheck == CHECK_FILTERED)
            {
                if (strcmp(strIn, strOut) != 0)
                    printf("%s\n", strIn);
            }
            else
            {
                if (strcmp(strIn, strOut) == 0)
                    printf("%s\n", strIn);
            }
        }
    }
    else if (pWord != 0)
    {
        Filter.Filter(pWord);
        printf("%s", pWord);
        const char *pRule = Filter.LastMatchedRule();
        if (pRule != nullptr)
            printf(" (used rule '%s')", Filter.LastMatchedRule());
        printf("\n");
    }
    else if (bCheckTestFile)
    {
        return _CheckWords(Filter, pProgramName, pTestFileName)
            ? EXIT_SUCCESS 
            : EXIT_FAILURE;
    }
    else if (bCheckRuleFile)
    {
        Filter.Check();
    }
    return EXIT_SUCCESS;
}

}
