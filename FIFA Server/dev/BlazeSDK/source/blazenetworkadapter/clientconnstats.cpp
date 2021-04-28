/*************************************************************************************************/
/*!
\file
    clientconnstats.cpp

\note
    \verbatim

    clientconnstats is used to track connection statistics for each client

    \endverbatim

\attention
    (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "DirtySDK/dirtysock.h"
#include "BlazeSDK/internal/blazenetworkadapter/clientconnstats.h"

namespace Blaze
{

namespace BlazeNetworkAdapter
{
    ClientConnStats::ClientConnStats() :
        mDistProcTime(0),
        mDistWaitTime(0),
        mRpackLost(0),
        mLpackLost(0),
        mIdpps(0),
        mOdpps(0),
        mInputDistTotal(0),
        mOutputDistTotal(0),
        mDistOutboundProcessTotal(0),
        mDistOutboundProcessTimeTotal(0),
        mDistInboundWaitTimeTotal(0),
        mDistInboundDequeuedInputTotal(0),
        mIsDistStatValid(false)
    {
        ds_memclr(&mPrevLinkStat, sizeof(mPrevLinkStat));
    }

    /*! ************************************************************************************************/
    /*! \brief update pps dist statistics
    ***************************************************************************************************/
    void ClientConnStats::updateDistPpsStats(uint32_t curInputDistTotal, uint32_t curOutputDistTotal, uint32_t period)
    {
        // skip first samples
        if ((mInputDistTotal != 0) && (mOutputDistTotal != 0))
        {
            mIdpps = calculateRate(mInputDistTotal, curInputDistTotal, period);
            mOdpps = calculateRate(mOutputDistTotal, curOutputDistTotal, period);
            mIsDistStatValid = true;
        }
        else
        {
            mIsDistStatValid = false;
        }

        mInputDistTotal = curInputDistTotal;
        mOutputDistTotal = curOutputDistTotal;
    }

    /*! ************************************************************************************************/
    /*! \brief update avg time stats from netgamedist
    ***************************************************************************************************/
    void ClientConnStats::updateDistTimeStats(uint32_t curInboundWaitTimeTotal, uint32_t curInboundDequeuedInputTotal, uint32_t curDistOutboundProcessTimeTotal, uint32_t curDistOutboundProcessTotal)
    {
        mDistProcTime = calculateAvgTime(mDistOutboundProcessTimeTotal, mDistOutboundProcessTotal, curDistOutboundProcessTimeTotal, curDistOutboundProcessTotal);
        mDistWaitTime = calculateAvgTime(mDistInboundWaitTimeTotal, mDistInboundDequeuedInputTotal, curInboundWaitTimeTotal, curInboundDequeuedInputTotal);

        mDistOutboundProcessTimeTotal = curDistOutboundProcessTimeTotal;
        mDistOutboundProcessTotal = curDistOutboundProcessTotal;
        mDistInboundWaitTimeTotal = curInboundWaitTimeTotal;
        mDistInboundDequeuedInputTotal = curInboundDequeuedInputTotal;
    }

    /*! ************************************************************************************************/
    /*! \brief update stats from netgamelink
    ***************************************************************************************************/
    void ClientConnStats::updateLinkStats(NetGameLinkStatT *pCurLinkStat)
    {
        mRpackLost  = calculatePercentLost(mPrevLinkStat.rpacklost, mPrevLinkStat.lpacksent, pCurLinkStat->rpacklost, pCurLinkStat->lpacksent);
        mLpackLost  = calculatePercentLost(mPrevLinkStat.lpacklost, mPrevLinkStat.rpacksent, pCurLinkStat->lpacklost, pCurLinkStat->rpacksent);
        mLnaksent   = pCurLinkStat->lnaksent - mPrevLinkStat.lnaksent;
        mRnaksent   = pCurLinkStat->rnaksent - mPrevLinkStat.rnaksent;
        mLpackSaved = pCurLinkStat->lpacksaved - mPrevLinkStat.lpacksaved;
       
        ds_memcpy(&mPrevLinkStat, pCurLinkStat, sizeof(NetGameLinkStatT));
    }

    /*! ************************************************************************************************/
    /*! \brief calculate percentage lost
    ***************************************************************************************************/
    float ClientConnStats::calculatePercentLost(uint32_t prevLostValue, uint32_t prevSentValue, uint32_t curLostValue, uint32_t curSentValue)
    {
        if ((curLostValue < prevLostValue) || (curSentValue < prevSentValue))
        {
            return(0);
        }
        else
        {
            uint32_t deltaLost = curLostValue - prevLostValue;
            uint32_t deltaSent = curSentValue - prevSentValue;

            return((deltaSent != 0) ? (float)deltaLost * 100.0f / (float)deltaSent : 0);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief calculate avg times
    ***************************************************************************************************/
    float ClientConnStats::calculateAvgTime(uint32_t prevTotalTime, uint32_t prevTotalCount, uint32_t curTotalTime, uint32_t curTotalCount)
    {
        float avgTime;

        if (curTotalCount == prevTotalCount)
        {
            mIsDistStatValid = false;
            return(0);
        }

        avgTime = (float)NetTickDiff(curTotalTime, prevTotalTime) / (float)(curTotalCount - prevTotalCount);
        return(avgTime);
    }

    /*! ************************************************************************************************/
    /*! \brief calculate the rates
    ***************************************************************************************************/
    uint32_t ClientConnStats::calculateRate(uint32_t prevValue, uint32_t curValue, uint32_t period)
    {
        uint32_t uResult;

        // rollover detection
        if (curValue < prevValue)
        {
            uResult = 0xFFFFFFFF - (prevValue - curValue) + 1;
        }
        else
        {
            uResult = curValue - prevValue;
        }

        uResult /= (period / 1000);

        return(uResult);
    }
}
}