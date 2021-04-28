#pragma once

#include <SonarCommon/Common.h>

#if defined(_WIN32)
typedef SSIZE_T ssize_t;
#endif

namespace sonar
{
    template<typename T, unsigned int entryCount>
    class CircularQueue
    {
    public:

        CircularQueue()
            : qFront(0)
            , qLength(0)
        {
        }

        // Producer thread functions

        bool is_full()
        {
            return qLength == entryCount;
        }

        T* next_free()
        {
            CriticalSection::Locker lock(cs);
            if (is_full())
                return NULL;

            unsigned int nextFree = (qFront + qLength) % entryCount;
            entries[nextFree].reset();
            return &entries[nextFree];
        }

        void push_back()
        {
            if (is_full())
                return;

            ++qLength;
        }

        // Consumer thread functions

        bool is_empty()
        {
            return qLength == 0;
        }

        T* pop_front()
        {
            CriticalSection::Locker lock(cs);
            if (is_empty())
                return NULL;

            T* popped = &entries[qFront];
            qFront = ++qFront % entryCount;
            --qLength;
            return popped;
        }

    private:

        CriticalSection cs;
        T entries[entryCount];
        unsigned int qFront; // index to next free entry
        unsigned int qLength; // number of queued items
    };

    typedef SonarVector<char> Buffer;


	class ITimeProvider
	{
	public:
		virtual common::Timestamp getMSECTime() const = 0;
	};


	class AbstractTransport
	{
	public:
		virtual ~AbstractTransport(void) { };

        virtual void close() = 0;
        virtual bool isClosed() const = 0;

		/*
		Returns false on unrecoverable and permanent failure. */
		virtual bool rxMessagePoll(void *buffer, size_t *cbBuffer, ssize_t& socketResult) = 0;
		virtual bool txMessageSend(const void *buffer, size_t cbBuffer, ssize_t& socketResult) = 0;
        virtual sockaddr_in getLocalAddress() const { struct sockaddr_in local_addr; return local_addr; }
        virtual sockaddr_in getRemoteAddress() const { struct sockaddr_in remote_addr; return remote_addr; }

    private:
	};

	class INetworkProvider
	{
	public:
		virtual AbstractTransport *connect(CString &address) const = 0;
	};

	class IDebugLogger
	{
	public:
		virtual void printf(const char *format, ...) const = 0;
	};

    class IStatsProvider
    {
    public:

        virtual void resetAll() = 0;
        virtual void resetNonCumulativeStats() = 0;

        virtual unsigned int& audioBytesSent() = 0;
        virtual unsigned int& audioBytesReceived() = 0;

        virtual float& capturePreGainMin() = 0;
        virtual float& capturePreGainMax() = 0;
        virtual unsigned int& capturePreGainClip() = 0;
        virtual float& capturePostGainMin() = 0;
        virtual float& capturePostGainMax() = 0;
        virtual unsigned int& capturePostGainClip() = 0;
        virtual unsigned int& captureDeviceLost() = 0;
        virtual unsigned int& captureNoAudio() = 0;

        virtual float& playbackPreGainMin() = 0;
        virtual float& playbackPreGainMax() = 0;
        virtual unsigned int& playbackPreGainClip() = 0;

        virtual float& playbackPostGainMin() = 0;
        virtual float& playbackPostGainMax() = 0;
        virtual unsigned int& playbackPostGainClip() = 0;

        virtual unsigned int& audioMessagesSent() = 0;
        virtual unsigned int audioMessagesSent() const = 0;
        
        virtual unsigned int& audioMessagesReceived() = 0;
        virtual unsigned int audioMessagesReceived() const = 0;
        
        virtual unsigned int& audioMessagesOutOfSequence() = 0;
        virtual unsigned int audioMessagesOutOfSequence() const = 0;
        
        virtual unsigned int& audioMessagesLost() = 0;
        virtual unsigned int audioMessagesLost() const = 0;

        virtual unsigned int& badCid() = 0;
        virtual unsigned int& truncatedMessages() = 0;
        virtual unsigned int& audioMixClipping() = 0;
        virtual unsigned int& socketRecvError() = 0;
        virtual unsigned int& socketSendError() = 0;

