/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_UTIL_DISPATCHER_H
#define BLAZE_UTIL_DISPATCHER_H


#include "EASTL/fixed_vector.h"

namespace Blaze
{
    /**
     *  Dispatches a callback to several observers
     */
    template<class T, int T_SIZE = 8> 
    class Dispatcher
    {
    public:
        void dispatch(void (T::*mfn)())  const
        {
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
        }

        template<typename P1> 
            void dispatch(void (T::*mfn)(P1), P1 p1) const
        {
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
        }


        template<typename P1, typename P2> 
            void dispatch(void (T::*mfn)(P1, P2), P1 p1, P2 p2) const
        {
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
        }


        template<typename P1, typename P2, typename P3> 
            void dispatch(void (T::*mfn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) const
        {
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
        }

        template<typename P1, typename P2, typename P3, typename P4> 
            void dispatch(void (T::*mfn)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4) const
        {
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
        }


        template<typename P1, typename P2, typename P3, typename P4, typename P5> 
            void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) const
        {
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
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6> 
            void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5, P6), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) const
        {
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
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7> 
            void dispatch(void (T::*mfn)(P1, P2, P3, P4, P5, P6, P7), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) const
        {
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
        }


        BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)())  const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)();
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }

        template<typename P1> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1), P1 p1) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }


        template<typename P1, typename P2> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2), P1 p1, P2 p2) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }


        template<typename P1, typename P2, typename P3> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2, p3);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }

        template<typename P1, typename P2, typename P3, typename P4> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4) const
        {
            VERIFY_WORKER_FIBER();


            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2, p3, p4);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }


        template<typename P1, typename P2, typename P3, typename P4, typename P5> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2, P3, P4, P5), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2, p3, p4, p5);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2, P3, P4, P5, P6), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2, p3, p4, p5, p6);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }

        template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7> 
            BlazeRpcError dispatchReturnOnError(BlazeRpcError (T::*mfn)(P1, P2, P3, P4, P5, P6, P7), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) const
        {
            VERIFY_WORKER_FIBER();

            BlazeRpcError err = ERR_OK;
            ++mDispatchLevel;
            for (typename DispatcheeVector::const_iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr != nullptr)
                {
                    err = ((*itr)->*mfn)(p1, p2, p3, p4, p5, p6, p7);
                    if (err != ERR_OK)
                        break;
                }
                // Intentionally swallow the error because it's not fatal.
                err = ERR_OK;
            }
            --mDispatchLevel;
            return err;
        }



        Dispatcher() :
            mDispatcheeCount(0),
            mDispatchLevel(0)
        {
        }

        virtual ~Dispatcher()
        {
        }

        void addDispatchee(T& dispatchee)
        {     
            //check to make sure we don't have duplicates
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == &dispatchee)
                {
#ifdef BLAZE_CLIENT_SDK
                    BlazeVerify(false && "Trying to add dispatchee twice!");
#else
                    EA_FAIL_MSG("Trying to add dispatchee twice!");
#endif
                    return;
                }
            }

            //Increment count now that we know we're going to add it
            ++mDispatcheeCount;

            if (mDispatchLevel == 0)
            {
                // we are not dispatching; see if we can reclaim an erased dispatchee slot
                // NOTE: we can't reclaim slots if we're in the middle of a dispatch,
                //    because we might reclaim a slot that the dispatch's iterator will traverse...
                for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                    enditr = mDispatchees.end();
                    itr != enditr; ++itr)
                {
                    if (*itr == nullptr)
                    {
                        *itr = &dispatchee;
                        return;
                    }
                }
            }

            // we either couldn't reclaim a slot, or we weren't allowed to reclaim (because we're dispatching)
            mDispatchees.push_back(&dispatchee);
        }

        void removeDispatchee(T& dispatchee)
        {
            //Because erasing during dispatching messes things up, we just set it to nullptr.
            for (typename DispatcheeVector::iterator itr = mDispatchees.begin(), 
                enditr = mDispatchees.end();
                itr != enditr; ++itr)
            {
                if (*itr == &dispatchee)
                {
                    *itr = nullptr;
                    --mDispatcheeCount;  //Only subtract once we know its here
                    break;
                }
            }
        }

        void clear()
        {
            mDispatchees.clear();
            mDispatcheeCount = 0;
        }

        size_t getDispatcheeCount() const { return mDispatcheeCount; }

        T *getDispatcheeAtIndex(size_t index) {return mDispatchees[index];}

        bool isEmpty() const { return mDispatcheeCount == 0;}
    private:

        typedef eastl::fixed_vector<T*, T_SIZE> DispatcheeVector;
        DispatcheeVector mDispatchees;
        size_t mDispatcheeCount;

        // used to signal when we're in a dispatch loop (mDispatchLevel > 0)
        mutable int mDispatchLevel;
    };
}

#endif //BLAZE_UTIL_DISPATCHER
