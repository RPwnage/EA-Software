/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_XMLDECODER_H
#define EA_TDF_XMLDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <coreallocator/icoreallocator_interface.h>
#include <EATDF/tdfvisit.h>
#include <EATDF/codec/tdfdecoder.h>
#include <EAIO/EAStream.h>
#include <UTFXml/XmlReader.h>

namespace EA
{
namespace TDF
{

class EATDF_API XmlDecoder : public EA::TDF::TdfDecoder
{
    EA_TDF_NON_ASSIGNABLE(XmlDecoder);
public:
    XmlDecoder(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) : mAllocator(alloc), mReader(&alloc) {}
    ~XmlDecoder() override {}
    bool decode(EA::IO::IStream& iStream, EA::TDF::Tdf& tdf, const MemberVisitOptions& options = MemberVisitOptions()) override;
    static const char8_t* getClassName() { return "xml2"; }
    const char8_t* getName() const override { return XmlDecoder::getClassName(); }

private:
    template <typename T>
    bool readInteger(T& val);

    bool readBool(bool& val);

    bool readFloat(float& val);

    bool readValue(EA::TDF::TdfGenericReference& ref);

    bool readTdfFields(const char8_t* name, TdfGenericReference& ref);

    bool readMapFields(TdfGenericReference& ref);
    bool readVariableFields(TdfGenericReference& ref);
    bool readGenericFields(TdfGenericReference& ref);
    bool readVector(TdfVectorBase& vector);
    bool readUnion(TdfGenericReference& ref);

    bool readString(TdfString& val);
    bool readTimeValue(TimeValue& val);
    bool readEnum(int32_t& val, const TypeDescriptionEnum& enumMap);
    bool readBitfield(TdfBitfield& val);

    // This is the wrapper function for XmlReader read function.
    // Skip the \n and extra whitespace, since UTFXML does not filter it out.
    bool readSkipNewLine(bool skipErrorLog = false);

    bool readXmlObject(const char8_t* name, TdfGenericReference& ref);

    bool readObjectTypeFields(ObjectType& type);
    bool readObjectIdFields(ObjectId& id);
    bool readBlobFields(TdfBlob& blob);

    bool beginValue();
    bool endValue();

    bool skip();
    bool isInWhiteSpace() const;

    EA::Allocator::ICoreAllocator& mAllocator;
    EA::XML::XmlReader mReader;
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_XMLDECODER_H

