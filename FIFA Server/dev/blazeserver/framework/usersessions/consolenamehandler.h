/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONSOLENAMEHANDLER_H
#define BLAZE_CONSOLENAMEHANDLER_H

/*** Include files *******************************************************************************/

#include "framework/usersessions/namehandler.h"
#include "framework/lobby/lobby/lobbynames.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class ConsoleNameHandler

    This implementation of the NameHandler interface matches the 1st party console naming rules
    for PS3 and Xbox 360 (whitespace and case is ignored).
*/
/*************************************************************************************************/
class ConsoleNameHandler : public NameHandler
{
public:
    size_t generateHashCode(const char8_t* name) const override
    {
        return (size_t)LobbyNameHash(name);
    }

    int32_t compareNames(const char8_t* name1, const char8_t* name2) const override
    {
        return LobbyNameCmp(name1, name2);
    }

    bool generateCanonical(const char8_t* name, char8_t* canonical, size_t len) const override
    {
        return (LobbyNameCreateCanonical(name, canonical, len) != -1);
    }

    static const char8_t* getType() { return "console"; }
};

} // Blaze

#endif // BLAZE_CONSOLENAMEHANDLER_H

