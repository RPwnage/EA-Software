/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef RPCPERFORMANCE_H
#define RPCPERFORMANCE_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/socketchannel.h"
#include "framework/component/message.h"
#include "framework/component/rpcproxysender.h"
#include "framework/component/rpcproxyresolver.h"

#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;
class SocketChannel;
class Channel;
class InetAddress;
class TdfEncoder;
class TdfDecoder;
class Protocol;

namespace Stress
{

class RpcPerformance
{
public:

	static	RpcPerformance&	GetInstance		();

   						   ~RpcPerformance	();

			void			Add				(uint16_t componentId, uint16_t commandId, int payloadSize, int responseTime, BlazeRpcError err);

private:
    typedef struct
    {
		uint16_t	ComponentId;
		uint16_t	CommandId;
		
		int			CallingCount;

		int			PayloadSizeMax;
		int			PayloadSizeMin;
		int			PayloadSizeAvg;
		
		int			ResponseTimeMax;
		int			ResponseTimeMin;
		int			ResponseTimeAvg;

        eastl::hash_map<Blaze::BlazeRpcError, int> Errors;

    } PeformanceInfo;


	// Data list
    typedef eastl::hash_map<uint32_t/*WORD(component,rpc)*/, PeformanceInfo> PerformanceList;
	PerformanceList mDataList;

	// File to store the data
	FILE*				mPerformanceFile;
	EA::TDF::TimeValue	mLastSaveTime;

private:
					// singleton constructors 
					RpcPerformance	();

	void			Flush			();

};

} // namespace Stress
} // namespace Blaze

#endif // RPCPERFORMANCE_H

