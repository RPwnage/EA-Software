/*************************************************************************************************/
/*!
    \file   coopseasonidentityprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CoopSeasonIdentityProvider

    This class implements the IdentityProvider interface and provides a mechanism to do identity
    lookups for coop season.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"

// clubs includes
#include "coopseason/coopseasonidentityprovider.h"
#include "coopseason/coopseasonslaveimpl.h"
#include "coopseason/coopseasondb.h"

namespace Blaze
{
namespace CoopSeason
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


#define CONVERT_FUNCTOR(X, TYPE) (*((TYPE *) ((FunctorBase *) &X))); 


/*** Public Methods ******************************************************************************/

CoopSeasonIdentityProvider::CoopSeasonIdentityProvider(CoopSeasonSlaveImpl* component)
    : mComponent(component),
		mCache(16384, BlazeStlAllocator("CoopSeasonIdentityProvider::mCache", CoopSeasonSlave::COMPONENT_MEMORY_GROUP), IdentityCache::RemoveCb(&CoopSeasonIdentityProvider::identityCacheRemoveCb))
{
}

BlazeRpcError CoopSeasonIdentityProvider::getIdentities(BlazeObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results)
{
    if(bobjType != ENTITY_TYPE_COOP)
    {
        return ERR_SYSTEM; // TODO: log error
    }
    
    if (keys.empty())
    {
        results.clear();
        return ERR_OK;
    }

    BlazeRpcError err = ERR_OK;
    CoopIdList coopIdList;

    bool foundRegular = false;
    results.reserve(keys.size());
    
    // Pull as many entries out of the cache as possible
    EntityIdList::const_iterator itr = keys.begin();
    while (itr != keys.end())
    {
        EntityId key = *itr++;
        if (key != 0)
        {
            CoopIdName* name = nullptr;
            if (mCache.get(key, name))
            {
                if (!name->empty())
                {
                    foundRegular = true;
                    IdentityInfo* info = results.allocate_element();
                    results[key] = info;
                    info->setBlazeObjectId(BlazeObjectId(bobjType, key));
                    info->setIdentityName(name->c_str());
                }
            }
            else
                coopIdList.push_back(key);
        }
    }

    // IF there are keys that weren't in the cache
    if (!coopIdList.empty())
    {
        // Lookup the rest of the keys from the DB
        err = ERR_SYSTEM;
        DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
        if (conn == nullptr)
            return ERR_SYSTEM;

        // remove duplicate ids such that the second id will not be added back to cache with an empty club name
        coopIdList.erase(eastl::unique(coopIdList.begin(), coopIdList.end()), coopIdList.end());

        CoopSeasonDb coopSeasonDb;
        coopSeasonDb.setDbConnection(conn);
        CoopPairDataList coopPairDataList;

        err = coopSeasonDb.getCoopIdentities(&coopIdList, &coopPairDataList);
        if (err == Blaze::ERR_OK /*|| err == Blaze::CLUBS_ERR_INVALID_CLUB_ID*/)
        {
            // Add the entity to the cache
			Blaze::BlazeIdVector idVector;
            CoopPairDataList::const_iterator citr = coopPairDataList.begin();
            CoopPairDataList::const_iterator cend = coopPairDataList.end();
            for(; citr != cend; ++citr)
            {
                const CoopPairData *coopPair = *citr;

				idVector.push_back(coopPair->getMemberOneBlazeId());
				idVector.push_back(coopPair->getMemberTwoBlazeId());
            }

			BlazeIdToUserInfoMap userInfoMap;
			err = gUserSessionManager->lookupUserInfoByBlazeIds(idVector, userInfoMap);
			if (err == Blaze::ERR_OK)
			{
				CoopPairDataList::const_iterator cpItr = coopPairDataList.begin();
				CoopPairDataList::const_iterator cpEnd = coopPairDataList.end();
				for(; cpItr != cpEnd; ++cpItr)
				{
					const CoopPairData *coopPair = *cpItr;
					BlazeIdToUserInfoMap::const_iterator identItrA = userInfoMap.find(coopPair->getMemberOneBlazeId());
					BlazeIdToUserInfoMap::const_iterator identItrB = userInfoMap.find(coopPair->getMemberTwoBlazeId());
					if (identItrA != userInfoMap.end() && identItrB != userInfoMap.end())
					{
						UserInfoPtr identityPtrA = identItrA->second;
						UserInfoPtr identityPtrB = identItrB->second;

						eastl::string temp;
						temp.sprintf("%s + %s", identityPtrA->getIdentityName(), identityPtrB->getIdentityName());

						CoopIdName* name = BLAZE_NEW CoopIdName(temp.c_str());

						mCache.add(coopPair->getCoopId(), name);
						IdentityInfo* info = results.allocate_element();
						results[coopPair->getCoopId()] = info;
						info->setBlazeObjectId(BlazeObjectId(bobjType, coopPair->getCoopId()));
						info->setIdentityName(name->c_str());
						*eastl::find(coopIdList.begin(), coopIdList.end(), coopPair->getCoopId()) = 0;
						foundRegular = true;
					}
				}

				// push an empty club name for each non-resolved identity
				// to save us from unnecessary database trips
				CoopIdList::const_iterator cidItr = coopIdList.begin();
				while (cidItr != coopIdList.end())
				{
					EntityId key = *cidItr++;
					if (key != 0)
					{
						CoopIdName* name = BLAZE_NEW CoopIdName("");
						mCache.add(key, name);
					}
				}
			}
			else
			{
				ERR_LOG("[getIdentities:" << this << "] error lookupUsersById (" << err << ")");
			}
        }
        else 
        {
            ERR_LOG("[getIdentities:" << this << "] error accessing database (" << err << ")");
//            mComponent->mPerfEventDatabaseError.add(1);
        }
    }

    if (!foundRegular)
        err = Blaze::COOPSEASON_ERR_PAIR_NOT_FOUND;
    else
        err = Blaze::ERR_OK;
        
    return err;
}

