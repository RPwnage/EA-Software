//  *************************************************************************************************
//
//   File:    playermodule.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include <fstream>
#include <string>

#include "framework/blaze.h"
#include "./playermodule.h"


namespace Blaze {
namespace Stress {


class PlayerInstance;


StressModule* PlayerModule::create()
{
    return BLAZE_NEW PlayerModule();
}

PlayerModule::~PlayerModule()
{
}

bool PlayerModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    bool ret = StressModule::initialize(config);
    if (ret)
    {
        ret = StressConfigInst.initialize(config, this->getTotalConnections());
    }
    if (ret)
    {
        mSignalUserIndex = (int32_t)(rand() % getTotalConnections()) + 1;    // +1 since we use pre-decrement operator --mBattlelogUserIndex in the validation check.
    }
	/*if (ret)
	{
		ret = readPlayerGeoIPDataInfo();
	}*/
    return ret;
}

StressInstance* PlayerModule::createInstance(StressConnection* connection, Login* login)
{
    
    //Do not send login here.
    StressInstance* instance = BLAZE_NEW PlayerInstance(this, connection, login);
    return instance;
}

PlayerDetails*  PlayerModule::findPlayerInCommonMap(BlazeId playerId)
{
    PlayerDetailsMap::iterator thisPlayer = mPlayerDetailsMap.find(playerId);
    if (thisPlayer != mPlayerDetailsMap.end())
    {
        return thisPlayer->second.getPlayerDetails();
    }
    else
    {
        return NULL;
    }
}

void PlayerModule::addPlayerToCommonMap(StressPlayerInfo *playerInfo)
{
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[player::addPlayerToList]: player " << playerInfo->getPlayerId());
    mAllPlayerIds.insert(playerInfo->getPlayerId());
    //  if this is the first time this player has appeared in this exe then his details will be added to the map
    //   other wise we'll just reset few variables and set hime alive.
    PlayerDetailsMap::iterator thisPlayer = mPlayerDetailsMap.find(playerInfo->getPlayerId());
    if (thisPlayer != mPlayerDetailsMap.end())
    {
        thisPlayer->second.setPlayerAlive(playerInfo);
    }
    else
    {
        PlayerDetails playerDetails(playerInfo);
        thisPlayer = mPlayerDetailsMap.insert(eastl::pair<PlayerId, PlayerDetails>(playerInfo->getPlayerId(), playerDetails)).first;  //  [mPlayerInfo->getPlayerId()] = playerDetails;
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[player::addPlayerToList]: player accountID" << thisPlayer->second.getAccountId());
		eslAccountList.push_back(thisPlayer->second.getAccountId());
		playerAccountMap.insert(eastl::pair<BlazeId, BlazeId> (playerInfo->getPlayerId(), thisPlayer->second.getAccountId()));
    }
    playerInfo->setMyDetailsInCommonMap(thisPlayer->second.getPlayerDetails());
}

void PlayerModule::removePlayerFromCommonMap(StressPlayerInfo *playerInfo)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[player::removePlayerFromCommonMap]: player " << playerInfo->getPlayerId());
    PlayerDetailsMap::iterator thisPlayer = mPlayerDetailsMap.find(playerInfo->getPlayerId());
    if (thisPlayer != mPlayerDetailsMap.end())
    {
        thisPlayer->second.setPlayerDead();
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[player::removePlayerFromCommonMap]: player " << playerInfo->getPlayerId() << " is not present in playerDetailsMap");
    }
    playerInfo->setMyDetailsInCommonMap(NULL);
}


PlayerId PlayerModule::getMultiplayerPartyInvite()
{
    PlayerId invitesId = 0;
    mMutex.Lock();
    PlayerIdList::iterator it = mMultiplayerInvites.begin();
    if (it != mMultiplayerInvites.end())
    {
        invitesId = *it;
        mMultiplayerInvites.erase(it);
    }
    mMutex.Unlock();
    return invitesId;
}

void PlayerModule::addMultiplayerPartyInvite(PlayerId invitesId, int32_t numInvites)
{
    mMutex.Lock();
    for (int32_t i = 0; i < numInvites; i++)
    {
        mMultiplayerInvites.push_back(invitesId);
    }
    mMutex.Unlock();
}

