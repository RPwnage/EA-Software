/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MESSAGE_H
#define BLAZE_MESSAGE_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/protocol/rpcprotocol.h"
#include "framework/connection/session.h"
#include "EASTL/intrusive_ptr.h"
#include "EATDF/tdfobjectid.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace google
{
namespace protobuf
{
class Message;
}
}
namespace Blaze
{

class PeerInfo;
class RawBuffer;

typedef eastl::intrusive_ptr<RawBuffer> RawBufferPtr;

// Represents an incoming Request or Response - will later be used to build a Request or Response
// subclass instance (e.g., AuthenticationRequest)
class Message
{
public:
    Message(SlaveSessionId slaveSessionId, UserSessionId userSessionId, PeerInfo& peerInfo,
        const RpcProtocol::Frame &frame, RawBufferPtr payload, const google::protobuf::Message* request);
 
    SlaveSessionId getSlaveSessionId() const { return mSlaveSessionId; }
    UserSessionId getUserSessionId() const { return mUserSessionId; }
    PeerInfo& getPeerInfo() const { return mPeerInfo; }
    uint32_t getConnectionIndex() const { return mFrame.userIndex; }
    const RpcProtocol::Frame &getFrame() const { return mFrame; }
    
    EA::TDF::ComponentId getComponent() const { return mFrame.componentId; }
    CommandId getCommand() const { return mFrame.commandId; }
    uint32_t getMsgNum() const { return mFrame.msgNum; }

    uint32_t getLocale() const { return mLocale; }

    RawBufferPtr& getPayloadPtr() { return mPayload; }
    const google::protobuf::Message* getProtobufRequest() const { return mProtobufRequest; }
protected:
    SlaveSessionId mSlaveSessionId;
    UserSessionId mUserSessionId;
    PeerInfo& mPeerInfo;
    const RpcProtocol::Frame mFrame;
    RawBufferPtr mPayload;    
    uint32_t mLocale;

    const google::protobuf::Message* mProtobufRequest;
    NON_COPYABLE(Message)
};

} // Blaze

#endif // BLAZE_MESSAGE_H

