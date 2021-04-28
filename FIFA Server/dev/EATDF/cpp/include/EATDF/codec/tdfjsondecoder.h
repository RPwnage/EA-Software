/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_JSONDECODER_H
#define EA_TDF_JSONDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/tdfvisit.h>
#include <EAJson/JsonReader.h>
#include <EAJson/JsonCallbackReader.h>
#include <EAIO/EAStream.h>
#include <EATDF/codec/tdfdecoder.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

class EATDF_API JsonReaderWithoutAsserts : public EA::Json::JsonReader
{
public:
    JsonReaderWithoutAsserts(EA::Allocator::ICoreAllocator* pAllocator = nullptr, size_t bufferBlockSize = 0) : 
        EA::Json::JsonReader(pAllocator, bufferBlockSize) 
    { 
        // We don't want asserts (aka. crashes) on our server if we can handle the bad data.
        mbAssertOnSyntaxError = false; 
    }
};

class EATDF_API JsonDecoder : public EA::TDF::TdfDecoder
{
    EA_TDF_NON_ASSIGNABLE(JsonDecoder);
public:
    JsonDecoder(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) : mAllocator(alloc), mReader(&alloc){}

    bool decode(EA::IO::IStream& iStream, Tdf& tdf, const MemberVisitOptions& options = MemberVisitOptions()) override;
    static const char8_t* getClassName() { return "json"; }
    const char8_t* getName() const override { return JsonDecoder::getClassName(); }

private:
    bool readValue(EA::TDF::TdfGenericReference& ref); 
    bool isBasicDataType(EA::Json::EventType e)
    {
        return (e == EA::Json::kETDouble ||
            e == EA::Json::kETInteger ||
            e == EA::Json::kETString ||
            e == EA::Json::kETBool);
    }

    template <typename T>
    bool readInteger(T& val)
    {
        if (isBasicDataType(mReader.GetEventType()))
        {
            val = static_cast<T>(mReader.GetInteger());
            return true;
        }
        else if (mReader.GetEventType() == EA::Json::kETNull)
            return true;
        return false;
    }

    bool readBool(bool& val)
    {
        if (isBasicDataType(mReader.GetEventType()))
        {
            val = mReader.GetBool();
            return true;
        }
        else if (mReader.GetEventType() == EA::Json::kETNull)
            return true;

        return false;
    }

    template <typename T>
    bool readFloat(T& val)
    {
        if (isBasicDataType(mReader.GetEventType()))
        {
            val = static_cast<T>(mReader.GetDouble());
            return true;
        }
        else if (mReader.GetEventType() == EA::Json::kETNull)
            return true;

        return false;
    }

    bool readTdfFields(const char8_t* name, TdfGenericReference& ref);
    bool readMapFields(const char8_t* name, TdfGenericReference& ref);
    bool readVariableFields(const char8_t* name, TdfGenericReference& ref);
    bool readGenericFields(const char8_t* name, TdfGenericReference& ref);
    bool readVector(TdfVectorBase& vector);

    bool readString(TdfString& val);
    bool readTimeValue(TimeValue& val);
    bool readEnum(int32_t& val, const TypeDescriptionEnum& enumMap);
    bool readBitfield(TdfBitfield& val);
    bool readJsonObject(TdfGenericReference& ref, bool (JsonDecoder::*objValueCallback)(const char8_t*, TdfGenericReference&));
    bool readObjectTypeFields(const char8_t* name, TdfGenericReference& ref);
    bool readObjectIdFields(const char8_t* name, TdfGenericReference& ref);
    bool readBlobFields(const char8_t* name, TdfGenericReference& ref);

    // Skips the children of the current node
    bool skipJsonObjectValue();

    bool readSubfieldObject(TdfGenericReference& ref, uint32_t subFieldTag);
    EA::Allocator::ICoreAllocator& mAllocator;
    JsonReaderWithoutAsserts mReader;
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_JSONDECODER_H

