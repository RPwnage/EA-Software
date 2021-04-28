/*************************************************************************************************/
/*!
    \file lbapi.h

    Leaderboard API header.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZESDK_API_H
#define BLAZESDK_API_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazestateprovider.h"
#include "BlazeSDK/apidefinitions.h"

namespace Blaze
{
    class BlazeHub;

    /*! ****************************************************************************/
    /*! \class API

        \brief Base class for all Blaze APIs.
    ********************************************************************************/
    class BLAZESDK_API API : protected BlazeStateEventHandler
    {
    public:
        /*! ****************************************************************************
            \name Base methods and members for all Blaze APIs
         */  
        virtual ~API();

        /*! ****************************************************************************/
        /*! \brief Returns the id of this API.
        ********************************************************************************/
        virtual APIId getId() const { return mId; }


        /*! ****************************************************************************/
        /*! \brief Logs any initialization parameters for this API to the log.
        ********************************************************************************/
        virtual void logStartupParameters() const = 0;

        /*! ****************************************************************************/
        /*! \brief Deactivates the API and cleans up any associated data.

              The base implementation will handle event notification and state changes.
              Implementing classes should override and clean up any data the API
              allocated since it was initialized.
        ********************************************************************************/
        virtual void deactivate() { mIsActive = false; }

        /*! ****************************************************************************/
        /*! \brief Reactivates the API for use after deactivation.
        ********************************************************************************/
        virtual void reactivate() { mIsActive = true; }

        
        /*! ****************************************************************************/
        /*! \brief Returns whether the API is active or not.
        ********************************************************************************/
        virtual bool isActive() const { return mIsActive; }

        /*! ****************************************************************************/
        /*! \brief Returns the API Id for this API.
        ********************************************************************************/
        APIId getAPIId() const {
            return mId;
        }

        /*! ****************************************************************************/
        /*! \brief Returns the BlazeHub owning this API.
        ********************************************************************************/
        BlazeHub* getBlazeHub() const {
            return mBlazeHub;
        }

    protected:

        /*! ****************************************************************************/
        /*! \brief Constructs an API.  The API will register to the BlazeHub upon
                   creation to monitor changes in the user state(s).

            \param blazeHub  The BlazeHub
        ********************************************************************************/
        API(BlazeHub &blazeHub);

        // From BlazeStateEventHandler //

         /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has made a connection to the blaze server.  This is user independent, as
            all local users share the same connection.
        ***************************************************************************************************/
        virtual void onConnected() { };

        /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has lost connection to the blaze server.  This is user independent, as
            all local users share the same connection.
        ***************************************************************************************************/
        virtual void onDisconnected(BlazeError errorCode) { };
        
        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has been authenticated against the blaze server.  
            The provided user index indicates which local user authenticated.  If multiple local users
            authenticate, this will trigger once per user.
        
            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onAuthenticated(uint32_t userIndex) { };

        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
            The provided user index indicates which local user  lost authentication.  If multiple local users
            loose authentication, this will trigger once per user.

            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onDeAuthenticated(uint32_t userIndex) { };

    protected:
       BlazeHub         *mBlazeHub;

    private:
        APIId           mId;
        bool            mIsActive;

    };

    class BLAZESDK_API MultiAPI : public API
    {
    public:
        virtual ~MultiAPI() {};

        /*! ****************************************************************************/
        /*! \brief Returns the user index associated with this API instance.
        ********************************************************************************/
        uint32_t getUserIndex() const { return mUserIndex; }

    protected:
        MultiAPI(BlazeHub &blazeHub, uint32_t userIndex);

    private:
        uint32_t mUserIndex;
    };

    class BLAZESDK_API SingletonAPI : public API
    {
    public: 
        virtual ~SingletonAPI() {};


    protected:
        SingletonAPI(BlazeHub &blazeHub);

    };
};
#endif // API_H
