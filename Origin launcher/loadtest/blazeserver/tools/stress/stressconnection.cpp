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
#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/sslsocketchannel.h"
#include "EATDF/tdf.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/sslcontext.h"
#include "framework/system/fiber.h"
#include "framework/connection/selector.h"

namespace Blaze
{
	namespace Stress
	{

		/*** Defines/Macros/Constants/Typedefs ***********************************************************/


		/*** Public Methods ******************************************************************************/

		const int32_t MAX_RETRY_ATTEMPTS = 10;

		StressConnection::StressConnection(int32_t id, bool secure, InstanceResolver& resolver)
			: mSecure(secure),
			mResolver(resolver),
			mChannel(NULL),
			mConnected(false),
			mSeqno(1),
			mRecvBuf(NULL),
			mIdent(id),
			mConnectionHandler(NULL),
			mLastRpcTimestamp(0),
			mLastReadTimestamp(0),
			mLastConnectionAttemptTimestamp(0),
			mClientType(CLIENT_TYPE_GAMEPLAY_USER),
			mResumeSession(false),
			mResumeSessionDelay(DEFAULT_RESUME_SESSION_DELAY_IN_MS),
			mIsResumingSession(false),
			mResumeSessionTimer(INVALID_TIMER_ID),
			mMigrating(false),
			mSendingPingToNewInstance(false),
			mResolvingInstanceByRdir(true),
			mRetryCount(0)
		{
			*mSessionKey = '\0';
		}

		bool StressConnection::initialize(const char8_t* protocolType, const char8_t* encoderType, const char8_t* decoderType, const uint32_t maxFrameSize)
		{
			mProtocol = (RpcProtocol *)ProtocolFactory::create(
				ProtocolFactory::getProtocolType(protocolType), maxFrameSize);
			mProtocol->setMaxFrameSize(maxFrameSize);
			mEncoder = static_cast<TdfEncoder*>(EncoderFactory::create(
				EncoderFactory::getEncoderType(encoderType)));
			mDecoder = static_cast<TdfDecoder*>(DecoderFactory::create(
				DecoderFactory::getDecoderType(decoderType)));

			if ((mProtocol == NULL) || (mEncoder == NULL) || (mDecoder == NULL))
			{
				if (mProtocol != NULL)
				{
					delete mProtocol;
					mProtocol = NULL;
				}
				if (mEncoder != NULL)
				{
					delete mEncoder;
					mEncoder = NULL;
				}
				if (mDecoder != NULL)
				{
					delete mDecoder;
					mDecoder = NULL;
				}
				return false;
			}
			return true;
		}


		StressConnection::~StressConnection()
		{
			if (mProtocol != NULL)
				delete mProtocol;
			if (mEncoder != NULL)
				delete mEncoder;
			if (mDecoder != NULL)
				delete mDecoder;
			if (mChannel != NULL)
			{
				delete mChannel;
				mChannel = NULL;
			}
			if (mRecvBuf != NULL)
				delete mRecvBuf;
			delete &mResolver;
		}

		bool StressConnection::connect(bool quiet)
		{
			//EA_ASSERT(!mConnected);
			BLAZE_TRACE_LOG(Log::CONNECTION, "[" << mIdent << "][StressConnection].connect: " << mAddress.getHostname() << ":" << (mAddress.getPort(InetAddress::HOST)));

			mLastConnectionAttemptTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();

			if (!mAddress.isResolved() && !mResolver.resolve(mClientType, &mAddress))
			{
				BLAZE_ERR_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].connect: unable to resolve instance address.");
				if (mMigrating)
				{
					reconnect();
				}
				return false;
			}

			mResolvingInstanceByRdir = false; // we have an valid coreSlave address to connect to
			BLAZE_INFO_LOG(Log::CONNECTION, "[" << mIdent << "] " << "Connecting instance to " << mAddress.getHostname() << ":" << (mAddress.getPort(InetAddress::HOST)));

			if (mSecure)
			{
				SslContextPtr sslContext = gSslContextManager->get();
				if (sslContext == NULL)
					return false;
				mChannel = BLAZE_NEW SslSocketChannel(mAddress, sslContext);
			}
			else
				mChannel = BLAZE_NEW SocketChannel(mAddress);

