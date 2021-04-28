/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_MAP_H
#define EA_TDF_MAP_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfobject.h>
#include <EATDF/tdfvisit.h>
#include <EATDF/tdfenuminfo.h>

#include <EASTL/vector_map.h>
#include <EASTL/sort.h>

#include <EATDF/tdffactory.h>

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

template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
struct TdfPrimitiveMapRegistrationHelper;

template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
struct TdfStructMapRegistrationHelper;


class EATDF_API TdfMapBase  : public TdfObject
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        ,   public TdfChangeTracker
#endif
{
    EA_TDF_NON_ASSIGNABLE(TdfMapBase);
public:
    ~TdfMapBase() override {}

    virtual void initMap(size_t count) = 0;
    virtual void clearMap() = 0;
    virtual bool isObjectCollection() const { return false; }
    virtual size_t mapSize() const = 0;

    /*! ***************************************************************/
    /*! \brief Returns the value of the member at the given index. 
              (using mapSize() to iterate over everything and get the key/values).

        \param[in] index  The index of the key/value to return.
        \param[out] outKey The key found.
        \param[out] outValue The value found.
        \return  True if the value is found, false if the index is out of bounds.
    ********************************************************************/
    virtual bool getValueByIndex(size_t index, TdfGenericReferenceConst &outKey, TdfGenericReferenceConst &outValue) const = 0;
    virtual bool getReferenceByIndex(size_t index, TdfGenericReference &outKey, TdfGenericReference &outValue) = 0;

    void copyInto(EA::TDF::TdfMapBase& obj, const EA::TDF::MemberVisitOptions& opt) const { copyIntoObject(obj, opt); }

    const TypeDescriptionMap& getTypeDescription() const override = 0;
    TdfType getKeyType() const { return getKeyTypeDesc().getTdfType(); }
    TdfType getValueType() const { return getValueTypeDesc().getTdfType(); }
    const TypeDescription& getKeyTypeDesc() const { return getTypeDescription().keyType; }
    const TypeDescription& getValueTypeDesc() const { return getTypeDescription().valueType; }


    // this is required if underlying type requires elements to be sorted in order
    // to make map searchable
    virtual void fixupElementOrder() {};

    virtual bool ignoreCaseKeyStringCompare() const { return false; }

    virtual void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &refMap) = 0;
    virtual bool visitMembers(TdfMemberVisitor& visitor, const TdfVisitContext& callingContext) = 0;
    virtual bool visitMembers(TdfMemberVisitorConst& visitor, const TdfVisitContextConst& callingContext) const = 0;

#if EA_TDF_INCLUDE_MEMBER_MERGE
    virtual void mergeMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &referenceValue) = 0;
#endif

protected:

    TdfMapBase(EA::Allocator::ICoreAllocator& allocator) : TdfObject() {}

    /*! \brief Returns whether incoming key can be cast/converted into the map actual key. */
    bool isCompatibleKey(const TdfGenericConst& key) const { return (key.getTdfId() == getKeyTypeDesc().getTdfId()); }
    bool isCompatibleValue(const TdfGenericConst& value) const { return (value.getTdfId() == getValueTypeDesc().getTdfId()); }

public:
    /*! ***************************************************************/
    /*! \brief Returns the value of the member at the given key.

         Note that for TdfMaps with values of type VariableTDF, the "Class" member of value will be filled out, but
         it must be checked for nullptr as it may or may not exist.

        \param[in] key The key of the value to return.
        \param[out] value The value found.
        \return  True if the value is found, false if the key is not found or of the wrong type.
    ********************************************************************/
    virtual bool getValueByKey(const TdfGenericConst &key, TdfGenericReferenceConst &outValue) const = 0;
    virtual bool getValueByKey(const TdfGenericConst &key, TdfGenericReference &outValue) = 0;

    /*! ***************************************************************/
    /*! \brief Inserts a key into the map, and returns a value that can be set.

        \param[in] key The key of the value to insert.
        \param[out] outValue Reference to the value field that can be used for setting.
        \return  True if the value is inserted.  False if the key is the wrong type or already present.
    ********************************************************************/
    virtual bool insertKeyGetValue(const TdfGenericConst& key, TdfGenericReference& outValue) = 0;
    
    /*! ***************************************************************/
    /*! \brief Erases a key/value from the map in a generic fashion.

        \param[in] key The key of the value to erase.
        \return  True if the value is erased.  False if the key is the wrong type or already present.
    ********************************************************************/
    virtual bool eraseValueByKey(const TdfGenericConst& key) = 0;


    /*! ***************************************************************/
    /*! \brief Returns true if the contents of the map are equal
    ********************************************************************/
    virtual bool equalsValue(const TdfMapBase& rhs) const = 0;


    // These two helper functions are used to get GenericReferences to the front values.
    // This is only useful if the Map has had initMap() called on it. 
    // This is used by gamereportexpression to determine the type of a TdfMapBase's Key/Value.
    virtual bool getFrontKey(TdfGenericReferenceConst &outKey) const   { TdfGenericReferenceConst outValue; return getValueByIndex(0, outKey, outValue); }
    virtual bool getFrontValue(TdfGenericReferenceConst &outValue) const { TdfGenericReferenceConst outKey; return getValueByIndex(0, outKey, outValue); }

    virtual EA::Allocator::ICoreAllocator& getAllocator() const = 0;
};

