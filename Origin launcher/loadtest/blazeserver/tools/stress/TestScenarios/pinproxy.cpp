//  *************************************************************************************************
//
//   File:    pin.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "pinproxy.h"

namespace Blaze {
	namespace Stress {
		//  *************************************************************************************************
		//  PIN Stress code 
		//  *************************************************************************************************
		PINProxy::PINProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
		{
			mHttpPINProxyConnMgr = NULL;
			mGovId = "";
			mTrackingtaglist = "";
			initializeConnections();
		}

		PINProxy::~PINProxy()
		{
			if (mHttpPINProxyConnMgr != NULL)
			{
				delete mHttpPINProxyConnMgr;
			}
		}

		bool PINProxy::initializeConnections()
		{
			if (initPINProxyHTTPConnection())
			{
				return true;
			}
			return false;
		}

		bool PINProxy::initPINProxyHTTPConnection()
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::initPINProxyHTTPConnection]: Initializing the HttpConnectionManager");
			mHttpPINProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2019-ps4-lt");
			if (mHttpPINProxyConnMgr == NULL)
			{
				BLAZE_ERR(BlazeRpcLog::usersessions, "[PINProxy::initPINProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
				return false;
			}
			const char8_t* serviceHostname = NULL;
			bool serviceSecure = false;

			HttpProtocolUtil::getHostnameFromConfig(StressConfigInst.getPINServerUri(), serviceHostname, serviceSecure);
			//HttpProtocolUtil::getHostnameFromConfig("https://pin-em-lt.data.ea.com", serviceHostname, serviceSecure);
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::initPINProxyHTTPConnection]: PINServerUri= " << StressConfigInst.getPINServerUri());

