/*************************************************************************************************/
/*!
\file clientconnstats.h


\attention
(c) Electronic Arts. All Rights Reserved.

*************************************************************************************************/

#ifndef BLAZE_NETWORKADAPTER_CLIENTCONNSTATS_H
#define BLAZE_NETWORKADAPTER_CLIENTCONNSTATS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "DirtySDK/game/netgamelink.h"

namespace Blaze
{

namespace BlazeNetworkAdapter
{
class BLAZESDK_API ClientConnStats
{
public:
    ClientConnStats();
    void updateDistPpsStats(const uint32_t curInputDistTotal, const uint32_t curOutputDistTotal, const uint32_t period); // update pps stats from netgamedist
    void updateDistTimeStats(const uint32_t curWaitTimeTotal, const uint32_t curDequeuedInputTotal, const uint32_t curDistInputProcessTimeTotal, const uint32_t curDistInputProcessTotal); // update avg time stats from netgamedist
    void updateLinkStats(NetGameLinkStatT *pLinkStat); // update stats from netgamelink

    // getters for stats values
    float getDistProcTime()     const { return(mDistProcTime); }
    float getDistWaitTime()     const { return(mDistWaitTime); }
    float getRpackLost()        const { return(mRpackLost); }
    float getLpackLost()        const { return(mLpackLost); }
    uint32_t getIdpps()         const { return(mIdpps); }
    uint32_t getOdmpps()        const { return(mOdpps); }
    uint32_t getLnaksent()      const { return(mLnaksent); }
    uint32_t getRnaksent()      const { return(mRnaksent); }
    uint32_t getLpackSaved()    const { return(mLpackSaved); }
    bool isDistStatValid()      const { return(mIsDistStatValid); }

private:
    float calculatePercentLost(const uint32_t prevLostValue, const uint32_t prevSentValue, const uint32_t curLostValue, const uint32_t curSentValue); // calculate percentage lost
    float calculateAvgTime(const uint32_t prevTotalTime, const uint32_t prevTotalCount, const uint32_t curTotalTime, const uint32_t curTotalCount); // calculate avg times
    uint32_t calculateRate(const uint32_t prevValue, const uint32_t curValue, const uint32_t period); // calculate the rates

    // metrics we are sending to pin
    float mDistProcTime;
    float mDistWaitTime;
    float mRpackLost;
    float mLpackLost;
    uint32_t mIdpps;
    uint32_t mOdpps;
    uint32_t mLnaksent;
    uint32_t mRnaksent;
    uint32_t mLpackSaved;

    // use to track prev sampled totals from dirtysock
    uint32_t mInputDistTotal;
    uint32_t mOutputDistTotal;
    uint32_t mDistOutboundProcessTotal;
    uint32_t mDistOutboundProcessTimeTotal;
    uint32_t mDistInboundWaitTimeTotal;
    uint32_t mDistInboundDequeuedInputTotal;
    NetGameLinkStatT mPrevLinkStat;

    // are samples valid
    bool mIsDistStatValid;

};//ClientConnStats

}//BlazeNetworkAdapter
}//Blaze

#endif
