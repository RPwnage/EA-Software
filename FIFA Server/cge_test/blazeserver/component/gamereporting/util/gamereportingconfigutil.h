/*************************************************************************************************/
/*!
    \file   gamereportingconfig.h
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_CONFIG_UTIL_H
#define BLAZE_GAMEREPORTING_CONFIG_UTIL_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereporting/gamereporttdf.h"
#include "framework/tdf/controllertypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace GameReporting
{
class GameTypeCollection;
class GameHistoryConfigParser;


class GameReportingConfigUtil
{
NON_COPYABLE(GameReportingConfigUtil);

public:
    GameReportingConfigUtil(GameTypeCollection& gameTypeCollection, GameHistoryConfigParser& gameHistoryConfig);
    ~GameReportingConfigUtil() {}

    static void validateGameHistory(const GameHistoryConfig& config, ConfigureValidationErrors& validationErrors);
    static void validateGameTypeReportConfig(const GameTypeReportConfig& config, ConfigureValidationErrors& validationErrors);
    static void validateReportGameHistory(const char8_t* gameReportName, const GameTypeReportConfig& config, const GameReportContext& context, eastl::list<const char8_t*>& gameHistoryTables, ConfigureValidationErrors& validationErrors);
    static void validateReportStatsServiceConfig(const GameTypeReportConfig& config, ConfigureValidationErrors& validationErrors);
    static void validateGameTypeConfig(const GameTypeConfigMap& config, ConfigureValidationErrors& validationErrors);
    static void validateGameHistoryReportingQueryConfig(const GameHistoryReportingQueryList& config, const GameTypeConfigMap& gameTypeConfig, ConfigureValidationErrors& validationErrors);
    static void validateGameHistoryReportingViewConfig(const GameHistoryReportingViewList& config, const GameTypeConfigMap& gameTypeConfig, ConfigureValidationErrors& validationErrors);
    static void validateSkillInfoConfig(const SkillInfoConfig& skillInfoConfig, ConfigureValidationErrors& validationErrors);
    static void validateCustomEventConfig(const EventTypes& eventTypes, ConfigureValidationErrors& validationErrors);

    bool init(const GameReportingConfig& gameReportingConfig);

private:
    GameTypeCollection& mGameTypeCollection;
    GameHistoryConfigParser& mGameHistoryConfig;
};

} // GameReporting
} // Blaze

#endif  // BLAZE_GAMEREPORTING_CONFIG_UTIL_H
