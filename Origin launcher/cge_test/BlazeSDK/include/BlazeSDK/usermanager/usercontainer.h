/*! ************************************************************************************************/
/*!
    \file usercontainer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_USER_CONTAINER_H
#define BLAZE_USER_CONTAINER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/usermanager/usermanager.h"


namespace Blaze
{

namespace UserManager
{
    class User;

    /*! ***********************************************************************************************/
    /*! \class UserContainer
    \ingroup _mod_initialization

    \brief Interface class to allow access to the private members of user manager addUser() and removeUser()
        without the need for a friend relationship.
    ***************************************************************************************************/
    class BLAZESDK_API UserContainer
    {
    public:

        ~UserContainer() {}

        /*! ************************************************************************************************/
        /*! \brief Creates (or updates the refCount of) the User with the supplied blazeId in the given UserManager. 
             Returns the updated user.

            \param[in] userManager the UserManager instance to add the user to.
            \param[in] blazeId the blazeId of the user to add/update
            \param[in] userName the name of the user (persona name)
            \param[in] platformInfo the platformInfo of the user
            \param[in] accountLocale The locale for the user.
            \param[in] accountCountry The country for the user.
            \param[in] statusFlags User status flags (currently only stores online status)
            \param[in] userNamespace userNamespace
            \return the user
        ***************************************************************************************************/
        const User* addUser(UserManager *userManager, BlazeId blazeId, const char8_t *userName, const PlatformInfo& platformInfo, 
                            Locale accountLocale = 0, uint32_t accountCountry = 0, const UserDataFlags *statusFlags = nullptr, const char8_t *userNamespace = nullptr)
        {
            // UM_TODO:: This addUser() implementation should be deprecated in favor of a simple addUser(BlazeId)
            // NOTE: Because extBlob is always nullptr, this operation cannot create 'complete' User objects on the PS4.
            // Sony Id is always omitted from User objects created through this code path until they are 'looked up', 
            // via the UserManager or or until another User 'subscribes' to them.
            return userManager->acquireUser(
                blazeId, 
                platformInfo,
                true,   // Assumes that the users in-game are primary.  Edge-case exists if a reservation was made for a secondary persona user. 
                userNamespace,
                userName,
                accountLocale,
                accountCountry,
                statusFlags,
                User::OWNED
            );
        }
        
        const User* addUser(UserManager *userManager, const UserIdentification &data)
        {
            return userManager->acquireUser(data, User::OWNED);
        }

        const User* addUser(UserManager *userManager, const UserData *data)
        {
            return userManager->acquireUser(data, User::OWNED);
        }

        /*! ************************************************************************************************/
        /*! \brief Destroy (or update the refCount of) the User with the supplied blazeId in the given UserManager. 

            \param[in] userManager the UserManager instance to remove the user from.
            \param[in] user the user to remove
        ***************************************************************************************************/
        void removeUser(UserManager *userManager, const User* user)
        {
            userManager->releaseUser(user);
        }

    };

} // namespace UserManager
} // namespace Blaze
#endif // BLAZE_USER_CONTAINER_H
