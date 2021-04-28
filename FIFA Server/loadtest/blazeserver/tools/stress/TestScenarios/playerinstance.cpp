//  *************************************************************************************************
//
//   File:    playerinstance.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./playerinstance.h"
#include "./player.h"

#include "associationlists/tdf/associationlists.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "util/tdf/utiltypes.h"
#include "bytevault/tdf/bytevault.h"

using namespace Blaze::Util;

namespace Blaze {
namespace Stress {

//  modify the log identifier value (100)
PlayerInstance::PlayerInstance(StressModule* owner, StressConnection* connection, Login* login, bool isSpartaSignalMock /* = false */)
    : StressInstance(owner, connection, login, 100), mIsSpartaSignalMock(isSpartaSignalMock)
{
    mPlayer			= NULL;
    mPlayerData		= BLAZE_NEW StressPlayerInfo();
}

PlayerInstance::~PlayerInstance()
{
    if (mPlayer) {
        delete mPlayer;
    }
    delete mPlayerData;
}

void PlayerInstance::createPlayerData()
{
    //  BlazeId, personaName and Connection group Id are not available at this point.
    mPlayerData->setPlayerId(0);
    mPlayerData->setConnGroupId(BlazeObjectId(0, 0, 0));
    mPlayerData->setPersonaName("");
    mPlayerData->setIdent(getIdent());
    ComponentProxy* proxy = BLAZE_NEW ComponentProxy(getConnection());
    mPlayerData->setComponentProxy(proxy);
    mPlayerData->setConnection(getConnection());
    mPlayerData->setOwner(getOwner());
    mPlayerData->setMyPlayerInstance(this);
    mPlayerData->setLogin(getLogin());
	if (StressConfigInst.getArubaEnabled() || StressConfigInst.getRecoEnabled())
    {
        ArubaProxyHandler* arubaService = BLAZE_NEW ArubaProxyHandler(mPlayerData);
        mPlayerData->setArubaProxyHandler(arubaService);
    }
	if (StressConfigInst.getUPSEnabled())
	{
		UPSProxyHandler* upsService = BLAZE_NEW UPSProxyHandler(mPlayerData);
		mPlayerData->setUPSProxyHandler(upsService);
	}
	if (StressConfigInst.getPINEnabled())
	{
		PINProxyHandler* pinProxy = BLAZE_NEW PINProxyHandler(mPlayerData);
		mPlayerData->setPINProxyHandler(pinProxy);
	}
}

void PlayerInstance::onDisconnected()
{
    if (mPlayer != NULL)
    {
        mPlayer->deregisterHandlers();
        mPlayer->removePlayerFromList();
    }
    mPlayerData->setConnection(NULL);
    /*if (mPlayerData->getLogin() != NULL)
    {
        mPlayerData->getLogin()->resetLoggedIn();
    }*/
    mPlayerData->setReconnectAttempt(true);
    if (mPlayer)
    {
        mPlayer->onDisconnected();
    }
}

void PlayerInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (mPlayer)
    {
        mPlayer->onAsync(component, type, payload);
    }
}

void PlayerInstance::reconnectHandler()
{
    start();
}

BlazeRpcError PlayerInstance::execute()
{
    createPlayerData();
    BLAZE_INFO(BlazeRpcLog::gamemanager, "[PlayerInstance::execute][%" PRIu64 "]: lobbywait timeout %u", mPlayerData->getPlayerId(), OS_CONFIG.maxLobbyWaitTime);
	uint16_t loginCounter = 0;
    while (1)
    {
        BlazeRpcError error = ERR_OK;
        PlayerType    playerType = INVALID;

        mPlayer = PlayerFactory::createRandomPlayer(mPlayerData, &playerType);

        if (!mPlayer)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[PlayerInstance::execute:: Failed in Creating a Player Object");
            return ERR_SYSTEM;
        }
		
        if (!mPlayer->isPlayerLoggedIn())
        {
            error = mPlayer->loginPlayer();
            if (error != ERR_OK)
            {
				loginCounter++;
                BLAZE_ERR_LOG(BlazeRpcLog::util, "[PlayerInstance::execute::Login failed with error = " << ErrorHelp::getErrorName(error));
                
				//  Login got failed. Wait for reLoginWaitTime and try again
				sleep(StressConfigInst.getCommonConfig()->reLoginWaitTime);

				PlayerFactory::recyclePlayerType(playerType, mPlayerData->getPlayerPingSite());
				if (mPlayer)
				{
					delete mPlayer;
					mPlayer = NULL;
				}

				if(loginCounter > 5)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::util, "[PlayerInstance::execute:: Failed while login retry attempts");					
					return ERR_SYSTEM;
				}
				else
				{						
					continue;
				}
            }
			else // login success
			{
				mPlayer->setNewPlayerSession(true);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[PlayerInstance::execute]:PID= " << mPlayerData->getPlayerId() << " login successful ");
				if(StressConfigInst.isGeoLocation_MM_Enabled() && mPlayer->isPlayerLocationSet() == false )
				{					
					//Set GEOIPDATA of the player
					mPlayer->setPlayerCityLocation();
					//userSessionsOverrideUserGeoIPData RPC to override the location on Blaze server
					error = userSessionsOverrideUserGeoIPData(mPlayerData);
					if(error != ERR_OK) { mPlayer->setPlayerLocation(false); }
					else
					{
						//userSessionsLookupUserGeoIPData to validate if the values are updated on Blaze
						LocationInfo location;
						error = userSessionsLookupUserGeoIPData(mPlayerData, mPlayerData->getPlayerId(), location);
						if(error != ERR_OK || mPlayerData->getLocation().latitude != location.latitude || mPlayerData->getLocation().longitude != location.longitude)
						{
							mPlayer->setPlayerLocation(false); 
						}
					}
				}
			}
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[PlayerInstance::execute]:PID= " << mPlayerData->getPlayerId() << " isNewPlayerSession= " << mPlayer->isNewPlayerSession());
        }
		else
		{
			mPlayer->setNewPlayerSession(false);
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[PlayerInstance::execute]:PID= " << mPlayerData->getPlayerId() << " isNewPlayerSession= " << mPlayer->isNewPlayerSession());
		}
        //  Sleep random time
        mPlayer->sleepRandomTime(5000, 10000);
		//sleep(StressConfigInst.getCommonConfig()->loginRateTime);
        //  Add mPlayer to Global List
        mPlayer->addPlayerToList();
        //  Wait for NOTIFY_USERAUTHENTICATED to read mPlayer data
        //sleep(10000);
        //  simulate mPlayer load
        error = mPlayer->simulateLoad();
        //  Based on the probability user logs out from server
        mPlayer->logoutPlayer();
        //  Remove mPlayer from Global List
        mPlayer->removePlayerFromList();
        //  unsubscribe from notifications
        mPlayer->deregisterHandlers();
        //  Wait for some time to drain out any notifications in queue
        mPlayer->sleep(OS_CONFIG.maxLobbyWaitTime);

        if (error == ERR_DISCONNECTED)
        {
            //  To throttle the connections
            mPlayer->sleepRandomTime(OS_CONFIG.maxLobbyWaitTime, OS_CONFIG.maxLobbyWaitTime * 2);
			if (mPlayerData->getLogin() != NULL)
			{
				mPlayerData->getLogin()->resetLoggedIn();
			}
        }

        //  Done with the mPlayer, Delete mPlayer instance
        if (mPlayer) 
		{
            delete mPlayer;
            mPlayer = NULL;
        }
        PlayerFactory::recyclePlayerType(playerType, mPlayerData->getPlayerPingSite());
        if (StressInstance::getConnection() == NULL || !getConnection()->connected())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[PlayerInstance::execute::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_SYSTEM;
        }
    }
    return 0;
}

}  // namespace Stress
}  // namespace Blaze
