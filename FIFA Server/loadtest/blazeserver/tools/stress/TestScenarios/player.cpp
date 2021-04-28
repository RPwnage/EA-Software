//  *************************************************************************************************
//
//   File:    player.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./player.h"
#include "stats/tdf/stats.h"

namespace Blaze {
namespace Stress {
using namespace Util;
using namespace Stats;

Player::Player(StressPlayerInfo* playerData, ClientType type)
    : mPlayerInfo(playerData), mProcessInviteFiberExited(true), mDestructing(false), mLocationSet(false), mClientType(type), mIsNewPlayerSession(false) 
{
    
    mIsNewPlayerSession = false;
    //   Add async handlers
    if (mPlayerInfo->isConnected())
    {
        mPlayerInfo->getConnection()->addAsyncHandler(UserSessionsSlave::COMPONENT_ID, UserSessionsSlave::NOTIFY_USERAUTHENTICATED, mPlayerInfo->getMyPlayerInstance());
		mPlayerInfo->getConnection()->addAsyncHandler(GameReporting::GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION, mPlayerInfo->getMyPlayerInstance());
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Async handlers are not registered as there is no connection to server for the player: " << mPlayerInfo->getPlayerId());
    }
}

Player::~Player()
{
    mDestructing = true;
    if (mPlayerInfo->isConnected())
    {
        mPlayerInfo->getConnection()->removeAsyncHandler(UserSessionsSlave::COMPONENT_ID, UserSessionsSlave::NOTIFY_USERAUTHENTICATED, mPlayerInfo->getMyPlayerInstance());
		mPlayerInfo->getConnection()->removeAsyncHandler(GameReporting::GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION, mPlayerInfo->getMyPlayerInstance());
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Async handlers are not de-registered as there is no connection to server for the player: " << mPlayerInfo->getPlayerId());
    }
    GameplayConfig* ConfigPtr = StressConfigInst.getCommonConfig();
    if (ConfigPtr)
    {
        mPlayerInfo->incrementGamesPerSessionCount();
        if (mPlayerInfo->getGamesPerSessionCount() > ConfigPtr->maxGamesPerSession) {
            mPlayerInfo->setGamesPerSessionCount(0);
        }
    }
    //  wait for the invitesProcessing fiber to quit.
    int loops = 0;
    while (!(mProcessInviteFiberExited))
    {
        sleep(StressConfigInst.getCommonConfig()->processInvitesFiberSleep / 2);
        if (loops++ > 5)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "waited too long for invitesProcessing fiber to quit for the player: " << mPlayerInfo->getPlayerId());
        }
    }
    mPlayerInfo = NULL;
}

void Player::sleep(uint32_t timeInMs)
{
    BlazeRpcError err = gSelector->sleep(timeInMs * 1000, (TimerId*)NULL, &mSleepEvent);
    mSleepEvent.reset();
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Player::sleep()]: failed to sleep.");
    }
}

void Player::sleepRandomTime(uint32_t minMs, uint32_t maxMs)
{
    int32_t sleepTime = minMs + Random::getRandomNumber(maxMs - minMs) + 1000;
    sleep(sleepTime);
}

void Player::wakeFromSleep()
{
    if (mSleepEvent.isValid()) {
        Fiber::signal(mSleepEvent, ERR_OK);
    }
}