template <typename K, typename Compare = eastl::less<K>, bool ignoreCaseStringCompare = false >
class TdfObjectMap : public TdfMapBase
{
    EA_TDF_NON_COPYABLE(TdfObjectMap);
public:

    typedef typename eastl::vector_map<K,TdfObjectPtr,Compare,EA_TDF_ALLOCATOR_TYPE> TdfObjectPtrMap;
    typedef typename TdfObjectPtrMap::mapped_type mapped_type;
    typedef typename TdfObjectPtrMap::size_type size_type;
    typedef typename TdfObjectPtrMap::iterator iterator;
    typedef typename TdfObjectPtrMap::value_type value_type;
    typedef typename TdfObjectPtrMap::pointer pointer;
    typedef typename TdfObjectPtrMap::const_pointer const_pointer;
    typedef typename TdfObjectPtrMap::reference reference;
    typedef typename TdfObjectPtrMap::const_reference const_reference;   
    typedef typename TdfObjectPtrMap::key_type key_type;
    typedef typename TdfObjectPtrMap::const_iterator const_iterator;
    typedef typename TdfObjectPtrMap::reverse_iterator reverse_iterator;
    typedef typename TdfObjectPtrMap::const_reverse_iterator const_reverse_iterator;
    typedef typename TdfObjectPtrMap::key_compare key_compare;
    typedef typename TdfObjectPtrMap::value_compare value_compare;


    typedef TdfObjectMap<K,Compare,ignoreCaseStringCompare> this_type;

    // Ctors and Dtors
    TdfObjectMap( EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t *debugMemName = "TdfObjectMap")
    : TdfMapBase(allocator), mTdfObjectPtrMap(EA_TDF_ALLOCATOR_TYPE(debugMemName, &allocator))
    {
    }

    virtual TdfObject* allocate_element() = 0;
    const TypeDescriptionMap& getTypeDescription() const override = 0;

    bool ignoreCaseKeyStringCompare() const override { return ignoreCaseStringCompare; }
    bool isObjectCollection() const override { return true; }
    
private:
    // Helper function to properly allocate strings:
    value_type getDefaultValue(const key_type& keyCasted)
    {
        return value_type(TdfPrimitiveAllocHelper<key_type>()(keyCasted, getAllocator()), mapped_type());  // The TdfObjectPtr doesn't use an allocator, but the Key might. 
    }

public:
    /*! ***************************************************************/
    /*! \brief  Reserves and creates default, empty members.
                Will delete currently pointed to elements first.
    *******************************************************************/
    void initMap(size_t count) override
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfObjectPtrMap.clear();

        // Resize allocates elements, so we have to set an allocator for the string keys that can possibly be created:
        mTdfObjectPtrMap.resize(static_cast<size_type>(count), getDefaultValue(TdfPrimitiveAllocHelper<key_type>()(getAllocator())));

        for (iterator i = mTdfObjectPtrMap.begin(), e = mTdfObjectPtrMap.end(); i != e; ++i)
            i->second = allocate_element(); // populate with empty value elements
    }

    bool insertKeyGetValue(const TdfGenericConst& key, TdfGenericReference& outValue) override
    {
        eastl::pair<iterator, bool> itr = this->insertKey(key);
        if (itr.first != this->end())
        {
            if (itr.second)
            {
                //we need to allocate an element here.
                itr.first->second = allocate_element();
            }
            outValue.setRef(*itr.first->second);
        }
        return itr.second;
    }

    bool eraseValueByKey(const TdfGenericConst& key) override
    {        
        iterator itr = this->getIterator(key);
        if (itr != this->end())
        {
            return this->erase(itr->first) != 0;
        }

        return false;
    }

#if EA_TDF_INCLUDE_MEMBER_MERGE
    //  extends the existing map to include members from the reference and visits this map's members.
    void mergeMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &refValue) override
    {
        EA_ASSERT_MSG(refValue.getTdfId() == getTdfId(), "Attempted to merge maps of differing types.");
        MARK_SET_CHANGE_TRACKING;

        const this_type &refTyped = static_cast<const this_type &>(refValue);
        for (const_iterator ri = refTyped.begin(), re = refTyped.end(); ri != re; ++ri)
        {
            eastl::pair<iterator, bool> inserter = this->insert(*ri);
            iterator i = inserter.first;
            if (inserter.second)
            {                
                i->second = allocate_element();
                TdfGenericReference val(*(i->second));                
                val.setValue(TdfGenericReference(*(ri->second)));
            }
            //  do we need to revisit the key (meant to maintain visitMembers type behavior.)

            TdfGenericReferenceConst refFirst(ri->first);
            TdfGenericReferenceConst refSecond(ri->second);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->first), &refFirst);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->second), &refSecond);
        }
    }
#endif        
        
    void copyIntoObject(TdfObject& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const override { copyInto(static_cast<this_type&>(lhs), options); }
    virtual void copyInto(this_type &lhs, const MemberVisitOptions& options = MemberVisitOptions()) const 
    { 
        if (this == &lhs)
        {
            return;
        }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        lhs.markSet();
#endif

        lhs.reserve(mTdfObjectPtrMap.size());
        for (const_iterator i = mTdfObjectPtrMap.begin(), e = mTdfObjectPtrMap.end(); i != e; ++i)
        {
            TdfObject* newElem = lhs.allocate_element();
            i->second->copyIntoObject(*newElem, options);

            // Here we copy into the lhs map using the lhs's allocator for the primitive (string)
            lhs[TdfPrimitiveAllocHelper<key_type>()(i->first, lhs.getAllocator())] = newElem;
        }
    }

    /*! ***************************************************************/
    /*! \brief  Releases all memory pointed to by this vector including
                externally allocated elements added to with insert()/[].
    *******************************************************************/
    void release() { clearMap(); }

    void clearMap() override
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfObjectPtrMap.clear();
    }

    size_t mapSize() const override { return mTdfObjectPtrMap.size(); }


