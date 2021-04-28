/*************************************************************************************************/
/*!
    \file   wipestats_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class WipeStatsCommand

    Provides client side ability to wipe stats.  The call is wrapped in an access 
    control permission and is not exported to the client side. The following operations 
    are available:
    1.  Delete by category and keyscope
    2.  Delete by category, keyscope and entity id
    3.  Delete by keyscope
    4.  Delete by keyscope and entity id
    5.  Delete by entity Id
    6.  Delete by category
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/wipestats_stub.h"

namespace Blaze
{
namespace Stats
{

class WipeStatsCommand : public WipeStatsCommandStub
{
public:
    WipeStatsCommand(Message *message, WipeStatsRequest *request, StatsSlaveImpl* componentImpl) 
    : WipeStatsCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    WipeStatsCommandStub::Errors execute() override
    {
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_WIPE_STATS))
        {
            WARN_LOG("[WipeStatsCommand].execute: attempted to wipe stats, no permission!");
            return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
        }

        BlazeRpcError error = Blaze::ERR_OK;

        switch( mRequest.getOperation())
        {
            case WipeStatsRequest::DELETE_BY_CATEGORY_KEYSCOPE_USERSET:
            {
                const CategoryMap* categoryMap = mComponent->getConfigData()->getCategoryMap();
                CategoryMap::const_iterator catIter = categoryMap->find(mRequest.getCategoryName());
                if (catIter == categoryMap->end())
                {
                    ERR_LOG("[WipeStatsCommand].execute(): Could not wipe stats for unknown category " << mRequest.getCategoryName());            
                    return STATS_ERR_UNKNOWN_CATEGORY;
                }
                const StatCategory* cat = catIter->second;

                DeleteStatsByUserSetRequest req;
                StatDeleteByUserSet* statDelete = req.getStatDeletes().pull_back();
                statDelete->setCategory(mRequest.getCategoryName());
                mRequest.getKeyScopeNameValueMap().copyInto(statDelete->getKeyScopeNameValueMap());
                statDelete->setUserSetId(mRequest.getEntityObjectId());
                statDelete->getPeriodTypes().reserve(STAT_NUM_PERIODS);

                for (int32_t period = STAT_PERIOD_ALL_TIME; period < STAT_NUM_PERIODS; ++period)
                {
                    if (cat->isValidPeriod(period))
                    {
                        statDelete->getPeriodTypes().push_back(period);
                    }
                }                

                error = mComponent->deleteStatsByUserSet(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_CATEGORY_KEYSCOPE_ENTITYID:
            {
                const CategoryMap* categoryMap = mComponent->getConfigData()->getCategoryMap();
                CategoryMap::const_iterator catIter = categoryMap->find(mRequest.getCategoryName());
                if (catIter == categoryMap->end())
                {
                    ERR_LOG("[WipeStatsCommand].execute(): Could not wipe stats for unknown category " << mRequest.getCategoryName());            
                    return STATS_ERR_UNKNOWN_CATEGORY;
                }
                const StatCategory* cat = catIter->second;

                DeleteStatsRequest req;
                StatDelete* statDelete = req.getStatDeletes().pull_back();
                statDelete->setCategory(mRequest.getCategoryName());
                mRequest.getKeyScopeNameValueMap().copyInto(statDelete->getKeyScopeNameValueMap());
                statDelete->setEntityId(mRequest.getEntityId());
                statDelete->getPeriodTypes().reserve(STAT_NUM_PERIODS);

                for (int32_t period = STAT_PERIOD_ALL_TIME; period < STAT_NUM_PERIODS; ++period)
                {
                    if (cat->isValidPeriod(period))
                    {
                        statDelete->getPeriodTypes().push_back(period);
                    }
                }                

                error = mComponent->deleteStats(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_KEYSCOPE_USERSET:
            {
                DeleteAllStatsByKeyScopeAndUserSetRequest req;
                mRequest.getKeyScopeNameValueMap().copyInto(req.getKeyScopeNameValueMap());
                req.setUserSetId(mRequest.getEntityObjectId());

                error = mComponent->deleteAllStatsByKeyScopeAndUserSet(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_KEYSCOPE_ENTITYID:
            {
                DeleteAllStatsByKeyScopeAndEntityRequest req;
                mRequest.getKeyScopeNameValueMap().copyInto(req.getKeyScopeNameValueMap());
                req.setEntityType(mRequest.getEntityObjectId().type);
                req.setEntityId(mRequest.getEntityObjectId().id);

                error = mComponent->deleteAllStatsByKeyScopeAndEntity(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_ENTITYID:
            {
                DeleteAllStatsByEntityRequest req;
                req.setEntityType(mRequest.getEntityObjectId().type);
                req.setEntityId(mRequest.getEntityObjectId().id);

                error = mComponent->deleteAllStatsByEntity(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_CATEGORY:
            {
                DeleteStatsByCategoryRequest req;
                StatDeleteByCategory* statDeleteByCategory = req.getStatDeletes().pull_back();
                statDeleteByCategory->setCategory(mRequest.getCategoryName());                

                error = mComponent->deleteStatsByCategory(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_CATEGORY_ENTITYID:
            {
                const CategoryMap* categoryMap = mComponent->getConfigData()->getCategoryMap();
                CategoryMap::const_iterator catIter = categoryMap->find(mRequest.getCategoryName());
                if (catIter == categoryMap->end())
                {
                    ERR_LOG("[WipeStatsCommand].execute(): Could not wipe stats for unknown category " << mRequest.getCategoryName());            
                    error = Blaze::STATS_ERR_UNKNOWN_CATEGORY;
                    break;
                }
                const StatCategory* cat = catIter->second;

                DeleteAllStatsByCategoryAndEntityRequest req;
                req.setEntityId(mRequest.getEntityId());
                req.setCategory(mRequest.getCategoryName());
                req.getPeriodTypes().reserve(STAT_NUM_PERIODS);
                for(int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
                {
                    if(cat->isValidPeriod(periodType))
                    {
                        req.getPeriodTypes().push_back((Stats::StatPeriodType)periodType);
                    }
                }
                error = mComponent->deleteAllStatsByCategoryAndEntity(req);
                break;
            }
            case WipeStatsRequest::DELETE_BY_CATEGORY_KEYSCOPE:
            {
                Stats::DeleteStatsByKeyScopeRequest req;
                StatDeleteByKeyScope* statDeleteByKeyscope = req.getStatDeletes().pull_back();
                mRequest.getKeyScopeNameValueMap().copyInto(statDeleteByKeyscope->getKeyScopeNameValueMap());
                statDeleteByKeyscope->setCategory(mRequest.getCategoryName());

                error = mComponent->deleteStatsByKeyScope(req);
                break;
            }
            default:
            {
                error = Blaze::STATS_ERR_INVALID_OPERATION;
                break;
            }
        }
        
        if (error == Blaze::ERR_OK)
        {
            BlazeId blazeId = INVALID_BLAZE_ID; // default super user
            if (gCurrentUserSession != nullptr)
            {
                blazeId = gCurrentUserSession->getBlazeId();
            }
            INFO_LOG("[WipeStatsCommand].execute: User [" << blazeId << "] wiped stats ("
                << WipeStatsRequest::WipeStatsOperationToString(mRequest.getOperation()) << "), had permission.");
        }
        return (commandErrorFromBlazeError(error));
    }
private:
    StatsSlaveImpl* mComponent;
};

DEFINE_WIPESTATS_CREATE()

} // Stats
} // Blaze
