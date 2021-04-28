/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_VECTOR_H
#define EA_TDF_VECTOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfobject.h>
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfvisit.h>

#include <EASTL/vector.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    #define MARK_SET_CHANGE_TRACKING markSet();
#else
    #define MARK_SET_CHANGE_TRACKING
#endif

namespace EA
{

namespace TDF
{

template <typename T>
struct TdfPrimitiveVectorRegistrationHelper;

template <typename T>
struct TdfStructVectorRegistrationHelper;

class EATDF_API TdfVectorBase : public TdfObject
#if EA_TDF_INCLUDE_CHANGE_TRACKING
,   public TdfChangeTracker
#endif
{
    EA_TDF_NON_ASSIGNABLE(TdfVectorBase);
public:
    ~TdfVectorBase() override {}
    const TypeDescriptionList& getTypeDescription() const override = 0;

    TdfType getValueType() const { return getValueTypeDesc().getTdfType(); }
    const TypeDescription& getValueTypeDesc() const { return getTypeDescription().valueType; }

    virtual bool isObjectCollection() const { return false; }

    virtual void resizeVector(size_t count, bool clearVector) = 0;

    // For backwards compatibility. This will call resizeVector(count, clearVector) where clearVector is a default
    // value defined by the derived class (true for TdfObjectVectors, false for TdfPrimitiveVectors).
    virtual void resizeVector(size_t count) = 0;

    virtual void initVector(size_t count) = 0;
    virtual void clearVector() = 0;
    virtual void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, const TdfVectorBase &refVector) = 0;
    virtual bool visitMembers(TdfMemberVisitor& visitor, const TdfVisitContext& callingContext) = 0;
    virtual bool visitMembers(TdfMemberVisitorConst& visitor, const TdfVisitContextConst& callingContext) const = 0;
    virtual size_t vectorSize() const = 0;

    /*! ***************************************************************/
    /*! \brief Returns the value of the member at the given index.

         Note that for TdfLists with values of type VariableTDF, the "Class" member of value will be filled out, but
         it must be checked for nullptr as it may or may not exist.

        \param[in] index  The index of the value to return.
        \param[out] value The value found.
        \return  True if the value is found, false if the index is out of bounds.
    ********************************************************************/
    virtual bool getValueByIndex(size_t index, TdfGenericReferenceConst &value) const = 0;
    virtual bool getReferenceByIndex(size_t index, TdfGenericReference &value) = 0;

    /*! ***************************************************************/
    /*! \brief Pull/Push a new value to the end of the list. Get that value as a reference.
        \param[out] value Reference to the value added.
    ********************************************************************/
    virtual void pullBackRef(TdfGenericReference& outValue) = 0;

    virtual void popBackRef() = 0;

    bool equalsValue(const TdfVectorBase& rhs) const; 

    void copyInto(EA::TDF::TdfVectorBase& obj, const EA::TDF::MemberVisitOptions& opt) const { copyIntoObject(obj, opt); }

    // This helper functions is used to get GenericReferences to the front value.
    // This is only useful if the Vector has had initVector() called on it. 
    // This is used by gamereportexpression to determine the type of a TdfVectorBase's Value type.
    virtual bool getFrontValue(TdfGenericReferenceConst &outValue) const { return getValueByIndex(0, outValue); }

    virtual EA::Allocator::ICoreAllocator& getAllocator() const = 0;

protected:
    TdfVectorBase(EA::Allocator::ICoreAllocator& allocator) : TdfObject() {}
};

class EATDF_API TdfObjectVector : public TdfVectorBase
{
    EA_TDF_NON_ASSIGNABLE(TdfObjectVector);
public:
    typedef eastl::vector<TdfObjectPtr, EA_TDF_ALLOCATOR_TYPE> TdfObjectPtrVector;
    typedef TdfObjectPtrVector::size_type size_type;
    typedef TdfObjectPtrVector::iterator iterator;
    typedef TdfObjectPtrVector::reference reference;
    typedef TdfObjectPtrVector::const_reference const_reference;
    typedef TdfObjectPtrVector::value_type value_type;
    typedef TdfObjectPtrVector::const_iterator const_iterator;
    typedef TdfObjectPtrVector::reverse_iterator reverse_iterator;
    typedef TdfObjectPtrVector::const_reverse_iterator const_reverse_iterator;

public:
    virtual bool isObjectCollection() const override { return true; }

