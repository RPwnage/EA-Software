/*! ***************************************************************************/
/*!
    \file 
    
    Header file for memory pool routines.

    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_MEMORYPOOL_H
#define BLAZE_MEMORYPOOL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/alloc.h"

namespace Blaze
{

    struct MemNode;
    
    /*! ****************************************************************************/
    /*! \class  MemPool
        \brief  Hoisted implementation of the pool operations used by MemPool template
    ********************************************************************************/
    class BLAZESDK_API MemNodeList
    {
        NON_COPYABLE(MemNodeList);
    public:
        MemNodeList(MemoryGroupId memGroupId);
        ~MemNodeList();
        void reserve(size_t nodeCount, size_t nodeSize, const char8_t* reservedDebugName);
        void* alloc(size_t nodeSize, const char8_t* overflowDebugName);
        void free(void* node);

    private:
        MemNode* mFree;
        MemNode* mHead;
        MemNode* mTail;
        EA::Allocator::ICoreAllocator* mAllocator;
    };


    /*! ****************************************************************************/
    /*! \class  MemPool
        \brief  Low overhead, high performance type-safe memory pool with overflow support

        \note This class is based on a standard overflow pool. If the number of outstanding 
        allocations remains below numObjects specified in the call to MemPool<T>::reserve(), 
        then all the allocations will be satisfied from the reserved memory area.
        
        \note The reserve() method should be called once before the reserved memory area will
        be used. Delaying the call to MemPool<T>::reserve() is useful for scenarios when the 
        pool size is not known when the pool constructor is being called. No memory will be 
        allocated until the reserve() method is called.
        
        \note It's important to remember that the pool destructor can only free the reserved
        memory area owned by the pool, it does not call any object destructors.
        This means that all objects that are constructed within the memory that was obtained 
        by calling MemPool<T>::alloc(), must likewise be freed by calling MemPool<T>::free() 
        before the pool itself can be safely destructed.

        \note For safety the user of the pool's alloc() method (MemPool<T>::alloc()) should always check the returned
        pointer before attempting to initialize an object with the memory returned from the pool.

        \note Because the MemPool<T>::free() method calls the destructor of the pool-managed object,
        the user has two choices when designing pool-managed classes:
        1) The destructor of class T must be public
            \code
            class T {
            public:
                ~T();
            }
            \endcode
        2) The MemPool<T> must be made a friend of class T:
            \code
            class T {
                friend class MemPool<T>;
            private:
                ~T();
            }
            \endcode
    ********************************************************************************/
    template<class T>
    class MemPool
    {
        NON_COPYABLE(MemPool);
    public:
        MemPool(MemoryGroupId memGroupId);
        void reserve(size_t numObjects, const char8_t* reservedDebugName = "PoolMem");
        void* alloc(const char8_t* overflowDebugName = "OverflowMem");
        void free(T*);

    private:
        MemNodeList mNodeList;
    };

    template<typename T>
    inline MemPool<T>::MemPool(MemoryGroupId memGroupId)
    : mNodeList(memGroupId)
    {
    }

    template<typename T>
    inline void MemPool<T>::reserve(size_t numObjects, const char8_t* reservedDebugName)
    {
        const size_t nodeSize = (sizeof(T) > sizeof(uintptr_t)) ? sizeof(T) : sizeof(uintptr_t);
        mNodeList.reserve(numObjects, nodeSize, reservedDebugName);
    }

    template<typename T>
    inline void* MemPool<T>::alloc(const char8_t* overflowDebugName)
    {
        const size_t nodeSize = (sizeof(T) > sizeof(uintptr_t)) ? sizeof(T) : sizeof(uintptr_t);
        return mNodeList.alloc(nodeSize, overflowDebugName);
    }

    template<typename T>
    inline void MemPool<T>::free(T* obj)
    {
        if(obj != nullptr)
        {
            obj->~T();
            mNodeList.free(obj);
        }
    }

    /*! ****************************************************************************/
    /*! \class  MemList
        \brief  Low overhead, high performance type-safe memory pool with overflow support

        Similar to MemPool except that each object T is allocated individually instead
        of as one initial large block.
    ********************************************************************************/
    template<class T>
    class MemList
    {
        NON_COPYABLE(MemList);
    public:
        MemList(MemoryGroupId memGroupId);
        ~MemList();
        void reserve(size_t numObjects, const char8_t* reservedDebugName = "MemList");
        void trim();
        void* alloc(const char8_t* overflowDebugName = "OverflowMemList");
        void free(T*);
        bool isOverflowed();

    private:
        // a simple struct to re-use existing T object memory to maintain a list of unused T objects
        struct MemListNode
        {
            void* next;
        };

        void increment(const char8_t* debugName);

        void* mListHead;
        size_t mReservedCount;  // the number of objects we want to pre-alloc
        size_t mAllocatedCount; // running count of actual object allocations
        EA::Allocator::ICoreAllocator* mAllocator;
    };

    template<typename T>
    inline MemList<T>::MemList(MemoryGroupId memGroupId)
    : mListHead(nullptr), mReservedCount(0), mAllocatedCount(0), mAllocator(Allocator::getAllocator(memGroupId))
    {
    }

    template<typename T>
    inline MemList<T>::~MemList()
    {
        trim();
        BlazeAssertMsg(mAllocatedCount == 0, "[MemList<T>] possible memory leak");
    }

    template<typename T>
    inline void MemList<T>::reserve(size_t numObjects, const char8_t* reservedDebugName)
    {
        while (mAllocatedCount < numObjects)
        {
            increment(reservedDebugName);
        }

        mReservedCount = numObjects;
    }

    template<typename T>
    inline void MemList<T>::trim()
    {
        while (mListHead != nullptr)
        {
            void* node = mListHead;
            mListHead = (reinterpret_cast<MemListNode*>(node))->next;
            CORE_FREE(mAllocator, node);
            --mAllocatedCount;
        }
    }

    template<typename T>
    inline void* MemList<T>::alloc(const char8_t* overflowDebugName)
    {
        if (mListHead == nullptr)
        {
            increment(overflowDebugName);
        }
        void* node = mListHead;
        mListHead = (reinterpret_cast<MemListNode*>(node))->next;
        return node;
    }

    template<typename T>
    inline void MemList<T>::free(T* obj)
    {
        if (obj != nullptr)
        {
            obj->~T();
            if (mAllocatedCount > mReservedCount)
            {
                CORE_FREE(mAllocator, obj);
                --mAllocatedCount;
            }
            else
            {
                // return to "pool" of T objects
                (reinterpret_cast<MemListNode*>(obj))->next = mListHead;
                mListHead = obj;
            }
        }
    }

    template<typename T>
    inline void MemList<T>::increment(const char8_t* debugName)
    {
        void* node = CORE_ALLOC(mAllocator, sizeof(T), debugName, EA::Allocator::MEM_PERM);
        (reinterpret_cast<MemListNode*>(node))->next = mListHead;
        mListHead = node;
        ++mAllocatedCount;
    }

    template<typename T>
    inline bool MemList<T>::isOverflowed()
    {
        return mAllocatedCount > mReservedCount;
    }

}// namespace Blaze

#endif    // BLAZE_MEMORYPOOL_H
