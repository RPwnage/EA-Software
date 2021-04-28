/*************************************************************************************************/
/*!
    \file uuid.h

    Contains helper functions to generate a UUID.

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_UUID_H
#define BLAZE_UUID_H

#include "framework/util/random.h"
#include "framework/tdf/uuid.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

/*! ************************************************************************************************/
/*!
    \brief A uuid generator helper class.
*************************************************************************************************/
class UUIDUtil
{
public:

    /*! ************************************************************************************************/
    /*! \brief generate a UUID using 
            *  uuid_generate on Linux (http://linux.die.net/man/3/uuid_generate)
            *  UuidCreate on Windows (http://msdn.microsoft.com/en-us/library/aa379205.aspx)

        [param, out] uuid - UUID generated in string format like f47ac10b-58cc-4372-a567-0e02b2c3d479
                             total lenth is 36 chars
        See http://en.wikipedia.org/wiki/UUID

        \param[out]uuid - the newly generated uuid
    ***************************************************************************************************/
    static void generateUUID(UUID& uuid);
    static void generateSecretText(const UUID& uuid, EA::TDF::TdfBlob& secretBlob);
    static void retrieveUUID(const EA::TDF::TdfBlob& secretBlob, UUID& uuid);
};

} // Blaze
#endif // BLAZE_UUID_H
