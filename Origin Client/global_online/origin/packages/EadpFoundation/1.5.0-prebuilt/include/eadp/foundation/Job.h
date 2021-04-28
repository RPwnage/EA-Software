// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

namespace eadp
{
namespace foundation
{

/*!
 * @brief The basic work unit which can be scheduled on Hub to run asynchronously.
 *
 * The job can be used to do anything, but normally is used to offload time-consuming task from blocking
 * application's main UI thread.
 */
class EADPSDK_API IJob
{
public:
    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IJob() = default;

    /*!
     * @brief The main execution body of the job.
     *
     * This function will be called repeatedly until the isDone() is true, then the job will be cleaned up.
     */
    virtual void execute() = 0;

    /*!
     * @brief Release all the retained resource for the job
     *
     * This function will be the last function called to the job for releasing related resource.
     */
    virtual void cleanup() = 0;

    /*!
     * @brief Check if the job is finished.
     *
     * Derived class should use to indicate if job is finished and can be cleaned up.
     */
    virtual bool isDone() = 0;
};

}
}

