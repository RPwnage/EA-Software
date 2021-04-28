/*************************************************************************************************/
/*!
    \file usergroup.h


    \attention
    (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_USER_GROUP_H
#define BLAZE_USER_GROUP_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazeobject.h"

namespace Blaze
{
    /*! ************************************************************************************************/
    /*! \class UserGroup

        \brief Opaque interface implemented by any class that represents a collection of users and implements
        the BlazeObject interface in order to provide clients with a EA::TDF::ObjectId corresponding to this UserGroup.
    ***************************************************************************************************/
    class BLAZESDK_API UserGroup : public BlazeObject {};
    
    
    
    /*! ************************************************************************************************/
    /*! \class UserGroupProvider

        \brief interface implemented by a class that provides an ability to map a EA::TDF::ObjectId(component id, 
         user group type and user group id) to an underlying UserGroup
    ***************************************************************************************************/
    class BLAZESDK_API UserGroupProvider
    {
    public:
        virtual ~UserGroupProvider() {}

    public:
        /*! ************************************************************************************************/
        /*! \brief When called returns a UserGroup contained by the class implementing this interface.

            \param[in] bobjId - blaze object id that will be mapped to the UserGroup
            \return UserGroup - underlying UserGroup object mapped by the provided EA::TDF::ObjectId
        ***************************************************************************************************/
        virtual UserGroup* getUserGroupById(const EA::TDF::ObjectId& bobjId) const = 0;
    };
}


#endif  // BLAZE_USER_GROUP_H