			// Turn off the linger option for the socket managed by mChannel.  This limits the number of sockets that remain
			// in the TIME_WAIT state.  This can be important due to the number of local ports that are used by a machine running
			// 10s of 1000s of stress clients.
			mChannel->setLinger(false, 0);

			// don't resolve the IP more than once to avoid burning CPU on frequent connect/disconnect
			mChannel->setResolveAlways(false);

			BlazeRpcError rc = mChannel->connect();
			if (rc != ERR_OK)
			{
				if (quiet)
				{
					BLAZE_TRACE_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].connect: "
						"channel failed to connect to " << mAddress.getHostname() << ":" << mAddress.getPort(InetAddress::HOST) << " (resumable error), err: "
						<< ErrorHelp::getErrorName(rc));
				}
				else
				{
					BLAZE_ERR_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].connect: "
						"channel failed to connect to " << mAddress.getHostname() << ":" << mAddress.getPort(InetAddress::HOST) << " (resumable error), err: "
						<< ErrorHelp::getErrorName(rc));
				}

				disconnect(false);
				// clear the address for reconnecting to redirector for new instance
				mAddress.clearResolved();

				if (mMigrating)
				{
					reconnect();
				}
				return false;
			}
			else if (mMigrating)
			{
				BLAZE_INFO_LOG(Log::CONNECTION,
					"[" << mIdent << "] [StressConnection].connect: connected to new server " << mAddress.getHostname() << ":" << mAddress.getPort(InetAddress::HOST)
					<< " during migration");
			}

			mChannel->setHandler(this);
			mChannel->addInterestOp(Channel::OP_READ);
			mConnected = true;

			if (mMigrating)
			{
				mSendingPingToNewInstance = true;
				if (mConnectionHandler != NULL)
				{
					mConnectionHandler->onUserSessionMigrated(this); // stress instance will send PING rpc
				}
			}

			return true;
		}

		void StressConnection::disconnect(bool resumeSession)
		{
			if (mIsResumingSession && !resumeSession && mConnectionHandler != NULL)
			{
				// we've failed to resume the session; trigger the handler
				mConnectionHandler->onSessionResumed(this, false);
			}

			if (mChannel != NULL)
			{
				mChannel->disconnect();
				delete mChannel;
				mChannel = NULL;
			}

			if (mConnected)
			{
				mConnected = false;

				if (!mMigrating)
				{
					// clear out the send queues
					mSentRequestBySeqno.clear();
					mSendQueue.clear();

					typedef eastl::vector<SemaphoreId> SemaphoreIdList;
					SemaphoreIdList semaphoreIds;
					semaphoreIds.reserve(mProxyInfo.size());
					for (ProxyInfoBySeqno::const_iterator i = mProxyInfo.begin(), e = mProxyInfo.end(); i != e; ++i)
						semaphoreIds.push_back(i->second.semaphoreId);
					mProxyInfo.clear();
					// signal all pending semaphores
					for (SemaphoreIdList::const_iterator i = semaphoreIds.begin(), e = semaphoreIds.end(); i != e; ++i)
						gFiberManager->signalSemaphore(*i, ERR_DISCONNECTED);

					if (!resumeSession)
						*mSessionKey = '\0';

					// trigger the handler
					if (mConnectionHandler != NULL)
						mConnectionHandler->onDisconnected(this, resumeSession);
				}
			}

			if (mResumeSessionTimer != INVALID_TIMER_ID)
			{
				gSelector->cancelTimer(mResumeSessionTimer);
				mResumeSessionTimer = INVALID_TIMER_ID;
			}

			mIsResumingSession = false;

			if (resumeSession)
			{
				TimeValue nextAttempt = (mLastConnectionAttemptTimestamp + mResumeSessionDelay * 1000);
				mResumeSessionTimer = gSelector->scheduleFiberTimerCall(nextAttempt, this, &StressConnection::resumeSession,
					"StressConnection::resumeSession", Fiber::CreateParams(Fiber::STACK_LARGE));
			}
			else
			{
				// force connect() to resolve the IP again unless we're attempting to resume the session
				mAddress.clearResolved();
			}
		}

		void StressConnection::setConnectionHandler(ConnectionHandler* handler)
		{
			mConnectionHandler = handler;
		}

		BlazeRpcError StressConnection::sendRequest(ComponentId component, CommandId command, const EA::TDF::Tdf* request,
			EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory/* = Blaze::Log::CONNECTION*/,
			const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = NULL*/)
		{
			VERIFY_WORKER_FIBER();

			if (!mConnected)
			{
				BLAZE_ERR_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendRequest: "
					"send failed, not connected.");
				return ERR_SYSTEM;
			}

			// Frame the request
			RawBufferPtr buffer = BLAZE_NEW RawBuffer(2048);
			Blaze::RpcProtocol::Frame outFrame;
			outFrame.componentId = component;
			outFrame.commandId = command;
			outFrame.messageType = RpcProtocol::MESSAGE;

			if (mMigrating && (*mSessionKey != '\0'))
			{
				// if user logout, sessionkey is reset
				outFrame.setSessionKey(mSessionKey);
			}

			mProtocol->setupBuffer(*buffer);
			if (request != NULL)
			{
				if (!mEncoder->encode(*buffer, *request, &outFrame))
				{
					BLAZE_WARN_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].sendRequest: "
						"failed to encode request for component: " << BlazeRpcComponentDb::getComponentNameById(outFrame.componentId) <<
						" rpc: " << BlazeRpcComponentDb::getCommandNameById(outFrame.componentId, outFrame.commandId, NULL));
				}
			}
			outFrame.msgNum = mSeqno++;

			int32_t frameSize = (int32_t)buffer->datasize();
			mProtocol->framePayload(*buffer, outFrame, frameSize, *mEncoder);

			if (!mMigrating)
			{
				// Hold the request until receives response in case migration happens.
				mSentRequestBySeqno[outFrame.msgNum] = buffer;
				mSendQueue.push_back(buffer);
				if (!flushSendQueue())
				{
					// flushSendQueue() will already output sufficient logging.
					return ERR_SYSTEM;
				}

				BLAZE_TRACE_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendRequest: frame(" << frameSize << ") " << component
					<< "/" << command << ", " << outFrame.msgNum);
			}
			else
			{
				if (!mSendingPingToNewInstance)
				{
					// HOLD HERE
					BLAZE_INFO_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendRequest: Hold frame(" << frameSize << ") " << component
						<< "/" << command << ", " << outFrame.msgNum << " while usersession migrating");
					mSentRequestBySeqno[outFrame.msgNum] = buffer;
					mSendQueue.push_back(buffer);
				}
				else
				{
					// sending Ping rpc to new server
					BLAZE_INFO_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendRequest: frame(" << frameSize << ") " << component
						<< "/" << command << ", " << outFrame.msgNum);
					writeFrame(buffer);
					mSendingPingToNewInstance = false;
				}
			}

			// Save info to response can be processed when it is received
			BlazeRpcError result;
			uint32_t payloadSize = 0;
			uint64_t payloadEncodingTimeUs = 0;
			ProxyInfo info;
			info.semaphoreId = gFiberManager->getSemaphore("StressConnection::sendRequest");
			info.error = &result;
			info.response = responseTdf;
			info.errorResponse = errorTdf;
			info.payloadSize = &payloadSize;
			info.payloadEncodingTimeUs = &payloadEncodingTimeUs;
			mProxyInfo[outFrame.msgNum] = info;

			mLastRpcTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();

			BlazeRpcError sleepError = gFiberManager->waitSemaphore(info.semaphoreId);
			if (sleepError != ERR_OK)
			{
				BLAZE_WARN_LOG(Log::CONNECTION,
					"[" << mIdent << "] [StressConnection].sendRequest: "
					"failed waiting for semaphore, err: " <<
					ErrorHelp::getErrorDescription(sleepError));
				return sleepError;
			}

			// Record the finished time
			int64_t endTime = TimeValue::getTimeOfDay().getMicroSeconds();
			int64_t msTimeSpan = endTime - mLastRpcTimestamp;

			//BLAZE_INFO_LOG(Log::CONNECTION, "Request = " << BlazeRpcComponentDb::getCommandNameById(outFrame.componentId, outFrame.commandId, NULL) << " Request== " << request);

			BLAZE_TRACE_LOG(Log::CONNECTION,
				"[" << mIdent << "]" <<
				BlazeRpcComponentDb::getComponentNameById(outFrame.componentId) <<
				":" << BlazeRpcComponentDb::getCommandNameById(outFrame.componentId, outFrame.commandId, NULL) << " " <<
				"ResultCode = " << result << " " <<
				"PayloadSize = " << payloadSize << " " <<
				"ResponseTime = " << (float_t)(msTimeSpan / 1000.0) << " ms");

			BLAZE_TRACE_LOG(Log::CONNECTION, "Error = " << errorTdf << " Response = " << responseTdf);

			return result;
		}


		void StressConnection::addBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler)
		{
			mBaseAsyncHandlers[(component << 16) | type] = handler;
		}

		void StressConnection::removeBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler)
		{
			mBaseAsyncHandlers.erase((component << 16) | type);
		}

		void StressConnection::addAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler)
		{
			AsyncHandlerList& handlers = mAsyncHandlers[(component << 16) | type];
			for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
			{
				if (*itr == handler)
				{
					return;//already added
				}
			}
			handlers.push_back(handler);
		}

		void StressConnection::removeAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler)
		{
			AsyncHandlerMap::iterator mapItr = mAsyncHandlers.find((component << 16) | type);
			if (mapItr != mAsyncHandlers.end())
			{
				AsyncHandlerList& handlers = mapItr->second;
				for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
				{
					if (*itr == handler)
					{
						handlers.erase(itr);
						break;
					}
				}
			}
		}

		/*** Private Methods *****************************************************************************/


		void StressConnection::onRead(Channel& channel)
		{
			EA_ASSERT(&channel == mChannel);
			while (mConnected)
			{
				if (mRecvBuf == NULL)
					mRecvBuf = BLAZE_NEW RawBuffer(512 * 1024); // The max payload size is same as the setting in BlazeSDK

				size_t needed = 0;
				if (!mProtocol->receive(*mRecvBuf, needed))
				{
					BLAZE_WARN_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].onRead: "
						"protocol receive failed, disconnecting (non-resumable error).");

					disconnect();
					return;
				}
				if (needed == 0)
				{
					processIncomingMessage();
					continue;
				}

				// Make sure the buffer is big enough to read everything we want
				if (mRecvBuf->acquire(needed) == NULL)
				{
					BLAZE_WARN_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].onRead: "
						"recv buffer failed to acquire " << needed << " bytes, disconnecting (non-resumable error).");
					disconnect();
					return;
				}

				// Try and read data off the channel
				int32_t bytesRead = mChannel->read(*mRecvBuf, needed);

				if (bytesRead == 0)
				{
					// No more data currently available
					mLastReadTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();
					return;
				}

				if (bytesRead < 0)
				{
					BLAZE_INFO_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].onRead: "
						"peer closed channel, disconnecting (resumable error).");

					if (mResolvingInstanceByRdir)
					{
						disconnect(false);
					}
					else
					{
						// connect to a random instance from redirector to see if our usersession is migrated
						mMigrating = false; // user migrating to another instance not supported in stress
						disconnect(false);
						gFiberManager->scheduleCall<StressConnection, bool>(this, &StressConnection::connect, false);
					}
					return;
				}
			}
		}

		void StressConnection::processIncomingMessage()
		{
			// Decode a frame
			RpcProtocol::Frame inFrame;
			mProtocol->process(*mRecvBuf, inFrame, *mDecoder);

			if (mIsResumingSession)
			{
				if (inFrame.errorCode == ERR_OK)
				{
					mIsResumingSession = false;
					if (mConnectionHandler != NULL)
						mConnectionHandler->onSessionResumed(this, true);
				}
				else
				{
					// Disconnect here, since this probably means the server refused 
					// the session key provided in the attempt to resume the session
					BLAZE_INFO_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].processIncomingMessage: "
						"failed to resume session, disconnecting (non-resumable error).");
					disconnect();
				}
			}

			RawBuffer* payload = mRecvBuf;
			mRecvBuf = NULL;
			int payloadSize = payload->datasize();

			if (inFrame.getSessionKey() != NULL)
				blaze_strnzcpy(mSessionKey, inFrame.getSessionKey(), sizeof(mSessionKey));

			if (inFrame.messageType == RpcProtocol::NOTIFICATION)
			{
				AsyncHandlerMap::iterator async = mAsyncHandlers.find((inFrame.componentId << 16) | inFrame.commandId);
				if (((async != mAsyncHandlers.end()) && !async->second.empty()) || mBaseAsyncHandlers.find((inFrame.componentId << 16) | inFrame.commandId) != mBaseAsyncHandlers.end())
				{
					//dispatch to subscribers
					gSelector->scheduleDedicatedFiberCall<StressConnection, uint16_t, uint16_t, Blaze::RawBuffer*>(this, &StressConnection::dispatchOnAsyncFiber, inFrame.componentId, inFrame.commandId, payload, BlazeRpcComponentDb::getNotificationNameById(inFrame.componentId, inFrame.commandId));
				}
			}
			else if (inFrame.messageType == RpcProtocol::REPLY || inFrame.messageType == RpcProtocol::ERROR_REPLY)
			{
				ProxyInfoBySeqno::iterator pi = mProxyInfo.find(inFrame.msgNum);
				if (pi != mProxyInfo.end())
				{
					ProxyInfo proxyInfo = pi->second;
					// Return the payload size
					*proxyInfo.payloadSize = payloadSize;
					mProxyInfo.erase(pi);

					*proxyInfo.error = static_cast<BlazeRpcError>(inFrame.errorCode);

					if (inFrame.messageType == RpcProtocol::REPLY && proxyInfo.response != NULL)
					{
						if (mMigrating)
						{
							// we have received the PING_REPLY from the new server
							mSendingPingToNewInstance = false;
							mRetryCount = 0; // reset the counter
							// Now we release all the holding RPCs
							for (SentRequestBySeqno::const_iterator i = mSentRequestBySeqno.begin(), e = mSentRequestBySeqno.end(); i != e; ++i)
							{
								BLAZE_INFO_LOG(Log::CONNECTION, "[StressConnection].processIncomingMessage:"
									" Re-send held request (Seqno: " << i->first << ") after usersession migration complete.");
								RawBufferPtr frame = i->second;
								writeFrame(frame);
							}
							mSentRequestBySeqno.clear(); // release all holding requests

							// The entire queue has been drained so clear any write ops on the selector and migration complete
							mChannel->removeInterestOp(Channel::OP_WRITE);
							mMigrating = false;
						}

						BlazeRpcError err = Blaze::ERR_OK;
						int64_t start = TimeValue::getTimeOfDay().getMicroSeconds();

						err = mDecoder->decode(*payload, *proxyInfo.response);
						int64_t end = TimeValue::getTimeOfDay().getMicroSeconds();
						*proxyInfo.payloadEncodingTimeUs = end - start;
						if (err != Blaze::ERR_OK)
						{
							BLAZE_WARN_LOG(Log::CONNECTION,
								"[" << mIdent << "] [StressConnection].processIncomingMessage: "
								"failed to decode response for component: " << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId)
								<< ", rpc: " << BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId, NULL)
								<< ", err: " << ErrorHelp::getErrorName(err));
						}
					}
					else if (inFrame.messageType == RpcProtocol::ERROR_REPLY && proxyInfo.errorResponse != NULL)
					{
						BlazeRpcError err = Blaze::ERR_OK;
						err = mDecoder->decode(*payload, *proxyInfo.errorResponse);
						if (err != Blaze::ERR_OK)
						{
							BLAZE_WARN_LOG(Log::CONNECTION,
								"[" << mIdent << "] [StressConnection].processIncomingMessage: "
								"failed to decode error response for component: " << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId)
								<< ", rpc: " << BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId, NULL)
								<< ", err: %s" << ErrorHelp::getErrorName(err));
						}
					}

					gFiberManager->signalSemaphore(proxyInfo.semaphoreId);
				}

				// migrating to the instance provided in the Fire2Metadata
				if (inFrame.movedToAddr.getIp(InetAddress::Order::HOST) != 0)
				{
					mMigrating = false;
					disconnect(false);

					mAddress = inFrame.movedToAddr;
					BLAZE_INFO_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].processIncoming: Migrating to new server " << mAddress.getIp() << ".");

					gFiberManager->scheduleCall<StressConnection, bool>(this, &StressConnection::connect, false);
				}

				// Ack the request if migration not happens
				if (inFrame.errorCode != ERR_MOVED)
				{
					SentRequestBySeqno::iterator si = mSentRequestBySeqno.find(inFrame.msgNum);
					if (si != mSentRequestBySeqno.end())
					{
						mSentRequestBySeqno.erase(si);
					}
				}
			}
			else if (inFrame.messageType == RpcProtocol::PING)
			{
				BLAZE_TRACE_LOG(Log::CONNECTION, "[StressConnection].processIncomingMessage:"
					" Received PING with msgId(" << inFrame.msgNum << "), userIndex(" << inFrame.userIndex << ")");

				Fiber::CreateParams params;
				params.namedContext = "StressConnection::sendPingReply";
				gFiberManager->scheduleCall<StressConnection, BlazeRpcError, uint16_t, uint16_t, uint32_t>(this, &StressConnection::sendPingReply, inFrame.componentId, inFrame.commandId, inFrame.msgNum, NULL, params);
			}
			else if (inFrame.messageType == RpcProtocol::PING_REPLY)
			{
				BLAZE_TRACE_LOG(Log::CONNECTION, "[StressConnection:" << this << "].processIncomingMessage:"
					" Received PING_REPLY with msgId(" << inFrame.msgNum << "), userIndex(" << inFrame.userIndex << ")");
			}

			// we must delete the payload here, TDFs no longer take ownership of the payload
			delete payload;
		}

		void StressConnection::dispatchOnAsyncFiber(uint16_t component, uint16_t command, RawBuffer* payload)
		{
			AsyncHandlerBaseMap::iterator basync = mBaseAsyncHandlers.find((component << 16) | command);
			if (basync != mBaseAsyncHandlers.end())
			{
				basync->second->onAsyncBase(component, command, payload);
				return;
			}

			AsyncHandlerMap::iterator async = mAsyncHandlers.find((component << 16) | command);
			if (async != mAsyncHandlers.end())
			{
				payload->mark();
				AsyncHandlerList& handlers = async->second;
				for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
				{
					(*itr)->onAsync(component, command, payload);
					// we need to reset to mark, as each subscriber's onAsync may advance payload's data ptr to the tail
					payload->resetToMark();
				}
			}
		}

		void StressConnection::onWrite(Channel& channel)
		{
			if (!mMigrating)
				flushSendQueue();
		}

		void StressConnection::onError(Channel& channel)
		{
			BLAZE_WARN_LOG(Log::CONNECTION,
				"[" << mIdent << "] [StressConnection].onError: "
				"channel signaled error, disconnecting (resumable error).");
			disconnect(mResumeSession);

		}

		void StressConnection::onClose(Channel& channel)
		{
			BLAZE_INFO_LOG(Log::CONNECTION,
				"[" << mIdent << "] [StressConnection].onClose: "
				"channel signaled close, disconnecting (resumable error).");
			disconnect(mResumeSession);

		}

		bool StressConnection::writeFrame(RawBufferPtr frame)
		{
			int32_t res = mChannel->write(*frame);
			if (res < 0)
			{
				BLAZE_WARN_LOG(Log::CONNECTION,
					"[" << mIdent << "] [StressConnection].writeOutputQueue: "
					"failed writing output queue, socket error: " << res << ", disconnecting (resumable error).");

				disconnect(mResumeSession);
				return false;
			}

			if (frame->datasize() != 0)
			{
				// There's still more to write but the socket is full so enable write notifications
				mChannel->addInterestOp(Channel::OP_WRITE);
				return true;
			}

			return true;
		}

		bool StressConnection::flushSendQueue()
		{
			while (!mSendQueue.empty() && !mMigrating)
			{
				RawBufferPtr frame = mSendQueue.front();
				mSendQueue.pop_front();
				writeFrame(frame);
			}

			if (mConnected)
			{
				// The entire queue has been drained so clear any write ops on the selector
				mChannel->removeInterestOp(Channel::OP_WRITE);
			}

			return true;
		}

		BlazeRpcError StressConnection::sendPingReply(uint16_t component, uint16_t command, uint32_t msgId)
		{
			VERIFY_WORKER_FIBER();

			if (!mConnected)
			{
				BLAZE_ERR_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendPingReply: send failed, not connected.");
				return ERR_SYSTEM;
			}

			// Frame the request
			RawBuffer* frame = BLAZE_NEW RawBuffer(2048);
			Blaze::RpcProtocol::Frame outFrame;
			outFrame.componentId = component;
			outFrame.commandId = command;
			outFrame.messageType = RpcProtocol::PING_REPLY;

			mProtocol->setupBuffer(*frame);

			// no body to encode

			outFrame.msgNum = msgId;

			int32_t frameSize = (int32_t)frame->datasize();
			mProtocol->framePayload(*frame, outFrame, frameSize, *mEncoder);

			writeFrame(frame);

			BLAZE_TRACE_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].sendPingReply: frame(" << frameSize << ") "
				<< component << "/" << command << ", " << outFrame.msgNum);

			return ERR_OK;
		}

		void StressConnection::resumeSession()
		{
			ASSERT_WORKER_FIBER();

			mIsResumingSession = true;
			if (!mConnected)
			{
				// If connect() fails here, it will schedule another call 
				// to resumeSession() if appropriate
				if (!connect())
					return;
			}

			// Frame the request
			RawBuffer* frame = BLAZE_NEW RawBuffer(2048);
			Blaze::RpcProtocol::Frame outFrame;
			outFrame.componentId = 0;
			outFrame.commandId = 0;
			outFrame.messageType = RpcProtocol::PING;
			outFrame.userIndex = 0;
			outFrame.setSessionKey(mSessionKey);

			mProtocol->setupBuffer(*frame);

			// no body to encode

			outFrame.msgNum = mSeqno++;

			int32_t frameSize = (int32_t)frame->datasize();
			mProtocol->framePayload(*frame, outFrame, frameSize, *mEncoder);

			mSendQueue.push_back(frame);
			if (!flushSendQueue())
			{
				// writeOutputQueue() will already output sufficient logging, and
				// will schedule another call to resumeSession() if appropriate
				return;
			}

			BLAZE_TRACE_LOG(Log::CONNECTION, "[" << mIdent << "] [StressConnection].resumeSession: sent frame(" << frameSize << ") " << outFrame.msgNum);
		}

		void StressConnection::reconnect()
		{
			if (mRetryCount < MAX_RETRY_ATTEMPTS)
			{
				mRetryCount++;
				BlazeRpcError error = gSelector->sleep(2 * 1000 * 1000);
				if (error != Blaze::ERR_OK)
				{
					BLAZE_ERR_LOG(Log::CONNECTION,
						"[" << mIdent << "] [StressConnection].connect: Sleep expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
				}

				BLAZE_INFO_LOG(Log::CONNECTION,
					"[" << mIdent << "] [StressConnection].connect: Issue a retry attempt, current retry count(" << mRetryCount << ")");
				connect(false);
			}
			else
			{
				mMigrating = false;
				BLAZE_ERR_LOG(Log::CONNECTION,
					"[" << mIdent << "] [StressConnection].connect: Maximum number of reconnection attempts (" << MAX_RETRY_ATTEMPTS << ") has been reached.");
			}
		}

	} // namespace Stress
} // namespace Blaze

