/*************************************************************************************************/
/*!
    \file lbapilistener.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_STATS_LBAPILISTENER_H
#define BLAZE_STATS_LBAPILISTENER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif


namespace Blaze
{
namespace Stats
{

class LeaderboardTree;
/*! ****************************************************************************/
/*!
    \class LeaderboardAPIListener
    \brief interface required to be implemented by classes that wish to receive 
        notifications about Leaderboards events.

    \details Listeners must add themselves to LeaderboardAPI to be able to get notifications.  
    See LeaderboardAPI::addListener.
********************************************************************************/
class BLAZESDK_API LeaderboardAPIListener
{

public:
    virtual ~LeaderboardAPIListener() {}
    /*! ****************************************************************************/
    /*! \brief Notifies when a leaderboard is going to be deleted
        \param leaderboard - the leaderboard that is being deleted
    ********************************************************************************/
    virtual void onLeaderboardDelete(LeaderboardTree* leaderboard) = 0;
};

} //namespace Stats
} // namespace Blaze
#endif // BLAZE_STATS_LBAPILISTENER_H
