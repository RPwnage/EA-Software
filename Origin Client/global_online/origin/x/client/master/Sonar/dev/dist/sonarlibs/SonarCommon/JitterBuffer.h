#pragma once
#include <assert.h>
#include <SonarCommon/Common.h> // UINT32
#include <queue>    // std::priority_queue

#if defined(_WIN32)
#define PRId64 "I64d"
#else
#define __STDC_FORMAT_MACROS 
#include <inttypes.h>
#endif

namespace {
    static sonar::INT64 firstPush = 0;
    static unsigned int numPushes = 0;
    static sonar::INT64 firstPop = 0;
    static unsigned int numPops = 0;
};

namespace sonar
{
    // ASSUMPTION: 'pop' is called at the frame rate (i.e., every 20ms)
    template <typename _Ty, size_t _Capacity> class JitterBuffer
    {
    public:
        //
        // JitterBuffer element
        //
        class Element
        {
        public:
            Element(const _Ty &data, UINT32 priority)
                : m_data(data)
                , m_priority(priority)
            {
            }

            ~Element() {}

            _Ty const& data() const { return m_data; }
            UINT32 priority() const { return m_priority; }

        private:
            _Ty m_data;
            UINT32 m_priority;
        };

        //
        // JitterBuffer comparison
        //
        class Comparison
        {
        public:
            bool operator() (const Element& lhs, const Element& rhs) const
            {
                return (lhs.priority() > rhs.priority());
            }
        };

    public:
        //
        // JitterBuffer
        //
        JitterBuffer()
            : m_bufferSize(0)
            , m_threshold_75(3 * _Capacity / 4)
            , m_threshold_90(9 * _Capacity / 10)
            , m_state(JB_STOPPED)
        {
        }

        ~JitterBuffer()
        {
            while( !m_buffer.empty() )
            {
                m_buffer.pop();
            }
        }

        void setBufferSize( size_t size )
        {
            m_bufferSize = size;
            if( m_bufferSize > _Capacity )
            {
                m_bufferSize = _Capacity;
            }
        }

        _Ty top() const {
            if (!m_buffer.empty())
                return m_buffer.top().data();
            else
                return NULL;
        }

        void push(uint32_t frameNumber, const _Ty & data)
        {
            CriticalSection::Locker lock(m_cs);

            // drop overflow frame
            if( isFull() ) {
                if (sonar::common::ENABLE_JITTER_TRACING)
                    common::Log("jitterbuffer: dropping overflow, %d\n", frameNumber);
                return;
            }

            m_buffer.push(JitterBuffer::Element(data, frameNumber));

            // determine whether the jitter buffer should be buffering or dispensing
            if (m_state == JB_STOPPED) {
                if ((m_buffer.size() >= m_bufferSize) || ((frameNumber - m_buffer.top().priority()) >= m_bufferSize) )
                m_state = JB_RUNNING;
            }
        }

        bool pop(UINT32 frameNumber, _Ty * data)
        {
            CriticalSection::Locker lock(m_cs);

            // the frame number requests will be coming in monotonously increasing sequence
            // pull off and get rid of any old requests
            while (!empty() && m_buffer.top().priority() < frameNumber) {
                m_buffer.pop();
            }

            if (!empty() && m_buffer.top().priority() == frameNumber) {
                if (data) {
                    *data = m_buffer.top().data();
                }
                m_buffer.pop();
                return true;
            } else {
                if (data) {
                    *data = _Ty();
                }
            }

            return false;
        }

        void stop()
        {
            if (empty())
                m_state = JB_STOPPED;
        }

        void clear()
        {
            CriticalSection::Locker lock(m_cs);

            while( !m_buffer.empty() )
            {
                m_buffer.pop();
            }
        }

        bool empty() const
        {
            return m_buffer.empty();
        }

        bool isFull() const
        {
            return (m_buffer.size() == _Capacity);
        }

        bool isStopped() const {
            return m_state == JB_STOPPED;
        }

        size_t size() const
        {
            return m_buffer.size();
        }

        void reset()
        {
            if (sonar::common::ENABLE_JITTER_TRACING)
                common::Log("jitterbuffer: reset\n");

            CriticalSection::Locker lock(m_cs);

            m_state = JB_STOPPED;
            clear();
        }

    protected:

        enum State
        {
            JB_STOPPED = 0,
            JB_BUFFERING,
            JB_RUNNING
        };

    private:

        CriticalSection m_cs; // critical section used to manage access to the priority queue
        typedef std::priority_queue<Element, std::vector<Element>, Comparison> PriorityQueue;
        PriorityQueue m_buffer;
        size_t m_bufferSize; // frame buffer size
        const size_t m_threshold_75;
        const size_t m_threshold_90;
        State m_state;
    };
}