        virtual unsigned int& deltaPlaybackMean() = 0;
        virtual unsigned int& deltaPlaybackMax() = 0;
        virtual unsigned int& receiveToPlaybackMean() = 0;
        virtual unsigned int& receiveToPlaybackMax() = 0;

        virtual int& jitterBufferDepth() = 0;
        virtual int const jitterBufferDepth() const = 0;

        virtual int& jitterArrivalMean() = 0;
        virtual int& jitterArrivalMax() = 0;
        virtual int& jitterPlaybackMean() = 0;
        virtual int& jitterPlaybackMax() = 0;
        virtual int& jitterArrivalMeanCumulative() = 0;
        virtual int& jitterArrivalMaxCumulative() = 0;
        virtual int& jitterPlaybackMeanCumulative() = 0;
        virtual int& jitterPlaybackMaxCumulative() = 0;
        virtual int& jitterNumMeasurements() = 0;

        virtual unsigned int& playbackUnderrun() = 0;
        virtual unsigned int& playbackOverflow() = 0;
        virtual unsigned int& playbackDeviceLost() = 0;
        virtual unsigned int& dropNotConnected() = 0;
        virtual unsigned int& dropReadServerFrameCounter() = 0;
        virtual unsigned int& dropReadTakeNumber() = 0;
        virtual unsigned int& dropTakeStopped() = 0;
        virtual unsigned int& dropReadCid() = 0;
        virtual unsigned int& dropNullPayload() = 0;
        virtual unsigned int& dropReadClientFrameCounter() = 0;
        virtual unsigned int& jitterbufferUnderrun() = 0;
    };

    // An interface for an entity that can supply a list of blocked user ids.
    typedef SonarVector<String> BlockedList;

    class IBlockedListProvider
    {
    public:

        virtual const BlockedList& blockList() const = 0;
        virtual bool hasChanges() const = 0;
        virtual void clearChanges() = 0;
    };

    // Stats sent back from the client to the server
    struct ClientToServerStats
    {
        unsigned int sent;
        unsigned int received;
        unsigned int outOfSequence;
        unsigned int lost;
        unsigned int badCid;
        unsigned int truncated;
        unsigned int audioMixClipping;
        unsigned int socketRecvError;
        unsigned int socketSendError;
        unsigned int deltaPlaybackMean;
        unsigned int deltaPlaybackMax;
        unsigned int receiveToPlaybackMean;
        unsigned int receiveToPlaybackMax;
        int jitterBufferDepth;
        int jitterArrivalMean;
        int jitterArrivalMax;
        int jitterPlaybackMean;
        int jitterPlaybackMax;
        unsigned int playbackUnderrun;
        unsigned int playbackOverflow;
        unsigned int playbackDeviceLost;
        unsigned int dropNotConnected;
        unsigned int dropReadServerFrameCounter;
        unsigned int dropReadTakeNumber;
        unsigned int dropTakeStopped;
        unsigned int dropReadCid;
        unsigned int dropNullPayload;
        unsigned int dropReadClientFrameCounter;
        unsigned int jitterbufferUnderrun;
        unsigned int networkLoss;
        unsigned int networkJitter;
        unsigned int networkPing;
        unsigned int networkReceived;
        unsigned int networkQuality;

        ClientToServerStats()
            : sent(0)
            , received(0)
            , outOfSequence(0)
            , lost(0)
            , badCid(0)
            , truncated(0)
            , audioMixClipping(0)
            , socketRecvError(0)
            , socketSendError(0)
            , deltaPlaybackMean(0)
            , deltaPlaybackMax(0)
            , receiveToPlaybackMean(0)
            , receiveToPlaybackMax(0)
            , jitterBufferDepth(0)
            , jitterArrivalMean(0)
            , jitterArrivalMax(0)
            , jitterPlaybackMean(0)
            , jitterPlaybackMax(0)
            , playbackUnderrun(0)
            , playbackOverflow(0)
            , playbackDeviceLost(0)
            , dropNotConnected(0)
            , dropReadServerFrameCounter(0)
            , dropReadTakeNumber(0)
            , dropTakeStopped(0)
            , dropReadCid(0)
            , dropNullPayload(0)
            , dropReadClientFrameCounter(0)
            , jitterbufferUnderrun(0)
            , networkLoss(0)
            , networkJitter(0)
            , networkPing(0)
            , networkReceived(0)
            , networkQuality(0)
        {
        }
    };

