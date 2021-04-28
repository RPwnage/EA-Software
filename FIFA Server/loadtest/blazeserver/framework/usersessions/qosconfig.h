/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_QOS_CONFIG_H
#define BLAZE_QOS_CONFIG_H

/*** Include files *******************************************************************************/
#include "framework/tdf/qosdatatypes.h"
#include <EASTL/string.h>

namespace Blaze
{

extern const char8_t* UNKNOWN_PINGSITE;  // Used in cases where ping site information is missing. 
extern const char8_t* INVALID_PINGSITE;  // Used in cases where ping site information was invalid/uninitialized.


/*! *****************************************************************************/
/*! \class
\brief Interface to be implemented by anyone wishing to receive update ping site events. */
class PingSiteUpdateSubscriber
{
public:
    virtual ~PingSiteUpdateSubscriber() {}

    /* \brief The pingsites were updated. (global event) */
    virtual void onPingSiteMapChanges(const PingSiteInfoByAliasMap& pingSiteMap) {}
};


/*! *****************************************************************************/
/*! \class QosConfig
    \brief Manages Qos configuration.
    
*********************************************************************************/
class QosConfig
{
public:
    QosConfig();
    virtual ~QosConfig();

    //qos settings helper
    void validateQosSettingsConfig(const QosConfigInfo& config, ConfigureValidationErrors& validationErrors) const;
    void configure(const QosConfigInfo& config);

    // Attempt to refresh the qos ping sites.  Returns false for QoS 1.0 configs (which don't do the refresh).
    bool attemptRefreshQosPingSiteMap(bool configChange = false);
    void getQosPingSites(QosPingSitesResponse& response);

    const PingSiteInfoByAliasMap& getPingSiteInfoByAliasMap() const;
    
    // Get the QosSettings (includes the ping site list
    const QosConfigInfo& getQosSettings() const;

    // qos latency ping site functions

    /*!*********************************************************************************/
    /*! \brief check if the passed in alias is a valid alias we have in our config file

        \param[in] pingSiteAlias - ping site alias to be checked
        \return true if it's a valid alias, otherwise false
    ************************************************************************************/
    bool isKnownPingSiteAlias(const char8_t* pingSiteAlias) const;

    /*!*********************************************************************************/
    /*! \brief check if the passed in alias is a an acceptable ping site based on the
               name using (alphanum), '-', '_', ':'  characters.

        \param[in] pingSiteAlias - ping site alias to be checked
        \return true if it's a valid alias, otherwise false
    ************************************************************************************/
    bool isAcceptablePingSiteAliasName(const char8_t* pingSiteAlias) const;
    
    /*!*********************************************************************************/
    /*! \brief Makes sure the passed in map has values for the correct set of ping sites, 
               and change them to "invalid_pingsite" if they're not.

        \param[in] map - The map to examine.
        \return true if the set of ping site aliases in the map matches the configured set of aliases; false otherwise
    ************************************************************************************/
    bool validateAndCleanupPingSiteAliasMap(PingSiteLatencyByAliasMap &map) const;
    bool validatePingSiteAliasMap(const PingSiteInfoByAliasMap &map);

    // Subscription service for getting updates to the ping sites:
    void addSubscriber(PingSiteUpdateSubscriber& subscriber)
    {
        mDispatcher.addDispatchee(subscriber);
    }
    void removeSubscriber(PingSiteUpdateSubscriber& subscriber)
    {
        mDispatcher.removeDispatchee(subscriber);
    }

    bool isQosVersion1();

private:
    void refreshQosPingSiteMap();
    static PingSiteAlias pickNextBestQosPingSite(const PingSiteLatencyByAliasMap& latencyMap);

    QosConfigInfo mQosConfigInfo;
    TimerId mRefreshTimerId;
    EA::TDF::TimeValue mLastRefreshTime;  // We don't want to refresh too often, to avoid spamming the QoS server in the event of hacked clients. 
    eastl::string mLastRefreshProfile;

    Dispatcher<PingSiteUpdateSubscriber> mDispatcher;
};

}

#endif //BLAZE_QOS_CONFIG

