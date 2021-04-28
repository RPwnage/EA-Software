/*! ************************************************************************************************/
/*!
    \file finddedicatedservers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/finddedicatedservers_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/matchmaker/rules/expandedpingsiteruledefinition.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Get a list of dedicated servers matching the supplied criteria.  The servers are returned in lists, mapped by blaze server instance id.
    ***************************************************************************************************/
    class FindDedicatedServersCommand : public FindDedicatedServersCommandStub
    {
    public:
        FindDedicatedServersCommand(Message* message, FindDedicatedServersRequest* request, GameManagerSlaveImpl* componentImpl)
            :   FindDedicatedServersCommandStub(message, request), mComponent(*componentImpl)
        {}
        ~FindDedicatedServersCommand() override {}

    private:

        BlazeRpcError getGameListSnapshot(GetGameListRequest& gameListRequest, GetGameListSyncResponse& gameListResponse, MatchmakingCriteriaError error, const PingSiteAlias& preferredPingSite = "")
        {
            BlazeRpcError err = mComponent.getGameBrowser().processGetGameListSnapshotSync(gameListRequest, gameListResponse, error,
                mRequest.getMaxPlayerCapacity(), preferredPingSite.c_str(), mRequest.getNetTopology(), &mRequest.getUserSessionInfo());
            if (err != ERR_OK)
            {
                return err;
            }

            // This is a sorted list, and we want to know how many servers have matching fit scores:
            if (gameListResponse.getGameList().size() == 0)
            {
                TRACE_LOG("[FindDedicatedServersCommandStub].getGameListSnapshot: No available resettable dedicated servers found, on criteria:" <<
                    " preferred ping site '" << preferredPingSite.c_str() << "', owner best ping site '" << mRequest.getUserSessionInfo().getBestPingSiteAlias() << "', allow mismatched ping sites '" << (mRequest.getAllowMismatchedPingSites() ? "true" : "false") << "'," <<
                    " game protocol version string '" << gameListRequest.getCommonGameData().getGameProtocolVersionString() << "', max capacity " << mRequest.getMaxPlayerCapacity() << ","
                    " network topology " << (GameNetworkTopologyToString(mRequest.getNetTopology()) != nullptr ? GameNetworkTopologyToString(mRequest.getNetTopology()) : "<INVALID>") << ". Are sufficient dedicated servers available for the criteria?");
                return GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
            }

            // Given the game list returned, break it down by game master, and then send that up in the request:
            GameBrowserMatchDataList::const_iterator gameListIter = gameListResponse.getGameList().begin();
            GameBrowserMatchDataList::const_iterator gameListEnd = gameListResponse.getGameList().end();
            for (; gameListIter != gameListEnd; ++gameListIter)
            {
                GameId gameId = (*gameListIter)->getGameData().getGameId();

                GameIdList* gamesToReset = mResponse.getGamesByFitScoreMap()[(*gameListIter)->getFitScore()];
                if (gamesToReset == nullptr)
                    gamesToReset = mResponse.getGamesByFitScoreMap()[(*gameListIter)->getFitScore()] = mResponse.getGamesByFitScoreMap().allocate_element();
                gamesToReset->push_back(gameId);
            }

            return ERR_OK;
        }

        FindDedicatedServersCommandStub::Errors execute() override
        {
            // Find dedicated servers: 
            GameBrowserScenarioData scenarioData;
            GetGameListRequest& gameListRequest = scenarioData.getGetGameListRequest();
            MatchmakingCriteriaError error;

            GetGameListScenarioRequest request;
            if (*mRequest.getCreateGameTemplateName() != '\0')
            {
                CreateGameTemplateMap::const_iterator templateIter = mComponent.getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().find(mRequest.getCreateGameTemplateName());
                if (templateIter != mComponent.getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end())
                {
                    const auto& templateConfig = (*templateIter->second);
                    request.setGameBrowserScenarioName(templateConfig.getHostSelectionGameBrowserScenario());
                }
                else
                {
                    ERR_LOG("[FindDedicatedServersCommand] createGameTemplate(" << *mRequest.getCreateGameTemplateName() << ") does not exist.");
                    return commandErrorFromBlazeError(GAMEMANAGER_ERR_INVALID_TEMPLATE_NAME);
                }
            }
            if (request.getGameBrowserScenarioName()[0] == '\0')
            {
                request.setGameBrowserScenarioName(mComponent.getConfig().getGameBrowserScenariosConfig().getDedicatedServerLookupGBScenarioName());
            }

            // If a preferred list was used, then we use the list as a filter at first, and then 
            PingSiteAlias bestPreferredPingSite = "";
            if (!mRequest.getPreferredPingSites().empty())
            {
                request.getGameBrowserScenarioAttributes()[GameManager::Matchmaker::ExpandedPingSiteRuleDefinition::DEDICATED_SERVER_WHITELIST] = request.getGameBrowserScenarioAttributes().allocate_element();
                request.getGameBrowserScenarioAttributes()[GameManager::Matchmaker::ExpandedPingSiteRuleDefinition::DEDICATED_SERVER_WHITELIST]->set(mRequest.getPreferredPingSites());
                
                // The list should already be ordered, so we just use the first one as the 'best'
                bestPreferredPingSite = mRequest.getPreferredPingSites().front();
            }

            BlazeRpcError errCode = mComponent.getGameBrowser().transformScenarioRequestToBaseRequest(scenarioData, request, error);
            if (errCode != ERR_OK)
            {
                return commandErrorFromBlazeError(errCode);
            }

            // Populate dedicated server search criteria based on findDedicatedServerRulesMap
            for (const auto& rule : mRequest.getFindDedicatedServerRulesMap())
            {
                const RuleName& ruleName = rule.first;
                gameListRequest.getListCriteria().getDedicatedServerAttributeRuleCriteriaMap()[ruleName] = gameListRequest.getListCriteria().getDedicatedServerAttributeRuleCriteriaMap().allocate_element();
                GameManager::DedicatedServerAttributeRuleCriteria* gameRuleCriteria = gameListRequest.getListCriteria().getDedicatedServerAttributeRuleCriteriaMap()[ruleName];
                gameRuleCriteria->setDesiredValue(rule.second->getDesiredValue());
                gameRuleCriteria->setMinFitThresholdValue(rule.second->getMinFitThresholdValue());
            }

            // For DS search, we want to treat the search list as required, but anything else can be on or off.
            // This is different than Game searches, where we either require all platforms to match, or we treat everything that's not a player as optional.
            
            // Right now, a [pc] player can take a [pc,xone,ps4] dedicated server.  If that needs to change, this should be an optional config param (since it's global).
            // gameListRequest.getListCriteria().getPlatformRuleCriteria().setCrossplayMustMatch(mRequest.getClientPlatformOverrideList().size == 1);
            mRequest.getClientPlatformOverrideList().copyInto(gameListRequest.getListCriteria().getPlatformRuleCriteria().getClientPlatformListOverride());

            gameListRequest.getCommonGameData().setGameProtocolVersionString(mRequest.getGameProtocolVersionString());

            GetGameListSyncResponse gameListResponse;

            // First, do a lookup using the pingsite whitelist: 
            BlazeRpcError rc = getGameListSnapshot(gameListRequest, gameListResponse, error, bestPreferredPingSite);
            if (rc == GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND)
            {
                // If no servers were found with the preferred whitelist, then retry with the whitelist disabled:  (If mismatch is allowed)
                if (mRequest.getAllowMismatchedPingSites() && !gameListRequest.getListCriteria().getExpandedPingSiteRuleCriteria().getPingSiteWhitelist().empty())
                {
                    gameListRequest.getListCriteria().getExpandedPingSiteRuleCriteria().getPingSiteWhitelist().clear();
                    rc = getGameListSnapshot(gameListRequest, gameListResponse, error, bestPreferredPingSite);
                }
            }

            return commandErrorFromBlazeError(rc);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_FINDDEDICATEDSERVERS_CREATE()

} // namespace GameManager
} // namespace Blaze