    enum
    {
        // Number of stats sent in MSG_CLIENT_STATS message - should match the number of fields
        CLIENT_STATS_COUNT = 22
    };

    class ClientStats : public IStatsProvider
    {
    public:

        ClientStats() { memberClear(); }

        ClientStats(IStatsProvider& from) {
            memberCopy(from);
        }

        ClientStats& operator=(IStatsProvider& from) {
            memberCopy(from);
            return *this;
        }

        void memberClear() {
            audioBytesSent_ = 0;
            audioBytesReceived_ = 0;

            capturePreGainMin_ = 0.0f;
            capturePreGainMax_ = 0.0f;
            capturePreGainClip_ = 0;
            capturePostGainMin_ = 0.0f;
            capturePostGainMax_ = 0.0f;
            capturePostGainClip_ = 0;
            captureDeviceLost_ = 0;
            captureNoAudio_ = 0;

            playbackPreGainMin_ = 0.0f;
            playbackPreGainMax_ = 0.0f;
            playbackPreGainClip_ = 0;

            playbackPostGainMin_ = 0.0f;
            playbackPostGainMax_ = 0.0f;
            playbackPostGainClip_ = 0;

            audioMessagesSent_ = 0;
            audioMessagesReceived_ = 0;
            audioMessagesOutOfSequence_ = 0;
            audioMessagesLost_ = 0;

            badCid_ = 0;
            truncatedMessages_ = 0;
            audioMixClipping_ = 0;
            socketRecvError_ = 0;
            socketSendError_ = 0;

            deltaPlaybackMean_ = 0;
            deltaPlaybackMax_ = 0;
            receiveToPlaybackMean_ = 0;
            receiveToPlaybackMax_ = 0;

            jitterBufferDepth_ = 0;
            jitterArrivalMean_ = 0;
            jitterArrivalMax_ = 0;
            jitterPlaybackMean_ = 0;
            jitterPlaybackMax_ = 0;
            jitterArrivalMeanCumulative_ = 0;
            jitterArrivalMaxCumulative_ = 0;
            jitterPlaybackMeanCumulative_ = 0;
            jitterPlaybackMaxCumulative_ = 0;
            jitterNumMeasurements_ = 0;

            playbackUnderrun_ = 0;
            playbackOverflow_ = 0;
            playbackDeviceLost_ = 0;
            dropNotConnected_ = 0;
            dropReadServerFrameCounter_ = 0;
            dropReadTakeNumber_ = 0;
            dropTakeStopped_ = 0;
            dropReadCid_ = 0;
            dropNullPayload_ = 0;
            dropReadClientFrameCounter_ = 0;
            jitterbufferUnderrun_ = 0;
        }

