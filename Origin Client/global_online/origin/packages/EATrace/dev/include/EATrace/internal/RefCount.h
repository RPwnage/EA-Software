///////////////////////////////////////////////////////////////////////////////
// RefCount.h
//
// Copyright (c) 2007, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana.
//
// Implements basic reference counting functionality.
//
// This file is dependent on the eathread_atomic.h header file.
// As such, this is a cross-module dependency which is normally 
// something we try to refrain from doing.
//
// This file was copied from EACOM/RefCount.h
///////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_REFCOUNT_H
#define EATRACE_REFCOUNT_H


namespace EA
{
    namespace Trace
    {
        template <typename T>
        class AutoRefCount
        {
        public:
            typedef T element_type;

            T* mpObject;

        public:
            AutoRefCount()
                : mpObject(NULL) {}

            AutoRefCount(T* pObject)
                : mpObject(pObject) 
            { 
                if(mpObject)
                    mpObject->AddRef();
            } 

            AutoRefCount(const AutoRefCount& rhs) 
                : mpObject(rhs.mpObject) 
            { 
                if(mpObject)
                    mpObject->AddRef();
            }

            ~AutoRefCount() 
            {
                if(mpObject)
                    mpObject->Release();
            }

            AutoRefCount& operator=(const AutoRefCount& source)       
            {         
                return operator=(source.mpObject);
            }

            AutoRefCount& operator=(T* pObject)
            {         
                if(pObject != mpObject)
                {
                    T* const pTemp = mpObject;
                    if(pObject)
                        pObject->AddRef();
                    mpObject = pObject;
                    if(pTemp)
                        pTemp->Release();
                }
                return *this;
            }

            T& operator *()  const 
            {
                return *mpObject;
            }

            T* operator ->() const
            {
                return  mpObject;
            }

            operator T*() const 
            {
                return  mpObject;
            }

            void** AsPPVoidParam()
            {
                if(mpObject){
                    T* const pTemp = mpObject;
                    mpObject = NULL;
                    pTemp->Release();
                }

                #if defined(__GNUC__)       // Handle strict aliasing issues.
                    union {                 // This makes strict-aliasing compiler warnings go away,
                        T**    mppObject;   // but I don't believe this approach is correct. The problem is that we 
                        void** mppVoid;     // really want to make a union of T* and void* and not T** and void**.
                    } u = { &mpObject };    // However, that's not easy to do here without modifying this class' 
                                            // public interface.
                    return u.mppVoid;
                #else
                    return (void**)&mpObject;
                #endif
            }

            T** AsPPTypeParam()                     
            {
                if(mpObject){
                    T* const pTemp = mpObject;
                    mpObject = NULL;
                    pTemp->Release();
                }

                return &mpObject;
            } 

        }; // class AutoRefCount

    } // namespace Trace

} // namespace EA


#endif  // Header include guard
