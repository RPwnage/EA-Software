/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stressconnection.h"
#include "instanceresolver.h"
#include "rpcperformance.h"

#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/sslsocketchannel.h"
#include "EATDF/tdf.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/sslcontext.h"
#include "framework/system/fiber.h"

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

RpcPerformance& RpcPerformance::GetInstance()
{
	// Static instance 
	static RpcPerformance mInstance;

	return mInstance;
}

RpcPerformance::RpcPerformance()
{
	mPerformanceFile = NULL;
}

RpcPerformance::~RpcPerformance()
{
	if (mPerformanceFile != NULL)
	{
		fclose(mPerformanceFile);
		mPerformanceFile = NULL;
	}
	mDataList.clear();
}

void RpcPerformance::Add(uint16_t componentId, uint16_t commandId, int payloadSize, int responseTime, BlazeRpcError err)
{
	// Make the key
	uint32_t key = (componentId << 16 | commandId);

	PerformanceList::iterator pi = mDataList.find(key);
	if (pi != mDataList.end())
	{
		PeformanceInfo& perfInfo = pi->second;
		
		int totalPayloadSize = perfInfo.CallingCount * perfInfo.PayloadSizeAvg;
		int totalResponseTime = perfInfo.CallingCount * perfInfo.ResponseTimeAvg;

		// Increase the number of calling count
		perfInfo.CallingCount++;

		// Calculate the payload size data
		{
		if (payloadSize < perfInfo.PayloadSizeMin)
		{
			perfInfo.PayloadSizeMin = payloadSize;
		}
		if (payloadSize > perfInfo.PayloadSizeMax)
		{
			perfInfo.PayloadSizeMax = payloadSize;
		}
		perfInfo.PayloadSizeAvg = (totalPayloadSize + payloadSize) / perfInfo.CallingCount;
		}

		// Calculate the response time data
		if (responseTime < perfInfo.ResponseTimeMin)
		{
			perfInfo.ResponseTimeMin = responseTime;
		}
		if (responseTime > perfInfo.ResponseTimeMax)
		{
			perfInfo.ResponseTimeMax = responseTime;
		}

		perfInfo.ResponseTimeAvg = (totalResponseTime + responseTime) / perfInfo.CallingCount;

		if (err != ERR_OK)
		{
			perfInfo.Errors[err] += 1;
		}
	}
	else
	{
		PeformanceInfo perfInfo;
		perfInfo.ComponentId = componentId;
		perfInfo.CommandId = commandId;
		perfInfo.CallingCount = 1;
		perfInfo.PayloadSizeMin = perfInfo.PayloadSizeMax = perfInfo.PayloadSizeAvg = payloadSize;
		perfInfo.ResponseTimeMin = perfInfo.ResponseTimeMax = perfInfo.ResponseTimeAvg = responseTime;
		if (err != ERR_OK)
		{
			perfInfo.Errors[err] = 1;
		}

		mDataList[key] = perfInfo;
	}

	Flush();
}

// Flush the data to a file
void RpcPerformance::Flush()
{
	TimeValue now = TimeValue::getTimeOfDay();
	// Flush the data every 60 seconds
	int interval = 60 * 1000;
	if ( (now - mLastSaveTime).getMillis() < interval)
	{
		return;
	}
	mLastSaveTime = now;

	// Save to file
	if (mPerformanceFile == NULL)
	{
		uint32_t tyear, tmonth, tday, thour, tmin, tsec;
		TimeValue tnow = TimeValue::getTimeOfDay(); 
		tnow.getGmTimeComponents(tnow, &tyear, &tmonth, &tday, &thour, &tmin, &tsec);
		char8_t fName[256];
		blaze_snzprintf(fName, sizeof(fName), "%s/%s_%02d%02d%02d%02d%02d%02d.csv",(Logger::getLogDir()[0] != '\0')?Logger::getLogDir():".", "perf", tyear, tmonth, tday, thour, tmin, tsec);
    
		mPerformanceFile = fopen(fName, "wt");
		if (mPerformanceFile == NULL)
		{
			BLAZE_ERR_LOG(Log::SYSTEM,"[" << "RpcPerformance" << "] Failed to create the performance file("<< fName <<") in folder("<< Logger::getLogDir() << ")");
		}
		else
		{
			// Print the title
		}
	}
	else
	{
		// Overwrite the existing data - update only
		rewind(mPerformanceFile);

		char8_t timeStr[64];
		now.toString(timeStr, sizeof(timeStr));
		fprintf(mPerformanceFile, "LogTime, Component, Command, CallingCount, PayloadSizeMax, PayloadSizeMin, PayloadSizeAvg, ResponseTimeMax, ResponseTimeMin, ResponseTimeAvg, ErrorCount\n");

        for (PerformanceList::iterator i = mDataList.begin(), e = mDataList.end(); i != e; ++i)
		{
			PeformanceInfo& perfInfo = i->second;

			const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById( perfInfo.ComponentId);
			const char8_t* rpcName = BlazeRpcComponentDb::getCommandNameById(perfInfo.ComponentId, perfInfo.CommandId, NULL);
			if( blaze_strcmp(rpcName, "<UNKNOWN>") == 0)
			{
				rpcName = BlazeRpcComponentDb::getNotificationNameById(perfInfo.ComponentId, perfInfo.CommandId);
			}

			fprintf(mPerformanceFile, 
				"%s, %s, %s, "
				"%8d, " 
				"%8d, %8d, %8d, "
				"%8d, %8d, %8d, "
				"%8d "
				"\n", 
				timeStr, componentName, rpcName, 
				perfInfo.CallingCount, 
				perfInfo.PayloadSizeMax, perfInfo.PayloadSizeMin, perfInfo.PayloadSizeAvg, 
				perfInfo.ResponseTimeMax, perfInfo.ResponseTimeMin, perfInfo.ResponseTimeAvg, 
				(int)(perfInfo.Errors.size())
				);
		}

        fflush(mPerformanceFile);
	}
}

} // namespace Stress
} // namespace Blaze