        void memberCopy(IStatsProvider& from) {
            audioBytesSent_ = from.audioBytesSent();
            audioBytesReceived_ = from.audioBytesReceived();

            capturePreGainMin_ = from.capturePreGainMin();
            capturePreGainMax_ = from.capturePreGainMax();
            capturePreGainClip_ = from.capturePreGainClip();
            capturePostGainMin_ = from.capturePostGainMin();
            capturePostGainMax_ = from.capturePostGainMax();
            capturePostGainClip_ = from.capturePostGainClip();
            captureDeviceLost_ = from.captureDeviceLost();
            captureNoAudio_ = from.captureNoAudio();

            playbackPreGainMin_ = from.playbackPreGainMin();
            playbackPreGainMax_ = from.playbackPreGainMax();
            playbackPreGainClip_ = from.playbackPreGainClip();

            playbackPostGainMin_ = from.playbackPostGainMin();
            playbackPostGainMax_ = from.playbackPostGainMax();
            playbackPostGainClip_ = from.playbackPostGainClip();

            audioMessagesSent_ = from.audioMessagesSent();
            audioMessagesReceived_ = from.audioMessagesReceived();
            audioMessagesOutOfSequence_ = from.audioMessagesOutOfSequence();
            audioMessagesLost_ = from.audioMessagesLost();

            badCid_ = from.badCid();
            truncatedMessages_ = from.truncatedMessages();
            audioMixClipping_ = from.audioMixClipping();
            socketRecvError_ = from.socketRecvError();
            socketSendError_ = from.socketSendError();

            deltaPlaybackMean_ = from.deltaPlaybackMean();
            deltaPlaybackMax_ = from.deltaPlaybackMax();
            receiveToPlaybackMean_ = from.receiveToPlaybackMean();
            receiveToPlaybackMax_ = from.receiveToPlaybackMax();

            jitterBufferDepth_ = from.jitterBufferDepth();
            jitterArrivalMean_ = from.jitterArrivalMean();
            jitterArrivalMax_ = from.jitterArrivalMax();
            jitterPlaybackMean_ = from.jitterPlaybackMean();
            jitterPlaybackMax_ = from.jitterPlaybackMax();
            jitterArrivalMeanCumulative_ = from.jitterArrivalMeanCumulative();
            jitterArrivalMaxCumulative_ = from.jitterArrivalMaxCumulative();
            jitterPlaybackMeanCumulative_ = from.jitterPlaybackMeanCumulative();
            jitterPlaybackMaxCumulative_ = from.jitterPlaybackMaxCumulative();
            jitterNumMeasurements_ = from.jitterNumMeasurements();

            playbackUnderrun_ = from.playbackUnderrun();
            playbackOverflow_ = from.playbackOverflow();
            playbackDeviceLost_ = from.playbackDeviceLost();
            dropNotConnected_ = from.dropNotConnected();
            dropReadServerFrameCounter_ = from.dropReadServerFrameCounter();
            dropReadTakeNumber_ = from.dropReadTakeNumber();
            dropTakeStopped_ = from.dropTakeStopped();
            dropReadCid_ = from.dropReadCid();
            dropNullPayload_ = from.dropNullPayload();
            dropReadClientFrameCounter_ = from.dropReadClientFrameCounter();
            jitterbufferUnderrun_ = from.jitterbufferUnderrun();
        }

        unsigned int& audioBytesSent() { return audioBytesSent_; }
        unsigned int& audioBytesReceived() { return audioBytesReceived_; }

        float& capturePreGainMin() { return capturePreGainMin_; }
        float& capturePreGainMax() { return capturePreGainMax_; }
        unsigned int& capturePreGainClip() { return capturePreGainClip_; }
        float& capturePostGainMin() { return capturePostGainMin_; }
        float& capturePostGainMax() { return capturePostGainMax_; }
        unsigned int& capturePostGainClip() { return capturePostGainClip_; }
        unsigned int& captureDeviceLost() { return captureDeviceLost_; }
        unsigned int& captureNoAudio() { return captureNoAudio_; }

        float& playbackPreGainMin() { return playbackPreGainMin_; }
        float& playbackPreGainMax() { return playbackPreGainMax_; }
        unsigned int& playbackPreGainClip() { return playbackPreGainClip_; }

        float& playbackPostGainMin() { return playbackPostGainMin_; }
        float& playbackPostGainMax() { return playbackPostGainMax_; }
        unsigned int& playbackPostGainClip() { return playbackPostGainClip_; }

        unsigned int& audioMessagesSent() { return audioMessagesSent_; }
        unsigned int audioMessagesSent() const { return audioMessagesSent_; }

        unsigned int& audioMessagesReceived() { return audioMessagesReceived_; }
        unsigned int audioMessagesReceived() const { return audioMessagesReceived_; }

        unsigned int& audioMessagesOutOfSequence() { return audioMessagesOutOfSequence_; }
        unsigned int audioMessagesOutOfSequence() const { return audioMessagesOutOfSequence_; }

        unsigned int& audioMessagesLost() { return audioMessagesLost_; }
        unsigned int audioMessagesLost() const { return audioMessagesLost_; }

        unsigned int& badCid() { return badCid_; }
        unsigned int& truncatedMessages() { return truncatedMessages_; }
        unsigned int& audioMixClipping() { return audioMixClipping_; }
        unsigned int& socketRecvError() { return socketRecvError_; }
        unsigned int& socketSendError() { return socketSendError_; }