    void clearVector() override
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfVector.clear();
    }

    void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, const TdfVectorBase &refVector) override;
    bool visitMembers(TdfMemberVisitor &visitor, const TdfVisitContext& callingContext) override;
    bool visitMembers(TdfMemberVisitorConst &visitor, const TdfVisitContextConst& callingContext) const override;
    bool getValueByIndex(size_t index, TdfGenericReferenceConst &value) const override;
    bool getReferenceByIndex(size_t index, TdfGenericReference &value) override;

    void copyIntoObject(TdfObject& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const override 
    { 
        copyInto(static_cast<TdfObjectVector&>(lhs), options); 
    }

    void copyInto(TdfObjectVector& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const;

    virtual TdfObject* allocate_element() = 0;

    TdfObject* pull_back()
    {
        TdfObject* ptr = allocate_element();
        push_back(ptr);
        return ptr;
    }

    void pullBackRef(TdfGenericReference& value) override 
    {        
        value.setRef(*(pull_back()));
    }

    void popBackRef() override
    {
        if (!mTdfVector.empty())
            pop_back();
    }

    void reserve(size_t size)
    {
        // NOTE: All Tdf containers clear when doing a reserve()
        // this is now done for compatibility...
        mTdfVector.clear();
        mTdfVector.reserve(static_cast<size_type>(size));
    };

    size_type capacity() const { return mTdfVector.capacity(); }
    size_t vectorSize() const override { return mTdfVector.size(); }
    size_type size() const { return mTdfVector.size(); }
    bool empty() const { return mTdfVector.empty(); }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    using TdfVectorBase::markSet;
#endif

    void push_back(const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfVector.push_back(value);
    }

    void* push_back_uninitialized()
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfVector.push_back_uninitialized();
    }

    void pop_back()
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfVector.pop_back();
    }

    void clear()
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfVector.clear();
    }

    void reset()
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfVector.reset_lose_memory();
    }

    virtual void resize(size_type count, bool clearVector = true);

    const TdfObjectPtrVector& asVector() const { return mTdfVector; }
    TdfObjectPtrVector& asVector() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfVector; 
    }

    void release() { clear(); }

    void swap(TdfObjectVector& x)
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
        x.markSet();
#endif
        mTdfVector.swap(x.mTdfVector);
    }

    EA::Allocator::ICoreAllocator& getAllocator() const override
    {
        return *mTdfVector.get_allocator().mpCoreAllocator;
    }

protected:
    TdfObjectVector(EA::Allocator::ICoreAllocator& alloc, const char8_t* debugName)
        : TdfVectorBase(alloc),
          mTdfVector(EA_TDF_ALLOCATOR_TYPE(debugName, &alloc))
    {     
    }

    TdfObjectPtrVector mTdfVector;

};

/*! ***************************************************************/
/*! \class TdfPrimitiveVector
    \brief A vector wrapper class for storing lists of primitive objects.

*******************************************************************/
template <typename T>
class TdfPrimitiveVector : protected eastl::vector<T, EA_TDF_ALLOCATOR_TYPE>, public TdfVectorBase
{
    EA_TDF_NON_ASSIGNABLE(TdfPrimitiveVector);
public:
    template <typename T1, typename T2>
    friend struct eastl::pair;

