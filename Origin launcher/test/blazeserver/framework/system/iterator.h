/*************************************************************************************************/
/*!
    \file   iterator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ITERATOR_H
#define BLAZE_ITERATOR_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{


/*! ***************************************************************/
/*! \class Iterator
    \brief Iterator template interface.

    TODO: Add comment here.
    \nosubgrouping
*******************************************************************/
template<class T>
class Iterator
{
public:
    typedef T Type;

    virtual                 ~Iterator()     {}
    virtual bool            done() const    = 0;
    virtual Type            current() const = 0; /* only valid if !done() */
    virtual void            next()          = 0; /* only valid if !done() */
    virtual BlazeRpcError   begin()         = 0; /* initialize the iterator  */
    virtual void            end()           = 0; /* invalidate the iterator  */
};


template<class T>
class FetchIdentity
{
public:
    T operator()(T value) { return value; }
};

template<class T, class Map>
class FetchMapKey
{
public:
    T operator()(const typename Map::value_type& pair){ return pair.first; }
};

template<class T, class Map>
class FetchMapValue
{
public:
    T operator()(const typename Map::value_type& pair){ return pair.second; }
};

/*! ***************************************************************/
/*! \class ContainerIterator
    \brief Iterates over a container which is set instead of passed via begin.

    TODO: Add comment here.
    \nosubgrouping
*******************************************************************/
template<class T, class C, class F = FetchIdentity<T>, class I = typename C::const_iterator>
class ContainerIterator : public Iterator<T>
{
public:
    ContainerIterator(C &c, bool ownsContainer = false) : mCont(c), mOwnsContainer(ownsContainer) {}
    ~ContainerIterator() { if(mOwnsContainer) delete &mCont; }
    virtual bool            done() const        {return mIt == mCont.end();}
    virtual T               current() const     {return F()(*mIt);}
    virtual void            next()              {++mIt;}
    virtual BlazeRpcError   begin()             {mIt = mCont.begin(); return Blaze::ERR_OK;}
    virtual void            end()               {mIt = mCont.end(); }

protected:
    C&  mCont; /* container */
    I   mIt; /* iterator */
    bool mOwnsContainer;
    NON_COPYABLE(ContainerIterator);
};


/*! ***************************************************************/
/*! \class RangeIterator
    \brief Iterates over known range
    
    \nosubgrouping
*******************************************************************/

template<class T, class I, class F = FetchIdentity<T> >
class RangeIterator : public Iterator<T>
{
public:
    RangeIterator(I beginIt, I endIt) : mBeginIt(beginIt), mEndIt(endIt) {}
    virtual bool            done() const        {return mIt == mEndIt;}
    virtual T               current() const     {return F()(*mIt);}
    virtual void            next()              {++mIt;}
    virtual BlazeRpcError   begin()      
    {
        mIt = mBeginIt; 
        return Blaze::ERR_OK;
    }
    virtual void            end()               {mIt = mEndIt;}

protected:
    I   mIt; /* iterator */
    I   mBeginIt; /* begin iterator */
    I   mEndIt; /* end iterator */
};

/*! ***************************************************************/
/*! \class IteratorPtr
    \brief IteratorPtr concrete template class.

    TODO: Add comment here.
    \nosubgrouping
*******************************************************************/
template<class T>
class IteratorPtr
{
public:
    typedef Blaze::Iterator<T> Iterator;

    IteratorPtr();
    IteratorPtr(Iterator* it);
    IteratorPtr(const IteratorPtr& other);
    ~IteratorPtr();
    
    BlazeRpcError   acquire(Iterator* it);
    Iterator*       release();
    void            releaseAndDestroy();
    void            reset();
    bool            done() const;
    void            operator++() const;
    void            operator=(const IteratorPtr& other);
    T               operator*() const;
    
private:
    mutable Iterator*   mIt; /* needs to be mutable because of const copy constructor and assignment operator */
};

template<class T>
inline
IteratorPtr<T>::IteratorPtr() 
: mIt(nullptr)
{
}
template<class T>
inline
IteratorPtr<T>::IteratorPtr(Iterator* it)
: mIt(it)
{
    mIt->begin(); /* ignore error, because we don't support exceptions
                          and IteratorPtr gracefully handles this case anyway */
}
template<class T>
inline
IteratorPtr<T>::IteratorPtr(const IteratorPtr& other)
: mIt(other.mIt)
{
    other.mIt = nullptr;
    
}
template<class T>
inline
IteratorPtr<T>::~IteratorPtr()
{
    releaseAndDestroy();
}
template<class T>
inline BlazeRpcError
IteratorPtr<T>::acquire(Iterator* it)
{
    if(mIt)
    {
        mIt->end();
        delete mIt;
    } 
    mIt = it;
    return mIt->begin();
}
template<class T>
inline Iterator<T>*
IteratorPtr<T>::release()
{
    Iterator* it = mIt;

    if(mIt)
    {
        mIt->end();
        mIt = nullptr; 
    }

    return it;
}
template<class T>
inline void
IteratorPtr<T>::releaseAndDestroy()
{
    if (mIt != nullptr)
    {
        mIt->end();
        delete mIt;
        mIt = nullptr;
    }
}
template<class T>
inline void
IteratorPtr<T>::reset()
{
    if(mIt)
    {
        mIt->begin();
    }
}
template<class T>
inline bool
IteratorPtr<T>::done() const
{
    return mIt != nullptr ? mIt->done() : true;
}
template<class T>
inline void
IteratorPtr<T>::operator++() const
{
    mIt->next();
}
template<class T>
inline T
IteratorPtr<T>::operator*() const
{
    return mIt->current();
}
template<class T>
inline void
IteratorPtr<T>::operator=(const IteratorPtr& other)
{
    if(mIt)
    { 
        mIt->end(); 
        delete mIt;
    } 
    mIt = other.mIt; 
    other.mIt = nullptr;
}

} // Blaze

#endif // BLAZE_ITERATOR_H

