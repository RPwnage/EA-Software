/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_XMLENCODER_H
#define EA_TDF_XMLENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfvisit.h>
#include <EAIO/EAStream.h>
#include <UTFXml/XmlWriter.h>
#include <EATDF/codec/tdfencoder.h>

namespace EA
{
namespace TDF
{
const int32_t MAX_XML_ELEMENT_LENGTH = 128;


class EATDF_API TdfXmlWriter : public EA::XML::XmlWriter
{
public:
    TdfXmlWriter() {}
    ~TdfXmlWriter() override {}

    void setIndentLevel(size_t num);
    void setStateToChars();
    bool checkValueIsEmpty() const;

    // The following custom encoders remove various sanity checks that the XmlWriter performs which may slow down performance. 
    // They should only be used when the input data is known to be in a valid format. 
    // (i.e. No strings with the following characters-  &<>\"  )

    bool customBeginElement(const char* psElementName);
    bool customEndElement(const char* psElementName);

    bool customAppendAttribute(const char8_t* psAttrName, const char8_t* psValue);
    bool customAppendAttribute(const char8_t* psAttrName, const char8_t* psValue, size_t valueLength);
    bool customWriteCharData(const char8_t* psCharData, size_t nCount);
    bool customWriteWhitespace(size_t nChars);
    bool customWriteIndent();
    bool customWriteText(const char8_t* psCharData, size_t nCount);
    bool customCloseCurrentElement();
    void customWriteNewline();
    void setSimpleElement();
    void customWriteCloseTag();
    void customWriteCloseTagForEmptyElement();
    // We don't need to check escaped string since this is integer
    bool customWriteIntegerData(const char8_t* psIntData, size_t nCount);

    EA::IO::IStream& getOutputStream() const { return *mpOutputStream; }
};

class EATDF_API XmlEncoder : public EA::TDF::TdfEncoder, public EA::TDF::TdfMemberVisitorConst
{
    EA_TDF_NON_ASSIGNABLE(XmlEncoder);
public:
    XmlEncoder(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) : mAllocator(alloc), mEncOptions(nullptr) {}
    ~ XmlEncoder() override {}
    
    bool encode(EA::IO::IStream& iStream, const EA::TDF::Tdf& tdf, const EncodeOptions* encOptions = nullptr, const MemberVisitOptions& options = MemberVisitOptions()) override;

protected:
    // convert member name to lower case and truncate "response" keyword
    const char8_t* getSanitizedMemberName(const char8_t* objectName, char8_t elementName[MAX_XML_ELEMENT_LENGTH]);
    void writeInteger(int64_t value);
    void writeInteger(uint64_t value);
    void writeFloat(float value);

    bool visitValue(const EA::TDF::TdfVisitContextConst& context) override;

    bool getMemNameForGeneric(const EA::TDF::TdfVisitContextConst &context, char8_t memberName[MAX_XML_ELEMENT_LENGTH]);

    bool postVisitValue(const EA::TDF::TdfVisitContextConst& context) override;

    bool findAncestorMemberName(char8_t* name, size_t size, const EA::TDF::TdfVisitContextConst &context);
    bool findAncestorType(TdfType &type, const EA::TDF::TdfVisitContextConst &context);

    const char* getNameFromMemInfo(const TdfMemberInfo* memberInfo );

    void printString(const char8_t *s);
    void printString(const char8_t *s, size_t len);
    inline void printChar(char8_t c)
    {
        mWriter.WriteCharData(&c, 1);
    }
    void writeCharData(const char8_t*  psCharData);

    TdfXmlWriter mWriter;
    EA::Allocator::ICoreAllocator& mAllocator;
    const EA::TDF::EncodeOptions* mEncOptions;
};

} //namespace TDF
} //namespace EA
#endif // EA_TDF_XMLENCODER_H