#if EA_TDF_MAP_ELEMENT_FIXUP_ENABLED
    void fixupElementOrder() override
    {
        eastl::sort(begin(), end(), mTdfObjectPtrMap.value_comp());
    }
#endif

    void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &refValue) override 
    { 
        const this_type &referenceTyped = (const this_type &)refValue;
        // Remove any elements that aren't in the new type:
        for (iterator i = begin(); i != end();)
            i = (referenceTyped.find(i->first) == referenceTyped.end()) ? erase(i) : i + 1;

        for (auto ri : referenceTyped)
        {
            TdfGenericReferenceConst refFirst(ri.first), refSecond(ri.second);

            // We may have to allocate the element, since this is a non-primitive type and we need the TdfPtr to be valid:
            eastl::pair<iterator, bool> itr = this->insertKey(refFirst);
            if (itr.first != this->end() && itr.second)
                itr.first->second = allocate_element();

            iterator i = itr.first;
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->first), &refFirst);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->second), &refSecond);
        }
    }

    bool visitMembers(TdfMemberVisitor &visitor, const TdfVisitContext& callingContext) override 
    { 
        for (iterator i = begin(), e = end(); i != e; ++i)
        {
            TdfGenericReference key(i->first), value(i->second);
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            // Technically, the key may be unset if it's a Blob type, but nothing actually uses blob keys currently. 
            if (callingContext.getVisitOptions().onlyIfSet && !value.isTdfObjectSet())
                continue;
#endif

            TdfVisitContext c(callingContext, key, value);                        
            if (!visitor.visitContext(c))
                return false;
        }
        return true;
    }

    bool visitMembers(TdfMemberVisitorConst &visitor, const TdfVisitContextConst& callingContext) const override 
    { 
        for (const_iterator i = begin(), e = end(); i != e; ++i)
        {
            TdfGenericReferenceConst key(i->first), value(i->second);
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            // Technically, the key may be unset if it's a Blob type, but nothing actually uses blob keys currently. 
            if (callingContext.getVisitOptions().onlyIfSet && !value.isTdfObjectSet())
                continue;
#endif

            TdfVisitContextConst c(callingContext, key, value);
            if (!visitor.visitContext(c))
                return false;
        }
        return true;

    }

    bool getValueByIndex(size_t index, TdfGenericReferenceConst &outKey, TdfGenericReferenceConst &outValue) const override
    {
        if (index < size())
        {
            const value_type& keyValuePair = at(static_cast<size_type>(index));
            outKey.setRef(keyValuePair.first);
            outValue.setRef(keyValuePair.second);
            return true;
        }

        return false;
    }

    bool getReferenceByIndex(size_t index, TdfGenericReference &outKey, TdfGenericReference &outValue) override
    {
        if (index < size())
        {
            value_type& keyValuePair = at(static_cast<size_type>(index));
            outKey.setRef(keyValuePair.first);
            outValue.setRef(keyValuePair.second);
            return true;
        }

        return false;
    }

    bool getValueByKey(const TdfGenericConst& key, TdfGenericReferenceConst &value) const override
    {
        const_iterator itr = this->getIteratorConst(key);
        if (itr != this->end())
        {
            value.setRef(itr->second);
            return true;
        }
        return false;
    }

    bool getValueByKey(const TdfGenericConst& key, TdfGenericReference &value) override
    {
        iterator itr = this->getIterator(key);
        if (itr != this->end())
        {        
            value.setRef(itr->second);
            return true;
        }
        return false;
    }

    bool equalsValue(const TdfMapBase& rhs) const override 
    {
        if (getTdfId() != rhs.getTdfId() || mapSize() != rhs.mapSize())
            return false;

        for (const_iterator i = begin(), e = end(), ri = ((const this_type&) rhs).begin(), re = ((const this_type&) rhs).end(); i != e && ri != re; ++i, ++ri)
        {
            TdfGenericReferenceConst key(i->first), rkey(ri->first), value(i->second), rvalue(ri->second);
            if (!key.equalsValue(rkey) || !value.equalsValue(rvalue))
                return false;
        }

        return true;
    }

    void reserve(size_t size)
    {
        // NOTE: All Tdf containers clear when doing a reserve()
        // this is now done for compatibility, and to make copyInto() safe.
        MARK_SET_CHANGE_TRACKING;
        clear();
        mTdfObjectPtrMap.reserve(static_cast<size_type>(size));
    };

    void clear() { clearMap(); }

    void swap(this_type& rhs)
    {
        mTdfObjectPtrMap.swap(rhs.mTdfObjectPtrMap);
    }

    eastl::pair<iterator, bool> insert(const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.insert(value);
    }

    eastl::pair<iterator, bool> insert(const key_type& key)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.insert(key);
    }

    iterator insert(iterator position, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.insert(position, value);
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        mTdfObjectPtrMap.insert(first, last);
    }
        
    iterator erase(iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.erase(position);
    }

    iterator erase(iterator first, iterator last)
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.erase(first, last);
    }

    reverse_iterator erase(reverse_iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.erase(position);
    }

    reverse_iterator erase(reverse_iterator first, reverse_iterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.erase(first, last);
    }

    size_type erase(const key_type& k)
    {
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.erase(k);
    }

    void resize(size_t count) 
    {
        MARK_SET_CHANGE_TRACKING;
        // Resize allocates elements, so we have to set an allocator for the string keys that can possibly be created:
        mTdfObjectPtrMap.resize(static_cast<size_type>(count), getDefaultValue(TdfPrimitiveAllocHelper<key_type>()(getAllocator())));
    }

    const key_compare& key_comp() const { return mTdfObjectPtrMap.key_comp(); }
    key_compare&       key_comp() { return mTdfObjectPtrMap.key_comp(); }

    const value_compare& value_comp() const { return mTdfObjectPtrMap.value_comp(); }
    value_compare&       value_comp() { return mTdfObjectPtrMap.value_comp(); }

    iterator       begin() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.begin(); 
    }
    const_iterator begin() const { return mTdfObjectPtrMap.begin(); }
    
    iterator       end() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.end(); 
    }
    const_iterator end() const { return mTdfObjectPtrMap.end(); }
    
    reverse_iterator       rbegin() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.rbegin(); 
    }
    const_reverse_iterator rbegin() const { return mTdfObjectPtrMap.rbegin(); }
    
    reverse_iterator       rend() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.rend(); 
    }
    const_reverse_iterator rend() const { return mTdfObjectPtrMap.rend(); }
    
    size_type size() const { return mTdfObjectPtrMap.size(); }
    bool      empty() const { return mTdfObjectPtrMap.empty(); }
    size_type capacity() const { return mTdfObjectPtrMap.capacity(); }

    iterator       find(const key_type& k) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.find(k); 
    }
    const_iterator find(const key_type& k) const { return mTdfObjectPtrMap.find(k); }

    template <typename U, typename BinaryPredicate>
    iterator find_as(const U& u, BinaryPredicate predicate) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.find_as(u, predicate); 
    }

    template <typename U, typename BinaryPredicate>
    const_iterator find_as(const U& u, BinaryPredicate predicate) const { return mTdfObjectPtrMap.find_as(u, predicate); }

    size_type count(const key_type& k) { return mTdfObjectPtrMap.count(k); }

    iterator lower_bound(const key_type& k) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.lower_bound(k); 
    }
    const_iterator lower_bound(const key_type& k) const { return mTdfObjectPtrMap.lower_bound(k); }

    iterator upper_bound(const key_type& k) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.upper_bound(k); 
    }
    const_iterator upper_bound(const key_type& k) const { return mTdfObjectPtrMap.upper_bound(k); }

    eastl::pair<iterator, iterator>             equal_range(const key_type& k) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.equal_range(k); 
    }
    eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const { return mTdfObjectPtrMap.equal_range(k); }

    reference at(size_type n) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap.at(n); 
    }
    const_reference at(size_type n) const { return mTdfObjectPtrMap.at(n); }

    mapped_type& operator[](const key_type& k) 
    { 
        MARK_SET_CHANGE_TRACKING;
        return mTdfObjectPtrMap[k]; 
    }

    virtual EA::Allocator::ICoreAllocator& getAllocator() const override
    {
        return *mTdfObjectPtrMap.get_allocator().mpCoreAllocator;
    }