    typedef typename eastl::vector<T, EA_TDF_ALLOCATOR_TYPE> base_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::iterator iterator;
    typedef typename base_type::reference reference;
    typedef typename base_type::const_reference const_reference;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::const_iterator const_iterator;
    typedef typename base_type::reverse_iterator reverse_iterator;
    typedef typename base_type::const_reverse_iterator const_reverse_iterator;
    typedef typename base_type::pointer pointer;
    typedef typename base_type::const_pointer const_pointer;
    typedef typename EA::TDF::TdfPrimitiveVector<T> this_type;
public:
    TdfPrimitiveVector(const this_type& obj)
        : base_type(EA_TDF_ALLOCATOR_TYPE("TdfPrimitiveVector", &obj.getAllocator()) ), TdfVectorBase(obj.getAllocator())
    {
        obj.copyIntoObject(*this);
    }

public:
    // Ctors and Dtors
    TdfPrimitiveVector( EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t *debugMemName = "TdfPrimitiveVector")
        : base_type(EA_TDF_ALLOCATOR_TYPE(debugMemName, &allocator) ), TdfVectorBase(allocator)
    {
    }

#if !defined(EA_COMPILER_NO_RVALUE_REFERENCES)
    TdfPrimitiveVector(this_type&& x)
        : base_type(EA_TDF_ALLOCATOR_TYPE("TdfPrimitiveVector", &(x.getAllocator()))), TdfVectorBase(x.getAllocator())
    {
        swap(x);
    }
#endif

    const TypeDescriptionList& getTypeDescription() const override { return static_cast<const TypeDescriptionList&>(TypeDescriptionSelector<this_type>::get()); }

    //Can't define this in base b/c of resize()
    void initVector(size_t count) override
    {
        MARK_SET_CHANGE_TRACKING;
        clear();
        resizeVector(count);
    }

    void resizeVector(size_t count) override
    {
        resizeVector(count, false);
    }

    void resizeVector(size_t count, bool clearVector) override
    {
        MARK_SET_CHANGE_TRACKING;
        if (clearVector)
            clear();

        resize(static_cast<size_type>(count), TdfPrimitiveAllocHelper<value_type>()(getAllocator()));
    }

    void pullBackRef(TdfGenericReference& value) override 
    {
        // Calling push_back() initializes a value (without an allocator), so we always pass an allocated object in.
        push_back(TdfPrimitiveAllocHelper<value_type>()(getAllocator()));
        value.setRef(back());
    }

    void copyIntoObject(TdfObject& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const override 
    { 
        copyInto(static_cast<this_type&>(lhs), options); 
    }
    
    void copyInto(this_type& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const
    {
        if (this == &lhs)
        {
            return;
        }

        // Initialize the list to the correct allocators:
        lhs.initVector(base_type::size());

        // Quick direct-copy if we have the same allocator (or we're not a list of strings): 
        if (lhs.get_allocator() == base_type::get_allocator() || getValueType() != TDF_ACTUAL_TYPE_STRING)
        {
            lhs.asVector() = *this;
        }
        else
        {
            // Special case for the string primitive's allocator: 
            // (All strings need to be reallocated with lhs vector's allocator)
            const_iterator itr = begin(), eitr = end();
            iterator lhs_itr = lhs.begin(), lhs_eitr = lhs.end();
            for (; itr != eitr && lhs_itr != lhs_eitr; ++itr, ++lhs_itr)
            {
                // Since we already allocated all the strings with initVector, we just set the values here
                (*lhs_itr) = (*itr); 
            }
        }
    }

    void swap(this_type& x)
    {     
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
        x.markSet();
#endif
        base_type::swap(x);
    }

    void clearVector() override
    {
        MARK_SET_CHANGE_TRACKING;
        clear();
    }

    void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, const TdfVectorBase &refVector) override
    {
        const this_type &referenceVectorTyped = (const this_type &) refVector;
        const_iterator ri = referenceVectorTyped.begin();
        for(iterator i = base_type::begin(), e = base_type::end(); i != e; ++i, ++ri)
        {
            TdfGenericReferenceConst ref(*ri);
            visitor.visitReference(rootTdf, parentTdf, tag, TdfGenericReference(*i), &ref);
        }
    }

    bool visitMembers(TdfMemberVisitor &visitor, const TdfVisitContext& callingContext) override
    {        
        for (iterator i = base_type::begin(), e = base_type::end(); i != e; ++i)
        {
            uint64_t index = static_cast<uint64_t>((i - base_type::begin()));            
            TdfGenericReferenceConst key(index);
            TdfGenericReference value(*i);
            TdfVisitContext c(callingContext, key, value);            
            if (!visitor.visitContext(c))
                return false;
        }
        return true;
    }

    bool visitMembers(TdfMemberVisitorConst &visitor, const TdfVisitContextConst& callingContext) const override
    {        
        for (const_iterator i = base_type::begin(), e = base_type::end(); i != e; ++i)
        {
            uint64_t index = static_cast<uint64_t>((i - base_type::begin()));
            TdfGenericReferenceConst key(index);
            TdfGenericReferenceConst value(*i);
            TdfVisitContextConst c(callingContext, key, value);            
            if (!visitor.visitContext(c))
                return false;
        }
        return true;
    }

    size_t vectorSize() const override { return base_type::size(); }

    bool getValueByIndex(size_t index, TdfGenericReferenceConst &value) const override
    {
        if (index < size())
        {
            value.setRef(at(static_cast<size_type>(index)));
            return true;
        }

        return false;
    }

    bool getReferenceByIndex(size_t index, TdfGenericReference &value) override
    {
        if (index < size())
        {
            value.setRef(at(static_cast<size_type>(index)));
            return true;
        }

        return false;
    }

    void popBackRef() override
    {
        if (!empty())
            pop_back();
    }

    void reserve(size_t size)
    {
        // NOTE: All Tdf containers clear when doing a reserve()
        // this is now done for compatibility...
        base_type::clear();
        base_type::reserve(static_cast<size_type>(size));
    };

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    using TdfVectorBase::markSet;
#endif

    void resize(size_type n, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::resize(n, TdfPrimitiveAllocHelper<value_type>()(value, getAllocator()));
    }

    void resize(size_type n)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::resize(n, TdfPrimitiveAllocHelper<value_type>()(getAllocator()));
    }

