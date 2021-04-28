/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef REDIRECTOR_CONNECTIONPROFILE_H
#define REDIRECTOR_CONNECTIONPROFILE_H

/*** Include files *******************************************************************************/

#include "framework/util/shared/blazestring.h"
#include "redirector/tdf/redirectortypes_server.h"
#include "redirector/tdf/redirector_configtypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Redirector
{

class ConnectionProfile : public  SingleProfilesData
{
    NON_COPYABLE(ConnectionProfile);

public:

    ConnectionProfile(EA::Allocator::ICoreAllocator& alloc = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), 
        const char8_t* debugName = "") 
        : SingleProfilesData(alloc, debugName) { }

    bool match(const char8_t* channel, const char8_t* protocol, const char8_t* encoder, const char8_t* decoder) const
    {
        return ((blaze_stricmp(channel, mChannel.c_str()) == 0)
            && (blaze_stricmp(protocol, mProtocol.c_str()) == 0)
            && (blaze_stricmp(encoder, mEncoder.c_str()) == 0)
            && (blaze_stricmp(decoder, mDecoder.c_str()) == 0));
    }
};

} // Redirector
} // Blaze

#endif // REDIRECTOR_CONNECTIONPROFILE_H