BlazeRpcError Player::preAuth()
{
    Util::PreAuthRequest    request;
    Util::PreAuthResponse   response;
	if (PLATFORM_XONE == StressConfigInst.getPlatform())
	{
		request.getClientInfo().setPlatform(xone);
	}
	if (PLATFORM_PS4 == StressConfigInst.getPlatform())
	{
		request.getClientInfo().setPlatform(ps4);
	}
	request.getClientData().setCountry(21843);
    request.getClientData().setIgnoreInactivityTimeout(false);
    request.getClientData().setLocale(mPlayerInfo->getOwner()->getLocale());
    request.getClientData().setServiceName(mPlayerInfo->getLogin()->getOwner()->getServiceName());
    request.getClientData().setClientType(getClientType());
    request.getFetchClientConfig().setConfigSection(mPlayerInfo->getLogin()->getOwner()->getPreAuthClientConfig());
	request.setLocalAddress(356125194);

	ClientInfo & clientInfo = request.getClientInfo();
	if (StressConfigInst.getPlatform() == PLATFORM_PC)
	{
		clientInfo.setPlatform(pc);
	}

	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		clientInfo.setPlatform(ps4);
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		clientInfo.setPlatform(xone);
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS5)
	{
		clientInfo.setPlatform(ps5);
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_XBSX)
	{
		clientInfo.setPlatform(xbsx);
	}
    BlazeRpcError err = mPlayerInfo->getComponentProxy()->mUtilProxy->preAuth(request, response);
	//char8_t buff[20480];
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "preauth platform type:" << "\n" << request.print(buff, sizeof(buff)));

    if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[Player::preAuth: preAuth() failed with error = " << ErrorHelp::getErrorName(err));
    }
    return err;
}

BlazeRpcError Player::postAuth()
{
    Util::PostAuthRequest   postAuthReq;
    Util::PostAuthResponse  postAuthResp;
    //  postAuthReq.setMacAddress("78:2B:CB:0B:27:94");
	//eastl::string uniqueDeviceId = "";
	//uniqueDeviceId.sprintf("%" PRId64 "%s", mPlayerInfo->getPlayerId());
	//postAuthReq.setUniqueDeviceId(uniqueDeviceId.c_str());
	postAuthReq.setUniqueDeviceId("");
    BlazeRpcError err = mPlayerInfo->getComponentProxy()->mUtilProxy->postAuth(postAuthReq, postAuthResp);
    if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[Player::postAuth failed with error = " << ErrorHelp::getErrorName(err));
    }
    return err;
}

void Player::addPlayerToList()
{
    ((PlayerModule*)mPlayerInfo->getOwner())->addPlayerToCommonMap(mPlayerInfo);
    Selector::FiberCallParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &Player:: invitesProcessingFiber, "Player::ProcessInvitesFiber()", params);
}

void Player::removePlayerFromList()
{
    ((PlayerModule*)mPlayerInfo->getOwner())->removePlayerFromCommonMap(mPlayerInfo);
}

//  This function is rewritten from Login::login() function
BlazeRpcError Player::loginPlayer()
{
    BlazeRpcError err = ERR_OK;
    if (isPlayerLoggedIn())
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "User previously logged in " << mPlayerInfo->getPlayerId());
        return ERR_DUPLICATE_LOGIN;
    }
    if (StressConfigInst.IsIncrementalLoginEnabled())
    {
        if (mPlayerInfo->getTotalLogins() >= StressConfigInst.getMaxIncrementalLoginsPerConnection()) {
            return ERR_SYSTEM;    //  reached maximum logins, just return
        }
    }
    err = preAuth();
    if (err != ERR_OK) {
        return err;
    }
    mPlayerInfo->getOwner()->incPendingLogins();
	
	err = preAuthRPCcalls();
	//LOGIN
    err = mPlayerInfo->getLogin()->login();
    //   notify derived object via virtual call
    onLogin(err);
    if (err == ERR_OK)
    {
        mPlayerInfo->setLogInTime();
        mPlayerInfo->setGamesPerSessionCount(0);
        mPlayerInfo->getOwner()->incSucceededLogins();
        mPlayerInfo->IncrementLoginCount();
    }
    else
    {
        mPlayerInfo->getOwner()->incFailedLogins();
        return err;
    }
    //  PostAuth calls
    err = postAuth();
    return err;
}

BlazeRpcError Player::preAuthRPCcalls()
{
	BlazeRpcError err = ERR_OK;
    FetchConfigResponse response;
	//err = fetchClientConfig(mPlayerInfo, "OSDK_ROSTER", response);
	err = fetchClientConfig(mPlayerInfo, "OSDK_CORE", response);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[Player::preAuthRPCcalls: fetchClientConfig() failed with error = " << ErrorHelp::getErrorName(err));
    }
	return err;
}