protected:
   
    iterator getIterator(const TdfGenericConst& key)
    {
        iterator itr = end();
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = find(keyCasted);
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = find(keyCasted);
            }
        }
        return itr;
    }

    const_iterator getIteratorConst(const TdfGenericConst& key) const
    {
        const_iterator itr = end();
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = find(keyCasted);
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = find(keyCasted);
            }
        }
        return itr;
    }

    eastl::pair<iterator, bool> insertKey(const TdfGenericConst& key)
    {
        eastl::pair<iterator, bool> itr = eastl::make_pair(end(), false);
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = this->insert(getDefaultValue(keyCasted));
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = this->insert(getDefaultValue(keyCasted));
            }
        }
        return itr;
    }

    TdfObjectPtrMap mTdfObjectPtrMap;

};

/*! ***************************************************************/
/*! \class TdfPrimitiveMap
    \brief A map wrapper class for storing structured values

*******************************************************************/
template <typename K, typename V, typename Compare = eastl::less<K>, bool ignoreCaseStringCompare = false >
class TdfPrimitiveMap : protected eastl::vector_map<K, V,Compare,EA_TDF_ALLOCATOR_TYPE>, public TdfMapBase
{
    EA_TDF_NON_ASSIGNABLE(TdfPrimitiveMap);
public:
    template <typename T1, typename T2>
    friend struct eastl::pair;

    typedef typename eastl::vector_map<K,V,Compare,EA_TDF_ALLOCATOR_TYPE> base_type;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::pointer pointer;
    typedef typename base_type::const_pointer const_pointer;
    typedef typename base_type::reference reference;
    typedef typename base_type::const_reference const_reference;   
    typedef typename base_type::key_type key_type;
    typedef typename base_type::mapped_type mapped_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::iterator iterator;
    typedef typename base_type::const_iterator const_iterator;
    typedef typename base_type::reverse_iterator reverse_iterator;
    typedef typename base_type::const_reverse_iterator const_reverse_iterator;
    typedef typename eastl::pair<iterator, bool> insert_return_type;
    typedef V ValType;

