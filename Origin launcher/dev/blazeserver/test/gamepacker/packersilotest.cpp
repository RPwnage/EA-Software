/*************************************************************************************************/
/*!
    \file createpackersessiontest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamepacker/packersilotest.h"
#include "gamepacker/handlers.h"
#include "gamepacker/packer.h"

namespace BlazeServerTest
{
namespace GamePacker
{

using namespace Packer;

struct DummyUtilityMethods : public PackerUtilityMethods {
    GameId mGameId = 0;

    Time utilPackerMeasureTime() override
    {
        return 0;
    }

    GameId utilPackerMakeGameId(PackerSilo& packer) override
    {
        return ++mGameId;
    }

    void utilPackerUpdateMetric(PackerSilo& packer, PackerMetricType type, int64_t delta) override
    {
    }

    void utilPackerLog(PackerSilo& packer, LogLevel level, const char8_t* fmt, va_list args) override
    {
        ;
    }

    bool utilPackerTraceEnabled(TraceMode mode) const override
    {
        return false;
    }

    bool utilPackerTraceFilterEnabled(TraceMode mode, int64_t entityId) const override
    {
        return false;
    }

    int32_t utilPackerRand() override
    {
        EA_FAIL_MSG("Deliberately disabled.");
        return 0;
    }

} gDummyUtilityMethods;

//////////// TESTS

TEST_F(PackerSiloTest, partiallyOverlappedFields)
{
    // This test checks that packing 3 parties referencing mutually overlapping but disjoint fields into a 3 player game 
    // results in a correct operation of the field table (the adding logic modifies the field value table)

    const PartyId partyIds[] = { 1, 2, 3 }; // note: these values also double as player ids, and player property values
    const PropertyValue playerFieldIndexTable[3][2] = { {0, 1}, {1, 2}, {0, 1} };
    static const uint32_t GAME_CAPACITY = EAArrayCount(partyIds);

    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(gDummyUtilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability

    packer.setVar(GameTemplateVar::GAME_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_COUNT, 1);

    const auto* propertyName =  "prop1" ;
    packer.addProperty(propertyName);
    const char* fieldNames[3] = { "field1" , "field2", "field3" };
    const FieldDescriptor* fields[3] = {};
    int i = 0;
    for (auto* field : fieldNames)
        fields[i++] = &packer.addPropertyField(propertyName, field);

    GameQualityFactorConfig cfg;
    cfg.mBestValue = GAME_CAPACITY;
    cfg.mWorstValue = 0;
    cfg.mTransform = "size";
    ASSERT_TRUE(packer.addFactor(cfg));

    cfg.mBestValue = 0;
    cfg.mWorstValue = 100;
    cfg.mTransform = "avg";
    cfg.mPropertyName = propertyName;
    cfg.mOutputPropertyName = propertyName;
    ASSERT_TRUE(packer.addFactor(cfg));

    int partyIndex = 0;
    for (auto partyId : partyIds)
    {
        auto dummyExpiryTime = partyId;
        packer.addPackerParty(partyId, dummyExpiryTime, dummyExpiryTime);
        auto& party = *packer.getPackerParty(partyId);
        auto playerId = partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        for (auto partyFieldIndex : playerFieldIndexTable[partyIndex++])
        {
            PropertyValue fieldValue = partyId; // dummy value, used as debugging marker
            party.setPlayerPropertyValue(fields[partyFieldIndex], partyPlayerIndex, &fieldValue);
        }
    }

    packer.packGames(nullptr);
}

TEST_F(PackerSiloTest, partiallyOverlappedFieldsPartyEvict)
{
    // This test checks that packing 3 parties referencing mutually overlapping but disjoint fields into a 2 player game 
    // results in a correct operation of the field table (both the adding and the eviction logic modify the field value table)

    const PartyId partyIds[] = { 1, 2, 3 }; // note: these values also double as player ids, and player property values
    const Time partyPriories[] = { 3, 2, 1 }; // for this test its important for the parties to be processed in decresasing order of their property values, so we set priorities accordingly
    const uint32_t playerFieldIndexTable[3][2] = { {0, 1}, {0, 1}, {1, 2} }; // field indexes
    static const uint32_t GAME_CAPACITY = 2;

    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(gDummyUtilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability

    packer.setVar(GameTemplateVar::GAME_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_COUNT, 1);

    const auto* propertyName = "prop1";
    packer.addProperty(propertyName);
    const char* fieldNames[3] = { "field1" , "field2", "field3" };
    const FieldDescriptor* fields[3] = {};
    int i = 0;
    for (auto* field : fieldNames)
        fields[i++] = &packer.addPropertyField(propertyName, field);

    GameQualityFactorConfig cfg;
    cfg.mBestValue = GAME_CAPACITY;
    cfg.mWorstValue = 0;
    cfg.mTransform = "size";
    ASSERT_TRUE(packer.addFactor(cfg));

    cfg.mBestValue = 0;
    cfg.mWorstValue = 3;
    cfg.mTransform = "avg";
    cfg.mPropertyName = propertyName;
    cfg.mOutputPropertyName = propertyName;
    ASSERT_TRUE(packer.addFactor(cfg));

    int32_t partyIndex = 0;
    for (auto partyId : partyIds)
    {
        auto expiryTime = partyPriories[partyIndex];
        packer.addPackerParty(partyId, partyIndex, expiryTime);
        auto& party = *packer.getPackerParty(partyId);
        auto playerId = partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        for (auto partyFieldIndex : playerFieldIndexTable[partyIndex])
        {
            PropertyValue fieldValue = partyId; // reuse party id as field value, it suits our purpose for this test
            party.setPlayerPropertyValue(fields[partyFieldIndex], partyPlayerIndex, &fieldValue);
        }
        partyIndex++;
    }

    packer.packGames(nullptr);
}


TEST_F(PackerSiloTest, partiallyOverlappedFieldsComputeSortedFieldScores)
{
    // This test checks that packing 3 parties referencing mutually overlapping but disjoint fields into a 2 player game 
    // results in a correct operation of the computeSortedFieldScores() method (before this was fixed it asserted). 
    // The objective is to test that the set bits for the non-overlapping values in a field column in a game that is not improved (1st game)
    // end up correctly bypassed due to lack of player participation of a temporary field when performing the field score computation.

    const PartyId partyIds[] = { 1, 2, 3 }; // note: these values also double as player ids
    const Time partyPriories[] = { 1, 2, 3 }; // for this test its important for the parties to be processed in decresasing order of their property values, so we set priorities accordingly
    const int32_t playerFieldIndexTable[3][2] = { {0, -1}, {1, -1}, {0, 1} }; // field indexes
    const int32_t playerFieldValueTable[3][2] = { {1, -1}, {-1, 2}, {10, 2} }; // field values
    static const uint32_t GAME_CAPACITY = 2;

    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(gDummyUtilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability

    packer.setVar(GameTemplateVar::GAME_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_COUNT, 1);

    const auto* propertyName = "prop1";
    packer.addProperty(propertyName);
    const char* fieldNames[3] = { "field1" , "field2", "field3" };
    const FieldDescriptor* fields[3] = {};
    int i = 0;
    for (auto* field : fieldNames)
        fields[i++] = &packer.addPropertyField(propertyName, field);

    GameQualityFactorConfig cfg;
    cfg.mBestValue = GAME_CAPACITY;
    cfg.mWorstValue = 0;
    cfg.mTransform = "size";
    ASSERT_TRUE(packer.addFactor(cfg));

    cfg.mBestValue = 0;
    cfg.mWorstValue = 3;
    cfg.mTransform = "avg";
    cfg.mPropertyName = propertyName;
    cfg.mOutputPropertyName = propertyName;
    ASSERT_TRUE(packer.addFactor(cfg));

    int32_t partyIndex = 0;
    for (auto partyId : partyIds)
    {
        auto expiryTime = partyPriories[partyIndex];
        packer.addPackerParty(partyId, partyIndex, expiryTime);
        auto& party = *packer.getPackerParty(partyId);
        auto playerId = partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        ASSERT_TRUE(partyPlayerIndex == 0);
        for (auto playerFieldIndex : playerFieldIndexTable[partyIndex])
        {
            if (playerFieldIndex < 0) continue; // deliberately skip -1 indexes
            PropertyValue fieldValue = playerFieldValueTable[partyIndex][playerFieldIndex];
            party.setPlayerPropertyValue(fields[playerFieldIndex], partyPlayerIndex, &fieldValue);
        }
        partyIndex++;
    }

    packer.packGames(nullptr);

    ASSERT_TRUE(packer.getGames().size() == 2);

    for (auto& gameItr : packer.getGames())
    {
        auto& game = gameItr.second;
        for (auto& factor : packer.getFactors())
        {
            auto* inputPropertyDescriptor = factor.getInputPropertyDescriptor();
            if (inputPropertyDescriptor == nullptr)
                continue; // skip factors that don't take any input properties, since they do not choose property fields
            Packer::FieldScoreList fieldScoreList;
            factor.computeSortedFieldScores(fieldScoreList, game);
        }
    }
}

TEST_F(PackerSiloTest, pack128PlayersIn8Teams)
{
    // This test checks that packing 128 players into a game with 8 teams 
    // using 3 quality factors results in correct number of players allocated to each team
    static const uint32_t GAME_CAPACITY = 128;
    static const uint32_t TEAM_COUNT = 8;
    static const uint32_t TEAM_CAPACITY = GAME_CAPACITY / TEAM_COUNT;
    static const uint32_t MAX_FIELDS = 1;

    PartyId partyIds[GAME_CAPACITY] = {}; // note: these values also double as player ids, and player property values
    Time partyPriories[GAME_CAPACITY] = {}; // for this test its important for the parties to be processed in decresasing order of their property values, so we set priorities accordingly
    for (int32_t i = 0; i < EAArrayCount(partyIds); ++i)
    {
        partyIds[i] = i+1; // populate ids
        partyPriories[i] = partyIds[i];
    }
    uint32_t playerFieldIndexTable[GAME_CAPACITY][1]; // field indexes
    memset(playerFieldIndexTable, 0, sizeof(playerFieldIndexTable));

    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(gDummyUtilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability

    packer.setVar(GameTemplateVar::GAME_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, TEAM_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_COUNT, TEAM_COUNT);

    const auto* propertyName = "prop1";
    packer.addProperty(propertyName);
    const char* fieldNames[MAX_FIELDS] = { "field1" };
    const FieldDescriptor* fields[MAX_FIELDS] = {};
    for (int32_t i = 0; i < EAArrayCount(fieldNames); ++i)
    {
        fields[i] = &packer.addPropertyField(propertyName, fieldNames[i]);
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = GAME_CAPACITY;
        cfg.mWorstValue = 0;
        cfg.mTransform = "size"; // game size
        ASSERT_TRUE(packer.addFactor(cfg));
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = 0;
        cfg.mWorstValue = GAME_CAPACITY;
        cfg.mTransform = "avg"; // property average
        cfg.mPropertyName = propertyName;
        cfg.mOutputPropertyName = propertyName;
        ASSERT_TRUE(packer.addFactor(cfg));
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = TEAM_CAPACITY;
        cfg.mWorstValue = 0;
        cfg.mTransform = "size.min"; // min teamsize
        ASSERT_TRUE(packer.addFactor(cfg));
    }

    int32_t partyIndex = 0;
    for (auto partyId : partyIds)
    {
        auto expiryTime = partyPriories[partyIndex];
        packer.addPackerParty(partyId, partyIndex, expiryTime);
        auto& party = *packer.getPackerParty(partyId);
        auto playerId = partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        for (auto partyFieldIndex : playerFieldIndexTable[partyIndex])
        {
            PropertyValue fieldValue = partyId; // reuse party id as field value, it suits our purpose for this test
            party.setPlayerPropertyValue(fields[partyFieldIndex], partyPlayerIndex, &fieldValue);
        }
        partyIndex++;
    }

    packer.packGames(nullptr);

    ASSERT_TRUE(packer.getGames().size() == 1);

    auto& game = packer.getGames().begin()->second;
    // validate all teams are full
    for (TeamIndex i = 0; i < TEAM_COUNT; ++i)
    {
        auto teamPlayerCount = game.getTeamPlayerCount(GameState::PRESENT, i);
        ASSERT_TRUE(teamPlayerCount == TEAM_CAPACITY);
    }
}

TEST_F(PackerSiloTest, partyEvictionCorrectlyReleasesFieldReferences)
{
    // this test validates that eviction of a party that references multiple property fields by one factor results in correct evaluation of remaining fields by the following factor.
    // before the fix the code below would result in an infinite loop

    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(gDummyUtilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability
    packer.setMaxPackerIterations(100); // IMPORTANT: limit the max number of iterations to quickly catch the problem

    // This test checks that packing 128 players into a game with 8 teams 
    // using 3 quality factors results in correct number of players allocated to each team
    static const uint32_t GAME_CAPACITY = 4;
    static const uint32_t TEAM_COUNT = 2;
    static const uint32_t TEAM_CAPACITY = GAME_CAPACITY / TEAM_COUNT;

    packer.setVar(GameTemplateVar::GAME_CAPACITY, GAME_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, TEAM_CAPACITY);
    packer.setVar(GameTemplateVar::TEAM_COUNT, TEAM_COUNT);

    struct property_info
    {
        const char8_t* propertyName;
        eastl::vector<const char8_t*> propertyFields;
    };

    property_info propertyInfos[] = {
        {"pingSiteLatencies", { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc"/*,"aws-nrt","aws-syd"*/} },
        {"attributes[progress]", {"0","1","2","3","4","5"} },
        {"attributes[level]", {"MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall"} },
        {"pingSiteLatencies", { "aws-nrt","aws-syd"} },
    };


    eastl::hash_map<eastl::string, const FieldDescriptor*> fields;

    for (auto& info : propertyInfos)
    {
        for (auto& field : info.propertyFields)
        {
            auto ret = fields.insert(field);
            EA_ASSERT(ret.second); // verify all field names are unique for now for ease of insertion
            ret.first->second = &packer.addPropertyField(info.propertyName, field);
        }
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = GAME_CAPACITY;
        cfg.mWorstValue = 0;
        cfg.mViableValue = 1;
        cfg.mTeamAggregate = PropertyAggregate::MINMAXRANGE;
        cfg.mTransform = "size"; // game size
        packer.addFactor(cfg);
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = 0;
        cfg.mViableValue = 150;
        cfg.mWorstValue = 200;
        cfg.mGranularity = 10;
        cfg.mTeamAggregate = PropertyAggregate::MINMAXRANGE;
        cfg.mTransform = "avg"; // property average
        cfg.mPropertyName = "pingSiteLatencies";
        cfg.mOutputPropertyName = "game.pingSite";
        packer.addFactor(cfg);
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = 0;
        cfg.mWorstValue = 5;
        cfg.mViableValue = 5;
        cfg.mTeamAggregate = PropertyAggregate::MINMAXRANGE;
        cfg.mTransform = "avg"; // property average
        cfg.mPropertyName = "attributes[progress]";
        cfg.mOutputPropertyName = "game.attributes[progress]";
        packer.addFactor(cfg);
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = 0;
        cfg.mWorstValue = 2;
        cfg.mViableValue = 2;
        cfg.mTeamAggregate = PropertyAggregate::MINMAXRANGE;
        cfg.mTransform = "avg"; // property average
        cfg.mPropertyName = "attributes[level]";
        cfg.mOutputPropertyName = "game.attributes[level]";
        packer.addFactor(cfg);
    }

    {
        GameQualityFactorConfig cfg;
        cfg.mBestValue = TEAM_CAPACITY;
        cfg.mWorstValue = 0;
        cfg.mTeamAggregate = PropertyAggregate::MINMAXRANGE;
        cfg.mViableValue = 0; // weird viable value
        cfg.mTransform = "size.min"; // min teamsize
        packer.addFactor(cfg);
    }

    struct party_info {
        PartyId partyId;
        bool immutable;
        Time expiryTime;
    };

    eastl::vector<party_info> partyInfos = {
        { 2651867230000, true, 9223372036854775807 },
        { 2647230050708, false, 1614353951193340    },
        { 2647230050709, false, 1614353951193341    },
        { 2647230050710, false, 1614353951193342    },
    };

    eastl::vector<eastl::vector<const char8_t*>> partyFieldNames = {
        { "aws-fra","1","MP_Discarded"                                                                                                                                                                                                  },
        { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc","0","1","2","3","4","5","MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall"                      },
        { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc","0","1","2","3","4","5","MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall"                      },
        { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc","0","1","2","3","4","5","MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall","aws-nrt","aws-syd"  },
    };

    eastl::vector<eastl::vector<PropertyValue>> partyFieldValues = {
        { 0,0,0                                                     },
        { 24,2,93,15,172,154,0,1,2,3,4,5,0,0,0,0,0,0,0,0            },
        { 2,27,71,13,177,142,0,1,2,3,4,5,0,0,0,0,0,0,0,0            },
        { 124,144,73,132,176,21,0,1,2,3,4,5,0,0,0,0,0,0,0,0,104,143 },
    };

    for (size_t i = 0; i < partyInfos.size(); ++i)
    {
        auto& pinfo = partyInfos[i];
        auto& pfields = partyFieldNames[i];
        auto& pvalues = partyFieldValues[i];

        packer.addPackerParty(pinfo.partyId, i, pinfo.expiryTime, pinfo.immutable);
        auto& party = *packer.getPackerParty(pinfo.partyId);
        if (pinfo.immutable)
            party.mGameId = 62721350051; // dummy game id
        auto playerId = pinfo.partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        auto numFields = pfields.size();
        for (size_t j = 0; j < numFields; ++j)
        {
            auto fdItr = fields.find(pfields[j]);
            EA_ASSERT(fdItr != fields.end());
            if (pinfo.immutable)
            {
                party.setPlayerPropertyValue(fdItr->second, partyPlayerIndex, nullptr); // immutable field has all property values unset
            }
            else
            {
                PropertyValue fieldValue = pvalues[j];
                party.setPlayerPropertyValue(fdItr->second, partyPlayerIndex, &fieldValue);
            }
        }

        packer.packGames(nullptr);
    }

    eastl::vector<party_info> partyInfos2 = {
        { 2645533680332, false, 1614353954596961    },
        { 2645533680333, false, 1614353954596962    },
    };

    eastl::vector<eastl::vector<const char8_t*>> partyFieldNames2 = {
        { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc","0","1","2","3","4","5","MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall","aws-nrt","aws-syd"  },
        { "aws-dub","aws-fra","aws-iad","aws-lhr","aws-sin","aws-sjc","0","1","2","3","4","5","MP_Discarded","MP_Hourglass","MP_Irreversible","MP_Kaleidoscope","MP_LongHaul","MP_Orbital","MP_Ridge","MP_TheWall","aws-nrt","aws-syd"  },
    };

    eastl::vector<eastl::vector<PropertyValue>> partyFieldValues2 = {
        { 124,144,73,132,176,21,0,1,2,3,4,5,0,0,0,0,0,0,0,0,104,143 },
        { 124,144,73,132,176,21,0,1,2,3,4,5,0,0,0,0,0,0,0,0,104,143 },
    };

    for (size_t i = 0; i < partyInfos2.size(); ++i)
    {
        auto& pinfo = partyInfos2[i];
        auto& pfields = partyFieldNames2[i];
        auto& pvalues = partyFieldValues2[i];

        packer.addPackerParty(pinfo.partyId, i, pinfo.expiryTime, pinfo.immutable);
        auto& party = *packer.getPackerParty(pinfo.partyId);
        auto playerId = pinfo.partyId;
        auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);
        auto numFields = pfields.size();
        for (size_t j = 0; j < numFields; ++j)
        {
            auto fdItr = fields.find(pfields[j]);
            EA_ASSERT(fdItr != fields.end());
            PropertyValue fieldValue = pvalues[j];
            party.setPlayerPropertyValue(fdItr->second, partyPlayerIndex, &fieldValue);
        }
    }

    packer.packGames(nullptr); // before fix infinite loop would occur here

    ASSERT_TRUE(packer.getGames().size() == 2);
}

}//ns 
}//ns BlazeServerTest