    // Helper 'AsSet' functions to treat the list like a set (since sets are not a primitive in TDF):
    void eraseDuplicatesAsSet()
    {
        markSet();
        for (iterator curIter = base_type::begin(); curIter != base_type::end(); ++curIter)
        {
            for (iterator remainIter = curIter + 1; remainIter != base_type::end(); )
            {
                if (*curIter == *remainIter)
                    remainIter = base_type::erase(remainIter);
                else
                    ++remainIter;
            }
        }
    }

    iterator findAsSet(const value_type& value) 
    {
        markSet();
        iterator curIter = base_type::begin();
        iterator endIter = base_type::end();
        for (; curIter != endIter; ++curIter)
        {
            if (*curIter == value)
                break;
        }
        return curIter;
    }
    const_iterator findAsSet(const value_type& value) const
    {
        const_iterator curIter = base_type::begin();
        const_iterator endIter = base_type::end();
        for (; curIter != endIter; ++curIter)
        {
            if (*curIter == value)
                break;
        }
        return curIter;
    }

    void insertAsSet(const value_type& value)
    {
        markSet();
        if (findAsSet(value) == base_type::end())
            base_type::push_back(value);
    }

    iterator eraseAsSet(const value_type& value)
    {
        iterator eraseIter = findAsSet(value);
        if (eraseIter != base_type::end())
            return erase(eraseIter);

        return eraseIter;
    }

    void push_back(const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::push_back(TdfPrimitiveAllocHelper<value_type>()(value, getAllocator()));
    }

