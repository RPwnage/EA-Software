/*************************************************************************************************/
/*!
\file usersessionsubscriber.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SESSION_SUBSCRIBER_H
#define BLAZE_USER_SESSION_SUBSCRIBER_H

namespace Blaze
{

class UserSession;
class UserSessionMaster;
class UserSessionExistenceData;

/*! *****************************************************************************/
/*! \class
\brief Interface to be implemented by anyone wishing to receive user session events. */
class UserSessionSubscriber
{
public:
    virtual ~UserSessionSubscriber() {}

    /* GLOBAL EVENTS */

    /* \brief A user session is created somewhere in the cluster. (global event) */
    virtual void onUserSessionExistence(const UserSession& userSession) {}

    /* \brief A user session is changed somewhere in the cluster. (global event) */
    virtual void onUserSessionMutation(const UserSession& userSession, const UserSessionExistenceData& newExistenceData) {}

    /* \brief A user session is destroyed somewhere in the cluster. (global event) */
    virtual void onUserSessionExtinction(const UserSession& userSession) {}
};


/*! *****************************************************************************/
/*! \class
\brief Interface to be implemented by anyone wishing to receive events regarding locally owned UserSessions. */
class LocalUserSessionSubscriber
{
public:
    virtual ~LocalUserSessionSubscriber() {}

    /* \brief A local user was logged in. */
    virtual void onLocalUserSessionLogin(const UserSessionMaster& userSession) {}

    /* \brief A local user was logged out. */
    virtual void onLocalUserSessionLogout(const UserSessionMaster& userSession) {}

    /* \brief A local user that is monitoring its connectivity has succeeded it's check (after already failing previously). */
    virtual void onLocalUserSessionResponsive(const UserSessionMaster& userSession) {}

    /* \brief A local user that is monitoring its connectivity has failed it's check (after already succeeding previously). */
    virtual void onLocalUserSessionUnresponsive(const UserSessionMaster& userSession) {}

    /*! \brief  A local user's account info (namely, country or language) was updated. */
    virtual void onLocalUserAccountInfoUpdated(const UserSessionMaster& userSession) {}

    /*! \brief  A local user was imported. */
    virtual void onLocalUserSessionImported(const UserSessionMaster& userSession) {}

    /*! \brief  A local user was exported. */
    virtual void onLocalUserSessionExported(const UserSessionMaster& userSession) {}
};

} // namespace Blaze

#endif // BLAZE_USER_SESSION_SUBSCRIBER_H