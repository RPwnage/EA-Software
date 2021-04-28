/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_JSONENCODER_H
#define EA_TDF_JSONENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/internal/config.h>
#include <EAJson/JsonWriter.h>
#include <EAJson/JsonCallbackReader.h>
#include <EATDF/tdfvisit.h>
#include <EAIO/EAStream.h>
#include <EATDF/codec/tdfencoder.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{
    class IStreamToJsonWriteStream;

class EATDF_API JsonEncoder : public EA::TDF::TdfEncoder, public EA::TDF::TdfMemberVisitorConst
{
    EA_TDF_NON_ASSIGNABLE(JsonEncoder);
public:
    JsonEncoder(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) : mAllocator(alloc), mIncludeTDFMetaData(true) {}
    ~JsonEncoder() override {}

    struct FormatOptions {
        FormatOptions() 
        {
            options[Json::JsonWriter::kFormatOptionIndentSpacing] = 4;
            options[Json::JsonWriter::kFormatOptionLineEnd] = '\n';
        }

        int32_t options[Json::JsonWriter::kFormatOptionCount];
    };

    bool encode(EA::IO::IStream& iStream, const Tdf& tdf, const EncodeOptions* encOptions = nullptr, const MemberVisitOptions& options = MemberVisitOptions()) override;
    static const char8_t* getClassName() { return "json"; }
    virtual const char8_t* getName() const { return JsonEncoder::getClassName(); }
    void setFormatOptions(const FormatOptions& options) { mFormatOptions = options; }

    void setIncludeTDFMetaData(bool includeTDFMetaData)  { mIncludeTDFMetaData = includeTDFMetaData; }
    bool getIncludeTDFMetaData() const { return mIncludeTDFMetaData; }

private:
    bool visitValue(const TdfVisitContextConst& context) override;
    bool postVisitValue(const TdfVisitContextConst& context) override;
    bool writeObjectType(const ObjectType& objType);
    bool writeGenericObjValue(const TdfVisitContextConst &context);

    const char8_t* getMemNameFromMemInfo(const TdfVisitContextConst &context)
    {
        const TdfMemberInfo* memberInfo = context.getMemberInfo();
        return memberInfo->getMemberName();
    }

private:

    class BlazeJsonWriter : public EA::Json::JsonWriter
    {
    public:
        const IStreamToJsonWriteStream& getStream() const { return reinterpret_cast<IStreamToJsonWriteStream&>(*mpStream); }
        void decStackElementCount()
        {
            if (mStack[mIndentLevel].mElementCount <= 0)
            {
                EA_FAIL_MSG("Attempted to decrement element count below 0!");
                return;
            }
            --mStack[mIndentLevel].mElementCount;
        }
    };

    BlazeJsonWriter mWriter;
    FormatOptions mFormatOptions;
    EA::Allocator::ICoreAllocator& mAllocator;

    bool mIncludeTDFMetaData;
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_JSONENCODER_H