        unsigned int& deltaPlaybackMean() { return deltaPlaybackMean_; }
        unsigned int& deltaPlaybackMax() { return deltaPlaybackMax_; }
        unsigned int& receiveToPlaybackMean() { return receiveToPlaybackMean_; }
        unsigned int& receiveToPlaybackMax() { return receiveToPlaybackMean(); }

        int& jitterBufferDepth() { return jitterBufferDepth_; }
        int const jitterBufferDepth() const { return jitterBufferDepth_; }

        int& jitterArrivalMean() { return jitterArrivalMean_; }
        int& jitterArrivalMax() { return jitterArrivalMax_; }
        int& jitterPlaybackMean() { return jitterPlaybackMean_; }
        int& jitterPlaybackMax() { return jitterPlaybackMax_; }
        int& jitterArrivalMeanCumulative() { return jitterArrivalMeanCumulative_; }
        int& jitterArrivalMaxCumulative() { return jitterArrivalMaxCumulative_; }
        int& jitterPlaybackMeanCumulative() { return jitterPlaybackMeanCumulative_; }
        int& jitterPlaybackMaxCumulative() { return jitterPlaybackMaxCumulative_; }
        int& jitterNumMeasurements() { return jitterNumMeasurements_; }

        unsigned int& playbackUnderrun() { return playbackUnderrun_; }
        unsigned int& playbackOverflow() { return playbackOverflow_; }
        unsigned int& playbackDeviceLost() { return playbackDeviceLost_; }
        unsigned int& dropNotConnected() { return dropNotConnected_; }
        unsigned int& dropReadServerFrameCounter() { return dropReadServerFrameCounter_; }
        unsigned int& dropReadTakeNumber() { return dropReadTakeNumber_; }
        unsigned int& dropTakeStopped() { return dropTakeStopped_; }
        unsigned int& dropReadCid() { return dropReadCid_; }
        unsigned int& dropNullPayload() { return dropNullPayload_; }
        unsigned int& dropReadClientFrameCounter() { return dropReadClientFrameCounter_; }
        unsigned int& jitterbufferUnderrun() { return jitterbufferUnderrun_; }

        // stuff to eliminate

        void resetAll() {}
        void resetNonCumulativeStats() {}

    private:

        unsigned int audioBytesSent_;
        unsigned int audioBytesReceived_;

        float capturePreGainMin_;
        float capturePreGainMax_;
        unsigned int capturePreGainClip_;
        float capturePostGainMin_;
        float capturePostGainMax_;
        unsigned int capturePostGainClip_;
        unsigned int captureDeviceLost_;
        unsigned int captureNoAudio_;

        float playbackPreGainMin_;
        float playbackPreGainMax_;
        unsigned int playbackPreGainClip_;

        float playbackPostGainMin_;
        float playbackPostGainMax_;
        unsigned int playbackPostGainClip_;

        unsigned int audioMessagesSent_;
        unsigned int audioMessagesReceived_;
        unsigned int audioMessagesOutOfSequence_;
        unsigned int audioMessagesLost_;

        unsigned int badCid_;
        unsigned int truncatedMessages_;
        unsigned int audioMixClipping_;
        unsigned int socketRecvError_;
        unsigned int socketSendError_;

        unsigned int deltaPlaybackMean_;
        unsigned int deltaPlaybackMax_;
        unsigned int receiveToPlaybackMean_;
        unsigned int receiveToPlaybackMax_;

        int jitterBufferDepth_;
        int jitterArrivalMean_;
        int jitterArrivalMax_;
        int jitterPlaybackMean_;
        int jitterPlaybackMax_;
        int jitterArrivalMeanCumulative_;
        int jitterArrivalMaxCumulative_;
        int jitterPlaybackMeanCumulative_;
        int jitterPlaybackMaxCumulative_;
        int jitterNumMeasurements_;

        unsigned int playbackUnderrun_;
        unsigned int playbackOverflow_;
        unsigned int playbackDeviceLost_;
        unsigned int dropNotConnected_;
        unsigned int dropReadServerFrameCounter_;
        unsigned int dropReadTakeNumber_;
        unsigned int dropTakeStopped_;
        unsigned int dropReadCid_;
        unsigned int dropNullPayload_;
        unsigned int dropReadClientFrameCounter_;
        unsigned int jitterbufferUnderrun_;

        int tail_; // field to demarcate the end of valid data
    };

}
