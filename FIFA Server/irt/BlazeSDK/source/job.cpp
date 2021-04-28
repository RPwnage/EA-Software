/*! **********************************************************************************************/
/*!
    \file

    The Job template classes provide a way to define a function call with parameters from
    with an object.  Difference from Functors is that parameters for the call are stored within
    the object versus supplied by the caller on the callback's execution.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/job.h"
#include "BlazeSDK/jobscheduler.h"

namespace Blaze
{
    void Job::setAssociatedTitleCbObject(const void *obj) {
        mAssociatedTitleCbObject = obj;
    }

    void Job::setAssociatedTitleCb(const FunctorBase& titleCb) {
        mAssociatedTitleCbObject = titleCb.getObject();
        mAssociatedTitleCb = titleCb;
    }

    bool Job::isAssociatedObject(const void *obj) const {
        return obj != nullptr && (obj == getAssociatedObject() || obj == mAssociatedTitleCbObject);
    }

    void Job::addTitleCbAssociatedObject(JobScheduler *jobScheduler, const JobId& jobId, const FunctorBase& titleCb)
    {
        if (jobId == INVALID_JOB_ID)
            return;

        Job *job = static_cast<Job*>(jobScheduler->getJob(jobId));
        if (job != nullptr)
        {
            job->setAssociatedTitleCb(titleCb);
        }
        else
        {
            //  Job no longer exists or was invalid (could the former condition be a valid case?)
            BlazeAssert(job != nullptr);        
        }
    }
}