			InetAddress* inetAddress;
			uint16_t portNumber = 443;
			bool8_t	isSecure = true;
			inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
			if (inetAddress != NULL)
			{
				mHttpPINProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::initPINProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
				return false;
			}
			return true;
		}

		void PINProxyHandler::simulatePINLoad()
		{
			bool mSuccess = false;
			//triggerids - fifalivemsg (50%), fifahubgen4 (50%) 
			//triggerids - futhubgen4 (50%), fifahubgen4 (50%) - new values
			mSuccess = getPINMessage();
			if (!mSuccess)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[PINProxyHandler::simulatePINLoad]: [" << mPlayerData->getPlayerId() << "]: getPINMessage failed.");
			}
			else
			{
				mSuccess = sendTracking();
				if (!mSuccess)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[PINProxyHandler::simulatePINLoad]: [" << mPlayerData->getPlayerId() << "]: sendTracking failed.");
				}
			}
			reset();
		}

		bool PINProxy::getMessage(eastl::string pinEvent)
		{
			bool result = false;
			time_t now;
			time(&now);
			char buf[sizeof "2012-05-24T10:00:23Z"];
			strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));

			PINHttpResult httpResult;
			eastl::string jsonData;
			const char8_t *httpHeaders[4];

			/*httpHeaders[0] = "x-ea-game-id:FIFA19XBO";
			httpHeaders[1] = "x-ea-game-id-type:easku";
			httpHeaders[2] = "x-ea-taxv:2.0";
			httpHeaders[3] = "x-ea-env:test";
			httpHeaders[4] = "x-ea-lint-level:2";*/

			httpHeaders[0] = "x-ea-game-id-type:server";
			httpHeaders[1] = "x-ea-taxv:1.1";
			httpHeaders[2] = "x-ea-env:prod";
			httpHeaders[3] = "x-ea-game-id:ARUBA";
			//httpHeaders[4] = "x-ea-lint-level:2";

			/*{"clientid":"fifa18","plat":"ps4","tid":"312054","tidt":"projectId","pidm":{"personaid":"1000170900231"},"age":"0","language":"en","country":"US","triggerids":["fifahubgen4"]}*/
			//jsonData.append("{\"clientid\":\"fifa18\",\"plat\":\"ps4\",\"tid\":\"312054\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"1000170900231\"},\"age\":\"0\",\"language\":\"en\",\"country\":\"US\",\"triggerids\":[\"fifahubgen4\"]}");
			//fifa18_ltc
			//fifa18_lta
			//fifa19_test
			jsonData.append("{\"taxv\":\"2.0\",\"sdkt\":\"telemetry\",\"tid\":\"");
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				jsonData.append("FFA19XBO");
			}
			else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				jsonData.append("ps4");
			}
			jsonData.append("\",\"tidt\":\"easku\",\"rel\":\"test\",\"v\":\"1.3425\",\"sdkv\":\"1.2.3\",\"ts_post\":\"");
			jsonData.append(buf);
			jsonData.append("\",\"sid\":\"3F2504E0-4F89-11D3-9A0C-0305E82C3301\",\"plat\":\"");
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				jsonData.append("xbox_one");
			}
			else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				jsonData.append("ps4");
			}
			jsonData.append("\",\"et\":\"client\",\"clientIP\":\"192.30.18.3\",\"mac\":\"23432345457\",\"didm\":{\"idfa\":\"69BD889F-13D0-4EBB-B9E7-5C6C9C174E1D\",\"idfv\":\"599F9C00-92DC-4B5C-9464-7971F01F8370\"},\"type\":\"online\",\"loc\":\"en_US\",\"custom\":{\"jflag\":\"1\",\"pflag\":\"0\",\"carrier\":\"302220\",\"networkAccess\":\"W\",\"hwID\":\"2757\",\"end_reason\":\"quit\",\"teams_stats\":[{\"Fact.BLOCKS_SUCCESSFULL\":\"2\", \"Fact.CLEARANCES\":\"1\", \"Fact.CORNERS\":\"1\", \"Fact.CROSSES_ATTEMPTED\":\"1\", \"Fact.CROSSES_COMPLETED\":\"4\", \"Fact.DISTANCE_DRIBBLED\":\"3\", \"Fact.FOULS\":\"2\", \"Fact.FOULS_SUFFERED\":\"1\", \"Fact.FREE_KICKS_TAKEN\":\"1\", \"Fact.GOALS\":\"4\", \"Fact.GOALS_CONCEDED\":\"2\", \"Fact.GOAL_KICKS\":\"1\", \"Fact.IS_SEVERELY_INJURED\":\"1\", \"Fact.OFFSIDES\":\"1\", \"Fact.OWN_GOALS\":\"1\", \"Fact.PASSES_COMPLETED\":\"1\", \"Fact.PASS_ATTEMPTS\":\"1\", \"Fact.PENALTIES_ATTEMPTS\":\"1\", \"Fact.PENALTY_GOALS\":\"3\", \"Fact.RED_CARDS\":\"4\", \"Fact.SAVES\":\"5\", \"Fact.SHOOT_OUT_ATTEMPTS\":\"6\", \"Fact.SHOOT_OUT_GOALS\":\"5\", \"Fact.SHOTS_ON_GOAL\":\"4\", \"Fact.SHOT_ATTEMPTS\":\"2\", \"Fact.TACKLES\":\"3\", \"Fact.THROW_INS\":\"2\", \"Fact.TIME_IN_POSSESSION\":\"2\", \"Fact.YELLOW_CARDS\":\"2\", \"Setting.dbid\":\"112977\", \"Setting.difficulty\":\"1\", \"Setting.halfLen\":\"6\", \"Setting.isCPU\":\"0\", \"Setting.isOppCPU\":\"1\", \"Setting.oppDbid\":\"1793\", \"Trait.TEAM_TRAIT_COUNTER_ATTACK\":\"1\", \"Trait.TEAM_TRAIT_CROSSING\":\"1\", \"Trait.TEAM_TRAIT_NO_PRESSURE\":\"1\", \"Trait.TEAM_TRAIT_SQUAD_ROTATION\":\"1\"},{\"Fact.BLOCKS_SUCCESSFULL\":\"1\", \"Fact.CLEARANCES\":\"1\", \"Fact.CORNERS\":\"1\", \"Fact.CROSSES_ATTEMPTED\":\"1\", \"Fact.CROSSES_COMPLETED\":\"1\", \"Fact.DISTANCE_DRIBBLED\":\"1\", \"Fact.FOULS\":\"1\", \"Fact.FOULS_SUFFERED\":\"1\", \"Fact.FREE_KICKS_TAKEN\":\"1\", \"Fact.GOALS\":\"1\", \"Fact.GOALS_CONCEDED\":\"1\", \"Fact.GOAL_KICKS\":\"1\", \"Fact.IS_SEVERELY_INJURED\":\"1\", \"Fact.OFFSIDES\":\"1\", \"Fact.OWN_GOALS\":\"1\", \"Fact.PASSES_COMPLETED\":\"2\", \"Fact.PASS_ATTEMPTS\":\"3\", \"Fact.PENALTIES_ATTEMPTS\":\"2\", \"Fact.PENALTY_GOALS\":\"1\", \"Fact.RED_CARDS\":\"1\", \"Fact.SAVES\":\"1\", \"Fact.SHOOT_OUT_ATTEMPTS\":\"1\", \"Fact.SHOOT_OUT_GOALS\":\"4\", \"Fact.SHOTS_ON_GOAL\":\"2\", \"Fact.SHOT_ATTEMPTS\":\"1\", \"Fact.TACKLES\":\"1\", \"Fact.THROW_INS\":\"0\", \"Fact.TIME_IN_POSSESSION\":\"10\", \"Fact.YELLOW_CARDS\":\"0\", \"Setting.dbid\":\"1793\", \"Setting.difficulty\":\"1\", \"Setting.halfLen\":\"6\", \"Setting.isCPU\":\"1\", \"Setting.isOppCPU\":\"0\", \"Setting.oppDbid\":\"112977\", \"Trait.TEAM_TRAIT_DEFEND_THE_LEAD\":\"1\", \"Trait.TEAM_TRAIT_GENERIC_ATTACK\":\"1\", \"Trait.TEAM_TRAIT_GENERIC_DEFENCE\":\"1\", \"Trait.TEAM_TRAIT_SQUAD_ROTATION\":\"1\"}]},\"events\":[{\"cdur\": \"0\",\"core\": {\"en\":\"");
			jsonData.append(pinEvent);
			jsonData.append("\",\"pid\":\"");

			char8_t	personaID[256];
			memset(personaID, '\0', sizeof(personaID));
			blaze_snzprintf(personaID, sizeof(personaID), "%" PRId64, mPlayerData->getPlayerId());
			jsonData.append(personaID);
			jsonData.append("\",\"pidt\":\"persona\",\"pidm\":{\"nucleus\":\"");
			char8_t nucleusID[256];
			memset(nucleusID, '\0', sizeof(nucleusID));
			blaze_snzprintf(nucleusID, sizeof(nucleusID), "%" PRId64, mPlayerData->getPlayerId());
			jsonData.append(nucleusID);
			jsonData.append("\"},\"dob\":\"1993-05\",\"sid\":\"3F2504E0-4F89-11D3-9A0C-0305E82C3301\",\"s\":\"1\",\"lev\":\"4\",\"clientIP\":\"192.30.18.3\",\"mac\":\"23432345457\",\"didm\":{\"idfa\":\"69BD889F-13D0-4EBB-B9E7-5C6C9C174E1D\",\"idfv\":\"599F9C00-92DC-4B5C-9464-7971F01F8370\"},\"ts_event\":\"");
			jsonData.append(buf);
			jsonData.append("\",\"exid\":\"\",");

			if (pinEvent == "user_onboarding_stats")
			{
				jsonData.append("\"mid\":\"STMA_11565162581054\",\"alt_mode_type\":\"REGL\",\"user_type\": {\"controller_config\":\"REGULAR\",\"is_fixed_player\":\"true\",\"difficulty\":\"SEMI_PRO\",\"match_length\":\"1\"},\"dribble_stats\": {\"dribble_attempt\":\"2\",\"dribble_success\":\"0\",\"dribble_fail_missed_pass_chance\":\"0\",\"dribble_fail_missed_shot_chance\":\"0\",\"dribble_fail_out_of_play\":\"0\"},\"pass_stats\": {\"pass_attempt\":\"1\",\"pass_success\":\"1\",\"pass_fail_over_power\":\"0\",\"pass_fail_offside\":\"0\",\"pass_fail_receiver_pressure\":\"0\",\"pass_fail_course_clearance\":\"0\",\"pass_fail_facing_angle\":\"0\",\"pass_fail_too_much_power\":\"0\"},\"shot_stats\": {\"shot_attempt\":\"0\",\"shot_ontarget\":\"0\",\"shot_scored\":\"0\",\"pk_attempt\":\"0\",\"pk_ontarget\":\"0\",\"pk_scored\":\"0\",\"fk_attempt\":\"0\",\"fk_ontarget\":\"0\",\"fk_scored\":\"0\",\"shot_fail_over_power\":\"0\",\"shot_fail_saved\":\"0\",\"shot_fail_facing_angle\":\"0\",\"shot_fail_too_much_pressure\":\"0\",\"shot_fail_course_clearance\":\"0\",\"shot_fail_bad_timing\":\"0\"}, \"tackle_stats\": {\"tackle_attempt\":\"1\",\"tackle_success\":\"0\",\"tackle_fail_foul\":\"0\",\"tackle_fail_out_of_range\":\"0\",\"tackle_fail_from_behind\":\"0\",\"tackle_fail_risk_in_own_box\":\"0\"},");
			}

			jsonData.append("\"custom\": {\"underage_flag\":\"false\",\"gid\":\"79775657059831\"},\"status_code\":\"normal\",\"sdur\":\"26\",\"gdur\":\"26\",\"cdur\":\"26\"},\"matchday\":{\"is_matchday_on\":\"true\",\"curr_fixture_id\":\"0\",\"winning_team\":\"1\",\"home_team_id\":\"112977\",\"user_on_home_team\":\"true\",\"away_team_id\":\"1793\",\"user_on_away_team\":\"false\"},\"source\":\"Install\",\"status\":\"start\"}]}");

			//// PS4 - projectId - 312054
			//jsonData.append("\",\"tid\":\"");
			//if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			//{
			//	//jsonData.append("312093");
			//	jsonData.append(StressConfigInst.getProjectId());
			//}
			//else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			//{
			//	//jsonData.append("312054");
			//	jsonData.append(StressConfigInst.getProjectId());
			//}

			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

			OutboundHttpConnection::ContentPartList contentList;
			OutboundHttpConnection::ContentPart content;
			content.mName = "content";
			content.mContentType = JSON_CONTENTTYPE;
			content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
			content.mContentLength = jsonData.size();
			contentList.push_back(&content);

			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage] " << " [" << mPlayerData->getPlayerId() << "] getMessage URI[" << StressConfigInst.getPINGetMessageUri() << "]");

			result = sendPINHttpRequest(HttpProtocolUtil::HTTP_POST, StressConfigInst.getPINGetMessageUri(), NULL, 0, httpResult, contentList);

			if (false == result)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " PIN returned error for getMessage.");
				return false;
			}

			if (httpResult.hasError())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " PIN returned error for getMessage");
				return false;
			}

			char* govIdValue = httpResult.getGovid();
			if (govIdValue == NULL)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage]: " << mPlayerData->getPlayerId() << " govIdValue is null");
				return false;
			}
			getGovId().sprintf(govIdValue);
			BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage]::[" << mPlayerData->getPlayerId() << "] successful, GovId = " << getGovId().c_str());

			char* trackingtaglistValue = httpResult.getTrackingtaglist();
			if (trackingtaglistValue == NULL)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage]: " << mPlayerData->getPlayerId() << " trackingtaglistValue is null");
				return false;
			}
			getTrackingtaglist().sprintf(trackingtaglistValue);
			BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage]::[" << mPlayerData->getPlayerId() << "] successful, Trackingtaglist = " << getTrackingtaglist().c_str());
			return true;
		}

		bool PINProxy::sendTracking(eastl::string govID, eastl::string trackingList)
		{
			bool result = false;
			PINHttpResult httpResult;
			eastl::string jsonData;

			//{"govid":"7243276220036685486","clientid":"fifa18","plat":"ps4","tid":"312054","tidt":"projectId","pidm":{"personaid":"1000170900231"},"trackingtaglist":["M187dfTf43eeD187bfNintegration"]}
			//jsonData.append("{\"govid\":\"7243276220036685486\",\"clientid\":\"fifa18\",\"plat\":\"ps4\",\"tid\":\"312054\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"1000170900231\"},\"trackingtaglist\":[\"M187dfTf43eeD187bfNintegration\"]}");

			jsonData.append("{\"govid\":\"");
			jsonData.append(govID.c_str());
			jsonData.append("\",\"clientid\":\"fifa19_test\",\"plat\":\"");
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				jsonData.append("xbox_one");
			}
			else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				jsonData.append("ps4");
			}
			// PS4 - projectId - 312054
			jsonData.append("\",\"tid\":\"");
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				//jsonData.append("312093");
				jsonData.append(StressConfigInst.getProjectId());
			}
			else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				//jsonData.append("312054");
				jsonData.append(StressConfigInst.getProjectId());
			}
			jsonData.append("\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"");
			char8_t	personaID[256];
			memset(personaID, '\0', sizeof(personaID));
			blaze_snzprintf(personaID, sizeof(personaID), "%" PRId64, mPlayerData->getPlayerId());
			jsonData.append(personaID);
			jsonData.append("\"},\"trackingtaglist\":[\"");
			jsonData.append(trackingList.c_str());
			jsonData.append("\"]}");

			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::sendTracking] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

			OutboundHttpConnection::ContentPartList contentList;
			OutboundHttpConnection::ContentPart content;
			content.mName = "content";
			content.mContentType = JSON_CONTENTTYPE;
			content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
			content.mContentLength = jsonData.size();
			contentList.push_back(&content);

			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::sendTracking] " << " [" << mPlayerData->getPlayerId() << "] sendTracking URI[" << StressConfigInst.getPINTrackingUri() << "]");

			result = sendPINHttpRequest(HttpProtocolUtil::HTTP_POST, StressConfigInst.getPINTrackingUri(), NULL, 0, httpResult, contentList);

			if (false == result)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::sendTracking:result]:" << mPlayerData->getPlayerId() << " PIN returned error for sendTracking.");
				return false;
			}

			if (httpResult.hasError())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::SendTracking:hasError]:" << mPlayerData->getPlayerId() << " PIN returned error for sendTracking");
				return false;
			}
			return true;
		}


		bool PINProxy::sendPINHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList)
		{
			BlazeRpcError err = ERR_SYSTEM;
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::sendPINHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

			err = mHttpPINProxyConnMgr->sendRequest(method, URI, NULL, 0, httpHeaders, headerCount, &result, &contentList);

			if (ERR_OK != err)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[PINProxy::sendPINHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
				return false;
			}
			return true;
		}

		// if PIN Enabled
		bool  PINProxyHandler::getPINMessage()
		{
			bool result = false;
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "Starting PIN shared services!!")

			PINProxy* pinProxy = BLAZE_NEW PINProxy(mPlayerData);
			eastl::string pinEvents = "";
			ArubaProxyHandler * recoTriggerIds = new ArubaProxyHandler(mPlayerData);
			recoTriggerIds->getRecoTriggerIdsType();
			RecoTriggerIdsToStringMap::iterator itr = recoTriggerIds->getRecoTriggerIdsToStringMap().begin();
			int count = 0;
			for (;count < recoTriggerIds->getRecoTriggerIdsToStringMap().size(); count++, itr++)
			{
				if (itr->second == "3040")
				{
					pinEvents = "user_onboarding_stats";
				}
				else
				{
					pinEvents = "game_end";
				}
				result = pinProxy->getMessage(pinEvents);
				BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage] sending " << pinEvents << " " << itr->first << "==>" << itr->second << "]");
			}

			delete recoTriggerIds;

			ArubaProxyHandler * arubaTriggerIds = new ArubaProxyHandler(mPlayerData);
			arubaTriggerIds->getArubaTriggerIdsType();
			int i = 0;
			ArubaTriggerIdsToStringMap::iterator itr1 = arubaTriggerIds->getArubaTriggerIdsToStringMap().begin();
			for ( ; i < arubaTriggerIds->getArubaTriggerIdsToStringMap().size(); i++, itr1++)
			{
				// Sending specific PIN Events for all aruba trigger id's
				result = pinProxy->getMessage("game_end");
				BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINProxy::getMessage] " << " [ sending game_end event ]");
			}
			// result = pinProxy->getMessage();
			// check for all messages need to call sendtracking
			if (result)
			{
				setGovId(pinProxy->getGovId());
				setTrackingtaglist(pinProxy->getTrackingtaglist());
			}

			delete pinProxy;
			return result;
		}

		bool  PINProxyHandler::sendTracking()
		{
			PINProxy* pinProxy = BLAZE_NEW PINProxy(mPlayerData);
			bool result = pinProxy->sendTracking(getGovId(), getTrackingtaglist());
			delete pinProxy;
			return result;
		}

		void PINHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINHttpResult::setValue]:Received data = " << data);
			// Read govid
			const char* token = strstr(data, "govid");
			if (token != NULL)
			{
				const char* startPos = token + strlen("govid") + 2;
				char idValue[256] = { '\0' };
				bool foundStart = false;
				for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
				{
					if (foundStart && startPos[index] == '"')
					{
						break;
					}
					if (!foundStart && startPos[index] == '"')
					{
						foundStart = true;
						continue;

					}
					if (foundStart)
					{
						idValue[resindex] = startPos[index];
						resindex++;
					}
				}
				if (blaze_strcmp(idValue, "null") != 0)
				{
					blaze_strnzcpy(govid, idValue, sizeof(govid));
				}
				BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINHttpResult::setValue]:Received govid [" << govid << "] and data = " << data);
			}
			else
			{
				BLAZE_ERR(BlazeRpcLog::usersessions, "[PINHttpResult::setValue]: govid is null, data = %s ", data);
			}

			// Read trackingtag
			const char* tokenId = strstr(data, "trackingtag");
			if (tokenId != NULL)
			{
				const char* startPos = tokenId + strlen("trackingtag") + 1;
				char idValue[256] = { '\0' };
				bool foundStart = false;
				for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
				{
					if (foundStart && startPos[index] == ',')
					{
						break;
					}
					if (!foundStart && startPos[index] == ':')
					{
						foundStart = true;
						continue;
					}
					if (foundStart)
					{
						if (startPos[index] == '"') { continue; }
						if (startPos[index] == '}') { continue; }
						idValue[resindex] = startPos[index];
						resindex++;
					}
				}
				if (blaze_strcmp(idValue, "null") != 0)
				{
					blaze_strnzcpy(trackingtaglist, idValue, sizeof(trackingtaglist));
				}
				BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[PINHttpResult::setValue]:Received trackingtaglist [" << trackingtaglist << "] and data = " << data);
			}
			else
			{
				BLAZE_ERR(BlazeRpcLog::usersessions, "[PINHttpResult::setValue]: trackingtag is null, data = %s ", data);
			}
		}

		//  *************************************************************************************************
		//  PIN Stress code 
		//  *************************************************************************************************

	}  // namespace Stress
}  // namespace Blaze

