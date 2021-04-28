/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIREP2ROTOCOL_H
#define BLAZE_FIREP2ROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/shared/fire2frame.h"
#include "framework/protocol/rpcprotocol.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Fire2Protocol : public RpcProtocol
{
    NON_COPYABLE(Fire2Protocol);

public:
    Fire2Protocol();
    ~Fire2Protocol() override;

    Type getType() const override { return Protocol::FIRE2; }
    void setMaxFrameSize(uint32_t maxFrameSize) override;
    bool receive(RawBuffer& buffer, size_t& needed) override;
    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;

    uint32_t getFrameOverhead() const override { return Fire2Frame::HEADER_SIZE + Fire2Frame::METADATA_SIZE_MAX; }
    const char8_t* getName() const override { return Fire2Protocol::getClassName(); }
    bool isPersistent() const override { return true; }
    bool supportsRaw() const override { return true; }
    bool supportsClientPing() const override { return true; }
    bool supportsNotifications() const override { return true; }
    uint32_t getNextSeqno() override
    {
        mNextSeqno = Fire2Frame::getNextMessageNum(mNextSeqno);
        return mNextSeqno;
    }
    ClientType getDefaultClientType() const override { return CLIENT_TYPE_GAMEPLAY_USER; }

    static const char8_t* getClassName() { return "fire2"; }

private:
    size_t mHeaderOffset;
    enum { READ_HEADER, READ_BODY } mState;
    uint32_t mNextSeqno;
    uint32_t mMaxFrameSize;
};

} // Blaze

#endif // BLAZE_FIRE2PROTOCOL_H

