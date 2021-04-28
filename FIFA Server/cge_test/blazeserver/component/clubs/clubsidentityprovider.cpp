/*************************************************************************************************/
/*!
    \file   clubsidentityprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ClubsIdentityProvider

    This class implements the IdentityProvider interface and provides a mechanism to do identity
    lookups for clubs.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/clubsidentityprovider.h"
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubsdb.h"

namespace Blaze
{
namespace Clubs
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


#define CONVERT_FUNCTOR(X, TYPE) (*((TYPE *) ((FunctorBase *) &X))); 


/*** Public Methods ******************************************************************************/

ClubsIdentityProvider::ClubsIdentityProvider(ClubsSlaveImpl* component)
    : mComponent(component),
      mCache(16384, BlazeStlAllocator("ClubsIdentityProvider::mCache", ClubsSlave::COMPONENT_MEMORY_GROUP), IdentityCache::RemoveCb(&ClubsIdentityProvider::identityCacheRemoveCb))
{
}

BlazeRpcError ClubsIdentityProvider::getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results)
{
    if(bobjType != ENTITY_TYPE_CLUB)
    {
        return ERR_SYSTEM; // TODO: log error
    }

    results.clear();
    if (keys.empty())
    {
        return ERR_OK;
    }

    BlazeRpcError err = ERR_OK;
    ClubIdList clubIdList;

    bool foundRegular = false;
    results.reserve(keys.size());
    
    // Pull as many entries out of the cache as possible
    EntityIdList::const_iterator itr = keys.begin();
    while (itr != keys.end())
    {
        EntityId key = *itr++;
        if (key != 0)
        {
            ClubName* name = nullptr;
            if (mCache.get(key, name))
            {
                if (!name->empty())
                {
                    foundRegular = true;
                    IdentityInfo* info = results.allocate_element();
                    results[key] = info;
                    info->setBlazeObjectId(EA::TDF::ObjectId(bobjType, key));
                    info->setIdentityName(name->c_str());
                }
            }
            else
                clubIdList.push_back(key);
        }
    }

    // IF there are keys that weren't in the cache
    if (!clubIdList.empty())
    {
        // Lookup the rest of the keys from the DB
        err = ERR_SYSTEM;
        DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
        if (conn == nullptr)
            return ERR_SYSTEM;

        ClubsDatabase clubsDb;
        clubsDb.setDbConn(conn);
        ClubList resolvedClubList;

        err = clubsDb.getClubIdentities(&clubIdList, &resolvedClubList);
        if (err == Blaze::ERR_OK || err == Blaze::CLUBS_ERR_INVALID_CLUB_ID)
        {
            // getClubIdentities() returns resolvedClubList which contains all the resolved ones, add them to the results map as well as the cache
            ClubList::const_iterator citr = resolvedClubList.begin();
            ClubList::const_iterator cend = resolvedClubList.end();
            for(; citr != cend; ++citr)
            {
                const Club *club = *citr;
                ClubName* name = BLAZE_NEW ClubName(club->getName());
                mCache.add(club->getClubId(), name);
                IdentityInfo* info = results.allocate_element();
                results[club->getClubId()] = info;
                info->setBlazeObjectId(EA::TDF::ObjectId(bobjType, club->getClubId()));
                info->setIdentityName(name->c_str());
                foundRegular = true;
            }

            // Now, the results map only contains the resolved ones. Use it to find the non-resolved ones and add them to the cache as well to save us from unnecessary database trips.
            // Iterate over clubIdList and for entries that don't exist in the results map, insert into the cache with an empty club name.
            IdentityInfoByEntityIdMap::const_iterator resultsEnd = results.end();
            ClubIdList::const_iterator cidItr = clubIdList.begin();
            ClubIdList::const_iterator cidItrEnd = clubIdList.end();
            for (; cidItr != cidItrEnd; ++cidItr)
            {
                const EntityId key = *cidItr;

                // if the clubId key doesn't exist in the results map, it's a non-resolved one
                if (results.find(key) == resultsEnd)
                {
                    ClubName* name = BLAZE_NEW ClubName("");
                    mCache.add(key, name);
                }
            }
        }
        else 
        {
            ERR_LOG("[getIdentities] error accessing database (" << ErrorHelp::getErrorName(err) << ")");
        }
    }

    if (!foundRegular)
        err = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
    else
        err = Blaze::ERR_OK;
        
    return err;
}

BlazeRpcError ClubsIdentityProvider::getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& entityName, IdentityInfo& result)
{
    if(bobjType != ENTITY_TYPE_CLUB)
    {
        return Blaze::ERR_NOT_SUPPORTED; 
    }

    // Query the DB for the club
    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn == nullptr)
        return Blaze::ERR_SYSTEM;
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(conn);
    Club club;

    BlazeRpcError error = clubsDb.getClubIdentityByName(entityName.c_str(), &club);
    if (error == Blaze::ERR_OK)
    {
        ClubName* name = nullptr;
        if (mCache.get(club.getClubId(), name))
        {
            if (blaze_strcmp(name->c_str(), entityName.c_str()) != 0)
            {
                error = Blaze::ERR_ENTITY_NOT_FOUND;
            }
            else
            {
                result.setBlazeObjectId(EA::TDF::ObjectId(bobjType, club.getClubId()));
                result.setIdentityName(name->c_str());
            }
        }
        else 
        {
            // Add the entity to the cache
            ClubName* nameTemp = BLAZE_NEW ClubName(club.getName());
            mCache.add(club.getClubId(), nameTemp);
            result.setBlazeObjectId(EA::TDF::ObjectId(bobjType, club.getClubId()));
            result.setIdentityName(nameTemp->c_str());
        }
    }
    else if (error == Blaze::CLUBS_ERR_INVALID_ARGUMENT)
        error = Blaze::ERR_ENTITY_NOT_FOUND;

    return error;
}

void ClubsIdentityProvider::removeCacheEntry(EntityId key)
{
    mCache.remove(key);
}

void ClubsIdentityProvider::identityCacheRemoveCb(const EntityId& id, ClubName* club)
{
    delete club;
}

} // Clubs
} // Blaze

