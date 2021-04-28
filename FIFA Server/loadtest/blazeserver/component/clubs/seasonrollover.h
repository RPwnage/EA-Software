/*************************************************************************************************/
/*!
    \file   rollover.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CLUBSS_SEASONROLLOVER_H
#define BLAZE_CLUBSS_SEASONROLLOVER_H

#include "clubs/tdf/clubs.h"
#include "clubs/tdf/clubs_server.h"


namespace Blaze
{

namespace Clubs
{

class ClubsSlaveImpl;

class SeasonRollover
{
public:
    SeasonRollover()
    : mComponent(nullptr),
      mCurrentSeasonState(CLUBS_IN_SEASON),
      mRolloverItem(nullptr),
      mRollover(0),
      mStart(0),
      mRolloverTimerId(0),
      mNumRetry(0)
    {
    }
   
    ~SeasonRollover();

    void setComponent(ClubsSlaveImpl* component) { mComponent = component; }

    void parseRolloverData(const SeasonSettingItem &seasonSettings);
    int64_t getRolloverTimer(int64_t t);
    void setRolloverTimer();
    SeasonState getSeasonState() const { return mCurrentSeasonState; }

    void finishedEndOfSeasonProcessing() {
        mCurrentSeasonState = CLUBS_IN_SEASON;
        mNumRetry = 0;
    }

    void clear();
    int64_t getRollover() const { return mRollover; }
    int64_t getSeasonStart() const { return mStart; }

    static void gmTime(struct tm *tM, int64_t t);
    static int64_t mkgmTime(struct tm *tM);

private:
    void timerExpired();

    ClubsSlaveImpl* mComponent;
    int32_t mCurrentSeasonId;
    SeasonState mCurrentSeasonState;
    const RolloverItem* mRolloverItem;

    int64_t mRollover;
    int64_t mStart;

    TimerId mRolloverTimerId;
    int32_t mNumRetry;
};

} // namespace Stats
} // namespace Blaze
#endif
