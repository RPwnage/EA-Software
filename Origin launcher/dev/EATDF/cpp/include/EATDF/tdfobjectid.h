/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_OBJECTID_H
#define EA_TDF_OBJECTID_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

/*** Include files *******************************************************************************/
#include <EASTL/string.h>
#include <EASTL/core_allocator_adapter.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{
 

typedef uint16_t ComponentId;
typedef uint16_t EntityType;
typedef int64_t EntityId;

typedef eastl::basic_string<char8_t, EA_TDF_ALLOCATOR_TYPE> EATDFEastlString;

/*************************************************************************************************/
/*!
    \class ObjectType

    This class refers to a particular type in the context of it's owner component.
    A ObjectType consists of a component ID and an entity type which is unique to the
    component itself.
*/
/*************************************************************************************************/
struct EATDF_API ObjectType
{
    ComponentId component;
    EntityType type;

    ObjectType() : component(0), type(0) { }
    ObjectType(uint32_t val) : component((uint16_t) (val >> 16)), type((uint16_t) val) {}
    ObjectType(ComponentId c, EntityType t) : component(c), type(t) { }
    ObjectType(const ObjectType& rhs) : component(rhs.component), type(rhs.type) { }

    ObjectType& operator=(const ObjectType& rhs)
    {
        component = rhs.component;
        type = rhs.type;
        return *this;
    }

    bool operator==(const ObjectType& rhs) const
    {
        return ((component == rhs.component) && (type == rhs.type));
    }

    bool operator!=(const ObjectType& rhs) const
    {
        return !operator==(rhs);
    }

    bool operator<(const ObjectType& rhs) const
    {
        return ((component << 16) | type) < ((rhs.component << 16) | rhs.type);
    }
 
    size_t toString(char8_t* buf, size_t bufSize, char8_t separator = '/') const;

    EATDFEastlString toString(char8_t separator EA_TDF_EXT_ONLY_DEF_ARG('/'), EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) const
    {
        char8_t buf[128];
        toString(buf, sizeof(buf), separator);
        return EATDFEastlString(buf, EA_TDF_ALLOCATOR_TYPE("ObjectType::toString", &alloc));
    }

    static ObjectType parseString(const char8_t* value, char8_t separator = '/', const char8_t** savePtr = nullptr);

    uint32_t toInt() const { return (((uint32_t) component) << 16) + type; }

    bool isSet() const { return component != 0; }

    typedef const char8_t* (*ComponentNameFromIdFunc)(ComponentId id, bool* isKnown);
    typedef const char8_t* (*EntityTypeNameFromTypeFunc)(ComponentId compId, EntityType entityType, bool* isKnown);
    typedef ComponentId (*ComponentIdFromNameFunc)(const char8_t* componentName);
    typedef EntityType (*EntityTypeFromNameFunc)(ComponentId compId, const char8_t* entityName);

    static void setNameConversionFuncs(ComponentNameFromIdFunc componentNameFromIdFunc,
        EntityTypeNameFromTypeFunc entityTypeNameFromTypeFunc,
        ComponentIdFromNameFunc componentIdFromNameFunc,
        EntityTypeFromNameFunc entityTypeFromNameFunc)
    {
        *getComponentNameFromIdFunc() = componentNameFromIdFunc;
        *getEntityTypeNameFromTypeFunc() = entityTypeNameFromTypeFunc;
        *getComponentIdFromNameFunc() = componentIdFromNameFunc;
        *getEntityTypeFromNameFunc() = entityTypeFromNameFunc;
    }

private:

    static void getComponentNameFromId(ComponentId id, char8_t* buf, size_t bufSize);
    static void  getEntityTypeNameFromType(ComponentId compId, EntityType entityType, char8_t* buf, size_t bufSize);
    static ComponentId getComponentIdFromName(const char8_t* componentName);
    static EntityType getEntityTypeFromName(ComponentId compId, const char8_t* entityName);

    //These are done as functions to ensure order of initialization errors don't occur if someone decides to call the set method 
    //before this module has initialized.
    static ComponentNameFromIdFunc* getComponentNameFromIdFunc() { static ComponentNameFromIdFunc func = nullptr; return &func; }
    static EntityTypeNameFromTypeFunc* getEntityTypeNameFromTypeFunc() { static EntityTypeNameFromTypeFunc func = nullptr; return &func; }
    static ComponentIdFromNameFunc* getComponentIdFromNameFunc() { static ComponentIdFromNameFunc func = nullptr; return &func; }
    static EntityTypeFromNameFunc* getEntityTypeFromNameFunc() { static EntityTypeFromNameFunc func = nullptr; return &func; }

};

/*************************************************************************************************/
/*!
    \class ObjectId

    This class refers to a particular object in the context of it's owner component.
    A ObjectId consists of a ObjectType and a unique value identifying the instance
    of the given type.
*/
/*************************************************************************************************/
struct EATDF_API ObjectId
{
    EntityId id;
    ObjectType type;

    ObjectId() : id(0) { } 
    ObjectId(ComponentId c, EntityType t, EntityId i) : id(i), type(c, t) { }
    ObjectId(ObjectType t, EntityId i) : id(i), type(t) { }
    ObjectId(const ObjectId& rhs) : id(rhs.id), type(rhs.type) { }

    ObjectId& operator=(const ObjectId& rhs)
    {
        type = rhs.type;
        id = rhs.id;
        return *this;
    }

    bool operator==(const ObjectId& rhs) const
    {
        return ((id == rhs.id) && (type == rhs.type));
    }

    bool operator!=(const ObjectId& rhs) const
    {
        return !operator==(rhs);
    }

    bool operator<(const ObjectId& rhs) const
    {
        if (type.component != rhs.type.component)
            return (type.component < rhs.type.component);

        if (type.type != rhs.type.type)
            return (type.type < rhs.type.type);

        return (id < rhs.id);
    }

    size_t toString(char8_t* buf, size_t bufSize, char8_t separator = '/') const;
    
    EATDFEastlString toString(char8_t separator EA_TDF_EXT_ONLY_DEF_ARG('/'), EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) const
    {
        char8_t buf[256];
        toString(buf, sizeof(buf), separator);
        return EATDFEastlString(buf, EA_TDF_ALLOCATOR_TYPE("ObjectId::toString", &alloc));
    }

    static ObjectId parseString(const char8_t* value, char8_t separator = '/');
    
    template<class T>
    T toObjectId() const { return static_cast<T>(id); }

    bool isSet() const { return type.isSet(); }
};

// Invalid Object Type.
extern EATDF_API const ObjectType OBJECT_TYPE_INVALID;

// Invalid Object Id.
extern EATDF_API const ObjectId OBJECT_ID_INVALID;

} //namespace TDF
} //namespace EA

namespace eastl
{

// Define eastl hash implementations for ObjectType and ObjectId so they can be used
// in eastl hash_maps without requiring users to explicitly provide a hash in the template
// parameters.

template <>
struct hash<EA::TDF::ObjectType>
{
    size_t operator()(const EA::TDF::ObjectType& x) const
    {
        return static_cast<size_t>((x.component << 16) & x.type);
    }
};

template <>
struct hash<EA::TDF::ObjectId>
{
    size_t operator()(const EA::TDF::ObjectId& x) const
    {
        return static_cast<size_t>(x.id);
    }
};

} // namespace eastl


#endif // EA_TDF_OBJECTID_H

