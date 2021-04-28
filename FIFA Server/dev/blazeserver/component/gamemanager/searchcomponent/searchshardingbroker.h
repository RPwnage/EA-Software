/*! ************************************************************************************************/
/*!
    \file searchshardingbroker.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_SHARDING_BROKER_H
#define BLAZE_SEARCH_SHARDING_BROKER_H

#include "framework/slivers/slivermanager.h"
#include "gamemanager/rpc/searchslave_stub.h"
#include "gamemanager/tdf/search_server.h"
#include "EASTL/list.h"
#include "EASTL/map.h"

#include "framework/controller/controller.h"

namespace Blaze
{
namespace Search
{
    // This class is only to be inherited from
    class SearchShardingBroker : private SearchSlaveListener
    {
    protected:
        SearchShardingBroker();
        ~SearchShardingBroker() override;

        // establish connections to Search Slaves (returns false if it fails)
        bool registerForSearchSlaveNotifications();

    public:
        // the list of search slave instances
        typedef Blaze::InstanceIdList SlaveInstanceIdList;

        // static error conversion methods
        static BlazeRpcError convertSearchComponentErrorToMatchmakerError(BlazeRpcError searchRpcError);
        static BlazeRpcError convertSearchComponentErrorToGameManagerError(BlazeRpcError searchRpcError);
        /*! ***********************************************************************/
        /*! \class Component
            \brief Gets a set of search slave instances.
            \param[out] instanceList - the returned set of Search Slave InstanceIds
                If nullptr, the instanceList will have one Search Slave for every existing GameManager master.
        ***************************************************************************/
        void getFullCoverageSearchSet(SlaveInstanceIdList& instanceList) const;

    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_SEARCH_SHARDING_BROKER_H
