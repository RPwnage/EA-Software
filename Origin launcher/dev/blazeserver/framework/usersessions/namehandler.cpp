/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NameHandler

    This class provides an interface that UserSessionManager users to handle operations on a
    user name.  Different plaforms/namespaces may have different rules for comparing names and
    this interface hides those differences and allows a server to be configured differently
    depending on the target namespace.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/usersessions/namehandler.h"
#include "framework/usersessions/consolenamehandler.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/verbatimnamehandler.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/


NameHandler* NameHandler::createNameHandler(const char8_t* type)
{
    if (blaze_stricmp(type, ConsoleNameHandler::getType()) == 0)
        return BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "ConsoleNameHandler") ConsoleNameHandler();

    if (blaze_stricmp(type, VerbatimNameHandler::getType()) == 0)
        return BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "VerbatimNameHandler") VerbatimNameHandler();

    return nullptr;
}

const char8_t* NameHandler::getDefaultNameHandlerType()
{
    return ConsoleNameHandler::getType();
}

const char8_t* NameHandler::getValidTypes(char8_t* buffer, size_t len)
{
    blaze_snzprintf(buffer, len, "%s,%s",
            ConsoleNameHandler::getType(),
            VerbatimNameHandler::getType());
    return buffer;
}

} // Blaze

