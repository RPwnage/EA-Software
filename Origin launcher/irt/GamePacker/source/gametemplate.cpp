#include <EAStdC/EACType.h>
#include <EAStdC/EAString.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/gametemplate.h"

namespace Packer
{

// {{
// TODO: Eventually these vars will be replaced by a more expressive property system whereby values of properties like game.gameCapacity will probably be derived via expresssion evaluation.
// NOTE: Currently these property names mirror constants defined in matchmaking_properties_config_server.tdf, in the future both of these sets will be defined via a common derived property system.
const char8_t* GameTemplate::sConfigVarNames[(int32_t)GameTemplateVar::E_COUNT + 1] = {
    "game.teamCount",           // GameManager::PROPERTY_NAME_TEAM_COUNT
    "game.teamCapacity",        // GameManager::PROPERTY_NAME_TEAM_CAPACITY
    "game.participantCapacity", // GameManager::PROPERTY_NAME_PARTICIPANT_CAPACITY
    "invalid!"
};
// }}

bool GameTemplate::isLastFactorIndex(GameQualityFactorIndex factorIndex) const
{
    return mFactors.back().mFactorIndex == factorIndex;
}

void GameTemplate::setVar(GameTemplateVar var, ConfigVarValue value)
{
    EA_ASSERT(var < GameTemplateVar::E_COUNT);
    mConfigVars[(int32_t)var] = value;
}

bool GameTemplate::setVarByName(const char8_t* varName, ConfigVarValue value)
{
    auto result = false;
    int32_t i = 0;
    for (auto name : sConfigVarNames)
    {
        if (EA::StdC::Stricmp(varName, name) == 0)
        {
            mConfigVars[i] = value;
            result = true;
        }
        i++;
    }
    return result;
}

ConfigVarValue GameTemplate::getVar(GameTemplateVar var) const
{
    return mConfigVars[(int32_t)var];
}

float GameTemplate::evalVarExpression(const char8_t* expression) const
{
    int32_t i = 0;
    for (auto name : sConfigVarNames)
    {
        if (EA::StdC::Stricmp(expression, name) == 0)
        {
            return (float) mConfigVars[i];
        }
        i++;
    }
    // if not a variable then its a literal
    float result = EA::StdC::AtoF32(expression);
    return result;
}

bool GameTemplate::validateConfigVar(const char8_t* varName)
{
    for (auto name : sConfigVarNames)
    {
        if (EA::StdC::Stricmp(varName, name) == 0)
            return true;
    }
    return false;
}

GameTemplate::GameTemplate() : mViableGameCooldownThreshold(INT64_MAX)
{
    memset(mConfigVars, 0, sizeof(mConfigVars));
}


} // GamePacker