    typedef TdfPrimitiveMap<K,V, Compare,ignoreCaseStringCompare> this_type;

    // this is required to workaround what appears to be a compiler bug that causes fatal error C1001
    // to be thrown for WinRT builds when we try to call base_type::begin() or base_type::end().  
    // These methods are the only ones that trigger this bug, possibly because eastl::vector_map has using base_type::begin
    // and using base_type::end.  Note that base_container is essentially the parent class of base_type.getDefaultValue
    typedef typename eastl::vector<eastl::pair<K, V>, EA_TDF_ALLOCATOR_TYPE> base_container;


private:
    TdfPrimitiveMap(const this_type& obj)
        : base_type(EA_TDF_ALLOCATOR_TYPE("TdfPrimitiveMap", &obj.getAllocator())), TdfMapBase(obj.getAllocator())
    {
        obj.copyInto(*this);
    }

public:
    EA::Allocator::ICoreAllocator& getAllocator() const override
    {
        return *base_type::get_allocator().mpCoreAllocator;
    }

protected:
    iterator getIterator(const TdfGenericConst& key)
    {
        iterator itr = end();
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = find(keyCasted);
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = find(keyCasted);
            }
        }
        return itr;
    }

    const_iterator getIteratorConst(const TdfGenericConst& key) const
    {
        const_iterator itr = end();
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = find(keyCasted);
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = find(keyCasted);
            }
        }
        return itr;
    }
private:
    // Helper function to properly allocate strings:
    value_type getDefaultValue(const key_type& keyCasted)
    {
        return value_type( TdfPrimitiveAllocHelper<key_type>()(keyCasted, getAllocator()), TdfPrimitiveAllocHelper<mapped_type>()(getAllocator()));
    }
public:
    eastl::pair<iterator, bool> insertKey(const TdfGenericConst& key)
    {
        eastl::pair<iterator, bool> itr = eastl::make_pair(end(), false);
        if (TdfMapBase::isCompatibleKey(key))
        {
            const key_type& keyCasted = *reinterpret_cast<const key_type*>(key.asAny());
            itr = this->insert(getDefaultValue(keyCasted));
        }
        else if (getKeyTypeDesc().isIntegral())
        {
            // This special logic is to preserve back compat with integer maps (when the indexing value may not be the exact same integral type)
            key_type keyCasted;
            TdfGenericReference ref(keyCasted);
            if (key.convertToIntegral(ref))
            {
                itr = this->insert(getDefaultValue(keyCasted));
            }
        }
        return itr;
    }

public:
    // Ctors and Dtors
    TdfPrimitiveMap( EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t *debugMemName = "TdfPrimitiveMap")
        : base_type(EA_TDF_ALLOCATOR_TYPE(debugMemName, &allocator)), TdfMapBase(allocator)
    {
    }

#if !defined(EA_COMPILER_NO_RVALUE_REFERENCES)
    TdfPrimitiveMap(this_type&& x)
        : base_type(EA_TDF_ALLOCATOR_TYPE("TdfPrimitiveMap", &(x.getAllocator()))), TdfMapBase(x.getAllocator())
    {
        swap(x);
    }
#endif

    const base_type& asMap() const { return *this; }
    base_type& asMap() 
    { 
        MARK_SET_CHANGE_TRACKING;
        return *this; 
    }

    bool ignoreCaseKeyStringCompare() const override { return ignoreCaseStringCompare; }

    void initMap(size_t count) override
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::clear();
        base_type::resize(static_cast<size_type>(count), getDefaultValue(TdfPrimitiveAllocHelper<key_type>()(getAllocator())));  // Make sure all resized elements use the right allocators:
    }

    const TypeDescriptionMap& getTypeDescription() const override { return static_cast<const TypeDescriptionMap&>(TypeDescriptionSelector<this_type>::get()); }

    bool insertKeyGetValue(const TdfGenericConst& key, TdfGenericReference& outValue) override
    {
        eastl::pair<iterator, bool> itr = this->insertKey(key);
        if (itr.first != this->end())
        {
            outValue.setRef(itr.first->second);
        }
        return itr.second;
    }

    bool eraseValueByKey(const TdfGenericConst& key) override
    {        
        iterator itr = this->getIterator(key);
        if (itr != this->end())
        {
            return this->erase(itr) != 0;
        }

        return false;
    }


#if EA_TDF_INCLUDE_MEMBER_MERGE
    //  extends the existing map to include members from the reference and visits this map's members.
    void mergeMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &refValue) override
    {
        EA_ASSERT_MSG(refValue.getTdfId() == getTdfId(), "Attempted to merge maps of differing types.");
        const this_type &refTyped = static_cast<const this_type &>(refValue);
        for (const_iterator ri = refTyped.begin(), re = refTyped.end(); ri != re; ++ri)
        {        
            eastl::pair<iterator, bool> inserter = this->insert(*ri);
            iterator i = inserter.first;
            if (inserter.second)
            {
                TdfGenericReference val(i->second), ref(ri->second);
                val.setValue(ref);
            }
            TdfGenericReferenceConst refFirst(ri->first), refSecond(ri->second);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->first),&refFirst);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->second), &refSecond);
        }
    }
