/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROTOCOLFACTORY_H
#define BLAZE_PROTOCOLFACTORY_H

/*** Include files *******************************************************************************/

#include "framework/protocol/protocol.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ProtocolFactory
{
    NON_COPYABLE(ProtocolFactory);
    ProtocolFactory() {};
    ~ProtocolFactory() {};

public:

    static Protocol* create(Protocol::Type type, uint32_t maxFrameSize);
    static Protocol::Type getProtocolType(const char8_t* name);
};

} // Blaze

#endif // BLAZE_PROTOCOLFACTORY_H