BlazeRpcError Player::logoutPlayer()
{
    BlazeRpcError err = ERR_OK;
    //  logout based on the logout probability defined in Gameplay Config and always logout if
    //  player has been logged in for longer than a max certain period
    uint32_t probLogout = StressConfigInst.getLogoutProbability(getPlayerType());
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "Player::logoutPlayer:LogoutProbability= " << probLogout << " for playerType= " << getPlayerType() << " " << mPlayerInfo->getPlayerId());
    if ((Random::getRandomNumber(100) < (int)probLogout) || mPlayerInfo->isLoggedInForTooLong())
    {
        err = mPlayerInfo->getLogin()->logout();
        //   notify derived object via virtual call
        onLogout(err);
        if (err == ERR_OK)
        {
            mPlayerInfo->getOwner()->incSucceededLogouts();
            mPlayerInfo->resetLogInTime();

            if (StressConfigInst.IsIncrementalLoginEnabled())  //  if incremetnal login enabled, increment and set login id, which can be used for the next login
            {
                uint64_t nucId = mPlayerInfo->getLogin()->getStressLoginRequest()->getAccountId();
                uint64_t NewNucId = nucId + StressConfigInst.getIncrementalLoginIdDelta();
                char8_t emailBuf[256];
                char8_t personaBuf[256];
                blaze_snzprintf(emailBuf, sizeof(emailBuf), StressConfigInst.getPlayerEmailFormat(), (NewNucId - StressConfigInst.getBaseAccountId()));
                blaze_snzprintf(personaBuf, sizeof(personaBuf), StressConfigInst.getPlayerPersonaFormat(), (NewNucId - StressConfigInst.getBaseAccountId()));
                mPlayerInfo->getLogin()->getStressLoginRequest()->setEmail(emailBuf);
                mPlayerInfo->getLogin()->getStressLoginRequest()->setPersonaName(personaBuf);
                mPlayerInfo->getLogin()->getStressLoginRequest()->setAccountId(NewNucId);
            }
        }
        else
        {
            mPlayerInfo->getOwner()->incFailedLogouts();
        }
    }
    else
    {
        err = ERR_CANCELED;
    }
    return err;
}


//   This function would be called from StressInstance::login()
void Player::onLogin(BlazeRpcError result)
{
    if (result != ERR_OK)
    {
        if (AUTH_ERR_EXISTS == result)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "::onLogin: login failed. Error = %s", ErrorHelp::getErrorName(result));
        }
        return;
    }
    mPlayerInfo->setViewId(0);
}

//   creates a new login object with new start index
//   which effectively creates a new player and this player disappears.
void Player::renewPlayer()
{
    LoginManager* loginManager = mPlayerInfo->getLogin()->getOwner();
    if (!(loginManager->getPsu()))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Player::renewPlayer]: PSU not given in arguments. can not renew the player");
        return;
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Player::renewPlayer]:player " << mPlayerInfo->getPlayerId() << " getting renewed");
    //  calls LoginManager::getLogin() with new startIndex..
    Login* login = NULL;
    Login* tmpLogin = mPlayerInfo->getLogin();
    tmpLogin->incrementMyStartIndex();
    if (!(mPlayerInfo->getConnection()))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Player::renewPlayer]:player " << mPlayerInfo->getPlayerId() << "got disconnected.");
        return;
    }
    //   create a new login object with incremented startIndex. so it will be a completely new player object.
    login = loginManager->getLogin(mPlayerInfo->getConnection(), mPlayerInfo->getOwner()->isStressLogin(), tmpLogin->getMyStartIndex());
    mPlayerInfo->setLogin(login);
    delete tmpLogin;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Player::renewPlayer]:player " << mPlayerInfo->getPlayerId() << " got renewed");
}

//   This function would be called from StressInstance::logout()
void Player::onLogout(BlazeRpcError result)
{
    //renewPlayer();
    removePlayerFromList();
}
//  TODO
void Player::onDisconnected()
{
    removePlayerFromList();
}

