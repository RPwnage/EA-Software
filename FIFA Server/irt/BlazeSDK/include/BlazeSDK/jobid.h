/*! **********************************************************************************************/
/*!
    \file

    This header defines the JobId class, used to identify a particular transaction in the system.  This is 
    kept separate from the "Job" class definition because most files just need the id, not the full
    job definition.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_JOB_ID_H
#define BLAZE_JOB_ID_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"

namespace Blaze
{

const uint32_t INVALID_JOB_ID = 0;
const uint32_t MIN_JOB_ID = 1;
const uint32_t MAX_JOB_ID = 0x7FFFFFF;

/*! ***************************************************************************/
/*! \class JobId

    The JobId class identifies a job.  Besides being a ranged integer, it has
    an additional "wait flag" that signals how long the job will take to execute.
    If the wait flag is not set, the job will execute on the next idle loop.  If 
    the flag is set, the job will execute at some point in the future.  This can
    be used as a guide to determine whether or not to show a "spinner" UI after 
    executing some async operation. 
*******************************************************************************/
class BLAZESDK_API JobId
{
public:
    /*! ***************************************************************************/
    /*! \brief Default constructor.
    *******************************************************************************/
    JobId() : mId(INVALID_JOB_ID) {}

    /*! ***************************************************************************/
    /*! \brief Constructor with a given id and wait flag value.
    *******************************************************************************/
    JobId(uint32_t _id, bool wait = false) : mId(_id | (wait ? SHOW_WAIT_UI_FLAG : 0)) {}
    
    /*! ***************************************************************************/
    /*! \brief Copy constructor.
    *******************************************************************************/
    JobId(const JobId &other) : mId(other.mId) {}

    /*! ***************************************************************************/
    /*! \brief Assignment operator.
    *******************************************************************************/
    JobId &operator=(const JobId &other)
    {
        mId = other.mId;
        return *this;
    }

    /*! ***************************************************************************/
    /*! \brief Assignment operator with an integer.
    *******************************************************************************/
    JobId &operator=(uint32_t _id)
    {
        mId = _id;
        return *this;
    }

    /*! ***************************************************************************/
    /*! \brief Equality operator.
    *******************************************************************************/
    bool operator==(const JobId &other) const
    {
        return get() == other.get();
    }

    /*! ***************************************************************************/
    /*! \brief Inequality operator.
    *******************************************************************************/
    bool operator!=(const JobId &other) const
    {
        return get() != other.get();
    }

    /*! ***************************************************************************/
    /*! \brief Less than operator.
    *******************************************************************************/
    bool operator<(const JobId &other) const
    {
        return get() < other.get();
    }

    /*! ***************************************************************************/
    /*! \brief Gets the id value, ignoring the wait flag.
    *******************************************************************************/
    uint32_t get() const { return (mId & ~SHOW_WAIT_UI_FLAG);}

    /*! ***************************************************************************/
    /*! \brief Sets the wait flag to true.
    *******************************************************************************/
    void setWaitFlag() { mId |= SHOW_WAIT_UI_FLAG; }

    /*! ***************************************************************************/
    /*! \brief Sets the wait flag to false.
    *******************************************************************************/
    void clearWaitFlag() { mId &= ~SHOW_WAIT_UI_FLAG; }

    /*! ***************************************************************************/
    /*! \brief Returns true if the wait flag is set.

        If the wait flag is cleared, the operation will only take one idle loop 
        to return.  The game can use this to decide whether or not to show the 
        user a "waiting for result" ui.
    *******************************************************************************/
    bool isWaitSet() const { return (mId & SHOW_WAIT_UI_FLAG) != 0; }

private:
    const static uint32_t SHOW_WAIT_UI_FLAG = 0x8000000;

    //The actual value
    uint32_t mId;
};

}       // namespace Blaze

#endif  // BLAZE_JOB_ID_H
