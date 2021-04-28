// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Job.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

// A base class for anything that is a 'work unit'. This may not necessarily be an RPC but something that somebody
// may want to run in future (next idle call etc.). The execute method will be called
// until Job indicates that it is done.
class EADPSDK_API JobBase : public IJob
{
public:
    virtual ~JobBase()
    {
        cleanup();
    }

    void cleanup() override
    {
    }

    bool isDone() override
    {
        return m_done;
    }

    EA_NON_COPYABLE(JobBase);

protected:
    // only for subclassing
    JobBase()
        : m_done(false)
    {
    }

    bool m_done;
};
    
template<typename Host>
class InitiateJob : public JobBase
{
public:
    InitiateJob(Host* host)
    : m_host(host)
    {
    }
    
    void execute() override
    {
        m_host->initiate();
        m_done = true;
    }
protected:
    Host* m_host;
};

}
}
}