BlazeRpcError CoopSeasonIdentityProvider::getIdentityByName(BlazeObjectType bobjType, const EntityName& entityName, IdentityInfo& result)
{
    if(bobjType != ENTITY_TYPE_COOP)
    {
        return Blaze::ERR_NOT_SUPPORTED; 
    }
	
	//split entity name
	eastl::string temp = entityName.c_str();
	int pos = temp.find("+");
	eastl::string nameA = temp.substr(0, pos -  1);
	eastl::string nameB = temp.substr(pos + 1, temp.size());
	
	Blaze::PersonaNameVector personaNameVector;
	personaNameVector.push_back(nameA.c_str());
	personaNameVector.push_back(nameB.c_str());

	PersonaNameToUserInfoMap userInfoMap;
	BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaNames(personaNameVector, userInfoMap);

	if (err == ERR_OK)
	{
		PersonaNameToUserInfoMap::const_iterator identItrA = userInfoMap.find(nameA.c_str());
		PersonaNameToUserInfoMap::const_iterator identItrB = userInfoMap.find(nameB.c_str());
		if (identItrA != userInfoMap.end() && identItrB != userInfoMap.end())
		{
			UserInfoPtr identityPtrA = identItrA->second;
			UserInfoPtr identityPtrB = identItrB->second;

			// Query the DB for the club
			DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
			if (conn == nullptr)
				return Blaze::ERR_SYSTEM;
			CoopSeasonDb coopDb;
			coopDb.setDbConnection(conn);

			CoopPairData coopPairData;
			err = coopDb.fetchCoopIdDataByBlazeIds(identityPtrA->getBlazeObjectId().id, identityPtrB->getBlazeObjectId().id, coopPairData);
			if (err == ERR_OK)
			{
				CoopIdName* name = nullptr;
				if (mCache.get(coopPairData.getCoopId(), name))
				{
					if (blaze_strcmp(name->c_str(), entityName.c_str()) != 0)
					{
						err = Blaze::ERR_ENTITY_NOT_FOUND;
					}
					else
					{
						result.setBlazeObjectId(BlazeObjectId(bobjType, coopPairData.getCoopId()));
						result.setIdentityName(name->c_str());
					}
				}
				else 
				{
					// Add the entity to the cache
					CoopIdName* idName = BLAZE_NEW CoopIdName(entityName.c_str());
					mCache.add(coopPairData.getCoopId(), idName);
					result.setBlazeObjectId(BlazeObjectId(bobjType, coopPairData.getCoopId()));
					result.setIdentityName(idName->c_str());
				}
			}
			else
			{
				err = Blaze::ERR_ENTITY_NOT_FOUND;
			}
		}
		else
		{
			err = Blaze::ERR_ENTITY_NOT_FOUND;
		}
	}
	
    return err;
}

void CoopSeasonIdentityProvider::removeCacheEntry(EntityId key)
{
    mCache.remove(key);
}

void CoopSeasonIdentityProvider::identityCacheRemoveCb(const EntityId& id, CoopIdName* name)
{
	delete name;
}
} // Clubs
} // Blaze

