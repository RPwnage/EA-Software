/*! *************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef BLAZESDK_TYPES_DISPATCHER_H
#define BLAZESDK_TYPES_DISPATCHER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blaze_eastl/fixed_vector.h"
#include "BlazeSDK/jobscheduler.h"

namespace Blaze
{
    /**
     *    Dispatches a callback to several observers
     */
    template<class T, int T_SIZE = 8> 
    class Dispatcher
    {
    public:
        
        void dispatch(const char8_t* jobName, void (T::*mfn)())  const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)();
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }

        template<typename P1> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1), P1 p1) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }


        template<typename P1, typename P2> 
        void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2), P1 p1, P2 p2) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }


        template<typename P1, typename P2, typename P3> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2, p3);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }

        template<typename P1, typename P2, typename P3, typename P4> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2, p3, p4);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }


        template<typename P1, typename P2, typename P3, typename P4, typename P5> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2, P3, P4, P5), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2, p3, p4, p5);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2, P3, P4, P5, P6), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2, p3, p4, p5, p6);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7> 
            void dispatch(const char8_t* jobName, void (T::*mfn)(P1, P2, P3, P4, P5, P6, P7), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) const
        {
            BLAZE_SDK_SCOPE_TIMER(jobName);
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    ((*itr)->*mfn)(p1, p2, p3, p4, p5, p6, p7);
                }
            }
            --mDispatchLevel;
            addPendingDispatchees();
        }


        Dispatcher() :
            mDispatchees(MEM_GROUP_FRAMEWORK, MEM_NAME(MEM_GROUP_FRAMEWORK, "Dispatcher::mDispatchees")),
            mDispatchLevel(0),
            mPendingDispatchees(MEM_GROUP_FRAMEWORK, MEM_NAME(MEM_GROUP_FRAMEWORK, "Dispatcher::mPendingDispatchees"))
        {
        }

        virtual ~Dispatcher()
        {
            mDispatchees.clear();
        }

        void addDispatchee(T* dispatchee)
        {     
            //check to make sure we don't have duplicates
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == dispatchee)
                {
                    BLAZE_SDK_DEBUGF("[Dispatcher] Adding duplicate dispatchee %p.  Ignoring.\n", dispatchee);
                    return;
                }
            }

            if (mDispatchLevel > 0)
            {
                // If we are currently dispatching then just add the request to the pending
                // queue and process it once we have exited the dispatch loop
                mPendingDispatchees.push_back(dispatchee);
                return;
            }

            // we are not dispatching; see if we can reclaim an erased dispatchee slot
            // NOTE: we can't reclaim slots if we're in the middle of a dispatch,
            //    because we might reclaim a slot that the dispatch's iterator will traverse...
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == nullptr)
                {
                    *itr = dispatchee;
                    return;
                }
            }

            // we either couldn't reclaim a slot
            mDispatchees.push_back(dispatchee);
        }

        void removeDispatchee(T* dispatchee)
        {
            //Because erasing during dispatching messes things up, we just set it to nullptr.
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == dispatchee)
                {
                    *itr = nullptr;
                    return;
                }
            }

            // Not found in the dispatchee list so check it isn't pending insertion.
            // This can happen if the addDispatchee and removeDispatcher were both called
            // via a single dispatch() call.
            for (typename DispatcheeVector::iterator itr = mPendingDispatchees.begin(),
                enditr = mPendingDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == dispatchee)
                {
                    mPendingDispatchees.erase(itr);
                    return;
                }
            }
        }

        void clear()
        {
            mDispatchees.clear();
            mPendingDispatchees.clear();
        }

        size_t getDispatcheeCount()
        {
            // we have to iterate, since we null out slots instead of removing them.
            size_t dispatcheeCount = 0;
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                    ++dispatcheeCount;
            }
            return dispatcheeCount;
        }

        T *getDispatcheeAtIndex(size_t index) {return mDispatchees[index];}

    // Unnamed back compat versions: 
    
        void dispatch(void (T::*mfn)())  const { dispatch("d0", mfn); }
        template<typename P1>
        void dispatch(void (T::*mfn)(P1), P1 p1) const { dispatch("d1", mfn, p1); }
        template<typename P1, typename P2>
        void dispatch(void (T::*mfn)(P1, P2), P1 p1, P2 p2) const { dispatch("d2", mfn, p1, p2); }
        template<typename P1, typename P2, typename P3>
        void dispatch(void (T::*mfn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) const { dispatch("d3", mfn, p1, p2, p3); }
        template<typename P1, typename P2, typename P3, typename P4>
        void dispatch(void (T::*mfn)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4) const { dispatch("d4", mfn, p1, p2, p3, p4); }
        template<typename P1, typename P2, typename P3, typename P4, typename P5>
        void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) const { dispatch("d5", mfn, p1, p2, p3, p4, p5); }
        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
        void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5, P6), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) const { dispatch("d6", mfn, p1, p2, p3, p4, p5, p6); }
        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
        void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5, P6, P7), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) const { dispatch("d7", mfn, p1, p2, p3, p4, p5, p6, p7); }

    private:

        typedef fixed_vector<T*, T_SIZE, true> DispatcheeVector;
        DispatcheeVector mDispatchees;

        // used to signal when we're in a dispatch loop (mDispatchLevel > 0)
        mutable int mDispatchLevel;

        // used to hold dispatcher additions added while dispatching
        mutable DispatcheeVector mPendingDispatchees;

        void addPendingDispatchees() const
        {
            if (mDispatchLevel > 0)
                return;

            // Add all the pending dispactchee additions now that we're not in the middle
            // of disaptching.
            for (typename DispatcheeVector::iterator itr = mPendingDispatchees.begin(), 
                enditr = mPendingDispatchees.end();
                itr != enditr; ++itr)
            {
                // This is kind of nasty to do here but the alternative is to make
                // addDispatchee() const which would probably be more misleading since it
                // is a public method.
                const_cast<Dispatcher*>(this)->addDispatchee(*itr);
            }
            mPendingDispatchees.clear();
        }
    };

    template<class T, int T_SIZE = 8>     
    class DelayedDispatcher : public Dispatcher<T, T_SIZE>
    {
    public:
        DelayedDispatcher(JobScheduler *scheduler) : mScheduler(scheduler) {}

        void delayedDispatch(const char8_t* jobName, void (T::*mfn)()) const;

        template<typename P1> 
        void delayedDispatch(const char8_t* jobName, void (T::*mfn)(P1), P1 p1) const;

        template<typename P1, typename P2> 
        void delayedDispatch(const char8_t* jobName, void (T::*mfn)(P1, P2), P1 p1, P2 p2) const;

        
        // Unnamed back compat versions:
        void delayedDispatch(void (T::*mfn)()) const { delayedDispatch("dd0", mfn); }
        template<typename P1>
        void delayedDispatch(void (T::*mfn)(P1), P1 p1) const { delayedDispatch("dd1", mfn, p1); }
        template<typename P1, typename P2>
        void delayedDispatch(void (T::*mfn)(P1, P2), P1 p1, P2 p2) const { delayedDispatch("dd2", mfn, p1, p2); }
    private:
        JobScheduler *mScheduler;
    };

    template<class T, int T_SIZE> 
    void DelayedDispatcher<T, T_SIZE>::delayedDispatch(const char8_t* jobName, void (T::*mfn)()) const
    {
        mScheduler->scheduleMethod<Dispatcher<T, T_SIZE>, void (T::*)()>
            (jobName, this, &Dispatcher<T, T_SIZE>::dispatch, mfn);
    }

    template<class T, int T_SIZE>
    template<typename P1> 
    void DelayedDispatcher<T, T_SIZE>::delayedDispatch(const char8_t* jobName, void (T::*mfn)(P1), P1 p1) const
    {

        mScheduler->scheduleMethod<Dispatcher<T, T_SIZE>, void (T::*)(P1), P1>
            (jobName, this, &Dispatcher<T, T_SIZE>::dispatch, mfn, p1);
    }

    template<class T, int T_SIZE>
    template<typename P1, typename P2> 
    void DelayedDispatcher<T, T_SIZE>::delayedDispatch(const char8_t* jobName, void (T::*mfn)(P1, P2), P1 p1, P2 p2) const
    {
        mScheduler->scheduleMethod<Dispatcher<T, T_SIZE>, void (T::*)(P1, P2), P1, P2>
            (jobName, this, &Dispatcher<T, T_SIZE>::dispatch, mfn, p1, p2);
    }
}


#endif
