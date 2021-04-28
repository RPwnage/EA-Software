/*H*************************************************************************************************/
/*!
    \File    lobbytagexpr.c

    \Description
        This moduel implements a simple expression parser which uses a TagField record as
        a variable store and provides basic C type expression syntax. Assignment, comparison,
        logical, binary and basic arithmetic operations are all supported.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/*************************************************************************************************H*/

/*
NOTES:

This module allows for simple C-style expressions to be evaluated against a TagFeild record
which represents a variable store. Operator syntax and precedence is very similar to C though
error handling is limited. Unlike C, binary operations (&, |, ^) and logical operators (&&, ||)
each share the same precedent level respectively (& and && are different, & and | are the same).
A simple typing system understand the difference between numbers and strings though each are
cast to the other should the situation require it. An explicit concatenation operator (#) is
included for string append (rather than overloading +). The following operators are supported:

    (expr), "literal-string", literal-number, variable, !expr
    *, /, %
    +, -, #
    ==, !=, <, <=, >, >=
    &, |, ^
    &&, ||
    =
    statement ;
    { statement; statement; ... }

The #, == and != operators are all string based and because numeric values are internally
represented as strings, comparing a number/string with ==/!= is allowed. The TagFieldExpr
allows for an optional result to be returned. The result is the final expression that is
evaluated. All assignment and variable references take place within the TagField record that
is passed to the function. For example:

    initial tagfield record = ""
    expression:
        { a = 5; b = 10; a+b }
    final tagfield record = "a=5 b=10"
    tagfieldexpr output buffer = "15"
*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "lobbytagexpr.h"

/*** Defines ***************************************************************************/

#define EXPR_NUM    '#'     // item is a number
#define EXPR_STR    '$'     // item is a string
#define EXPR_VAR    '%'     // item is a variable
#define EXPR_ERR    0       // item is an error

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct ExprT        // expression state
{
    char *pEnvBuf;          // pointer to environment
    int32_t iEnvLen;            // length of environment

    char strValue[1024];    // current value buffer
    char *pValue;           // pointer to value buffer start
    int32_t iValue;             // length of value buffer
} ExprT;

/*** Function Prototypes ***************************************************************/

static const char *_ExprAssign(ExprT *pExpr, const char *pParse);
static const char *_ExprCondition(ExprT *pExpr, const char *pParse);

/*** Variables *************************************************************************/

/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _ExprNumber

    \Description
        Return numeric value of data object

    \Input pExpr - expression state
    
    \Output int32_t - integer value of data object (if string, atoi data)

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ExprNumber(ExprT *pExpr)
{
    int32_t iValue = 0;
    char *pValue = pExpr->pValue;

    // if its a variable reference, resolve variable to value
    if (*pValue == EXPR_VAR)
    {
        iValue = TagFieldGetNumber(TagFieldFind(pExpr->pEnvBuf, pValue+1), 0);
    }
    // else if number/string type convert ascii to integer
    else if ((*pValue == EXPR_NUM) || (*pValue == EXPR_STR))
    {
        for (++pValue; (*pValue >= '0') && (*pValue <= '9'); ++pValue)
        {
            iValue = (iValue * 10) + (*pValue & 15);
        }
    }

    return(iValue);
}

