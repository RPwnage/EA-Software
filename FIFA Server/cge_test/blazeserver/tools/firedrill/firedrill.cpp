#include <stdio.h>
#include <time.h>
#include <pcap.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "framework/blaze.h"
#include "framework/connection/socketutil.h"
#include "framework/protocol/shared/fire2frame.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/tdf/protocol.h" // for Fire2Metadata
#include "framework/util/shared/rawbuffer.h"
#include "framework/component/component.h"
#include "EATDF/tdf.h"
#include "EASTL/hash_set.h"
#include "EASTL/hash_map.h"

namespace firedrill
{
    using namespace Blaze;

    const static CommandId REPLICATION_MAP_CREATE_NOTIFICATION_ID = 0xFFF0;
    const static CommandId REPLICATION_MAP_DESTROY_NOTIFICATION_ID = 0xFFF1;
    const static CommandId REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID = 0xFFF2;
    const static CommandId REPLICATION_OBJECT_ERASE_NOTIFICATION_ID = 0xFFF3;
    const static CommandId REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID = 0xFFF4;
    const static CommandId REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID = 0xFFF5;
    const static CommandId REPLICATION_OBJECT_INSERT_NOTIFICATION_ID = 0xFFF6;

    struct ReassemblyInfo
    {
        uint32_t mSeqno;
        uint8_t* mPayload;
        size_t mPayloadSize;

        ReassemblyInfo()
            : mSeqno(0),
            mPayload(nullptr),
            mPayloadSize(0)
        {
        }

        ~ReassemblyInfo()
        {
            delete[] mPayload;
        }
    };
    typedef eastl::map<uint32_t, ReassemblyInfo> ReassemblyMap;

    struct EndpointInfo
    {
        uint32_t mSrcAddr;
        uint16_t mSrcPort;
        uint32_t mDstAddr;
        uint16_t mDstPort;
        RawBuffer mFrame;
        bool mAligned;
        uint32_t mNextSeqno;
        uint64_t mStartTime;
        uint64_t mEndTime;
        bool mSyn;
        bool mSynAck;
        bool mFin;
        bool mRst;
        FILE* mOut;

        ReassemblyMap mReassembly;

        EndpointInfo* mPair;

        EndpointInfo()
            : mSrcAddr(0),
            mSrcPort(0),
            mDstAddr(0),
            mDstPort(0),
            mFrame(1024),
            mAligned(false),
            mNextSeqno(0),
            mStartTime(0),
            mEndTime(0),
            mSyn(false),
            mSynAck(false),
            mFin(false),
            mRst(false),
            mOut(stdout),
            mPair(nullptr)
        {
        }

        ~EndpointInfo()
        {
            if ((mOut != nullptr) && (mOut != stdout))
                fclose(mOut);
        }
    };

    struct EndpointMetrics
    {
        uint32_t mCount;
        uint32_t mBytes;
        EndpointInfo* mEndpoint;

        EndpointMetrics() : mCount(0), mBytes(0), mEndpoint(nullptr) { }
    };

    typedef eastl::hash_map<uint64_t, int64_t> ReplicationUpdateMetricsMap;

    struct ReplicationCollectionMetrics
    {
        int32_t mUpdateCount;
        int32_t mCoalescedUpdateCount;
        ReplicationUpdateMetricsMap mUpdateMetrics;
        ReplicationCollectionMetrics() : mUpdateCount(0), mCoalescedUpdateCount(0) {}
    };

    typedef eastl::hash_map<eastl::string, EndpointInfo*> EndpointMap;
    static EndpointMap sEndpoints;

    typedef eastl::map<uint64_t, EndpointMetrics> MetricsMap;
    static MetricsMap sMetrics;

    typedef eastl::hash_map<uint64_t, ReplicationCollectionMetrics> ReplicationMetricsMap;
    static ReplicationMetricsMap sReplicationMetrics;

    static bool sPrintPayload = false;
    static bool sPrintFrames = false;
    static bool sPrintReplicationCoalesceStats = false;
    static int64_t sReplicationCoalesceDelayUsec = 50000;
    static bool sPrintEndpointsToSeparateFile = false;
    static uint64_t sTotalFrames = 0;
    static uint64_t sTotalPayloadSize = 0;
    static uint64_t sTotalDataSize = 0;
    static uint64_t sTotalFireSize = 0;
    static uint64_t sTotalPackets = 0;
    static uint64_t sTotalPacketRejects = 0;
    static uint64_t sTotalSeqnoRejects = 0;
    static uint64_t sTotal0PayloadRejects = 0;
    static uint64_t sTotalSeqnoMismatches = 0;
    static uint64_t sTotalRetransmissions = 0;
    static uint64_t sStartTime = 0;
    static uint64_t sEndTime = 0;
    static bool sFilterByType = false;
    static Fire2Frame::MessageType sFilterType = Fire2Frame::MESSAGE;
    static uint16_t sFilterComponent = RPC_COMPONENT_UNKNOWN;
    static uint16_t sFilterCommand = RPC_COMMAND_UNKNOWN;
    static uint32_t sDataLinkSize = sizeof(ether_header);