#endif

    void copyIntoObject(TdfObject& lhs, const MemberVisitOptions& options) const override { copyInto(static_cast<this_type&>(lhs), options); }
    void copyInto(this_type& lhs, const MemberVisitOptions& options = MemberVisitOptions()) const
    {
        if (this == &lhs)
        {
            return;
        }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        lhs.markSet();
#endif
        // If the key is a string type, we'll need to allocate the keys with the new allocator.
        lhs.reserve(base_type::size());
        for (const_iterator i = base_type::begin(), e = base_type::end(); i != e; ++i)
        {
            // Here we copy the element into the lhs using the lhs map's allocator  (not the base map's allocator)
            lhs[TdfPrimitiveAllocHelper<key_type>()(i->first, lhs.getAllocator())] = TdfPrimitiveAllocHelper<mapped_type>()(i->second, lhs.getAllocator());
        }
    }

    void swap(this_type& lhs)
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
        lhs.markSet();
#endif
        // Note: This only swaps if the allocators used by the maps are equal
        base_type::swap(lhs);
    }

    void clearMap() override
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::clear();
    }

    size_t mapSize() const override { return base_type::size(); }


#if EA_TDF_MAP_ELEMENT_FIXUP_ENABLED
    void fixupElementOrder() override
    {
        eastl::sort(begin(), end(), base_type::value_comp());
    }
