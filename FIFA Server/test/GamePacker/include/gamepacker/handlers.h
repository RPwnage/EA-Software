/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_HANDLERS_H
#define GAME_PACKER_HANDLERS_H

#include "gamepacker/common.h"

namespace Packer
{

struct Game;
struct Party;
struct PackerSilo;

struct PackerUtilityMethods
{
    virtual ~PackerUtilityMethods() {}
    virtual Time utilPackerMeasureTime() { return 0; }
    virtual Time utilPackerRetirementCooldown(PackerSilo& packer) { return 0; }
    virtual GameId utilPackerMakeGameId(PackerSilo& packer) { return -1; }
    virtual void utilPackerUpdateMetric(PackerSilo& packer, PackerMetricType type, int64_t delta) {}
    virtual void utilPackerLog(PackerSilo& packer, LogLevel level, const char8_t *fmt, va_list args) {}
    virtual bool utilPackerTraceEnabled(TraceMode mode) const { return false; }
    virtual bool utilPackerTraceFilterEnabled(TraceMode mode, int64_t entityId) const { return false; }
    virtual int32_t utilRand() { return 0; }
};

struct PackGameHandler
{
    virtual ~PackGameHandler() {}
    virtual void handlePackerImproveGame(const Game& game, const Party& incomingParty) {}
    virtual void handlePackerRetireGame(const Game& game) {}
    virtual void handlePackerEvictParty(const Game& game, const Party& evictedParty, const Party& incomingParty) {}
};

struct ReapGameHandler
{
    virtual ~ReapGameHandler() {}
    virtual void handlePackerReapGame(const Game& game, bool viable) {}
    virtual void handlePackerReapParty(const Game& game, const Party& party, bool expired) {}
};

}

#endif