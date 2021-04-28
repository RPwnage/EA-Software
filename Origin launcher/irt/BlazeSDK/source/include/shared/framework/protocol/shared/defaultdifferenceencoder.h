/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DEFAULTDIFFERENCEENCODER_H
#define BLAZE_DEFAULTDIFFERENCEENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/tdfencoder.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue) \
    if (mDiffEnabled && value == defaultValue) \
        return; \
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);

/*** Public Methods ******************************************************************************/
template <class ParentType>
class DefaultDifferenceEncoder : public ParentType
{
public:

    DefaultDifferenceEncoder();
    ~DefaultDifferenceEncoder() override;

    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override;
    // enumeration visitor
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TimeValue &value, const EA::TDF::TimeValue &referenceValue, const EA::TDF::TimeValue defaultValue) override { CHECK_DEFAULT(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }

    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override;


    const char8_t* getName() const override { return DefaultDifferenceEncoder::getClassName(); }
    Encoder::SubType getSubType() const override { return Encoder::DEFAULTDIFFERNCE; }

    static const char8_t* getClassName() { return "defaultdifference"; }
    
private:
    bool mDiffEnabled;    
};

/*** Public Methods ******************************************************************************/
template <class ParentType>
DefaultDifferenceEncoder<ParentType>::DefaultDifferenceEncoder()
: mDiffEnabled(false)
{
}

template <class ParentType>
DefaultDifferenceEncoder<ParentType>::~DefaultDifferenceEncoder()
{
}

template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    mDiffEnabled = true;
    const bool result = ParentType::visit(tdf, referenceValue);
    mDiffEnabled = false;
    return result;
}


template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue)
{
    mDiffEnabled = true;
    const bool result = ParentType::visit(tdf, referenceValue);
    mDiffEnabled = false;
    return result;
}

template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    bool temp = mDiffEnabled;
    mDiffEnabled = true;
    const bool result = ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
    return result;
}

template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    bool temp = mDiffEnabled;
    mDiffEnabled = true;
    const bool result = ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
    return result;
}

template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    bool temp = mDiffEnabled;
    mDiffEnabled = true;
    const bool result = ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
    return result;
}

template <class ParentType>
bool DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    bool temp = mDiffEnabled;
    mDiffEnabled = true;
    const bool result = ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
    return result;
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    if (value.vectorSize() == 0 && referenceValue.vectorSize() == 0)
        return;
    bool temp = mDiffEnabled;
    mDiffEnabled = false;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    if (value.mapSize() == 0 && referenceValue.mapSize() == 0)
        return;
    bool temp = mDiffEnabled;
    mDiffEnabled = false;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
    mDiffEnabled = temp;
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    if (mDiffEnabled && value.getBits() == 0)
        return;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    if (mDiffEnabled && blaze_strcmp(value.c_str(), defaultValue) == 0)
        return;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue, maxLength);
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    if (mDiffEnabled && value.getCount() == 0 && value.getSize() == 0)
        return;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue);
}

template <class ParentType>
void DefaultDifferenceEncoder<ParentType>::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    if (mDiffEnabled && value == defaultValue)
        return;
    ParentType::visit(rootTdf, parentTdf, tag, value, referenceValue, enumMap, defaultValue);
}

} // Blaze

#endif // BLAZE_DEFAULTDIFFERENCEENCODER_H

