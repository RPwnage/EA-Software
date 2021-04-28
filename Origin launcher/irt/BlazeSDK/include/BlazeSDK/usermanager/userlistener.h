/*************************************************************************************************/
/*!
    \file userlistener.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_USER_MANAGER_USER_LISTENER_H
#define BLAZE_USER_MANAGER_USER_LISTENER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/component/framework/tdf/userdefines.h"

namespace Blaze
{

namespace UserManager
{
    class User;
}

namespace UserManager
{
    /*! ***********************************************************************************************/
    /*! \class UserEventListener

        \brief Interface that can be registered with UserManager to receive User specific events
    ***************************************************************************************************/
    class BLAZESDK_API UserEventListener
    {
    public:
        virtual ~UserEventListener() { }
        
        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when extended user info data is 
                changed

            \param[in] blazeId Blaze id of user whose data has changed
        ********************************************************************************/
        virtual void onExtendedUserDataInfoChanged(Blaze::BlazeId blazeId) {};
        
        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when user is added to user manager
                cache

            \param[in] user User object for the added user
        ********************************************************************************/
        virtual void onUserAdded(const User& user) {};

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when user is updated

            \param[in] user User object for the updated user
        ********************************************************************************/
        virtual void onUserUpdated(const User& user) {};

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when user is removed from user manager
                cache

            \param[in] user User object of the removed user
        ********************************************************************************/
        virtual void onUserRemoved(const User& user) {};
        
    };

    /*! ***********************************************************************************************/
    /*! \typedef ExtendedUserDataInfoChangeListener

        \brief This typedef is provided for backwards compatibility
    ***************************************************************************************************/
    typedef UserEventListener ExtendedUserDataInfoChangeListener;

    class BLAZESDK_API PrimaryLocalUserListener
    {
    public:
        virtual ~PrimaryLocalUserListener() {}

        /*! ************************************************************************************************/
        /*! \brief Async notification that the primary local user has been changed.
        
            \param[in] primaryUserIndex - the new primary local user index.
        ***************************************************************************************************/
        virtual void onPrimaryLocalUserChanged(uint32_t primaryUserIndex) {};

        /*! ************************************************************************************************/
        /*! \brief Async notification that the primary local user has been authenticated.
                   Meaning, the first user has logged in.
        
            \param[in] primaryUserIndex - the index of the primary local user just authenticated.
        ***************************************************************************************************/
        virtual void onPrimaryLocalUserAuthenticated(uint32_t primaryUserIndex) {};

        /*! ************************************************************************************************/
        /*! \brief Async notification that the primary local user has been de-authenticated.
                   Meaning, the last user has logged out.
        
            \param[in] primaryUserIndex - the index of the primary local user just de-authenticated.
        ***************************************************************************************************/
        virtual void onPrimaryLocalUserDeAuthenticated(uint32_t primaryUserIndex) {};
    };
    
} // namespace UserManager
} // namespace Blaze
#endif // BLAZE_USER_MANAGER_USER_LISTENER_H
