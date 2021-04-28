/*! **********************************************************************************************/
/*!
    \file apijob.h

    Jobs wrapper for the API system.  These jobs can be used whenever an API call spans
    multiple RPC's, or there is a delay in making the callback which is dependent
    on an asynchronous message from the server.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_API_JOB_H
#define BLAZE_API_JOB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/allocdefines.h"
#include "BlazeSDK/api.h"

namespace Blaze
{

class RawBuffer;
class ComponentManager;
class JobScheduler;

/*! ***************************************************************/
/*! \class ApiJob

    \brief Job for handling async actions frequently used by API's
*/
    class BLAZESDK_API ApiJobBase: public Job
    {
    public:
        ApiJobBase(API* api, const FunctorBase& cb );
        ApiJobBase(API* api, uint32_t userIndex, const FunctorBase& cb );
        ApiJobBase(): mAPI(nullptr), mFunctorCb() {}    

        virtual ~ApiJobBase();


        /*! ***************************************************************************/
        /*! \brief execute 
        *******************************************************************************/
        virtual void execute() 
        {
            dispatchCb(ERR_OK);
        }

        /*! ***************************************************************************/
        /*! \brief This function runs when the job is canceled.
        *******************************************************************************/
        virtual void cancel(BlazeError err)
        {
            dispatchCb(err);
        }


        /*! ************************************************************************************************/
        /*! \brief returns the user index that created the job.
        ***************************************************************************************************/
        uint32_t getUserIndex() const { return mUserIndex; }

        /*! ************************************************************************************************/
        /*! \brief return the API associated with the job.
        ***************************************************************************************************/
        API* getAPI() const { return mAPI; }

        /*! ************************************************************************************************/
        /*! \brief clears the stored functor
        ***************************************************************************************************/
        void clearTitleCb();

    protected:

        /*! ***************************************************************************/
        /*! \brief Dispatches a callback based on the current setup reason
        *******************************************************************************/
        virtual void dispatchCb(BlazeError err) = 0;

        void getTitleCb(FunctorBase& cb) const
        {
            cb = mFunctorCb;
        }
  
        API* mAPI;
        FunctorBase mFunctorCb;

    private:
        uint32_t mUserIndex;
    };



    template<typename F1>
    class ApiJob : public ApiJobBase
    {
    protected:
        void dispatchCb(BlazeError err)
        {
            //  validate whether to dispatch the title callback based on the context
            F1 titleCb;
            getTitleCb(titleCb);
            Job::setExecuting(true);
            titleCb(err, getId());
            Job::setExecuting(false);
        }

    private:
        void getTitleCb(FunctorBase& cb) const
        {
            cb = mFunctorCb;
        }

    };


    template<typename F1, typename P1>
    class ApiJob1 : public ApiJobBase
    {
    public:
        ApiJob1(API* api, const FunctorBase& cb, P1 arg1)
            : ApiJobBase(api, 0, cb), 
            mArg1(arg1)
        {}

        ApiJob1(API* api, uint32_t userIndex, const FunctorBase& cb, P1 arg1)
            : ApiJobBase(api, userIndex, cb), 
            mArg1(arg1)
        {}

        void setArg1(P1 arg1)
        {
            mArg1 = arg1;
        }

    protected:
        void dispatchCb(BlazeError err)
        {
            //  validate whether to dispatch the title callback based on the context
            F1 titleCb;
            getTitleCb(titleCb);
            Job::setExecuting(true);
            titleCb(err, getId(), mArg1);
            Job::setExecuting(false);
        }

    private:

        P1 mArg1;
    };


    template<typename F1, typename P1, typename P2>
    class ApiJob2 : public ApiJobBase
    {
    public:
        ApiJob2(API* api, const FunctorBase& cb, P1 arg1, P2 arg2)
            : ApiJobBase(api, 0, cb), 
            mArg1(arg1),
            mArg2(arg2)
        {}

        ApiJob2(API* api, uint32_t userIndex, const FunctorBase& cb, P1 arg1, P2 arg2)
            : ApiJobBase(api, userIndex, cb), 
            mArg1(arg1),
            mArg2(arg2)
        {}

        void setArg1(P1 arg1)
        {
            mArg1 = arg1;
        }

        void setArg2(P2 arg2)
        {
            mArg2 = arg2;
        }

    protected:
        void dispatchCb(BlazeError err)
        {
            //  validate whether to dispatch the title callback based on the context
            F1 titleCb;
            getTitleCb(titleCb);
            Job::setExecuting(true);
            titleCb(err, getId(), mArg1, mArg2);
            Job::setExecuting(false);
        }

    private:

        P1 mArg1;
        P2 mArg2;
    };

}        // namespace Blaze

#endif    // BLAZE_RPC_JOB_H
