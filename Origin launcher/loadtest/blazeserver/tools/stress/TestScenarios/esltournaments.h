//  *************************************************************************************************
//
//   File:    friendlies.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef ESLTOURNAMENTS_H
#define ESLTOURNAMENTS_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./gameplayuser.h"
#include "./esltournamentsgamesession.h"
#include "component/messaging/tdf/messagingtypes.h"
#include "nucleusproxy.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class ESLTournaments : public GamePlayUser, public ESLTournamentsGameSession, public ReportTelemtryFiber {
        NON_COPYABLE(ESLTournaments);

    public:
		ESLTournaments(StressPlayerInfo* playerData);
        virtual ~ESLTournaments();
        virtual BlazeRpcError   simulateLoad();
        void                    addPlayerToList();
        void                    removePlayerFromList();
		void					lobbyCalls();
        virtual PlayerType      getPlayerType() {
            return ESLTOURNAMENTS;
        }
        virtual void            deregisterHandlers();
        virtual void            preLogoutAction();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			joinClub(ClubId clubID);
		OutboundHttpConnectionManager*		mHttpESLProxyConnMgr;
		
    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();
		typedef EA::TDF::TdfPrimitiveVector<PlayerId> CoopPlayerList; 
		typedef EA::TDF::TdfPrimitiveVector<AccountId> ESLAccountList;
		BlazeRpcError			startGame();
		bool					initESLProxyHTTPConnection();
		bool					sendESLHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList);
		bool					sendESLHttpLoginRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result);
		bool					expressLogin();
		BlazeRpcError			loginToBlaze();
		BlazeRpcError			trustedLogin();
		bool					getESLAccessToken();
		BlazeRpcError			createTournamentGame(int numOfPlayers, CoopPlayerList cooplist);
		BlazeRpcError			getTournamentGame();
		BlazeRpcError			cancelTournamentGame();

    private:
        bool                    mIsMarkedForLeaving;
		eastl::string			mESLAccessToken;
		char			mSessionKey[256];
};

class ESLHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	char			*mSessionKey;

public:
	ESLHttpResult() : mErr(ERR_OK)
	{

	}
	void setHttpError(BlazeRpcError err = ERR_SYSTEM)
	{
		mErr = err;
	}
	bool hasError()
	{
		return (mErr != ERR_OK) ? true : false;
	}

	void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
		size_t attributeCount)
	{
		//Do nothing here
	}

	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
	{
		const char* sessionKey = strstr(fullname, "sessionkey");
		if (sessionKey != NULL)
		{
			mSessionKey = new char[dataLen];
			memset(mSessionKey, '\0', dataLen);
			strncpy(mSessionKey, data, dataLen);
			mSessionKey[dataLen] = '\0';
		}
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

	char* getSessionKey()
	{
		if (strlen(mSessionKey) == 0)
			return NULL;

		char* accesstoken = new char[strlen(mSessionKey) + 1];
		memset(accesstoken, '\0', strlen(mSessionKey) + 1);
		strcpy(accesstoken, mSessionKey);

		return mSessionKey;
	}

};

}  // namespace Stress
}  // namespace Blaze

#endif  //   ESLTOURNAMENTS_H