    reference push_back()
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::push_back(TdfPrimitiveAllocHelper<value_type>()(getAllocator()));
        return back();
    }

    iterator insert(iterator position, const value_type& value)
    {
        markSet();
        return base_type::insert(position, TdfPrimitiveAllocHelper<value_type>()(value, getAllocator()));
    }

    void insert(iterator position, size_type n, const value_type& value)
    {
        markSet();
        base_type::insert(position, n, TdfPrimitiveAllocHelper<value_type>()(value, getAllocator()));
    }

    template <typename InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last)
    {
        markSet();
        typename eastl::iterator_traits<iterator>::difference_type pos = eastl::distance(base_type::mpBegin, position);
        for (; first != last; ++first, ++pos)
        {
            // position may be invalidated as we add to the list, so we just use an increasing offset value:
            base_type::insert(base_type::mpBegin + pos, TdfPrimitiveAllocHelper<value_type>()(*first, getAllocator()));
        }
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    void* push_back_uninitialized()
    {
        markSet();
        return base_type::push_back_uninitialized();
    }

    void pop_back()
    {
        markSet();
        base_type::pop_back();
    }

    iterator erase(iterator position)
    {
        markSet();
        return base_type::erase(position);
    }

    iterator erase(iterator first, iterator last)
    {
        markSet();
        return base_type::erase(first, last);
    }

    reverse_iterator erase(reverse_iterator position)
    {
        markSet();
        return base_type::erase(position);
    }

    reverse_iterator erase(reverse_iterator first, reverse_iterator last)
    {
        markSet();
        return base_type::erase(first, last);
    }

    iterator erase_unsorted(iterator position)
    {
        markSet();
        return base_type::erase_unsorted(position);
    }

    void clear()
    {
        markSet();
        base_type::clear();
    }

    void reset()
    {
        markSet();
        base_type::reset_lose_memory();
    }

    reference operator[](size_type n)
    {
        markSet();
        return base_type::operator[](n);
    }

    const_reference operator[](size_type n) const
    {
        return base_type::operator[](n);
    }

    void assign(size_type n, const value_type& value)
    {
        markSet();
        base_type::assign(n, value);
    }

    template <typename InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        markSet();
        base_type::assign(first, last);
    }

    iterator begin()
    {
        markSet();
        return base_type::begin();
    }

    const_iterator begin() const
    {
        return base_type::begin();
    }

    iterator end()
    {
        markSet();
        return base_type::end();
    }

    const_iterator end() const
    {
        return base_type::end();
    }

    reverse_iterator rbegin()
    {
        markSet();
        return base_type::rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return base_type::rbegin();
    }

    reverse_iterator rend()
    {
        markSet();
        return base_type::rend();
    }

    const_reverse_iterator rend() const
    {
        return base_type::rend();
    }

    pointer data()
    {
        markSet();
        return base_type::data();
    }

    const_pointer data() const
    {
        return base_type::data();
    }

    reference at(size_type n)
    {
        markSet();
        return base_type::at(n);
    }

    const_reference at(size_type n) const
    {
        return base_type::at(n);
    }

    reference front()
    {
        markSet();
        return base_type::front();
    }

    const_reference front() const
    {
        return base_type::front();
    }

    reference back()
    {
        markSet();
        return base_type::back();
    }

    const_reference back() const
    {
        return base_type::back();
    }

#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

    const base_type& asVector() const { return *this; }
    base_type& asVector() 
    {
        MARK_SET_CHANGE_TRACKING;
        return *this; 
    }


    using base_type::empty;
    using base_type::size;
    using base_type::capacity;
    using base_type::validate;
#if !EA_TDF_INCLUDE_CHANGE_TRACKING
    using base_type::begin;
    using base_type::end;
    using base_type::rbegin;
    using base_type::rend;
    using base_type::front;
    using base_type::back;
    using base_type::data;
    using base_type::at;
    using base_type::operator[];
    using base_type::pop_back;
    using base_type::erase;
    using base_type::erase_unsorted;
    using base_type::clear;
#if EASTL_RESET_ENABLED
    using base_type::reset;
#endif
    using base_type::reset_lose_memory;
    using base_type::assign;
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

    EA::Allocator::ICoreAllocator& getAllocator() const override
    {
        return *base_type::get_allocator().mpCoreAllocator;
    }

};

template <typename T>
struct TypeDescriptionSelector<TdfPrimitiveVector<T> >
{
    static const TypeDescriptionList& get()
    {   
        // Note: The TypeDescriptionList returned here still needs to be registered and fixed up in order to allocate a name and TdfId. (See tdffactory.h)
        static TypeDescriptionList result(TypeDescriptionSelector<T>::get(), (TypeDescriptionObject::CreateFunc) &TdfObject::createInstance<TdfPrimitiveVector<T> >);
#ifdef EA_TDF_REGISTER_ALL
        static TypeRegistrationNode registrationNode(result, true);
#endif
        return result;
    }
};



