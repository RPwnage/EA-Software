/*************************************************************************************************/
/*!
    \file   glickoskill.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_GLICKOSKILL
#define BLAZE_GAMEREPORTING_GLICKOSKILL

#include "EATDF/tdfobjectid.h"

namespace Blaze
{
namespace GameReporting
{

/*** Defines *****************************************************************/
#define WLT_WIN          0x0001   // Bit vector: user won the game
#define WLT_LOSS         0x0002   // Bit vector: user lost the game
#define WLT_TIE          0x0004   // bit vector: user tied the game

// New ranking formula tweaking parameters.
// Adjust only with *extreme caution*.
#define GLICKO_NEW_RATING_MINIMUM   1
#define GLICKO_MINIMUM_POINTS_GAIN  10
#define GLICKO_DEVIATION_CEILING    200
#define GLICKO_DEVIATION_FLOOR      30
#define GLICKO_C_SQUARED            651
#define GLICKO_Q                    0.0057564627324851142100449786367109 // calculated by ln(10) / 400, constant used in glicko algorithm
#define GLICKO_Q_SQUARED            0.000033136863190489987566010414928713 //above q, squared
#define GLICKO_PI_SQUARED           9.8696044010893586188344909998762
#define GLICKO_WIN                  1.0
#define GLICKO_TIE                  0.5
#define GLICKO_LOSS                 0.0
#define GLICKO_SECONDS_PER_DAY      86400 //60 seconds * 60 minutes * 24 hours

class GlickoSkill
{
public:
    GlickoSkill(int32_t skillValue = 0);
    ~GlickoSkill();

    struct GlickoSkillInfo
    {
        GlickoSkillInfo() : mWLT(0), mDampingPercent(0), mTeamId(0), mOnsetRd(0), mCurrentSkill(0), mCurrentRd(0)
        {
        }

        void set(int32_t wlt, int32_t dampingPercent, int32_t teamId, double onsetRd, int32_t currentGlickoSkill, int32_t currentGlickoRd)
        {
            mWLT = wlt;
            mDampingPercent = dampingPercent;
            mTeamId = teamId;
            mOnsetRd = onsetRd;
            mCurrentSkill = currentGlickoSkill;
            mCurrentRd = currentGlickoRd;
        }

        int32_t getWLT() const { return mWLT; }
        int32_t getDampingPercent() const { return mDampingPercent; }
        int32_t getTeamId() const { return mTeamId; }
        double getOnsetRd() const { return mOnsetRd; }
        int32_t getCurrentSkill() const { return mCurrentSkill; }
        int32_t getCurrentRd() const { return mCurrentRd; }

    private:
        int32_t mWLT;
        int32_t mDampingPercent;
        int32_t mTeamId;
        double mOnsetRd;
        int32_t mCurrentSkill;
        int32_t mCurrentRd;
    };

    const GlickoSkillInfo *getGlickoSkillInfo(EntityId entityId) const 
    {
        GlickoSkillInfoMap::const_iterator itr = mGlickoSkillInfoMap.find(entityId);
        return itr != mGlickoSkillInfoMap.end() ? &(itr->second) : nullptr; 
    }

    void addGlickoSkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId,
        int32_t periodsPassed, int32_t currentGlickoSkill, int32_t currentGlickoRd);

    void getNewGlickoSkillAndRd(EntityId entityId, int32_t &newGlickoSkill, int32_t &newGlickoRd);

private:
    double calcOnsetRd(int32_t userRd, int32_t periodsSinceLastRated) const;
    double determineGameResult(int32_t wlt, int32_t oppWlt) const;
    void calcSkillPointAndRdChange(const GlickoSkillInfo *skillInfo, int32_t &pointsDelta, int32_t &rdDelta) const;

private:
    int32_t mMaxSkillValue;

    typedef eastl::hash_map<EntityId, GlickoSkillInfo> GlickoSkillInfoMap;
    GlickoSkillInfoMap mGlickoSkillInfoMap;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_GLICKOSKILL

