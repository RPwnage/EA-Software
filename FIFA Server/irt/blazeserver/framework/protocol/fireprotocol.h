/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIREPROTOCOL_H
#define BLAZE_FIREPROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/shared/fireframe.h"
#include "framework/protocol/rpcprotocol.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class FireProtocol : public RpcProtocol
{
    NON_COPYABLE(FireProtocol);

public:
    FireProtocol();
    ~FireProtocol() override;

    Type getType() const override { return Protocol::FIRE; }
    void setMaxFrameSize(uint32_t maxFrameSize) override;
    bool receive(RawBuffer& buffer, size_t& needed) override;
    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;

    uint32_t getFrameOverhead() const override { return FireFrame::HEADER_SIZE + FireFrame::EXTRA_SIZE; }
    const char8_t* getName() const override { return FireProtocol::getClassName(); }
    bool isPersistent() const override { return true; }
    bool supportsRaw() const override { return true; }
    bool supportsNotifications() const override { return true; }
    uint32_t getNextSeqno() override
    {
        uint32_t seqno = mNextSeqno;
        mNextSeqno = (mNextSeqno + 1) & FireFrame::MSGNUM_MASK;
        return seqno;
    }
    ClientType getDefaultClientType() const override { return CLIENT_TYPE_GAMEPLAY_USER; }

    static const char8_t* getClassName() { return "fire"; }
protected:

private:
    size_t mHeaderOffset;
    enum { READ_HEADER, READ_EXTRA, READ_BODY } mState;
    uint32_t mNextSeqno;
    uint32_t mMaxFrameSize;
};

} // Blaze

#endif // BLAZE_FIREPROTOCOL_H

