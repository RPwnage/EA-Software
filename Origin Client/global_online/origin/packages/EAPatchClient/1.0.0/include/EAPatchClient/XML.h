///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/XML.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_XML_H
#define EAPATCHCLIENT_XML_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/Locale.h>
#include <EAStdC/EADateTime.h>
#include <UTFXml/XmlDomReader.h>
#include <UTFXml/XmlWriter.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace IO
    {
        class IStream;
    }

    namespace Patch
    {
        // XML string constants
        #define kXMLStringTrue                "true"   // <SomeBool>true</SomeBool>
        #define kXMLStringFalse               "false"  // <SomeBool>false</SomeBool>
        #define kXMLLocaleAttributeName       "L"      // <LocalizedString L="en_us">Hello</LocalizedString>
        #define kXMLFnMatchFlagsAttributeName "flags"  // <mFileFilterArray flags="CaseFold UnixPath">GameData/Extras/*</mFileFilterArray>


        /// XMLReader
        /// 
        /// Note: The user must call EndDocument if BeginDocument is called, for the purposes of 
        /// examining the error state. Then the user must call GetError (from the parent ErrorBase)
        /// in order to read that error state. This is enforced as a way to ensure errors are handled.
        ///
        /// Example usage:
        ///     struct MyStruct
        ///     {
        ///         bool            mBool
        ///         int64_t         mInt64;
        ///         StringArray     mStringArray;
        ///         LocalizedString mLocalizedString;
        ///     };
        ///
        ///     <MyStruct>
        ///         <mBool>0</mBool>
        ///         <mInt64>1234567812345678</mInt64>
        ///         <mStringArray>Str index 0</mStringArray>
        ///         <mStringArray>Str index 1</mStringArray>
        ///         <mStringArray>Str index 2</mStringArray>
        ///         <mLocalizedString L="en_us">Hello</mLocalizedString>
        ///         <mLocalizedString L="es_es">Hola</mLocalizedString>
        ///     </MyStruct>
        ///
        ///     XMLReader reader;
        ///     MyStruct  myStruct;
        ///     bool      bFound;
        ///
        ///     // The XML fields don't need to have the same names as the member variables.
        ///     // If a field is not found then the reader is set to be in error, and HasError
        ///     // will return true. You don't need to check bFound in order to catch errors.
        ///
        ///     reader.BeginDocument(&someFile);
        ///       reader.BeginParent("MyStruct", bFound);
        ///         reader.ReadValue("mBool",            bFound, myStruct.mBool);
        ///         reader.ReadValue("mInt64",           bFound, myStruct.mInt64);
        ///         reader.ReadValue("mStringArray",     bFound, myStruct.mStringArray);
        ///         reader.ReadValue("mLocalizedString", bFound, myStruct.mLocalizedString);
        ///       reader.EndParent("MyStruct");
        ///     reader.EndDocument();
        ///     
        ///     if(reader.HasError())
        ///     {
        ///         Error error = reader.GetError();
        ///         (do something with the error).
        ///     }
        ///
        class XMLReader : public ErrorBase
        {
        public:
            XMLReader();
           ~XMLReader();

            bool BeginDocument(EA::IO::IStream* pStream);
            bool EndDocument();

            bool BeginParent(const char8_t* pName, bool& bFound);
            bool BeginNextParent(const char8_t* pName, bool& bFound);
            bool EndParent(const char8_t* pName);

            // Generic read functions for common types.
            bool ReadValue(const char8_t* pName, bool& bFound, bool& value);
            bool ReadValue(const char8_t* pName, bool& bFound, int8_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, uint8_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, int16_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, uint16_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, int32_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, uint32_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, int64_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, uint64_t& value);
            bool ReadValue(const char8_t* pName, bool& bFound, float& value);
            bool ReadValue(const char8_t* pName, bool& bFound, double& value);
            bool ReadValue(const char8_t* pName, bool& bFound, char8_t* pValue, size_t valueCapacity);
            bool ReadValue(const char8_t* pName, bool& bFound, String& value);
            bool ReadValue(const char8_t* pName, bool& bFound, StringArray& value, size_t minRequiredCount);
            bool ReadValue(const char8_t* pName, bool& bFound, LocalizedString& value, size_t minRequiredCount);
            bool ReadValue(const char8_t* pName, bool& bFound, HashValueMD5& value);
            bool ReadValue(const char8_t* pName, bool& bFound, HashValueMD5Brief& value);
            bool ReadValue(const char8_t* pName, bool& bFound, FileFilterArray& value, size_t minRequiredCount);
            bool ReadValue(const char8_t* pName, bool& bFound, EA::StdC::DateTime& value);

            // Lower level API for iterating nodes.
            bool MoveToFirstChild(const char8_t* pName, bool& bFound);
            bool MoveToNextChild(const char8_t* pName, bool& bFound);

            // Reads the attribute of the child node set by MoveToFirstChild/MoveToNextChild.
            // bFound is set to true if the attribute exists, even if the attribute value is empty.
            bool ReadAttribute(const char8_t* pAttributeName, bool& bFound, String& attributeValue);

        protected:
            struct NodeIterator
            {
                EA::XML::DomNode*            mpParentNode;
                EA::XML::tNodeList::iterator mChildNode;

                NodeIterator(EA::XML::DomNode* pParentNode) : mpParentNode(pParentNode), mChildNode(mpParentNode->mChildList.begin()) {}
            };

            typedef eastl::fixed_vector<NodeIterator, 16, true, CoreAllocator> NodeIteratorStack;

        protected:
            void              HandleXmlError(EA::XML::XmlReader::ResultCode resultCode);
            EA::XML::DomNode* GetCurrentDomNode();
            String            GetCurrentDomNodeName();
            String            GetCurrentDomNodeText();
            bool              MoveToChild(const char8_t* pName, bool& bFound, bool bMoveToFirstChild);
            bool              GetNextChild(EA::XML::DomNode* pParent, EA::XML::tNodeList::iterator currentChild, 
                                           bool bGetFirstChild, const char8_t* pName, EA::XML::tNodeList::iterator& foundChild);
        protected:
            EA::XML::XmlDomReader mDomReader;
            EA::XML::DomDocument  mDomDocument;
            NodeIteratorStack     mNodeIteratorStack;       // Used for traversing the depths of the DOM tree.
            EA::IO::IStream*      mpStream;
            bool                  mbDocumentReadActive;     // Used for error checking.
            mutable String        mScratchString;           // Used for temporary reads and writes.
        };


        /// XMLWriter
        ///
        /// Note: The user must call EndDocument if BeginDocument is called, for the purposes of 
        /// examining the error state. Then the user must call GetError (from the parent ErrorBase)
        /// in order to read that error state. This is enforced as a way to ensure errors are handled.
        ///
        class XMLWriter : public ErrorBase
        {
        public:
            XMLWriter();
           ~XMLWriter();

            bool BeginDocument(EA::IO::IStream* pStream);
            bool EndDocument();

            bool BeginParent(const char8_t* pName);
            bool EndParent(const char8_t* pName);

            bool WriteValue(const char8_t* pName, bool value);
            bool WriteValue(const char8_t* pName, int8_t value, bool bHex);
            bool WriteValue(const char8_t* pName, uint8_t value, bool bHex);
            bool WriteValue(const char8_t* pName, int16_t value, bool bHex);
            bool WriteValue(const char8_t* pName, uint16_t value, bool bHex);
            bool WriteValue(const char8_t* pName, int32_t value, bool bHex);
            bool WriteValue(const char8_t* pName, uint32_t value, bool bHex);
            bool WriteValue(const char8_t* pName, int64_t value, bool bHex);
            bool WriteValue(const char8_t* pName, uint64_t value, bool bHex);
            bool WriteValue(const char8_t* pName, float value);
            bool WriteValue(const char8_t* pName, double value);
            bool WriteValue(const char8_t* pName, const char8_t* pValue, size_t valueLength = (size_t)-1);
            bool WriteValue(const char8_t* pName, const String& value);
            bool WriteValue(const char8_t* pName, const StringArray& value);
            bool WriteValue(const char8_t* pName, const LocalizedString& value);
            bool WriteValue(const char8_t* pName, const HashValueMD5& value);
            bool WriteValue(const char8_t* pName, const HashValueMD5Brief& value);
            bool WriteValue(const char8_t* pName, const FileFilterArray& value);
            bool WriteValue(const char8_t* pName, const EA::StdC::DateTime& value, bool bStringForm);

        protected:
            void HandleXmlError(EA::XML::XmlReader::ResultCode resultCode);

        protected:
            EA::XML::XmlWriter mXmlWriter;
            EA::IO::IStream*   mpStream;
            bool               mbDocumentWriteActive;    // Used for error checking.
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




