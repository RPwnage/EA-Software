/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_GAMETEMPLATE_H
#define GAME_PACKER_GAMETEMPLATE_H

#include "gamepacker/qualityfactor.h"

namespace Packer
{

struct GameTemplate
{
    ConfigVarValue mConfigVars[(int32_t)GameTemplateVar::E_COUNT + 1];
    int64_t mViableGameCooldownThreshold; // how much time the game can remain inactive and viable before being a candidate for reaping
    GameQualityFactorList mFactors; // [active] factors used for optimization
    static const char8_t* sConfigVarNames[(int32_t)GameTemplateVar::E_COUNT + 1];

    bool isLastFactorIndex(GameQualityFactorIndex factorIndex) const;
    void setVar(GameTemplateVar var, ConfigVarValue value);
    bool setVarByName(const char8_t* varName, ConfigVarValue value);
    ConfigVarValue getVar(GameTemplateVar var) const;
    float evalVarExpression(const char8_t* expression) const;
    static bool validateConfigVar(const char8_t* varName);
    static constexpr const char8_t* getConfigVarName(GameTemplateVar var);
    GameTemplate();
};

inline constexpr const char8_t* GameTemplate::getConfigVarName(GameTemplateVar var)
{
    return sConfigVarNames[(int32_t)var];
}

}
#endif