/*! ***************************************************************/
/*! \class TdfStructVector
    \brief A vector wrapper class for storing lists of objects derived from TdfObject

*******************************************************************/
template <typename T>
class TdfStructVector
    : public TdfObjectVector
{
    EA_TDF_NON_ASSIGNABLE(TdfStructVector);
public:
    template <typename T1, typename T2>
    friend struct eastl::pair;

    // to save space, the base class stores a vector<TdfObjectPtr>; each template specialization casts to/from TdfObjectPtr to the actual T*
    //  This relies on some of the EASTL vector impls (specifically, that vector::iterator is a simple pointer to T.
    typedef typename eastl::vector<tdf_ptr<T>, EA_TDF_ALLOCATOR_TYPE> TemplateVector;
    typedef typename eastl::vector<const tdf_ptr<T>, EA_TDF_ALLOCATOR_TYPE> ConstTemplateVector;
    typedef typename EA::TDF::TdfObjectVector base_type;
    typedef typename TemplateVector::size_type size_type;
    typedef typename TemplateVector::pointer pointer;
    typedef typename ConstTemplateVector::const_pointer const_pointer;
    typedef typename TemplateVector::reference reference;
    typedef typename ConstTemplateVector::const_reference const_reference;
    typedef typename TemplateVector::iterator iterator;
    typedef typename ConstTemplateVector::const_iterator const_iterator;
    typedef typename TemplateVector::reverse_iterator reverse_iterator;
    typedef typename ConstTemplateVector::const_reverse_iterator const_reverse_iterator;
    typedef TdfStructVector<T> this_type;
    typedef T* value_type;
    typedef const T* const_value_type;

public:
    TdfStructVector(const this_type& obj)
        :base_type(obj.getAllocator(), "TdfStructVector")
    {
        obj.copyIntoObject(*this);
    }

public:
    // Ctors and Dtors
    TdfStructVector( EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t *debugMemName = "TdfStructVector")
        : base_type(allocator, debugMemName)
    {
    }

#if !defined(EA_COMPILER_NO_RVALUE_REFERENCES)
    TdfStructVector(this_type&& x)
        : base_type(x.getAllocator(), "TdfStructVector")
    {
        swap(x);
    }
#endif
   
    const TypeDescriptionList& getTypeDescription() const override { return static_cast<const TypeDescriptionList&>(TypeDescriptionSelector<this_type>::get()); }

    //Can't define this in base b/c of resize()
    void initVector(size_t count) override
    {
        MARK_SET_CHANGE_TRACKING;
        clear();
        resizeVector(count);
    }

    void resizeVector(size_t count) override
    {
        resizeVector(count, true);
    }

    void resizeVector(size_t count, bool clearVector) override
    {
        MARK_SET_CHANGE_TRACKING;
        resize(static_cast<size_type>(count), clearVector);
    }

    iterator begin() 
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(mTdfVector.begin()); 
    }
    const_iterator begin() const { return const_iterator(mTdfVector.begin()); }

    iterator end() 
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(mTdfVector.end()); 
    }
    const_iterator end() const { return const_iterator(mTdfVector.end()); }

    reverse_iterator rbegin()
    {
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(iterator(mTdfVector.rbegin().base())); 
    }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(const_iterator(mTdfVector.rbegin().base())); }

    reverse_iterator rend()
    {
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(iterator(mTdfVector.rend().base())); 
    }
    const_reverse_iterator rend() const { return const_reverse_iterator(const_iterator(mTdfVector.rend().base())); }

    // using c cast instead of reinterpret_cast as the ps3-sn compiler will complain
    reference front() 
    {
        MARK_SET_CHANGE_TRACKING;
        return (reference)(mTdfVector.front()); 
    }
    const_reference front() const { return (const_reference)(mTdfVector.front()); }

    // using c cast instead of reinterpret_cast as the ps3-sn compiler will complain
    reference back() 
    {
        MARK_SET_CHANGE_TRACKING;
        return (reference)(mTdfVector.back()); 
    }
    const_reference back() const { return (const_reference)(mTdfVector.back()); }

    reference operator[](size_type n)
    {        
        MARK_SET_CHANGE_TRACKING;
        return (reference)mTdfVector.operator[](n);
    }

    const_reference operator[](size_type n) const { return (const_reference)mTdfVector.operator[](n); }

    // using c cast instead of reinterpret_cast as the ps3-sn compiler will complain
    reference at(size_type n) 
    {
        MARK_SET_CHANGE_TRACKING;
        return (reference)(mTdfVector.at(n)); 
    }
    const_reference at(size_type n) const { return (const_reference)(mTdfVector.at(n)); }

    pointer       data() 
    {
        MARK_SET_CHANGE_TRACKING;
        return (pointer) mTdfVector.data(); 
    }
    const_pointer data() const { return (const_pointer) mTdfVector.data(); }

    tdf_ptr<T> pull_back() { return (T*)base_type::pull_back(); }

    void push_back(T* value) { base_type::push_back(value); }

    reference push_back()
    {            
        MARK_SET_CHANGE_TRACKING;
        return (reference)(mTdfVector.push_back());
    }
    
    iterator insert(iterator position, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        T* newValue = allocate_element();
        value->copyInto(*newValue);
        return iterator(mTdfVector.insert(typename TdfObjectPtrVector::iterator(position), newValue));
    }

    void insert(iterator position, size_type n, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        typename eastl::iterator_traits<iterator>::difference_type pos = eastl::distance(mTdfVector.begin(), typename TdfObjectPtrVector::iterator(position));
        for (size_type i = 0; i < n; ++i)
        {
            insert(iterator(mTdfVector.begin() + pos), value);
        }
    }

    template <typename InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        typename eastl::iterator_traits<iterator>::difference_type pos = eastl::distance(mTdfVector.begin(), typename TdfObjectPtrVector::iterator(position));
        for (; first != last; ++first, ++pos)
        {
            // position may be invalidated as we add to the list, so we just use an increasing offset value:
            insert(iterator(mTdfVector.begin() + pos), *first);
        }
    }
    
    iterator erase(iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(mTdfVector.erase(typename TdfObjectPtrVector::iterator(position)));
    }

    iterator erase(iterator first, iterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(mTdfVector.erase(typename TdfObjectPtrVector::iterator(first), typename TdfObjectPtrVector::iterator(last)));
    }

    reverse_iterator erase(reverse_iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(mTdfVector.erase(typename TdfObjectPtrVector::reverse_iterator(position)));
    }

    reverse_iterator erase(reverse_iterator first, reverse_iterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(mTdfVector.erase(typename TdfObjectPtrVector::reverse_iterator(first), typename TdfObjectPtrVector::reverse_iterator(last)));
    }

    iterator erase_unsorted(iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(mTdfVector.erase_unsorted(typename TdfObjectPtrVector::iterator(position)));
    }
   
    T* allocate_element() override
    {
        return (T*)TdfObject::createInstance<T>(base_type::getAllocator(), "TdfVectorElement");
    }
   
    void swap(this_type& x) { TdfObjectVector::swap(x); }

};

template <typename T>
struct TypeDescriptionSelector<TdfStructVector<T> >
{
    static const TypeDescriptionList& get()
    {   
        // Note: The TypeDescriptionList returned here still needs to be registered and fixed up in order to allocate a name and TdfId. (See tdffactory.h)
        static TypeDescriptionList result(TypeDescriptionSelector<T>::get(), (TypeDescriptionObject::CreateFunc) &TdfObject::createInstance<TdfStructVector<T> >);
#if EA_TDF_REGISTER_ALL
        static TypeRegistrationNode registrationNode(result, true);
#endif
        return result;
    }
};

} //namespace TDF

} //namespace EA


namespace EA
{
    namespace Allocator
    {
        namespace detail
        {
            template <typename T>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfPrimitiveVector<T>* object) { delete object; }

            template <typename T>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfStructVector<T>* object) { delete object; }
        }
    }
}

#undef MARK_SET_CHANGE_TRACKING


#endif // EA_TDF_H

