/*************************************************************************************************/
/*!
\file   sponsoredevents_commands.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!

Implmentation of multiple commands defined for the sponsored event

*/
/***************
**********************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "sponsoredevents/rpc/sponsoredeventsslave/banuser_stub.h"
#include "sponsoredevents/rpc/sponsoredeventsslave/numusers_stub.h"
#include "sponsoredevents/rpc/sponsoredeventsslave/removeuser_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/updateuserflags_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/wipeuserstats_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/wipeuserstats_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/updateeventdata_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/getdbcredentials_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/checkuserregistration_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/registeruser_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/geteventsurl_stub.h" 
#include "sponsoredevents/rpc/sponsoredeventsslave/returnusers_stub.h" 

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

// sponsoredevents includes
#include "sponsoredeventsslaveimpl.h"
#include "sponsoredevents/tdf/sponsoredeventstypes.h"

namespace Blaze
{
	namespace SponsoredEvents
	{
#define SPONSORED_EVENT_STUB_CLASS(cls)					cls##Stub

#define SPONSORED_EVENT_COMMAND_IMPLEMENTATION(command, input, commandCode)																\
		class command : public SPONSORED_EVENT_STUB_CLASS(command)																		\
		{																																\
		public:																															\
			command(Message* message, input* request, SponsoredEventsSlaveImpl* componentImpl)											\
				: SPONSORED_EVENT_STUB_CLASS(command)(message, request), mComponent(componentImpl)										\
			{ }																															\
			virtual ~command() { }																										\
																																		\
		private:																														\
			SPONSORED_EVENT_STUB_CLASS(command)::Errors execute()																		\
			{																															\
			return static_cast<SPONSORED_EVENT_STUB_CLASS(command)::Errors>(mComponent->command commandCode );							\
			}																															\
			SponsoredEventsSlaveImpl* mComponent;																						\
		};																																\
																																		\
		SPONSORED_EVENT_STUB_CLASS(command)* SPONSORED_EVENT_STUB_CLASS(command)::create(Message *msg, input* request, SponsoredEventsSlave *component)\
		{																																\
		return BLAZE_NEW command(msg, request, static_cast<SponsoredEventsSlaveImpl*>(component));									\
		}

#define SPONSORED_EVENT_COMMAND_IMPLEMENTATION_NO_INPUT(command, commandCode)															\
		class command :	public SPONSORED_EVENT_STUB_CLASS(command)																		\
		{																																\
		public:																															\
			command(Message* message, SponsoredEventsSlaveImpl* componentImpl)															\
			: SPONSORED_EVENT_STUB_CLASS(command)(message), mComponent(componentImpl)													\
			{ }																															\
			virtual ~command() { }																										\
																																		\
		private:																														\
			SPONSORED_EVENT_STUB_CLASS(command)::Errors execute()																		\
			{																															\
				return static_cast<SPONSORED_EVENT_STUB_CLASS(command)::Errors>(mComponent->command commandCode );						\
			}																															\
			SponsoredEventsSlaveImpl* mComponent;																						\
		};																																\
																																		\
		SPONSORED_EVENT_STUB_CLASS(command)* SPONSORED_EVENT_STUB_CLASS(command)::create(Message *msg, SponsoredEventsSlave *component)	\
		{																																\
			return BLAZE_NEW command(msg, static_cast<SponsoredEventsSlaveImpl*>(component));											\
		}

		//implementing the commands
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(BanUserCommand, BanUserRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(NumUsersCommand, NumUsersRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(RemoveUserCommand, RemoveUserRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(UpdateUserFlagsCommand, UpdateUserFlagsRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(WipeUserStatsCommand, WipeUserStatsRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(UpdateEventDataCommand, UpdateEventDataRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(CheckUserRegistrationCommand, CheckUserRegistrationRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(RegisterUserCommand, RegisterUserRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION(ReturnUsersCommand, ReturnUsersRequest, (mRequest, mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION_NO_INPUT(GetDbCredentialsCommand, (mResponse));
		SPONSORED_EVENT_COMMAND_IMPLEMENTATION_NO_INPUT(GetEventsURLCommand, (mResponse));
		
#undef SPONSORED_EVENT_COMMAND_IMPLEMENTATION_NO_INPUT
#undef SPONSORED_EVENT_COMMAND_IMPLEMENTATION
#undef SPONSORED_EVENT_STUB_CLASS
	}
}
