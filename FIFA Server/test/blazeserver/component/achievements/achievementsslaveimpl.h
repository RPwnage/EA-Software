/*************************************************************************************************/
/*!
    \file   achievementsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_ACHIEVEMENTS_ACHIEVEMENTSSLAVEIMPL_H
#define BLAZE_ACHIEVEMENTS_ACHIEVEMENTSSLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "achievements/tdf/achievements.h"
#include "achievements/rpc/achievementsslave_stub.h"
#include "achievements/tdf/achievements_server.h"
#include "framework/connection/outboundhttpconnectionmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Achievements
{
// Forward declarations
class AchievementsConfig;

class AchievementsSlaveImpl : public AchievementsSlaveStub
{
public:
    AchievementsSlaveImpl() {}
    ~AchievementsSlaveImpl() override {}

    enum ConnectionManagerType
    {
        UNTRUSTED,
        TRUSTED,
        MAX
    };
    HttpConnectionManagerPtr getConnectionManagerPtr(ConnectionManagerType type) const;

    Encoder::Type getRequestEncoderType() const { return Encoder::JSON; } 

    void generateSignature(uint8_t* body, size_t bodyLen, int64_t creationTime, const char8_t* secretKey, char8_t* signature, size_t signatureLen) const;

    BlazeRpcError fetchUserOrServerAuthToken(AuxiliaryAuthentication& auxAuth, bool isTrustedConnection);

    // The achievement service returns time in seconds whereas TimeValue thinks of it in microseconds. We scale the time value by post processing the response.
    void scaleAchivementDataTimeValue(AchievementData& data);
    void scaleUserHistoryDataTimeValue(UserHistoryData& data);
private:
    bool onConfigure() override;
    bool onReconfigure() override;

}; //class AchievementsSlaveImpl

} // Achievements
} // Blaze
#endif // BLAZE_ACHIEVEMENTS_ACHIEVEMENTSSLAVEIMPL_H