void PlayerModule::deleteMultiplayerPartyInvite(PlayerId invitesId)
{
    mMutex.Lock();
    //   observed invalid position assert while debugging if erase done in a single loop
    while (true)
    {
        bool found = false;
        PlayerIdList::iterator start = mMultiplayerInvites.begin();
        for (; start != mMultiplayerInvites.end(); ++start)
        {
            if (invitesId == *start )
            {
                mMultiplayerInvites.erase(start);
                found = true;
                break;
            }
        }
        if (!found)
        {
            break;
        }
    }
    mMutex.Unlock();
}

PlayerId PlayerModule::getRandomPlayerId()
{
    if (mAllPlayerIds.size())
    {
        return(mAllPlayerIds[Random::getRandomNumber(mAllPlayerIds.size())]);
    }
    else
    {
        BLAZE_ERR(BlazeRpcLog::usersessions, "[getRandomPlayerId] player set is empty.");
        return 0;
    }
}
PlayerDetails* PlayerModule::getRandomPlayerFromCommonMap()
{
    PlayerDetails *playerDetails = NULL;
    for (int i = 0; i < 5; i++)
    {
        PlayerId randplayer = getRandomPlayerId();
        if (randplayer)
        {
            playerDetails = findPlayerInCommonMap(randplayer);
            if (playerDetails)
            {
                break;
            }
            else
            {
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[player::getRandomPlayerFromCommonMap]: player " << randplayer << " is not present in playerDetailsMap");
            }
        }
    }
    return playerDetails;
}

void PlayerModule::addPlayerLocationDataToMap(StressPlayerInfo *playerInfo, LocationInfo location)
{
	PlayerLocationMap::iterator it = mPlayerLocationMap.find(playerInfo->getPlayerId());
	if( it != mPlayerLocationMap.end())
	{
		it->second = location;
	}
	else
	{
		mPlayerLocationMap.insert(eastl::make_pair(playerInfo->getPlayerId(),location));
	}
}

bool PlayerModule::getPlayerLocationDataFromMap(EntityId playerId, LocationInfo& location)
{
	PlayerLocationMap::iterator it = mPlayerLocationMap.find(playerId);
	if( it == mPlayerLocationMap.end())
	{
		return false;		
	}
	else
	{
		location = it->second;
	}
	return true;
}

bool PlayerModule::readPlayerGeoIPDataInfo()
{
	//char8_t	filepath[64];
	//filepath[0] = '\0';		
	//const char8_t* geoipfilepath	=  StressConfigInst.geoipfilepath;
	//blaze_strnzcpy(filepath,geoipfilepath,sizeof(filepath));
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfigManager::parsePlayerGeoIPDataConfig] city location file=" << StressConfigInst.getPlayerGeoIPFilePath().c_str());

	//Read location data from the file US.csv
 	std::ifstream locationInformation(StressConfigInst.getPlayerGeoIPFilePath().c_str());

	if(locationInformation.is_open())
	{
		std::string line;
		while(std::getline(locationInformation,line))
		{
			eastl::string str = line.c_str();
			if (str.length() <= 0)
			{
				continue;
			}
		    char *token = NULL ;
			char * value ;
		    token = blaze_strtok (const_cast<char *>(str.c_str()),",", &value);
		    //int counter = 0 ;
			LocationInfo  info;
			// process each token
			info.latitude = atof(blaze_strtok(NULL, ",", &value));
			info.longitude = atof(blaze_strtok(NULL, ",", &value));
			mCityInfoMap.insert(eastl::make_pair(token,info));
			line.clear();
		}		
		BLAZE_TRACE_LOG(Log::SYSTEM, "PlayerModule::readPlayerGeoIPDataInfo - Size of location map : " << mCityInfoMap.size() << ".");
		return true;
	}
	BLAZE_ERR_LOG(Log::SYSTEM, "PlayerModule::readPlayerGeoIPDataInfo - Unable to read file US.csv ");		
	return false;
}

}  // namespace Stress
}  // namespace Blaze