void Player::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    Heat2Decoder decoder;
    //if (UserSessionsSlave::COMPONENT_ID != component)
    //{
    //    //   payload not handled by recipient.
    //    BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[Player::onAsync]: Payload not handled by us, deleting it.");
    //    return;
    //}
    switch (type)
    {
        case UserSessionsSlave::NOTIFY_USERAUTHENTICATED:
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[Player::onAsync]: received NOTIFY_USERAUTHENTICATED.");
                UserSessionLoginInfo notification;
                if (decoder.decode(*payload, notification) == ERR_OK)
                {
                    //  Read player data
                    mPlayerInfo->setPlayerId(notification.getBlazeId());
                    mPlayerInfo->setConnGroupId(notification.getConnectionGroupObjectId());
                    mPlayerInfo->setPersonaName(notification.getDisplayName());
                    mPlayerInfo->setExternalId(notification.getExtId());
					mPlayerInfo->setAccountId(notification.getAccountId());
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "Heat2Decoder failed to decode notification type UserAuthenticated in component UserSessions");
                }
                break;
            }
		case GameReportingSlave::NOTIFY_RESULTNOTIFICATION:
		{
			ResultNotification* notify = BLAZE_NEW ResultNotification;
			if (decoder.decode(*payload, *notify) != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "GameSession[" << mPlayerInfo->getPlayerId() << "]: Failed to decode");
				delete notify;
			}
			else
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameSession::onAsync]: Received  NOTIFY_RESULTNOTIFICATION." << mPlayerInfo->getPlayerId() << " GameReportingId=" << notify->getGameReportingId());
				//notification = notify;
			}
		}
		break;
        default:
            {
                //   payload not handled by recipient.
                BLAZE_TRACE_LOG(BlazeRpcLog::util, "[Player::onAsync]: Invalid notification.");
                break;
            }
    }
}

void Player:: invitesProcessingFiber()
{
    //   this fiber will handle a request to this player from another player(from same exe). requests could be a friend Request
    //   , coopgame request , party request or similar stuff where two players need to communicate offline(through first party or through origin).
    //   inviter will get this players details from commonplayerDetails map and set the invite. this fiber will call the corresponding handler
    //   after detecting an invite.
    if (!(mDestructing))
    {
        PlayerDetails* myDetails = NULL;
        mProcessInviteFiberExited = false;
        myDetails = mPlayerInfo->getMyDetailsfromCommonMap();
        if (myDetails)
        {
            processInvites(myDetails);
        }
        else
        {
            mProcessInviteFiberExited = true;
            return;
        }
        if (StressConfigInst.getCommonConfig()->processInvitesFiberSleep > 0)
        {
            int32_t delay = StressConfigInst.getCommonConfig()->processInvitesFiberSleep;
            TimeValue now = TimeValue::getTimeOfDay();
            TimeValue nextCycle = now + delay * 1000;
            gSelector->scheduleFiberTimerCall(nextCycle, this, &Player:: invitesProcessingFiber, "Player::invitesProcessingFiber()", Fiber::CreateParams(Fiber::STACK_SMALL));
        }
        else
        {
            return;
        }
    }
    else
    {
        mProcessInviteFiberExited = true;
        return;
    }
}

void Player::processInvites(PlayerDetails* myDetails)
{
}


void Player::setPlayerCityLocation()
{
	CityInfoMap& cityMap = ((PlayerModule*)mPlayerInfo->getOwner())->mCityInfoMap;
	CityInfoMap::iterator& cityIter = ((PlayerModule*)mPlayerInfo->getOwner())->mItrator;
	if(cityIter == cityMap.end())
	{
		cityIter = cityMap.begin();
	}
	LocationInfo locInfo =  cityIter->second;
	cityIter++;
	//Add location in player data
	mPlayerInfo->setLocation(locInfo);
	setPlayerLocation(true);

	//Add to global map
	((PlayerModule*)mPlayerInfo->getOwner())->addPlayerLocationDataToMap(mPlayerInfo, locInfo);


	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Player::setPlayerCityLocation]: latitude= " << mPlayerInfo->getLocation().latitude << " longitude= " <<mPlayerInfo->getLocation().longitude );
	return ;
}

}  // namespace Stress
}  // namespace Blaze
