/*************************************************************************************************/
/*!
    \file


    \attention
    (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_OBJECT_H
#define BLAZE_OBJECT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "EATDF/tdfobjectid.h"
#include "BlazeSDK/component/framework/tdf/entity.h"

namespace Blaze
{
    /*! ************************************************************************************************/
    /*! \class BlazeObject

        \brief interface implemented by any class that wishes to expose its EA::TDF::ObjectId(component id, 
         entity type and entity id) information
    ***************************************************************************************************/
    class BLAZESDK_API BlazeObject
    {
    public:
        virtual ~BlazeObject() {}

    public:
        /*! ************************************************************************************************/
        /*! \brief When called returns a EA::TDF::ObjectId from the class implementing this interface.

            \return EA::TDF::ObjectId - blaze object id information held by derived class
        ***************************************************************************************************/
        virtual EA::TDF::ObjectId getBlazeObjectId() const = 0;
    };
}


#endif  // BLAZE_USER_GROUP_H