    // A network capture may start in the middle of a frame so use some simple heuristics to align the
    // first frame onto a real frame boundary rather than at the start of data on the stream
    static bool alignFrame(EndpointInfo& endpoint)
    {
        uint32_t offset = 0;
        Fire2Frame frame;
        for (offset = 0; ; ++offset)
        {
            if ((endpoint.mFrame.datasize() - offset) < Fire2Frame::HEADER_SIZE)
            {
                // Don't have enough for a basic frame header
                break;
            }

            // Simple check to make sure the 16th byte in the Fire2Frame header is == 0
            // This byte is currently reserved, and should always be 0.
            // NOTE: This will need to be changed if the reserved byte ever gets used.
            if (*(endpoint.mFrame.data() + offset + 15) == 0)
            {
                frame.setBuffer(endpoint.mFrame.data() + offset);
                if ((endpoint.mFrame.datasize() - offset) < frame.getHeaderSize())
                {
                    // Don't have enough for a full frame header
                    break;
                }

                if ((frame.getMsgType() == Fire2Frame::MESSAGE)
                    || (frame.getMsgType() == Fire2Frame::REPLY)
                    || (frame.getMsgType() == Fire2Frame::NOTIFICATION)
                    || (frame.getMsgType() == Fire2Frame::ERROR_REPLY)
                    || (frame.getMsgType() == Fire2Frame::PING)
                    || (frame.getMsgType() == Fire2Frame::PING_REPLY))
                {
                    if ((frame.getOptions() & ~Fire2Frame::OPTION_MASK) == 0)
                    {
                        if (BlazeRpcComponentDb::isValidComponentId(frame.getComponent()))
                        {
                            bool known = true;
                            if ((frame.getMsgType() != Fire2Frame::PING) && (frame.getMsgType() != Fire2Frame::PING_REPLY))
                            {
                                if (frame.getMsgType() == Fire2Frame::NOTIFICATION)
                                {
                                    BlazeRpcComponentDb::getNotificationNameById(frame.getComponent(), frame.getCommand(), &known);
                                }
                                else
                                {
                                    BlazeRpcComponentDb::getCommandNameById(frame.getComponent(), frame.getCommand(), &known);
                                }
                            }
                            if (known)
                            {
                                endpoint.mAligned = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (offset > 0)
        {
            memmove(endpoint.mFrame.data(), endpoint.mFrame.data() + offset, endpoint.mFrame.datasize() - offset);
            endpoint.mFrame.trim(offset);
        }
        return endpoint.mAligned;
    }

    static FILE* selectOutFp(EndpointInfo& endpoint)
    {
        FILE* outFp = stdout;
        if (endpoint.mOut != stdout)
            outFp = endpoint.mOut;
        else if (endpoint.mPair != nullptr)
        {
            outFp = endpoint.mPair->mOut;
        }
        return outFp;
    }

    static bool processFrame(const struct timeval& timestamp, EndpointInfo& endpoint, uint32_t tcpSeqno)
    {
        if (!endpoint.mAligned)
        {
            if (!alignFrame(endpoint))
                return false;
        }


        FILE* outFp = selectOutFp(endpoint);

        size_t size = endpoint.mFrame.datasize();
        if (size >= Fire2Frame::HEADER_SIZE)
        {
            Fire2Frame frame(endpoint.mFrame.data());
            if (size < frame.getHeaderSize())
                return false;

            size -= frame.getHeaderSize();
            if (size >= frame.getPayloadSize())
            {
                time_t secs = timestamp.tv_sec;
                struct tm* t = localtime(&secs);

                sTotalFireSize += frame.getTotalSize();

                if ((!sFilterByType || (sFilterByType && (sFilterType == frame.getMsgType())))
                    && ((sFilterComponent == RPC_COMPONENT_UNKNOWN) || (sFilterComponent == frame.getComponent()))
                    && ((sFilterCommand == RPC_COMMAND_UNKNOWN) || (sFilterCommand == frame.getCommand())))
                {
                    Fire2Metadata currentMetadata;
                    if (frame.getMetadataSize() > 0)
                    {
                        RawBuffer* raw = BLAZE_NEW RawBuffer(endpoint.mFrame.data() + Fire2Frame::HEADER_SIZE, frame.getMetadataSize());
                        eastl::intrusive_ptr<RawBuffer> ptr(raw);
                        raw->put(frame.getMetadataSize());
                        Heat2Decoder decoder;
                        BlazeRpcError rc = decoder.decode(*raw, currentMetadata, true);
                        if (rc != Blaze::ERR_OK)
                        {
                            fprintf(outFp, "# Failed to decode metadata\n");
                        }
                    }

                    if (sPrintFrames)
                    {
                        // We have a full frame so print it out
                        fprintf(outFp, "%d/%02d/%02d-%02d:%02d:%02d.%06ld ", 1900 + t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, timestamp.tv_usec);
                        fprintf(outFp, "src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d len=%u type=%s cmd=%s.",
                            NIPQUAD(endpoint.mSrcAddr), ntohs(endpoint.mSrcPort),
                            NIPQUAD(endpoint.mDstAddr), ntohs(endpoint.mDstPort),
                            frame.getPayloadSize(),
                            Fire2Frame::MessageTypeToString(frame.getMsgType()),
                            BlazeRpcComponentDb::getComponentNameById(frame.getComponent()));

                        const char8_t* commandName;
                        bool known = false;
                        if (frame.getMsgType() == Fire2Frame::NOTIFICATION)
                            commandName = BlazeRpcComponentDb::getNotificationNameById(frame.getComponent(), frame.getCommand(), &known);
                        else
                            commandName = BlazeRpcComponentDb::getCommandNameById(frame.getComponent(), frame.getCommand(), &known);
                        if (known)
                            fprintf(outFp, "%s", commandName);
                        else
                            fprintf(outFp, "%d(0x%x)", frame.getCommand(), frame.getCommand());
                        BlazeRpcError err = BLAZE_ERROR_FROM_CODE(frame.getComponent(), currentMetadata.getErrorCode());
                        const char8_t* errMsg = ErrorHelp::getErrorName(err);
                        char8_t errNum[64];
                        if (strcmp(errMsg, "UNKNOWN") == 0)
                        {
                            blaze_snzprintf(errNum, sizeof(errNum), "0x%08x", err);
                            errMsg = errNum;
                        }
                        fprintf(outFp, " code=%s user=%d seqno=%u ", errMsg, frame.getUserIndex(), frame.getMsgNum());

                        uint32_t options = frame.getOptions();
                        if (options != Fire2Frame::OPTION_NONE)
                        {
                            fprintf(outFp, "opt=0x%x", options);
                            char8_t optionsBuf[256];
                            fprintf(outFp, "%s", Fire2Frame::optionsToString(options, optionsBuf, sizeof(optionsBuf)));
                        }
                        if (currentMetadata.isContextSet())
                            fprintf(outFp, " ctx=0x%" PRIx64 " ", currentMetadata.getContext());
                        if (currentMetadata.isSessionKeySet())
                            fprintf(outFp, " sessKey=%s ", currentMetadata.getSessionKey());
                        fprintf(outFp, "frame=%" PRIu64 " tcp_seqno=%u\n", sTotalFrames, tcpSeqno);

                        if (sPrintPayload && (frame.getPayloadSize() > 0))
                        {
                            EA::TDF::TdfPtr tdf = nullptr;
                            bool decodeReplicationTdf = false;
                            switch (frame.getMsgType())
                            {
                            case Fire2Frame::MESSAGE:
                            {
                                const CommandInfo* info = ComponentData::getCommandInfo(frame.getComponent(), frame.getCommand());
                                if (info != nullptr)
                                {
                                    tdf = info->createRequest("Request");
                                }
                            }
                            break;
                            case Fire2Frame::REPLY:
                            {
                                const CommandInfo* info = ComponentData::getCommandInfo(frame.getComponent(), frame.getCommand());
                                if (info != nullptr)
                                {
                                    tdf = info->createResponse("Response");
                                }
                            }
                            break;
                            case Fire2Frame::ERROR_REPLY:
                            {
                                const CommandInfo* info = ComponentData::getCommandInfo(frame.getComponent(), frame.getCommand());
                                if (info != nullptr)
                                {
                                    tdf = info->createErrorResponse("ErrorResponse");
                                }

                            }
                            break;
                            case Fire2Frame::NOTIFICATION:
                            {
                                const NotificationInfo* info = ComponentData::getNotificationInfo(frame.getComponent(), frame.getCommand());
                                if (info != nullptr)
                                {
                                    tdf = info->payloadTypeDesc != nullptr ? info->payloadTypeDesc->createInstance(*Allocator::getAllocator(MEM_GROUP_FRAMEWORK_DEFAULT), "Notification") : nullptr;
                                }


                                if (tdf == nullptr)
                                {
                                    switch (frame.getCommand())
                                    {
                                    case REPLICATION_MAP_CREATE_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationMapCreationUpdate();
                                        break;
                                    case REPLICATION_MAP_DESTROY_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationMapDeletionUpdate();
                                        break;
                                    case REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationUpdate();
                                        decodeReplicationTdf = true;
                                        break;
                                    case REPLICATION_OBJECT_ERASE_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationDeletionUpdate();
                                        break;
                                    case REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationSynchronizationComplete();
                                        break;
                                    case REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationMapSynchronizationComplete();
                                        break;
                                    case REPLICATION_OBJECT_INSERT_NOTIFICATION_ID:
                                        tdf = BLAZE_NEW ReplicationUpdate();
                                        decodeReplicationTdf = true;
                                        break;
                                    }
                                }
                            }
                            break;
                            case Fire2Frame::PING:
                            case Fire2Frame::PING_REPLY:
                                // no request/response tdf will be contained in a PING packet
                                break;
                            }
                            if (tdf != nullptr)
                            {
                                RawBuffer* raw = BLAZE_NEW RawBuffer(endpoint.mFrame.data() + frame.getHeaderSize(), frame.getPayloadSize());
                                eastl::intrusive_ptr<RawBuffer> ptr(raw);
                                raw->put(frame.getPayloadSize());
                                Heat2Decoder decoder;
                                BlazeRpcError rc = decoder.decode(*raw, *tdf);
                                if (rc != Blaze::ERR_OK)
                                {
                                    fprintf(outFp, "# Failed to decode payload\n");
                                }
                                else
                                {
                                    StringBuilder s;
                                    tdf->print(s, 1);
                                    fprintf(outFp, "%s\n", s.get());
                                    if (decodeReplicationTdf)
                                    {
                                        ReplicationUpdate* replTdf = (ReplicationUpdate*)tdf.get();
                                        CollectionId collectionId(replTdf->getComponentId(), replTdf->getCollectionId(), replTdf->getCollectionType());
                                        tdf = nullptr;

                                        const ComponentData* compInfo = ComponentData::getComponentData(replTdf->getComponentId());
                                        if (compInfo != nullptr)
                                        {
                                            const ReplicationInfo* replInfo = compInfo->getReplicationInfo(collectionId);
                                            if (replInfo != nullptr)
                                            {
                                                tdf = replInfo->itemTypeDesc != nullptr ? replInfo->itemTypeDesc->createInstance(*Allocator::getAllocator(MEM_GROUP_FRAMEWORK_DEFAULT), "Replication") : nullptr;
                                                if (tdf != nullptr)
                                                {
                                                    rc = decoder.decode(*raw, *tdf);
                                                    if (rc != Blaze::ERR_OK)
                                                    {
                                                        fprintf(outFp, "# Failed to decode payload\n");
                                                    }
                                                    else
                                                    {
                                                        StringBuilder s;
                                                        tdf->print(s, 1);
                                                        fprintf(outFp, "%s\n", s.get());
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                fprintf(outFp, "# unable to display payload for frame; unknown tdf\n");
                            }
                        }
                    }

                    if (sPrintReplicationCoalesceStats && (frame.getPayloadSize() > 0))
                    {
                        if (frame.getMsgType() == Fire2Frame::NOTIFICATION)
                        {
                            if (frame.getCommand() == REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID)
                            {
                                ReplicationUpdate update;
                                RawBuffer* raw = BLAZE_NEW RawBuffer(endpoint.mFrame.data() + frame.getHeaderSize(), frame.getPayloadSize());
                                eastl::intrusive_ptr<RawBuffer> ptr(raw);
                                raw->put(frame.getPayloadSize());
                                Heat2Decoder decoder;
                                BlazeRpcError rc = decoder.decode(*raw, update);
                                if (rc == Blaze::ERR_OK)
                                {
                                    uint64_t key = ((uint64_t)update.getComponentId() << 48) | ((uint64_t)update.getCollectionType() << 32) | (uint64_t)update.getCollectionId();
                                    int64_t time = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
                                    ReplicationCollectionMetrics& collMetrics = sReplicationMetrics[key];
                                    ++collMetrics.mUpdateCount;
                                    ReplicationUpdateMetricsMap::insert_return_type ret =
                                        collMetrics.mUpdateMetrics.insert(eastl::make_pair(update.getObjectId(), time));
                                    if (!ret.second)
                                    {
                                        if ((time > ret.first->second) && (time - ret.first->second <= sReplicationCoalesceDelayUsec))
                                        {
                                            ++collMetrics.mCoalescedUpdateCount;
                                        }
                                        ret.first->second = time;
                                    }
                                }
                            }
                        }
                    }

                    // update metrics
                    uint64_t metricsKey = ((uint64_t)frame.getComponent() << 48) | ((uint64_t)frame.getCommand() << 32) | (uint64_t)frame.getMsgType();
                    EndpointMetrics& metrics = sMetrics[metricsKey];
                    metrics.mEndpoint = &endpoint;
                    ++metrics.mCount;
                    metrics.mBytes += frame.getTotalSize();
                    if (endpoint.mStartTime == 0)
                        endpoint.mStartTime = (((uint64_t)timestamp.tv_sec) * 1000000) + timestamp.tv_usec;
                    if (sStartTime == 0)
                        sStartTime = (((uint64_t)timestamp.tv_sec) * 1000000) + timestamp.tv_usec;
                    endpoint.mEndTime = (((uint64_t)timestamp.tv_sec) * 1000000) + timestamp.tv_usec;
                    sEndTime = endpoint.mEndTime;
                }

                ++sTotalFrames;

                // Consume this frame
                size = frame.getTotalSize();
                memmove(endpoint.mFrame.data(), endpoint.mFrame.data() + size, endpoint.mFrame.datasize() - size);
                endpoint.mFrame.trim(size);

                return true;
            }
        }
        return false;
    }

    static void handleData(const struct timeval& timestamp, EndpointInfo& endpoint, const uint8_t* payload, uint32_t payloadLen, uint32_t tcpSeqno, bool realign)
    {
        if (realign)
        {
            // Drop whatever data is in the buffer and re-enable the alignment flag.  This happens when it is detected that
            // packets were lost (generally due to tcpdump losing packets).
            endpoint.mAligned = false;
            endpoint.mFrame.reset();
        }

        uint8_t* ptr = endpoint.mFrame.acquire(payloadLen);
        if (ptr != nullptr)
        {
            memcpy(ptr, payload, payloadLen);
            endpoint.mFrame.put(payloadLen);

            while (processFrame(timestamp, endpoint, tcpSeqno))
                ;
        }
        return;
    }

    static void drainReassemblyMap(const timeval& timestamp, EndpointInfo& endpoint, bool realign)
    {
        if (endpoint.mReassembly.size() > 0)
        {
            // We have one or more "future" packets previously captured so see if we can tack them on the end of this frame
            ReassemblyMap::iterator itr = endpoint.mReassembly.begin();
            while (itr != endpoint.mReassembly.end())
            {
                ReassemblyInfo& info = itr->second;
                if (endpoint.mNextSeqno == info.mSeqno)
                {
                    endpoint.mNextSeqno = info.mSeqno + info.mPayloadSize;
                    handleData(timestamp, endpoint, info.mPayload, info.mPayloadSize, info.mSeqno, realign);
                    itr = endpoint.mReassembly.erase(itr);
                    continue;
                }
                break;
            }
        }
    }

    static void callback(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet)
    {
        sTotalPackets++;

        const struct iphdr* ip = (struct iphdr *)(packet + sDataLinkSize);

        // Ensure payload is TCP
        if (ip->protocol != IPPROTO_TCP)
        {
            ++sTotalPacketRejects;
            return;
        }

        struct tcphdr* tcp = (struct tcphdr*)(((char8_t*)ip) + (ip->ihl * 4));
        uint8_t* payload = ((uint8_t*)tcp) + (tcp->doff * 4);
        size_t payloadSize = ntohs(ip->tot_len) - (uint32_t)(payload - ((uint8_t*)ip));

        char8_t key[256];
        blaze_snzprintf(key, sizeof(key), "%d.%d.%d.%d.%d-%d.%d.%d.%d.%d", NIPQUAD(ip->saddr), tcp->source, NIPQUAD(ip->daddr), tcp->dest);
        EndpointInfo* endpoint;
        EndpointMap::iterator find = sEndpoints.find_as(key);
        if (find != sEndpoints.end())
        {
            endpoint = find->second;
        }
        else
        {
            endpoint = new EndpointInfo;
            endpoint->mSrcAddr = ip->saddr;
            endpoint->mSrcPort = tcp->source;
            endpoint->mDstAddr = ip->daddr;
            endpoint->mDstPort = tcp->dest;
            sEndpoints[key] = endpoint;

            // Check if we've already seen the other end of this connection
            char8_t pairKey[256];
            blaze_snzprintf(pairKey, sizeof(pairKey), "%d.%d.%d.%d.%d-%d.%d.%d.%d.%d", NIPQUAD(ip->daddr), tcp->dest, NIPQUAD(ip->saddr), tcp->source);
            EndpointMap::iterator pairItr = sEndpoints.find_as(pairKey);
            if (pairItr != sEndpoints.end())
            {
                endpoint->mPair = pairItr->second;
                pairItr->second->mPair = endpoint;
            }
            else
            {
                if (sPrintFrames && sPrintEndpointsToSeparateFile)
                {
                    char fname[256];
                    blaze_snzprintf(fname, sizeof(fname), "%d.%d.%d.%d.%d-%d.%d.%d.%d.%d.txt", NIPQUAD(ip->saddr), tcp->source, NIPQUAD(ip->daddr), tcp->dest);
                    FILE* fp = fopen(fname, "w");
                    if (fp != nullptr)
                        endpoint->mOut = fp;
                }
            }
        }

        FILE* outFp = selectOutFp(*endpoint);

        if (tcp->syn)
        {
            endpoint->mSyn = true;
            return;
        }
        if (tcp->fin)
        {
            if (!endpoint->mFin)
            {
                endpoint->mFin = true;
                if (endpoint->mPair != nullptr)
                    endpoint->mPair->mFin = true;

                if (sPrintFrames)
                {
                    time_t secs = header->ts.tv_sec;
                    struct tm* t = localtime(&secs);
                    fprintf(outFp, "%d/%02d/%02d-%02d:%02d:%02d.%06ld ", 1900 + t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, header->ts.tv_usec);
                    fprintf(outFp, "src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d tcp-connection-closed\n",
                        NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                        NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort));
                }
            }
            return;
        }
        if (tcp->rst)
        {
            if (!endpoint->mRst)
            {
                endpoint->mRst = true;
                if (endpoint->mPair != nullptr)
                    endpoint->mPair->mRst = true;

                if (sPrintFrames)
                {
                    time_t secs = header->ts.tv_sec;
                    struct tm* t = localtime(&secs);
                    fprintf(outFp, "%d/%02d/%02d-%02d:%02d:%02d.%06ld ", 1900 + t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, header->ts.tv_usec);
                    fprintf(outFp, "src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d tcp-connection-reset\n",
                        NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                        NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort));
                }
            }
            return;
        }
        if (tcp->ack)
        {
            if (endpoint->mSyn && (endpoint->mPair != nullptr) && endpoint->mPair->mSyn && !endpoint->mSynAck)
            {
                endpoint->mSynAck = true;
                endpoint->mPair->mSynAck = true;
                if (sPrintFrames)
                {
                    time_t secs = header->ts.tv_sec;
                    struct tm* t = localtime(&secs);
                    fprintf(outFp, "%d/%02d/%02d-%02d:%02d:%02d.%06ld ", 1900 + t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, header->ts.tv_usec);
                    fprintf(outFp, "src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d tcp-connection-established\n",
                        NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                        NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort));
                }
                return;
            }
        }

        if (endpoint->mFin || endpoint->mRst)
        {
            // Ignore packets that come after the connection has already been closed
            return;
        }

        uint32_t tcpSeq = ntohl(tcp->seq);

        sTotalDataSize += ntohs(ip->tot_len);

        sTotalPayloadSize += payloadSize;

        if (payloadSize == 0)
        {
            ++sTotal0PayloadRejects;
            return;
        }

        if ((endpoint->mNextSeqno == 0) || (tcpSeq >= endpoint->mNextSeqno))
        {
            bool realign = (endpoint->mNextSeqno == 0);
            bool shouldHandleData = true;
            if (!realign)
            {
                // Do super simple TCP reassembly
                if (tcpSeq > endpoint->mNextSeqno)
                {
                    ReassemblyMap::iterator find = endpoint->mReassembly.find(tcpSeq);
                    if (find != endpoint->mReassembly.end())
                    {
                        // Retransmission of future packet
                        fprintf(outFp, "# src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d; TCP retransmission of future packet; tcp_seqno=%u\n",
                            NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                            NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort),
                            tcpSeq);
                        ++sTotalRetransmissions;
                        shouldHandleData = false;
                    }
                    else
                    {
                        // The packet is in the future so store it off for future use
                        ReassemblyInfo& info = endpoint->mReassembly[tcpSeq];
                        info.mSeqno = tcpSeq;
                        info.mPayload = new uint8_t[payloadSize];
                        info.mPayloadSize = payloadSize;
                        memcpy(info.mPayload, payload, payloadSize);
                        shouldHandleData = false;

                        if (endpoint->mReassembly.size() == 8)
                        {
                            // We only track a fixed number of packets for reassmembly.  Anything more than that and we assume there is just packet loss
                            // from the network capture so we re-align and start fresh
                            ReassemblyInfo& info = endpoint->mReassembly.begin()->second;
                            fprintf(outFp, "# src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d; re-alignment due to lost packets; missing %d bytes; expecting tcp_seqno=%u but got tcp_seqno=%u\n",
                                NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                                NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort),
                                info.mSeqno - endpoint->mNextSeqno, endpoint->mNextSeqno, info.mSeqno);
                            ++sTotalSeqnoMismatches;
                            endpoint->mNextSeqno = info.mSeqno;
                            drainReassemblyMap(header->ts, *endpoint, true);
                        }
                    }
                }
                else if (tcpSeq < endpoint->mNextSeqno)
                {
                    // Must be a TCP retransmission
                    fprintf(outFp, "# src=%d.%d.%d.%d:%d dst=%d.%d.%d.%d:%d; TCP retransmission; expecting tcp_seqno=%u but got tcp_seqno=%u\n",
                        NIPQUAD(endpoint->mSrcAddr), ntohs(endpoint->mSrcPort),
                        NIPQUAD(endpoint->mDstAddr), ntohs(endpoint->mDstPort),
                        endpoint->mNextSeqno, tcpSeq);
                    ++sTotalRetransmissions;
                    shouldHandleData = false;
                }
            }

            if (shouldHandleData)
            {
                endpoint->mNextSeqno = tcpSeq + payloadSize;
                handleData(header->ts, *endpoint, payload, payloadSize, tcpSeq, realign);

                drainReassemblyMap(header->ts, *endpoint, false);
            }
        }
        else
        {
            ++sTotalSeqnoRejects;
        }

    }

    static int usage(const char8_t* prg)
    {
        printf("Usage: %s -i inputFile [-f filter] [-F [-P [-u]] [-E]] [-s] [-m [-M]] [-t] [-c [-C cmd | -N notif ]]\n", prg);
        printf("    -f filter : Specify a pcap filter\n");
        printf("    -F        : Print FIRE frames\n");
        printf("    -P        : Print FIRE payload\n");
        printf("    -u        : Use Heat2Util.log instead of Tdf.print to print payload\n");
        printf("    -s        : Print packet stats\n");
        printf("    -m        : Print FIRE frame metrics\n");
        printf("    -M        : Print FIRE frame summary metrics\n");
        printf("    -t        : Print only FIRE frames of given type\n");
        printf("    -c        : Print only FIRE frames of given component\n");
        printf("    -C cmd    : Print only FIRE frames of given command\n");
        printf("    -N notif  : Print only FIRE frames of given notification type\n");
        printf("    -E        : Used with -F/-P to print each endpoint to its own file.\n");
        printf("    -r delay  : Print summary counts of replication updates that would be coalesced given a specified delay(usec)\n");
        return 1;
    }

    int firedrill_main(int argc, char** argv)
    {
        BlazeRpcComponentDb::initialize();

        pcap_t *handle;
        char8_t errbuf[PCAP_ERRBUF_SIZE];
        struct bpf_program filter;
        const char8_t* inputFile = nullptr;
        char8_t* filterDesc = nullptr;
        bool printStats = false;
        bool printMetrics = false;
        bool printSummaryMetrics = false;
        const char8_t* filterComponent = nullptr;
        const char8_t* filterCommand = nullptr;
        const char8_t* filterNotification = nullptr;

        int o;
        while ((o = getopt(argc, argv, "i:f:FPsmMt:r:c:C:N:E")) != -1)
        {
            switch (o)
            {
            case 'i':
                inputFile = optarg;
                break;

            case 'f':
                filterDesc = optarg;
                break;

            case 'F':
                sPrintFrames = true;
                break;

            case 'P':
                sPrintFrames = true;
                sPrintPayload = true;
                break;

            case 'r':
                sPrintReplicationCoalesceStats = true;
                sReplicationCoalesceDelayUsec = (uint64_t)atoll(optarg);
                break;

            case 's':
                printStats = true;
                break;

            case 'm':
                printMetrics = true;
                break;

            case 'M':
                printSummaryMetrics = true;
                break;

            case 't':
                sFilterByType = true;
                if (!Fire2Frame::parseMessageType(optarg, sFilterType))
                {
                    printf("Unknown frame type specified: %s\n", optarg);
                    return 1;
                }
                break;

            case 'c':
                filterComponent = optarg;
                break;

            case 'C':
                filterCommand = optarg;
                break;

            case 'N':
                filterNotification = optarg;
                break;

            case 'E':
                sPrintEndpointsToSeparateFile = true;
                break;

            default:
                return usage(argv[0]);
            }
        }

        if (inputFile == nullptr)
            return usage(argv[0]);

        if (((filterCommand != nullptr) || (filterNotification != nullptr)) && (filterComponent == nullptr))
        {
            printf("To specify a command/notification filter you must also specify a component filter.\n");
            return 1;
        }

        if (filterComponent != nullptr)
        {
            sFilterComponent = BlazeRpcComponentDb::getComponentIdByName(filterComponent);
            if (sFilterComponent == RPC_COMPONENT_UNKNOWN)
            {
                blaze_str2int(filterComponent, &sFilterComponent);
                if (sFilterComponent == RPC_COMPONENT_UNKNOWN)
                {
                    printf("Unknown component specified: %s\n", filterComponent);
                    return 1;
                }
            }

            if (filterCommand != nullptr)
            {
                sFilterCommand = BlazeRpcComponentDb::getCommandIdByName(sFilterComponent, filterCommand);
                if (sFilterCommand == RPC_COMMAND_UNKNOWN)
                {
                    blaze_str2int(filterCommand, &sFilterCommand);
                    if (sFilterCommand == RPC_COMMAND_UNKNOWN)
                    {
                        printf("Unknown command specified: %s\n", filterCommand);
                        return 1;
                    }
                }
            }

            if (filterNotification != nullptr)
            {
                sFilterCommand = BlazeRpcComponentDb::getNotificationIdByName(sFilterComponent, filterNotification);
                if (sFilterCommand == RPC_COMMAND_UNKNOWN)
                {
                    blaze_str2int(filterNotification, &sFilterCommand);
                    if (sFilterCommand == RPC_COMMAND_UNKNOWN)
                    {
                        printf("Unknown command specified: %s\n", filterNotification);
                        return 1;
                    }
                }
            }
        }

        handle = pcap_open_offline(inputFile, errbuf);
        if (handle == nullptr)
        {
            printf("Unable to open input file: %s\n", errbuf);
            exit(1);
        }

        if (filterDesc != nullptr)
        {
            pcap_compile(handle, &filter, filterDesc, 1, 0);
            pcap_setfilter(handle, &filter);
        }

        uint32_t dataLinkType = pcap_datalink(handle);
        switch (dataLinkType)
        {
        case DLT_LINUX_SLL:
            sDataLinkSize = 16; //Linux cooked sockets
            break;

        case DLT_EN10MB:
            sDataLinkSize = sizeof(struct ether_header);
            break;

        default:
            printf("Unknown datalink type %s (%d); only ethernet and linux cooked are supported.\n",
                pcap_datalink_val_to_name(dataLinkType), dataLinkType);
            return 1;
        }

        pcap_loop(handle, -1, callback, nullptr);

        /* And close the session */
        pcap_close(handle);

        if (printStats)
        {
            printf("total_payload: %" PRIu64 "\n", sTotalPayloadSize);
            printf("total_data: %" PRIu64 "\n", sTotalDataSize);
            printf("total_fire: %" PRIu64 "\n", sTotalFireSize);
            printf("total_packets: %" PRIu64 "\n", sTotalPackets);
            printf("total_packet_rejects: %" PRIu64 "\n", sTotalPacketRejects);
            printf("total_0_payload_rejects: %" PRIu64 "\n", sTotal0PayloadRejects);
            printf("total_seqno_rejects: %" PRIu64 "\n", sTotalSeqnoRejects);
            printf("total_seqno_mismatches: %" PRIu64 "\n", sTotalSeqnoMismatches);
            printf("total_retransmissions: %" PRIu64 "\n", sTotalRetransmissions);
        }

        if (printMetrics)
        {
            printf("%-32s %-44s %-12s %12s %12s %12s %12s\n", "COMPONENT", "COMMAND", "TYPE", "COUNT", "BYTES", "BYTES/PKT", "PKT/S");
            MetricsMap::iterator itr = sMetrics.begin();
            MetricsMap::iterator end = sMetrics.end();
            uint32_t totalCount = 0;
            uint32_t totalBytes = 0;
            for (; itr != end; ++itr)
            {
                uint64_t key = itr->first;
                uint16_t component = key >> 48;
                uint16_t command = (key >> 32) & 0xffff;
                Fire2Frame::MessageType type = (Fire2Frame::MessageType)(key & 0xffffffff);

                bool known;
                const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(component, &known);
                char8_t componentBuf[64];
                if (!known)
                {
                    blaze_snzprintf(componentBuf, sizeof(componentBuf), "0x%04x", component);
                    componentName = componentBuf;
                }
                char8_t commandBuf[64];
                const char8_t* commandName;
                if (type == Fire2Frame::NOTIFICATION)
                {
                    commandName = BlazeRpcComponentDb::getNotificationNameById(component, command, &known);
                }
                else
                {
                    commandName = BlazeRpcComponentDb::getCommandNameById(component, command, &known);
                }
                if (!known)
                {
                    blaze_snzprintf(commandBuf, sizeof(commandBuf), "%d", command);
                    commandName = commandBuf;
                }
                EndpointMetrics& metrics = itr->second;

                printf("%-32s %-44s %-12s %12d %12d %12.2f %12.4f\n", componentName, commandName, Fire2Frame::MessageTypeToString(type), metrics.mCount, metrics.mBytes, (double)metrics.mBytes / (double)metrics.mCount, (double)metrics.mCount / (double)((sEndTime - sStartTime) / 1000000));

                totalCount += metrics.mCount;
                totalBytes += metrics.mBytes;
            }
            if (printSummaryMetrics)
            {
                printf("total_frames=%d\n", totalCount);
                printf("total_bytes=%d\n", totalBytes);
                printf("total_duration=%" PRIu64 "us\n", (sEndTime - sStartTime));
                printf("bytes/second=%.4f\n", (double)totalBytes / (double)((sEndTime - sStartTime) / 1000000));
                printf("frames/second=%.4f\n", (double)totalCount / (double)((sEndTime - sStartTime) / 1000000));
            }
        }

        if (sPrintReplicationCoalesceStats)
        {
            uint32_t totalUpdates = 0;
            uint32_t totalCoalescedUpdates = 0;
            for (ReplicationMetricsMap::const_iterator i = sReplicationMetrics.begin(), e = sReplicationMetrics.end(); i != e; ++i)
            {
                uint64_t key = i->first;
                uint16_t component = (uint16_t)(key >> 48);
                uint16_t type = (uint16_t)((key >> 32) & 0x0000FFFF);
                uint32_t collection = (uint32_t)(key);
                printf("component=%d, collection=%d, collectionType=%d, updateCount=%d, updateCoalescedCount=%d\n",
                    (uint32_t)component, collection, (uint32_t)type, i->second.mUpdateCount, i->second.mCoalescedUpdateCount);
                totalUpdates += i->second.mUpdateCount;
                totalCoalescedUpdates += i->second.mCoalescedUpdateCount;
            }
            printf("totalUpdateCount=%d, totalUpdateCoalescedCount=%d\n", totalUpdates, totalCoalescedUpdates);
        }

        EndpointMap::iterator itr = sEndpoints.begin();
        EndpointMap::iterator end = sEndpoints.end();
        for (; itr != end; ++itr)
            delete itr->second;
        return(0);
    }

}