/*F********************************************************************************/
/*!
    \Function _ExprString

    \Description
        Convert current data object to string

    \Input pExpr - expression state
    
    \Output int32_t - length of string in characters

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ExprString(ExprT *pExpr)
{
    int32_t iLength = 0;
    char *pValue = pExpr->pValue;

    // if its a variable reference, resolve variable to value
    if (*pValue == EXPR_VAR)
    {
        iLength = TagFieldGetString(TagFieldFind(pExpr->pEnvBuf, pValue+1), pValue+1, pExpr->iValue-1, "");
        *pValue = EXPR_STR;
    }
    // else if number/string, force to string
    else if ((*pValue == EXPR_STR) || (*pValue == EXPR_NUM))
    {
        *pValue = EXPR_STR;
        iLength = (int32_t)strlen(pValue+1);
    }
    // else treat as empty string
    else
    {
        pValue[0] = EXPR_STR;
        pValue[1] = 0;
    }

    return(iLength);
}

/*F********************************************************************************/
/*!
    \Function _ExprPrimary

    \Description
        Parse a primary item

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprPrimary(ExprT *pExpr, const char *pParse)
{
    char *pValue = pExpr->pValue;

    // skip whitespace
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // recursive expression
    if (pParse[0] == '(')
    {
        pParse = _ExprCondition(pExpr, pParse+1);
        while ((*pParse != 0) && (*pParse <= ' '))
            ++pParse;
        if (pParse[0] == ')')
            ++pParse;
    }
    // logical negation
    else if (pParse[0] == '!')
    {
        pParse = _ExprPrimary(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, _ExprNumber(pExpr) == 0);
    }
    // literal string
    else if (pParse[0] == '"')
    {
        *pValue++ = EXPR_STR;
        TagFieldGetString(pParse, pValue, pExpr->iValue-1, "");
        for (++pParse; (*pParse >= ' ') && (*pParse != '"'); ++pParse)
            ;
        if (*pParse == '"')
            ++pParse;
    }
    // literal number
    else if (((pParse[0] >= '0') && (pParse[0] <= '9')) || (pParse[0] == '-'))
    {
        *pValue++ = EXPR_NUM;
        *pValue++ = *pParse++;
        while ((pParse[0] >= '0') && (pParse[0] <= '9'))
        {
            *pValue++ = *pParse++;
        }
        *pValue = 0;
    }
    // variable reference
    else if (pParse[0] >= '@')
    {
        *pValue++ = EXPR_VAR;
        while (((pParse[0] >= '0') && (pParse[0] <= '9')) || (pParse[0] >= '@'))
        {
            *pValue++ = *pParse++;
        }
        *pValue = 0;
    }
    // else parser error
    else
    {
        *pValue++ = EXPR_ERR;
        *pValue = 0;
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprMultDiv

    \Description
        Parse a multiplication, division or modulus operation

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprMultDiv(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprPrimary(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle multipication
    if ((pParse[0] == '*') && (pParse[1] != '*'))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprMultDiv(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue * _ExprNumber(pExpr));
    }
    // handle division
    else if ((pParse[0] == '/') && (pParse[1] != '/'))
    {
        int32_t iDiv;
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprMultDiv(pExpr, pParse+1);
        iDiv = _ExprNumber(pExpr);
        if (iDiv != 0)
            sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue / iDiv);
        else
            sprintf(pExpr->pValue, "%cDIV0", EXPR_NUM);
    }
    // handle modulus
    else if ((pParse[0] == '%') && (pParse[1] != '%'))
    {
        int32_t iMod;
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprMultDiv(pExpr, pParse+1);
        iMod = _ExprNumber(pExpr);
        if (iMod != 0)
            sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue % iMod);
        else
            sprintf(pExpr->pValue, "%cMOD0", EXPR_NUM);
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprAddSub

    \Description
        Parse an addition/subtraction operation

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprAddSub(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprMultDiv(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle addition
    if (pParse[0] == '+')
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprAddSub(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue + _ExprNumber(pExpr));
    }
    // handle subtraction
    else if (pParse[0] == '-')
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprAddSub(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue - _ExprNumber(pExpr));
    }
    // handle string append (get second string at end of first, saving/restoring char)
    else if (pParse[0] == '#')
    {
        char iRestore;
        int32_t iAppend = _ExprString(pExpr);
        pExpr->pValue += iAppend;
        pExpr->iValue -= iAppend;
        iRestore = pExpr->pValue[0];
        pParse = _ExprAddSub(pExpr, pParse+1);
        _ExprString(pExpr);
        pExpr->pValue[0] = iRestore;
        pExpr->pValue -= iAppend;
        pExpr->iValue += iAppend;
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprRelate

    \Description
        Parse relational operators

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprRelate(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprAddSub(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle compare equal
    if ((pParse[0] == '=') && (pParse[1] == '='))
    {
        int32_t iAppend = _ExprString(pExpr)+2;
        pExpr->pValue += iAppend;
        pExpr->iValue -= iAppend;
        pParse = _ExprAddSub(pExpr, pParse+2);
        _ExprString(pExpr);
        pExpr->pValue -= iAppend;
        pExpr->iValue += iAppend;
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, strcmp(pExpr->pValue, pExpr->pValue+iAppend) == 0);
    }
    // handle compare not equal
    else if ((pParse[0] == '!') && (pParse[1] == '='))
    {
        int32_t iAppend = _ExprString(pExpr)+2;
        pExpr->pValue += iAppend;
        pExpr->iValue -= iAppend;
        pParse = _ExprAddSub(pExpr, pParse+2);
        _ExprString(pExpr);
        pExpr->pValue -= iAppend;
        pExpr->iValue += iAppend;
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, strcmp(pExpr->pValue, pExpr->pValue+iAppend) != 0);
    }
    // handle substring compare
    else if ((pParse[0] == '~') && (pParse[1] == '='))
    {
        int32_t iAppend = _ExprString(pExpr)+2;
        pExpr->pValue += iAppend;
        pExpr->iValue -= iAppend;
        pParse = _ExprAddSub(pExpr, pParse+2);
        _ExprString(pExpr);
        pExpr->pValue -= iAppend;
        pExpr->iValue += iAppend;
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, strstr(pExpr->pValue, pExpr->pValue+iAppend) != NULL);
    }
    // handle compare less-than or equal
    else if ((pParse[0] == '<') && (pParse[1] == '='))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprRelate(pExpr, pParse+2);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, (iValue <= _ExprNumber(pExpr)));
    }
    // handle compare less-than
    else if (pParse[0] == '<')
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprRelate(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, (iValue < _ExprNumber(pExpr)));
    }
    // handle compare greater-than or equal
    else if ((pParse[0] == '>') && (pParse[1] == '='))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprRelate(pExpr, pParse+2);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, (iValue >= _ExprNumber(pExpr)));
    }
    // handle compare greater-than
    else if (pParse[0] == '>')
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprRelate(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, (iValue > _ExprNumber(pExpr)));
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprBinary

    \Description
        Parse binary operations (and, or, xor)

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprBinary(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprRelate(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle binary-and
    if ((pParse[0] == '&') && (pParse[1] != '&'))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprBinary(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue & _ExprNumber(pExpr));
    }
    // handle binary-or
    else if ((pParse[0] == '|') && (pParse[1] != '|'))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprBinary(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue | _ExprNumber(pExpr));
    }
    // handle binary-xor
    else if (pParse[0] == '^')
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprBinary(pExpr, pParse+1);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue ^ _ExprNumber(pExpr));
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprLogical

    \Description
        Parse logical operators (and, or)

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprLogical(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprBinary(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle logical-and
    if ((pParse[0] == '&') && (pParse[1] == '&'))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprLogical(pExpr, pParse+2);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue && _ExprNumber(pExpr));
    }
    // handle logical-or
    else if ((pParse[0] == '|') && (pParse[1] == '|'))
    {
        int32_t iValue = _ExprNumber(pExpr);
        pParse = _ExprLogical(pExpr, pParse+2);
        sprintf(pExpr->pValue, "%c%d", EXPR_NUM, iValue || _ExprNumber(pExpr));
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprAssign

    \Description
        Parse assignment operator

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprAssign(ExprT *pExpr, const char *pParse)
{
    char *pCopy;

    // get lower term first
    pParse = _ExprLogical(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle assignment
    if ((pParse[0] == '=') && (pParse[1] != '='))
    {
        int32_t iAppend = (int32_t)strlen(pExpr->pValue)+1;
        pExpr->pValue += iAppend;
        pExpr->iValue -= iAppend;
        pParse = _ExprAssign(pExpr, pParse+1);
        _ExprString(pExpr);
        TagFieldSetString(pExpr->pEnvBuf, pExpr->iEnvLen, pExpr->pValue-iAppend+1, pExpr->pValue+1);
        pExpr->pValue -= iAppend;
        pExpr->iValue += iAppend;
        // must do explicit copy due to src/dst overlap
        for (pCopy = pExpr->pValue; pCopy[iAppend] != 0; ++pCopy)
            pCopy[0] = pCopy[iAppend];
        *pCopy = 0;
    }

    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprCondition

    \Description
        Parse assignment operator

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprCondition(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprAssign(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle condition
    if (pParse[0] == '?')
    {
        // save the condition
        int32_t iCondition = _ExprNumber(pExpr);

        // parse first argument
        pParse = _ExprCondition(pExpr, pParse+1);
        while ((*pParse != 0) && (*pParse <= ' '))
            ++pParse;

        // figure out whether or save or skip data
        iCondition = (iCondition ? (int32_t)strlen(pExpr->strValue)+1 : 0);

        // grab second argument (overwriting first conditionally)
        if (pParse[0] == ':')
        {
            pExpr->pValue += iCondition;
            pExpr->iValue -= iCondition;
            pParse = _ExprCondition(pExpr, pParse+1);
            pExpr->pValue -= iCondition;
            pExpr->iValue += iCondition;
        }
    }
    return(pParse);
}

/*F********************************************************************************/
/*!
    \Function _ExprStatement

    \Description
        Parse an addition/subtraction operation

    \Input pExpr - expression state
    \Input pParse - current parse point
    
    \Output const char * - updated parse point

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static const char *_ExprStatement(ExprT *pExpr, const char *pParse)
{
    // get lower term first
    pParse = _ExprCondition(pExpr, pParse);
    while ((*pParse != 0) && (*pParse <= ' '))
        ++pParse;

    // handle optional statement terminator
    if (*pParse == ';')
    {
        ++pParse;
        while ((*pParse != 0) && (*pParse <= ' '))
            ++pParse;
    }

    return(pParse);
}

/*** Public Functions ******************************************************************/

