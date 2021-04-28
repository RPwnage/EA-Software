/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include <EATDF/tdfobjectid.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

namespace EA
{
namespace TDF
{
 
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
// Invalid Object Type.
const ObjectType OBJECT_TYPE_INVALID = ObjectType(0, 0);

// Invalid Object Id.
const ObjectId OBJECT_ID_INVALID = ObjectId(0, 0, 0);

/*** Public Methods ******************************************************************************/

void ObjectType::getComponentNameFromId(ComponentId id, char8_t* buf, size_t bufSize) 
{
    bool isKnown = false;
    if (*getComponentNameFromIdFunc() != nullptr)
    {
        EA::StdC::Strncpy(buf, (**getComponentNameFromIdFunc())(id, &isKnown), bufSize);
    }
    

    if (!isKnown)
    {
        EA::StdC::Snprintf(buf, bufSize, "%" PRIu16, id);
    }
}

void ObjectType::getEntityTypeNameFromType(ComponentId compId, EntityType entityType, char8_t* buf, size_t bufSize) 
{ 
    bool isKnown = false;
    if (*getEntityTypeNameFromTypeFunc() != nullptr)
    {
        EA::StdC::Strncpy(buf, (**getEntityTypeNameFromTypeFunc())(compId, entityType, &isKnown), bufSize);
    }
   
    if (!isKnown)
    {
        EA::StdC::Snprintf(buf, bufSize, "%" PRIu16, entityType); 
    }
}

ComponentId ObjectType::getComponentIdFromName(const char8_t* componentName) 
{ 
    if (*getComponentIdFromNameFunc() != nullptr)
    {
        return (**getComponentIdFromNameFunc())(componentName);
    }
    else
    {
        return 0; 
    }
}

EntityType ObjectType::getEntityTypeFromName(ComponentId compId, const char8_t* entityName) 
{ 
    if (*getEntityTypeFromNameFunc() != nullptr)
    {
        return (**getEntityTypeFromNameFunc())(compId, entityName);
    }
    else
    {
        return 0; 
    }
}


size_t ObjectType::toString(char8_t* buf, size_t bufSize, char8_t separator) const
{
    char compBuf[128];
    char typeBuf[128];
    getComponentNameFromId(component, compBuf, sizeof(compBuf));
    getEntityTypeNameFromType(component, type, typeBuf, sizeof(typeBuf));

    return EA::StdC::Snprintf(buf, bufSize, "%s%c%s", compBuf, separator, typeBuf);
}


// static
ObjectType ObjectType::parseString(const char8_t* value, char8_t separator, const char8_t** savePtr)
{
    if (value == nullptr)
        return OBJECT_TYPE_INVALID;

    char8_t tmpBuf[64];
    const char8_t* v = value;
    int32_t idx = 0;
    
    // Extract the component
    for(; (v[0] != '\0') && (v[0] != separator); ++v, ++idx)
        tmpBuf[idx] = v[0];
    if (v[0] == '\0')
        return OBJECT_TYPE_INVALID;
    ++v;
    tmpBuf[idx] = '\0';
    ComponentId componentId = 0;
    if ((tmpBuf[0] >= '0') && (tmpBuf[0] <= '9'))
    {
        componentId = (ComponentId) EA::StdC::AtoU32(tmpBuf); 
    }
    else
    {
        componentId = getComponentIdFromName(tmpBuf);
    }

    // Extract the entity type
    idx = 0;
    for(; (v[0] != '\0') && (v[0] != separator); ++v, ++idx)
        tmpBuf[idx] = v[0];
    tmpBuf[idx] = '\0';
    EntityType entityType = 0;
    if ((tmpBuf[0] >= '0') && (tmpBuf[0] <= '9'))
    {
        entityType = (EntityType) EA::StdC::AtoU32(tmpBuf); 
    }
    else
    {       
        entityType = getEntityTypeFromName(componentId, tmpBuf);
    }

    if (savePtr != nullptr)
        *savePtr = v;
    
    return ObjectType(componentId, entityType);
}

size_t ObjectId::toString(char8_t* buf, size_t bufSize, char8_t separator) const
{
    size_t remainder = type.toString(buf, bufSize, separator);
    return EA::StdC::Snprintf(buf + remainder, bufSize - remainder, "%c%" PRId64, separator, id);
}


// static
ObjectId ObjectId::parseString(const char8_t* value, char8_t separator)
{
    const char8_t* idPtr = nullptr;
    ObjectType type = ObjectType::parseString(value, separator, &idPtr);
    if (type == OBJECT_TYPE_INVALID)
        return OBJECT_ID_INVALID;

    if (*idPtr == '\0')
        return ObjectId(type, 0);

    if (*idPtr != separator)
        return OBJECT_ID_INVALID;

    ++idPtr;
    if ((*idPtr != '-') && ((*idPtr < '0') || (*idPtr > '9')))
        return OBJECT_ID_INVALID;

    EntityId id = (EntityId) EA::StdC::AtoI64(idPtr);

    return ObjectId(type, id);
}

} //namespace TDF
} //namespace EA

