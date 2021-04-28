/*************************************************************************************************/
/*!
    \file statsapilistener.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_STATS_STATSAPILISTENER_H
#define BLAZE_STATS_STATSAPILISTENER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif


namespace Blaze
{
namespace Stats
{

class StatsGroup;
class StatGroupList;

/*! ****************************************************************************/
/*!
    \class StatsAPIListener
    \brief interface required to be implemented by classes that wish to receive 
        notifications about Stats events.

    \details Listeners must add themselves to StatsApi to be able to get notifications.  
    See StatsAPI::addListener.
********************************************************************************/
class BLAZESDK_API StatsAPIListener
{

public:
    virtual ~StatsAPIListener() {}

    /*! ****************************************************************************/
    /*! \brief Notifies when a stat group is deleted
        \param group - the stat group being deleted
    ********************************************************************************/
    virtual void onStatGroupDelete(StatsGroup* group) = 0;

    /*! ****************************************************************************/
    /*! \brief Notifies when a stat group is deleted
        \param groupList - the stat group list being deleted
    ********************************************************************************/
    virtual void onStatGroupListDelete(StatGroupList* groupList) = 0;
};

} //namespace Stats
} // namespace Blaze
#endif // BLAZE_STATS_STATSAPILISTENER_H
