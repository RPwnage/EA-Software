/*************************************************************************************************/
/*!
    \file   arsontournamentorganizerslaveimpl.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef ARSON_TOURNAMENT_ORGANIZER_SLAVEIMPL_H
#define ARSON_TOURNAMENT_ORGANIZER_SLAVEIMPL_H

#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave_stub.h"
#include "framework/system/fiber.h"
#include "arson/tournamentorganizer/arsonexternaltournamentutil.h"

namespace Blaze
{

namespace GameManager { class ReplicatedGameData; }

namespace Arson
{
    class ArsonExternalTournamentUtil;
    class ArsonTournamentOrganizerConfig;

    class ArsonTournamentOrganizerSlaveImpl : public ArsonTournamentOrganizerSlaveStub
    {
    public:
        ArsonTournamentOrganizerSlaveImpl();
        ~ArsonTournamentOrganizerSlaveImpl() override;

        BlazeRpcError DEFINE_ASYNC_RET(getNextTeamExternalSessionName(eastl::string& result, const ArsonTournamentOrganizerConfig& config) const);
        BlazeRpcError DEFINE_ASYNC_RET(getMatchData(Blaze::GameManager::GameId matchId, Blaze::GameManager::ReplicatedGameData& matchData, Blaze::GameManager::ReplicatedGamePlayerList* matchPlayerData = nullptr));
        BlazeRpcError DEFINE_ASYNC_RET(testSendTournamentGameEvent(const Blaze::Arson::TestSendTournamentGameEventRequest& request, Blaze::Arson::TestSendTournamentGameEventResponse& response, bool testStartEvent));

        ArsonExternalTournamentUtil* getExternalTournamentUtil() const { return mExternalTournamentUtil; }
    private:
        //Component interface
        bool onConfigure() override;

        const Blaze::GameManager::ExternalHttpGamePlayerEventData* findPlayerEventData(const Blaze::Arson::TestSendTournamentGameEventRequest::ExternalHttpGamePlayerEventDataList& list, const Blaze::GameManager::ReplicatedGamePlayer& player) const;

    private:
        ArsonExternalTournamentUtil *mExternalTournamentUtil;
    };
} // Arson
} // Blaze

#endif // ARSON_TOURNAMENT_ORGANIZER_SLAVEIMPL_H

