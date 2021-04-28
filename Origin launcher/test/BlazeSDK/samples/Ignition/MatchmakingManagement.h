#ifndef MATCHMAKING_MANAGEMENT_H
#define MATCHMAKING_MANAGEMENT_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"

namespace Ignition
{
    class MatchmakingManagement :
        public Pyro::UiNodeWindow
    {
    public:
        MatchmakingManagement(const char8_t *id);
        virtual ~MatchmakingManagement();

        void showDebugInfo(const Blaze::GameManager::DebugTopResult *result, const Blaze::GameManager::DebugFindGameResults& mmFindGameResults);
        void showDebugInfo(const Blaze::GameManager::DebugSessionResult *result, const Blaze::GameManager::DebugCreateGameResults& mmCreateGameResults, Blaze::GameManager::FitScore maxFitScore);

    private:
        void fgDebugTable_RowSelectedChanged(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row);
        void cgDebugTable_RowSelectedChanged(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row);

        void updateConditionTable(const Blaze::GameManager::ReadableRuleConditionList& conditions);

    private:
        Blaze::GameManager::DebugTopResult mFindGameResult;
        Blaze::GameManager::DebugSessionResult mCreateGameResult;
    };

}

#endif