#endif

    void visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parent, uint32_t tag, const TdfMapBase &refValue) override
    {
        const this_type &referenceTyped = (const this_type &)refValue;
        // Remove any elements that aren't in the new type:
        for (iterator i = begin(); i != end();)
            i = (referenceTyped.find(i->first) == referenceTyped.end()) ? erase(i) : i + 1;

        for (auto ri : referenceTyped)
        {   // We don't care if the insert was new or not since we're going to visit the data either way:
            TdfGenericReferenceConst refFirst(ri.first), refSecond(ri.second);
            iterator i = insertKey(refFirst).first;
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->first), &refFirst);
            visitor.visitReference(rootTdf, parent, tag, TdfGenericReference(i->second), &refSecond);
        }
    }

    bool visitMembers(TdfMemberVisitor &visitor, const TdfVisitContext& callingContext) override
    {        
        for (iterator i = begin(), e = end(); i != e; ++i)
        {
            TdfGenericReference key(i->first), value(i->second);
            TdfVisitContext c(callingContext, key, value);                        
            if (!visitor.visitContext(c))
                return false;
        }
        return true;
    }


    bool visitMembers(TdfMemberVisitorConst &visitor, const TdfVisitContextConst& callingContext) const override
    {        
        for (const_iterator i = begin(), e = end(); i != e; ++i)
        {
            TdfGenericReferenceConst key(i->first), value(i->second);
            TdfVisitContextConst c(callingContext, key, value);                        
            if (!visitor.visitContext(c))
                return false;
        }
        return true;
    }

    bool getValueByIndex(size_t index, TdfGenericReferenceConst &outKey, TdfGenericReferenceConst &outValue) const override
    {
        if (index < size())
        {
            const typename base_type::value_type& keyValuePair = this->at(static_cast<size_type>(index));
            outKey.setRef(keyValuePair.first);
            outValue.setRef(keyValuePair.second);
            return true;
        }

        return false;
    }

    bool getReferenceByIndex(size_t index, TdfGenericReference &outKey, TdfGenericReference &outValue) override
    {
        if (index < size())
        {
            typename base_type::value_type& keyValuePair = this->at(static_cast<size_type>(index));
            outKey.setRef(keyValuePair.first);
            outValue.setRef(keyValuePair.second);
            return true;
        }

        return false;
    }

    bool getValueByKey(const TdfGenericConst& key, TdfGenericReferenceConst &value) const override
    {
        const_iterator itr = this->getIteratorConst(key);
        if (itr != this->end())
        {
            value.setRef(itr->second);
            return true;
        }
        return false;
    }

    bool getValueByKey(const TdfGenericConst& key, TdfGenericReference &value) override
    {
        iterator itr = this->getIterator(key);
        if (itr != this->end())
        {
            value.setRef(itr->second);
            return true;
        }
        return false;
    }

    bool equalsValue(const TdfMapBase& rhs) const override
    {
        if (getTdfId() != rhs.getTdfId() || mapSize() != rhs.mapSize())
            return false;

        for (const_iterator i = begin(), e = end(), ri = ((const this_type&) rhs).begin(), re = ((const this_type&) rhs).end(); i != e && ri != re; ++i, ++ri)
        {
            TdfGenericReferenceConst key(i->first), rkey(ri->first), value(i->second), rvalue(ri->second);
            if (!key.equalsValue(rkey) || !value.equalsValue(rvalue))
                return false;
        }

        return true;
    }

    void reserve(size_type size)
    {
        // NOTE: All Tdf containers clear when doing a reserve()
        // this is now done for compatibility, and to make copyInto() safe.
        MARK_SET_CHANGE_TRACKING;
        base_type::clear();
        base_type::reserve(size);
    };

    mapped_type& operator[](const K& k)
    {
        MARK_SET_CHANGE_TRACKING;

        // Custom implementation of the operator[] 
        iterator itLB(lower_bound(k));

        if((itLB == this->end()) || this->key_comp()(k, (*itLB).first))
            itLB = this->insert(itLB, getDefaultValue(k));                 // Setup the allocator correctly for strings
        return (*itLB).second;
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING

    eastl::pair<iterator, bool> insert(const value_type& value)
    {
        markSet();
        return base_type::insert(value);
    }

    eastl::pair<iterator, bool> insert(const key_type& key)
    {
        markSet();
        return base_type::insert(key);
    }

    iterator insert(iterator position, const value_type& value)
    {
        markSet();
        return base_type::insert(position, value);
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        markSet();
        base_type::insert(first, last);
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

    size_type erase(const key_type& k)
    {
        markSet();
        return base_type::erase(k);
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

    void clear() { clearMap(); }

    iterator begin()
    {
        markSet();
        return base_container::begin();
    }

    const_iterator begin() const
    {
        return base_container::begin();
    }

    iterator end()
    {
        markSet();
        return base_container::end();
    }

    const_iterator end() const
    {
        return base_container::end();
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

    iterator find(const key_type& k)
    {
        markSet();
        return base_type::find(k);
    }

    const_iterator find(const key_type& k) const
    {
        return base_type::find(k);
    }

    template <typename U, typename BinaryPredicate>
    iterator find_as(const U& u, BinaryPredicate predicate)
    {
        markSet();
        return base_type::find_as(u, predicate);
    }

    template <typename U, typename BinaryPredicate>
    const_iterator find_as(const U& u, BinaryPredicate predicate) const
    {
        return base_type::find_as(u, predicate);
    }

    iterator lower_bound(const key_type& k)
    {
        markSet();
        return base_type::lower_bound(k);
    }

    const_iterator lower_bound(const key_type& k) const
    {
        return base_type::lower_bound(k);
    }

    iterator upper_bound(const key_type& k)
    {
        markSet();
        return base_type::upper_bound(k);
    }

    const_iterator upper_bound(const key_type& k) const
    {
        return base_type::upper_bound(k);
    }

    eastl::pair<iterator, iterator> equal_range(const key_type& k)
    {
        markSet();
        return base_type::equal_range(k);
    }

    eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
    {
        return base_type::equal_range(k);
    }

#endif

    using base_type::key_comp;
    using base_type::value_comp;
    using base_type::empty;
    using base_type::size;
    using base_type::capacity;
    using base_type::count;
#if !EA_TDF_INCLUDE_CHANGE_TRACKING
    using base_type::begin;
    using base_type::end;
    using base_type::rbegin;
    using base_type::rend;
    using base_type::at;
    using base_type::find;
    using base_type::find_as;
    using base_type::lower_bound;
    using base_type::upper_bound;
    using base_type::equal_range;
    using base_type::insert;
    using base_type::erase;
    using base_type::assign;
    using base_type::clear;
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

};

template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
struct TypeDescriptionSelector<TdfPrimitiveMap<K,V,Compare,ignoreCaseStringCompare> >
{
    static const TypeDescriptionMap& get()
    {
        // Note: The TypeDescriptionMap returned here still needs to be registered and fixed up in order to allocate a name and TdfId. (See tdffactory.h)
        static TypeDescriptionMap result(TypeDescriptionSelector<K>::get(), TypeDescriptionSelector<V>::get(), 
            (TypeDescriptionObject::CreateFunc) &TdfObject::createInstance<TdfPrimitiveMap<K,V,Compare,ignoreCaseStringCompare> >);
#if EA_TDF_REGISTER_ALL
        static TypeRegistrationNode registrationNode(result, true);
#endif
        return result;
    }
};

/*! ***************************************************************/
/*! \class TdfStructMap
    \brief A map wrapper class for storing structured values
*******************************************************************/
template <typename K, typename V, typename Compare = eastl::less<K>, bool ignoreCaseStringCompare = false >
class TdfStructMap : public TdfObjectMap<K, Compare, ignoreCaseStringCompare>
{
    EA_TDF_NON_COPYABLE(TdfStructMap);
public:

    typedef typename eastl::vector_map<K,tdf_ptr<V>,Compare,EA_TDF_ALLOCATOR_TYPE> TemplateMap;
    typedef typename TemplateMap::mapped_type mapped_type;
    typedef typename TemplateMap::size_type size_type;
    typedef typename TemplateMap::iterator iterator;
    typedef typename TemplateMap::value_type value_type;
    typedef typename TemplateMap::key_type key_type;
    typedef typename TemplateMap::const_iterator const_iterator;
    typedef typename TemplateMap::reverse_iterator reverse_iterator;
    typedef typename TemplateMap::const_reverse_iterator const_reverse_iterator;
    typedef typename TemplateMap::reference reference;
    typedef typename TemplateMap::const_reference const_reference;
    typedef V ValType;

    typedef TdfObjectMap<K,Compare,ignoreCaseStringCompare> base_type;
    typedef TdfStructMap<K,V,Compare,ignoreCaseStringCompare> this_type;
    typedef typename base_type::iterator base_iterator;
    typedef typename base_type::const_iterator base_const_iterator;
    typedef typename base_type::reverse_iterator base_reverse_iterator;
    typedef typename base_type::key_type base_key_type;
    typedef typename base_type::value_type base_value_type;
    using base_type::markSet;
    using base_type::getAllocator;


    // Ctors and Dtors
    TdfStructMap( EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t *debugMemName = "TdfStructMap")
    : base_type(allocator, debugMemName)
    {
    }

    const TypeDescriptionMap& getTypeDescription() const override { return static_cast<const TypeDescriptionMap&>(TypeDescriptionSelector<this_type>::get()); }

    /*! ***************************************************************/
    /*! \brief  Allocate a new object that can be used to insert into
                this map ensuring that the correct allocator is used.
    *******************************************************************/
    V* allocate_element() override { return new_element(); }

    void swap(this_type& lhs)
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
        lhs.markSet();
#endif
        base_type::swap(lhs);
    }

    mapped_type& operator[](const K& k)
    {
        MARK_SET_CHANGE_TRACKING;
        return (mapped_type&)base_type::operator[](TdfPrimitiveAllocHelper<key_type>()(k, getAllocator()));
    }

    eastl::pair<iterator, bool> insert(const value_type& value) 
    {
        MARK_SET_CHANGE_TRACKING;
        eastl::pair<base_iterator, bool> basePair = base_type::insert((base_value_type&)value);
        return eastl::pair<iterator, bool>((iterator)basePair.first, basePair.second);
    }

    eastl::pair<iterator, bool> insert(const key_type& key)
    {
        MARK_SET_CHANGE_TRACKING;
        eastl::pair<base_iterator, bool> basePair = base_type::insert((base_key_type&)key);
        return eastl::pair<iterator, bool>((iterator)basePair.first, basePair.second);
    }

    iterator insert(iterator position, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        return base_type::insert(position, (base_value_type&)value);
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::insert(first, last);
    }

    iterator erase(iterator position)
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(base_type::erase((base_iterator)position));
    }

    iterator erase(iterator first, iterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        return iterator(base_type::erase((base_iterator)first, (base_iterator)last));
    }

    size_type erase(const key_type& k)
    {
        MARK_SET_CHANGE_TRACKING;
        return base_type::erase(k);
    }

    reverse_iterator erase(reverse_iterator position)
    {   
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(base_type::erase((base_reverse_iterator)position));
    }

    reverse_iterator erase(reverse_iterator first, reverse_iterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        return reverse_iterator(base_type::erase((base_reverse_iterator)first, (base_reverse_iterator)last));
    }

    void assign(size_type n, const value_type& value)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::assign(n, (base_value_type&)value);
    }

    template <typename InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        MARK_SET_CHANGE_TRACKING;
        base_type::assign(first, last);
    }

    iterator begin() { return iterator(base_type::begin()); }
    const_iterator begin() const { return const_iterator(base_type::begin()); }

    iterator end() { return iterator(base_type::end()); }
    const_iterator end() const { return const_iterator(base_type::end()); }

    reverse_iterator rbegin() { return reverse_iterator(iterator(base_type::rbegin().base()));}
    const_reverse_iterator rbegin() const { return const_reverse_iterator(const_iterator(base_type::rbegin().base())); }

    reverse_iterator rend() { return reverse_iterator(iterator(base_type::rend().base())); }
    const_reverse_iterator rend() const { return const_reverse_iterator(const_iterator(base_type::rend().base())); }

    iterator find(const key_type& k) { return iterator(base_type::find(k)); }
    const_iterator find(const key_type& k) const { return const_iterator(base_type::find(k)); }

    reference at(size_type n) { return (reference)base_type::at(n); }
    const_reference at(size_type n) const { return (const_reference)base_type::at(n); }

    iterator       lower_bound(const key_type& k) { return iterator(base_type::lower_bound(k)); }
    const_iterator lower_bound(const key_type& k) const { return const_iterator(base_type::lower_bound(k)); }

    iterator       upper_bound(const key_type& k) { return iterator(base_type::upper_bound(k)); }
    const_iterator upper_bound(const key_type& k) const { return const_iterator(base_type::upper_bound(k)); }

    eastl::pair<iterator, iterator> equal_range(const key_type& k)
    {
        eastl::pair<base_iterator, base_iterator> basePair = base_type::equal_range(k);
        return eastl::pair<iterator, iterator>((iterator)basePair.first, (iterator)basePair.second);
    }

    eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
    {
        eastl::pair<base_const_iterator, base_const_iterator> basePair = base_type::equal_range(k);
        return eastl::pair<const_iterator, const_iterator>((const_iterator)basePair.first, (const_iterator)basePair.second);
    }

    using base_type::key_comp;
    using base_type::value_comp;
    using base_type::empty;
    using base_type::size;
    using base_type::capacity;
    using base_type::count;
    using base_type::resize;
    using base_type::clear;

protected:
   
    virtual V* new_element() const
    {
        return (V*)TdfObject::createInstance<V>(getAllocator(), "TdfMapElement");
    }
};

template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
struct TypeDescriptionSelector<TdfStructMap<K,V,Compare,ignoreCaseStringCompare> >
{
    static const TypeDescriptionMap& get()
    {
        // Note: The TypeDescriptionMap returned here still needs to be registered and fixed up in order to allocate a name and TdfId. (See tdffactory.h)
        static TypeDescriptionMap result(TypeDescriptionSelector<K>::get(), TypeDescriptionSelector<V>::get(), 
            (TypeDescriptionObject::CreateFunc) &TdfObject::createInstance<TdfStructMap<K,V,Compare,ignoreCaseStringCompare> >);
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
            template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfPrimitiveMap<K, V, Compare, ignoreCaseStringCompare>* object) { delete object; }

            template <typename K, typename V, typename Compare, bool ignoreCaseStringCompare>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfStructMap<K, V, Compare, ignoreCaseStringCompare>* object) { delete object; }
        }
    }
}


#undef MARK_SET_CHANGE_TRACKING

#endif // EA_TDF_H

