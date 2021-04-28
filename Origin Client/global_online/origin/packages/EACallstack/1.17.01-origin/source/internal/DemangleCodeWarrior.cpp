/////////////////////////////////////////////////////////////////////////////
// DemangleCodeWarrior.cpp
//
// Copyright (c) 2008, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EACallstack/internal/DemangleCodeWarrior.h>


#if EACALLSTACK_CODEWARRIOR_DEMANGLE_ENABLED

#include <EAStdC/EAString.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAMemory.h>
#include EA_ASSERT_HEADER


namespace EA
{
namespace Callstack
{
namespace Demangle
{


struct CWOperator
{
    const char* pID;
    const char* pName;
};

const CWOperator gOperatorTable[] =
{
    { "ct",  "constructor"       }, // Followed by __ (two underscores)
    { "dt",  "destructor"        }, // Followed by __
    { "ami", "operator -="       }, // Followed by __
    { "mi",  "operator -"        }, //     "    "  "
    { "apl", "operator +="       },
    { "pl",  "operator +"        },
    { "as",  "operator ="        },
    { "eq",  "operator =="       },
    { "ml",  "operator *"        },
    { "co",  "operator ~"        },
    { "pp",  "operator ++"       },
    { "mm",  "operator --"       },
    { "rs",  "operator >>"       },
    { "ls",  "operator <<"       },
    { "lt",  "operator <"        },
    { "gt",  "operator >"        },
    { "rf",  "operator ->"       },
    { "dl",  "operator delete"   },
    { "dla", "operator delete[]" },
    { "nw",  "operator new"      },
    { "nwa", "operator new[]"    },
    { "cl",  "operator ()"       },
    { "vc",  "operator []"       },
    { "op",  "operator "         }  // Not followed by __
};


// Get the length of text that corresponds to the input <...> sequence.
// The length is the count of characters from pText (<) up to and including
// the matching > char. Nested template sequences are accounted for.
size_t GetTemplateLength(const char* pText, size_t textLength = (size_t)-1)
{
    size_t i, nestLevel;

    for(i = 1, nestLevel = 1; (i < textLength) && pText[i] && nestLevel; ++i)
    {
        if(pText[i] == '>')
            --nestLevel;
        else if(pText[i] == '<')
            ++nestLevel;
    }

    return i;
}


// Checks if the string points to the beginning of an operator name.
// An example of a symbol name which would have this is __dt__14ContentHandlerFv
// whereby pString points to dt__14ContentHandlerFv.
bool CheckForOperator(const char* pString, const char* pID, int* pnEaten)
{
    int nContiguousUnderscores = 0;
    int idx;

    for(idx = 0; pID[idx] && pString[idx]; ++idx)
    {
        if(pID[idx] != pString[idx])
            return false;
    }

    if(pString[idx] == '<')
    {
        *pnEaten = idx + 1;
        return true;
    }

    if(EA::StdC::Strcmp(pID, "op") == 0)
    {
        for(int i = idx; pString[i] && (nContiguousUnderscores != 2); ++i)
        {
            if(pString[i] == '_')
                nContiguousUnderscores++;
            else
                nContiguousUnderscores = 0;
        }

        if(nContiguousUnderscores != 2)
            return false; // Error
    }
    else
    {
        if((pString[idx] != '_') || (pString[idx + 1] != '_'))
            return false; // Error
    }

    *pnEaten = idx + 2;
    return true;
}


char* BuildArgString(char* pDest, char* pArg, int nAddComma)
{
    if(nAddComma) 
        *pDest++ = ',';

    while(*pArg)
    {
        if((signed char)*pArg < 0)
        {
            EA_ASSERT((signed char)pArg[1] >= 0);
            pArg++;
            const size_t nLen = (unsigned char)(*pArg++);
            EA::StdC::Memcpy(pDest, pArg, nLen);
            pDest += nLen;
            pArg  += nLen;
            *pDest = 0;
            continue;
        }
        else
        {
            const char* pAdd = 0;

            switch (*pArg)
            {
                case '%': pAdd = "const&";      break;
                case ',': pAdd = ", ";          break;
                case 'f': pAdd = "float";       break;
                case 'v': pAdd = "void";        break;
                case 'i': pAdd = "int";         break;
                case 'l': pAdd = "long";        break;
                case 'c': pAdd = "char";        break;
                case 'b': pAdd = "bool";        break;
                case 's': pAdd = "short";       break;
                case 'x': pAdd = "long long";   break;
                case 'd': pAdd = "double";      break;
                case 'e': pAdd = "...";         break;
                case 'R': pAdd = "&";           break;
                case 'P': pAdd = "*";           break;
                case 'C': pAdd = "const ";      break;
                case 'U': pAdd = "unsigned ";   break;
                case 'A':
                {
                    int i = 0;

                    while (pArg[i] && (EA::StdC::Isdigit(pArg[i]) || (pArg[i] == 'A') || (pArg[i] == '_')))
                        i++;

                    if(pArg[i] == 'R' || pArg[i] == 'P')
                    {
                        *pDest++ = '(';

                        while (pArg[i])
                        {
                            if(pArg[i] == 'R') 
                                *pDest++ = '&';

                            if(pArg[i] == 'P') 
                                *pDest++ = '*';

                            pArg[i] = '.';
                            i++;
                        }
                        *pDest++ = ')';
                    }
                    
                    *pDest++ = '[';
                    pArg++;

                    if(*pArg == '.')
                        pArg++;

                    while (EA::StdC::Isdigit(*pArg))
                        *pDest++ = *pArg++;

                    *pDest++ = ']';
                    *pDest = 0;
                    continue;
                }

                default:
                    break;
            }

            if(pAdd)
            {
                const char* pAdd1 = 0;
                const char* pAdd2 = 0;

                if(EA::StdC::Islower(*pArg))
                {
                    if(pArg[1] == 'U')
                    {
                        pAdd1 = "unsigned ";
                        pArg++;
                    }

                    if(pArg[1] == 'C')
                    {
                        pAdd2 = "const ";
                        pArg++;
                    }
                }

                if(pAdd2)
                {
                    EA::StdC::Strcpy(pDest, pAdd2);
                    pDest += EA::StdC::Strlen(pDest);
                }

                if(pAdd1)
                {
                    EA::StdC::Strcpy(pDest, pAdd1);
                    pDest += EA::StdC::Strlen(pDest);
                }

                EA::StdC::Strcpy(pDest, pAdd);
                pDest += EA::StdC::Strlen(pDest);
            }
        }

        pArg++;
    }

    if(pDest[-1] == ' ') 
        pDest--;                // Kill trailing spaces

    return pDest;
}


// Unpack names. Recursive function.
int Unpack(char* pOrigDest, char* pDest, char* pFuncname, const char* pCur, 
           int nType, int nToEat, int nArgsBegin, bool bGetFuncname, const char* pEnd)
{
  //const char* _pDebug;
    int         idx = 0;
    int         nEat = 0;
    int         nArgs = 0 + nArgsBegin;
    int         nParen = 0;
    int         isConstFunc = 0;
    int         nAddComma = 0;
  //int         isOkToEndArgs = 1;
    int         isKill = 0;           // kill unused return type..
    int         isReturnType = 0;
    int         nContiguousUnderscores = 0;
    int         nAllowExit = 0;
    const int   MAX_ARG = 1000;
    char        zArg[MAX_ARG];
    char*       pArg = &zArg[MAX_ARG - 1];
    char*       pReturnTypeStart = 0;

    *pArg = 0;

    EA_ASSERT(pDest);
    if(!pDest)
        return 0;

    do
    {
        //_pDebug = pCur + idx;
        //EA_ASSERT((pCur + idx) <= pEnd);

        // The function name can't just be copied - can contain template args
        if(bGetFuncname)
        {
            *pDest++ = pCur[idx];

            if(pCur[idx] == '_')
            {
                nContiguousUnderscores++;

                if(nContiguousUnderscores >= 2)
                {
                    if(pCur[idx + 1] == '_') 
                    {
                        idx++;
                        continue;
                    }

                    if((idx + 2) < nToEat)
                    {
                        // check list of operator stuff..
                        int iType = -1;

                        for (iType = 0; iType < (int)(sizeof(gOperatorTable) / sizeof(gOperatorTable[0])); iType++)
                        {
                            int nEaten;

                            if(CheckForOperator(pCur + idx + 1, gOperatorTable[iType].pID, &nEaten))
                            {
                                nType        = iType;
                                idx         += nEaten;
                                bGetFuncname = false;
                                pDest        = pOrigDest;
                                break;
                            }
                        }

                        if(nType == -1)
                        {
                            if((pCur[idx + 1] == 'Q') || EA::StdC::Isdigit(pCur[idx + 1]) || pCur[idx + 1] == 'F')
                            {
                                // member function.. needs to be outputted later! (actually is should have a Type)
                                pDest[-2]    = 0;               // kills '__'
                                EA::StdC::Strcpy(pFuncname, pOrigDest);
                                pDest        = pOrigDest;
                                bGetFuncname = false;
                                pOrigDest[0] = 0;     // debug
                                nType        = -2;
                            }
                        }

                        if((nType >= 0) && (EA::StdC::Strcmp(gOperatorTable[nType].pID, "op") == 0))        // operator '__op'
                        {
                            const int nCheck = Unpack(pOrigDest, pFuncname, 0, pCur + idx - 1, 0, 0, 1, false, pEnd);
                            idx += nCheck + 1;
                            continue;
                        }
                        
                        if(pCur[idx] == '<')
                        {
                            int    nCheck;
                            char*  pNext     = pFuncname + 1;
                            size_t nNewToEat = GetTemplateLength(pCur + idx);

                            pFuncname[0] = '<';
                            nCheck       = Unpack(pOrigDest, pNext, 0, pCur + idx, 0, (int)(unsigned)nNewToEat, 0, false, pEnd);
                            pNext       += EA::StdC::Strlen(pNext);
                            *pNext++     = '>';
                            *pNext       = 0;
                            idx         += nCheck + 1 + 2;
                            continue;
                        }
                    }
                }
            }
            else
                nContiguousUnderscores = 0;

            if(pCur[idx] == '<')
            {
                const size_t nNewToEat = GetTemplateLength(pCur + idx);

                Unpack(pOrigDest, pDest, 0, pCur + idx, 0, (int)(unsigned)nNewToEat, 1, false, pEnd);
                pDest += EA::StdC::Strlen(pDest);
                idx   += (int)(unsigned)nNewToEat;
                continue;
            }
            
            idx++;
            continue;
        }
        
        if((nArgs == 0) && (pCur[idx] == 'Q'))  // nargs is a test!!
        {
            char*       pQual  = pDest;
            int         nEat   = 0;
            int         nQual  = pCur[idx + 1] - '0';
            int         iQual;

            idx  += 2;
            nEat += 2;

            for (iQual = 0; iQual < nQual; iQual++)
            {
                const int nEaten = Unpack(pOrigDest, pQual, 0, pCur + idx, 0, 0, 0, false, pEnd);

                idx += nEaten;

                if(iQual < (nQual - 1))
                    EA::StdC::Strcat(pQual, "::");

                pQual = pQual + EA::StdC::Strlen(pQual);
            }

            pDest = pQual;
            nAllowExit = 1;
            continue;
        }
        
        if(!nArgs && pCur[idx] == 'F')        // Beginning of arguments.. 
        {
            nArgs = 1;
            nParen++;
            idx++;

            if(nType >= 0)
            {
                if(pDest > pOrigDest)
                {
                    EA::StdC::Strcpy(pDest, "::");
                    pDest += 2;
                }

                if((nType == 0) || (nType == 1))
                {
                    const char* pWork = pDest - 3;
                    int nMatch = 0;

                    while (pWork > pOrigDest)
                    {
                        if(*pWork == '>')
                            nMatch++;

                        if(*pWork == '<')
                            nMatch--;

                        if((nMatch == 0) && (*pWork == ':'))
                        {
                            pWork++;
                            break;
                        }

                        pWork--;
                    }
                    
                    if(nType == 1)
                        *pDest++ = '~';

                    while ((*pWork != ':') && (*pWork != '<'))
                        *pDest++ = *pWork++;

                    *pDest = 0;
                }
                else
                {
                    EA::StdC::Strcpy(pDest, gOperatorTable[nType].pName);

                    pDest += EA::StdC::Strlen(pDest);

                    if(pFuncname && pFuncname[0])
                    {
                        EA::StdC::Strcpy(pDest, pFuncname);
                        pDest += EA::StdC::Strlen(pDest);
                    }
                }

                nType = -1;
            }

            if(nType == -2)
            {
                EA::StdC::Sprintf(pDest, "%s%s", (pDest != pOrigDest) ? "::" : "", pFuncname);
                pDest += EA::StdC::Strlen(pDest);
            }

            *pDest++ = '(';

            continue;
        }
        
        if(!nArgs && (pCur[idx] == 'C'))  // const func! is declared before F
        {
            isConstFunc = 1;
            idx++;
            continue;
        }
        
        if((nArgs == 0) && EA::StdC::Isdigit(pCur[idx]))
        {
            char zNum[100] = "";
            int iNum = 0;
            int nNum;

            while (pCur[idx] && EA::StdC::Isdigit(pCur[idx]))
            {
                zNum[iNum++] = pCur[idx];
                idx++;
                nEat++;
            }

            if(pCur[idx] == '>')
            {
                idx -= iNum;

                if(nToEat > 0)
                    nNum = nToEat;
                else
                    nNum = nEat;            // 'digit>' cases..
            }
            else
            {
                if(pCur[idx] == ',')
                {
                    idx -= iNum;
                    nNum = nEat;
                }
                else
                {
                    zNum[iNum] = 0;
                    nNum = atoi(zNum);
                }
            }

            if(nToEat == 0)
                nToEat = nNum;

            for (int i = 0; i < nNum; i++, idx++)
            {
                *pDest++ = pCur[idx];
                nEat++;

                if(pCur[idx] == '<')
                {
                    const size_t nNewToEat = GetTemplateLength(pCur + idx);

                    Unpack(pOrigDest, pDest, 0, pCur + idx, 0, (int)(unsigned)nNewToEat, 1, false, pEnd);
                    pDest += EA::StdC::Strlen(pDest);
                    idx   += (int)(unsigned)nNewToEat - 1;
                    i     += (int)(unsigned)nNewToEat - 1;
                }
            }

            nAllowExit = 1;

            if(nToEat == 0)
            {
                *pDest = 0;
                return 0;
            }

            continue;
        }
        
        if(pCur[idx] == ',')
        { // cases like template args..
            *pDest++ = ',';
            *pDest++ = ' ';
            nAddComma = 0;
            idx++;
            nArgs = 1;
            continue;
        }

        // args parsing
        {
            bool isArgEnd = false;

            if(pCur[idx] == 'M')
            {
                int nCheck;
                if(nAddComma)
                    *pDest++ = ',';

                pReturnTypeStart = pDest; // We need to record start of pointer_to_func stuff in dest buffer as we need to insert the return type!
                *pDest++ = '(';
                nCheck = Unpack(pOrigDest, pDest, 0, pCur + idx + 1, 0, 0, 0, false, pEnd);
                pDest += EA::StdC::Strlen(pDest);
                EA::StdC::Strcpy(pDest, "::*)");
                pDest += EA::StdC::Strlen(pDest);
                idx += nCheck + 1;
                continue;
            }

            if(pCur[idx] == 'A')
            {
                size_t nDigits = 0;

                for(++idx; EA::StdC::Isdigit(pCur[idx]) || (pCur[idx] == '_') || (pCur[idx] == 'A'); ++idx)
                    nDigits++;

                EA::StdC::Memcpy(pArg - nDigits, pCur + idx - nDigits, nDigits);
                pArg -= nDigits;
                *--pArg = 'A';
                continue;
            }
            
            if(EA::StdC::Isdigit(pCur[idx]) || (pCur[idx] == 'Q'))
            {
                char         zName[512];
                bool         isSwap = false;
                const int    nCheck = Unpack(pOrigDest, zName, 0, pCur + idx, 0, 0, 0, false, pEnd);
                const size_t nNameLen = EA::StdC::Strlen(zName);

                if(pArg[0] == 'C')
                {
                    pArg++;
                    isSwap = true;
                }

                pArg -= nNameLen;
                EA::StdC::Memcpy(pArg, zName, nNameLen);

                //EA_ASSERT(nNameLen <= 255);
                *--pArg = (char)(unsigned char)nNameLen;
                *--pArg = (char)(unsigned char)0x80;

                if(isSwap)
                    *--pArg = 'C';

                idx += nCheck;
                isArgEnd = true;
            }
            
            {
                if(!isArgEnd)
                {
                    *--pArg = pCur[idx];

                    if(pCur[idx] == 'F')
                    {
                        pArg++;
                        if(pArg[0] == 'P')
                            pArg++;

                        //isOkToEndArgs = 0;

                        if(!pReturnTypeStart)
                        {
                            if(nAddComma)
                                *pDest++ = ',';
                            pReturnTypeStart = pDest;             // We need to record start of pointer_to_func stuff in dest buffer as we need to insert the return type!
                            EA::StdC::Strcpy(pDest, "(*)(");
                            pDest += EA::StdC::Strlen(pDest);
                        }
                        else
                            *pDest++ = '(';

                        nAddComma = 0;
                    }

                    if(pCur[idx] == '_')
                    {
                        //isOkToEndArgs = 1;

                        if(!pReturnTypeStart)
                            isKill = 1;
                        else
                        {
                            *pDest++ = ')';
                            nAddComma = 0;
                            isReturnType = 1;
                        }
                    }
                }
                
                // Lowercase letter ends an argument declaration..
                if(EA::StdC::Islower(pCur[idx]) || isArgEnd)
                {
                    char* pBefore = pDest;

                    if(!isKill)
                        pDest = BuildArgString(pDest, pArg, nAddComma);

                    if(isReturnType)
                    {
                        const size_t nReturnLen = EA::StdC::Strlen(pBefore);
                        const size_t nCopy      = (size_t)((pDest - pReturnTypeStart) + 1);
                        char*        pDest2     = pDest + nReturnLen - 1;
                        char*        pSrc       = pDest - 1;

                        for (size_t i = 0; i < nCopy; i++)
                        {
                            *pDest2 = *pSrc;
                            pDest2--;
                            pSrc--;
                        }

                        EA::StdC::Memcpy(pReturnTypeStart, pBefore + nReturnLen, nReturnLen);
                        *pDest = 0;
                        isReturnType = 0;
                        pReturnTypeStart = 0;
                    }

                    pArg       = &zArg[MAX_ARG - 1];
                    nAllowExit = 1;
                    nAddComma  = 1;
                }

                if(!isArgEnd)
                    idx++;
            }

            continue;
        }
    }
    while ((idx < nToEat) || ((nToEat == 0) && !nAllowExit));
    
    if(nParen)
    {
        if(pDest[-1] == ',')
            pDest -= 1;

        *pDest++ = ')';
    }
    
    if(isConstFunc)
    {
        EA::StdC::Strcpy(pDest, " const");
        pDest += EA::StdC::Strlen(pDest);
    }
    
    *pDest = 0;
    return idx;
}


size_t UnmangleSymbolCodeWarrior(const char* pSymbol, char* buffer, size_t /*bufferCapacity*/)
{
    char   pFuncname[512]; pFuncname[0] = 0;
    size_t nLen = EA::StdC::Strlen(pSymbol);

    buffer[0] = 0;
    Unpack(buffer, buffer, pFuncname, pSymbol, -1, (int)(unsigned)nLen, 0, true, pSymbol + nLen);

    return EA::StdC::Strlen(buffer);
}


} // namespace Demangle

} // namespace Callstack

} // namespace EA


#endif // EACALLSTACK_CODEWARRIOR_DEMANGLE_ENABLED