/*F********************************************************************************/
/*!
    \Function TagFieldExpr

    \Description
        Evaluate an expression using C-style expression parser and tagfield
        record for varaible storage / reference.

    \Input pRecord - tagfield encoded data record
    \Input iReclen - tagfield data record max length
    \Input pExpr - expression string
    \Input pValue - pointer to output value (NULL=no output)
    \Input iValue - length of output value buffer
    
    \Output int32_t - number of characters parsed

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t TagFieldExpr(char *pRecord, int32_t iReclen, const char *pExpr, char *pValue, int32_t iValue)
{
    ExprT State;
    const char *pMark;
    const char *pStart = pExpr;

    // setup expression parser state
    State.pValue = State.strValue;
    State.iValue = sizeof(State.strValue);
    State.pEnvBuf = pRecord;
    State.iEnvLen = iReclen;

    // skip past whitespace
    while ((*pExpr != 0) && (*pExpr <= ' '))
        ++pExpr;

    // check for compound statement
    if (*pExpr == '{')
    {
        ++pExpr;

        // do multiple statements
        for (;;)
        {
            // parse a single statement
            pMark = pExpr;
            pExpr = _ExprStatement(&State, pExpr);
            // quit at end of data
            if (*pExpr == 0)
            {
                break;
            }
            // if end of compound statement then quit
            if (*pExpr == '}')
            {
                ++pExpr;
                break;
            }
            // if no data was parsed, gobble one character
            if (pExpr == pMark)
            {
                ++pExpr;
            }
        }
    }
    else if (*pExpr == '(')
    {
        // parse the paren here so it becomes terminator
        pExpr = _ExprStatement(&State, pExpr+1);
        // skip trailing paren
        if (*pExpr == ')')
            ++pExpr;
    }
    else
    {
        // parse a single statement
        // this is open format so extra data after expr can get parsed
        pExpr = _ExprStatement(&State, pExpr);
    }

    // if they want a return value, convert to string and return
    if (pValue != NULL)
    {
        _ExprString(&State);
        TagFieldDupl(pValue, iValue, State.pValue+1);
    }

    // don't gobble trailing whitespace (caller may want it)
    while ((pExpr != pStart) && (pExpr[-1] <= ' '))
        --pExpr;

    // return number of characters parsed
    return(pExpr-pStart);
}

