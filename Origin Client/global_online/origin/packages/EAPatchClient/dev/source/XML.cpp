///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/XML.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/XML.h>
#include <EAPatchClient/Allocator.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Error.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/EAString.h>


namespace EA
{
namespace Patch
{

/////////////////////////////////////////////////////////////////////////////
// XML usage conventions
/////////////////////////////////////////////////////////////////////////////

// The following demonstrates how we represent structs in XML, by example.
//
// Example data struct:
//     struct Example
//     {
//         bool            mBool
//         int64_t         mInt64;
//         uint32_t        mUint32;
//         HashValueMD5    mHashMD5;
//         String          mString;
//         StringArray     mStringArray;
//         LocalizedString mLocalizedString;
//         FileFilterArray mFileFilterArray;
//         PatchEntryArray mPatchEntryArray;
//         StdC::DateTime  mDateTime;
//     };
//
// Associated XML:
//     The element names don't really need to be identical to the variable names. We just 
//     make them the same here so it's easier to follow.
//
//     <Example>
//         <Bool>0</Bool>
//
//         <Uint64>1234567812345678</Uint64>
//
//         <Int32>0xBAADCODE</Int32>
//
//         <HashMD5>8343a4bc3f248abb30902d1bc754a3e4</HashMD5>
//
//         <String>Example UTF8 string</String>
//
//         <StringArray>Str index 0</StringArray>
//         <StringArray>Str index 1</StringArray>
//         <StringArray>Str index 2</StringArray>
//
//         <LocalizedString L="en_us">Hello</LocalizedString>
//         <LocalizedString L="es_es">Hola</LocalizedString>
//
//         <FileFilterArray flags="CaseFold UnixPath">GameData/Plugins/*</FileFilterArray>
//         <FileFilterArray flags="CaseFold UnixPath">GameData/Plugins/*</FileFilterArray>
//
//         <PatchEntryArray>
//             <PatchEntry>
//                 <bFile>1</bFile>
//                 <bForceOverwrite>0</bForceOverwrite>
//                 <bFileUnchanged>0</bFileUnchanged>
//                 <FileURL>http://abc/def</FileURL>
//                 <RelativePath>Dir/Dir/File.txt</RelativePath>
//                 <AccessRights>Blah Blah Blah</AccessRights>
//                 <FinalHashValue>8343a4bc3f248abb30902d1bc754a3e4</FinalHashValue>
//                 <LocaleArray>en_us</LocaleArray>
//                 <LocaleArray>es_es</LocaleArray>
//                 <LocaleArray>gr_gr</LocaleArray>
//             </PatchEntry>
//             <PatchEntry>
//                 <bFile>1</bFile>
//                 <bForceOverwrite>0</bForceOverwrite>
//                 <bFileUnchanged>0</bFileUnchanged>
//                 <FileURL>http://abc/def</FileURL>
//                 <RelativePath>Dir/Dir/File.txt</RelativePath>
//                 <AccessRights>Blah Blah Blah</AccessRights>
//                 <FinalHashValue>8343a4bc3f248abb30902d1bc754a3e4</FinalHashValue>
//                 <LocaleArray>en_us</LocaleArray>
//                 <LocaleArray>es_es</LocaleArray>
//                 <LocaleArray>gr_gr</LocaleArray>
//             </PatchEntry>
//         </PatchEntryArray>
//     </Example>
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// XMLReader
/////////////////////////////////////////////////////////////////////////////

XMLReader::XMLReader()
  : mDomReader()
  , mDomDocument()
  , mNodeIteratorStack()
  , mpStream(NULL)
  , mbDocumentReadActive(false)
  , mScratchString()
{
}


XMLReader::~XMLReader()
{
    EAPATCH_ASSERT(!mbDocumentReadActive); // The user must match a BeginDocument with EndDocument, regardless of if there was an intervening error.
}


void XMLReader::HandleXmlError(EA::XML::XmlReader::ResultCode /*resultCode*/)
{
    // To do.
    EAPATCH_FAIL_MESSAGE("XMLReader::HandleXmlError");
    // Decide what to do, based on resultCode.
    // kEAPatchErrorFileXMLError
    // kEAPatchErrorFileReadFailure
    // kEAPatchErrorIdMemoryFailure
    // ErrorBase::HandleError(kECFatal, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pErrorContext = NULL);
}


bool XMLReader::BeginDocument(EA::IO::IStream* pStream)
{
    EAPATCH_ASSERT(!mbDocumentReadActive && mNodeIteratorStack.empty());
    mbDocumentReadActive = true;

    if(mbSuccess)
    {
        EAPATCH_ASSERT(EA::Patch::GetAllocator());

        mpStream = pStream;
        mDomReader.SetFlag(EA::XML::XmlReader::kIncludePrologue, true);
        mDomReader.SetAllocator(EA::Patch::GetAllocator());
        mDomReader.PushInputStream(pStream, EA::XML::kReadEncodingUTF8, NULL, -1); // To consider: set the third argument (stream URI).

        mbSuccess = mDomReader.Build(mDomDocument);
        EAPATCH_ASSERT(mbSuccess && (mDomReader.GetResultCode() == EA::XML::XmlReader::kSuccess));

        if(mbSuccess)
            mNodeIteratorStack.push_back(NodeIterator(&mDomDocument));
    }

    return mbSuccess;
}


bool XMLReader::EndDocument()
{
    EAPATCH_ASSERT(mbDocumentReadActive);
    mbDocumentReadActive = false;

    if(!mbSuccess)
    {
        // To do: This handling is not complete. We need to recognize that bFound was false during 
        // reading vs. an XML encoding error.
        // This could have failed due to a stream IO error, memory error, or an XML encoding error.
        // The UTFXml enum ResultCode defines the errors as UTFXml defines them.
        HandleXmlError(mDomReader.GetResultCode());
    }

    mNodeIteratorStack.clear();
    mDomDocument.Clear();
    mDomReader.Reset();

    return mbSuccess;
}


bool XMLReader::BeginParent(const char8_t* pName, bool& bFound)
{
    if(mbSuccess)
    {
        // The document node itself is the first node, which should always be present in the stack.
        EAPATCH_ASSERT(mbDocumentReadActive && !mNodeIteratorStack.empty());
 
        if(XMLReader::MoveToFirstChild(pName, bFound) && bFound)
        {
            const NodeIterator& nodeIterator = mNodeIteratorStack.back();
            mNodeIteratorStack.push_back(NodeIterator(*nodeIterator.mChildNode));
        }
    }

    return mbSuccess;
}


bool XMLReader::BeginNextParent(const char8_t* pName, bool& bFound)
{
    if(mbSuccess)
    {
        // The document node itself is the first node, which should always be present in the stack.
        EAPATCH_ASSERT(mbDocumentReadActive && !mNodeIteratorStack.empty());
 
        if(XMLReader::MoveToNextChild(pName, bFound) && bFound)
        {
            const NodeIterator& nodeIterator = mNodeIteratorStack.back();
            mNodeIteratorStack.push_back(NodeIterator(*nodeIterator.mChildNode));
        }
    }

    return mbSuccess;
}


bool XMLReader::EndParent(const char8_t* /*pName*/)
{
    if(mbSuccess)
    {
        // The document itself is the first element, which must always be present in the stack.
        EAPATCH_ASSERT(mbDocumentReadActive && (mNodeIteratorStack.size() > 1));
        // To do: Assert that pName matches the name used with BeginParent().

        mNodeIteratorStack.pop_back();
    }

    return mbSuccess;
}


bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, String& value)
{
    value.clear();

    if(mbSuccess)
    {
        if(MoveToFirstChild(pName, bFound) && bFound)
            value = GetCurrentDomNodeText();
    }

    return mbSuccess;
}



bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, bool& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);

        if(bFound)
        {
            // To consider: Make it an error if the following doesn't match.
            EAPATCH_ASSERT(mScratchString == "1"            || mScratchString == "0" || 
                           mScratchString == kXMLStringTrue || mScratchString == kXMLStringFalse);
        }

        value = (bFound && (mScratchString != "0") && (mScratchString != "false")); // To do: validate the string more rigorously.
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, int8_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I8d", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, uint8_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I8u", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, int16_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I16d", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, uint16_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I16u", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, int32_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I32d", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, uint32_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            EA::StdC::Sscanf(mScratchString.c_str(), "%I32u", &value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, int64_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            mbSuccess = EA::Patch::StringToInt64(mScratchString.c_str(), value, 0);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, uint64_t& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            mbSuccess = EA::Patch::StringToUint64(mScratchString.c_str(), value, 0);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, float& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);

        if(bFound)
        {
            double d;
            mbSuccess = EA::Patch::StringToDouble(mScratchString.c_str(), d);
            // To do: Error check this conversion to verify data isn't being lost.
            value = (float)d;
        }
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, double& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            mbSuccess = EA::Patch::StringToDouble(mScratchString.c_str(), value);
    }

    return mbSuccess;
}

bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, char8_t* pValue, size_t valueCapacity)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);
        if(bFound)
            mbSuccess = (EA::StdC::Strlcpy(pValue, mScratchString.c_str(), valueCapacity) < valueCapacity);
    }

    return mbSuccess;
}


// Example: 
//      <String>Hello world</String>
//      <String>Hello world</String>
//      <String>Hello world</String>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, StringArray& stringArray, size_t minRequiredCount)
{
    size_t foundCount = 0;

    if(mbSuccess)
    {
        // We are reading an array, and so we need to read multiple child elements.
        // So we do by doing MoveToFirstChild/MoveToNextChild iteration ourselves.
        // The mbSuccess return value doesn't indicate if the move to next child found 
        // the next child, it indicates if the XML object is in error. 

        for(mbSuccess = MoveToFirstChild(pName, bFound);
            mbSuccess && bFound;
            mbSuccess = MoveToNextChild(pName, bFound))
        {
            stringArray.push_back(GetCurrentDomNodeText());
            foundCount++;
        }
    }

    // Despite how many items we found above, the following defines how we return bFound to the caller:
    bFound = ((foundCount >= minRequiredCount) && mbSuccess);

    return mbSuccess;
}


// Example: 
//      <LocalizedString L="en_us">Hello</LocalizedString>
//      <LocalizedString L="es_es">Hola</LocalizedString>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, LocalizedString& value, size_t minRequiredCount)
{
    size_t foundCount = 0;

    value.Clear();

    if(mbSuccess)
    {
        // We are reading an array, and so we need to read multiple child elements.
        // So we do by doing MoveToFirstChild/MoveToNextChild iteration ourselves.
        // The mbSuccess return value doesn't indicate if the move to next child found 
        // the next child, it indicates if the XML object is in error. 
        String sLocaleName;
        String sLocalizedValue;

        for(mbSuccess = MoveToFirstChild(pName, bFound);
            mbSuccess && bFound;
            mbSuccess = MoveToNextChild(pName, bFound))
        {
            if(ReadAttribute(kXMLLocaleAttributeName, bFound, sLocaleName) && bFound && !sLocaleName.empty())
            {
                sLocalizedValue = GetCurrentDomNodeText();
                value.AddString(sLocaleName.c_str(), sLocalizedValue.c_str());
                foundCount++;
            }
            else
                mbSuccess = false;
        }
    }

    // Despite how many items we found above, the following defines how we return bFound to the caller:
    bFound = ((foundCount >= minRequiredCount) && mbSuccess);

    return mbSuccess;
}


// Example: 
//      <HashMD5>8343a4bc3f248abb30902d1bc754a3e4</HashMD5>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, HashValueMD5& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString) && (mScratchString.length() >= 32);

        if(bFound)
        {
            EA::StdC::Sscanf(mScratchString.c_str(), "%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x", 
                              &value.mData[0], &value.mData[1], &value.mData[ 2], &value.mData[ 3], &value.mData[ 4], &value.mData[ 5], &value.mData[ 6], &value.mData[ 7], 
                              &value.mData[8], &value.mData[9], &value.mData[10], &value.mData[11], &value.mData[12], &value.mData[13], &value.mData[14], &value.mData[15]);
        }
    }

    return mbSuccess;
}


// Example: 
//      <HashMD5Brief>4bc3f58343a4a3e4</HashMD5Brief>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, HashValueMD5Brief& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString) && (mScratchString.length() >= 16);

        if(bFound)
        {
            EA::StdC::Sscanf(mScratchString.c_str(), "%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x%02I8x", 
                              &value.mData[0], &value.mData[1], &value.mData[2], &value.mData[3],
                              &value.mData[4], &value.mData[5], &value.mData[6], &value.mData[7]);
        }
    }

    return mbSuccess;
}


// Example: 
//      <FileFilterArray flags="CaseFold UnixPath">GameData/Plugins/</FileFilterArray>
//      <FileFilterArray flags="CaseFold UnixPath">GameData/Extras/*</FileFilterArray>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, FileFilterArray& value, size_t minRequiredCount)
{
    size_t foundCount = 0;

    value.clear();

    if(mbSuccess)
    {
        // We are reading an array, and so we need to read multiple child elements.
        // So we do by doing MoveToFirstChild/MoveToNextChild iteration ourselves.
        // The mbSuccess return value doesn't indicate if the move to next child found 
        // the next child, it indicates if the XML object is in error. 
        FileFilter fileFilter;
        String     sFnMatchFlags;

        for(mbSuccess = MoveToFirstChild(pName, bFound);
            mbSuccess && bFound;
            mbSuccess = MoveToNextChild(pName, bFound))
        {
            if(ReadAttribute(kXMLFnMatchFlagsAttributeName, bFound, sFnMatchFlags))
            {
                // An empty filter spec is unmatchable, which is something the user probably doesn't
                // want to do. The only case is when you want to make all matches fail. But our system
                // has other ways of the user specifying this. To consider: Assert if mFilterSpec is 
                // empty, and/or possibly change an empty mFilterSpec to "*".
                fileFilter.mFilterSpec = GetCurrentDomNodeText();
                fileFilter.mMatchFlags = StringToFnMatchFlags(sFnMatchFlags.c_str());
                value.push_back(fileFilter);
                foundCount++;
            }
            else
                mbSuccess = false;
        }
    }

    // Despite how many items we found above, the following defines how we return bFound to the caller:
    bFound = ((foundCount >= minRequiredCount) && mbSuccess);

    return mbSuccess;
}


// Example: 
//      <DateTime>12345678</DateTime>
//      <DateTime>2012-06-15 23:59:59 GMT</DateTime>
bool XMLReader::ReadValue(const char8_t* pName, bool& bFound, EA::StdC::DateTime& value)
{
    if(mbSuccess)
    {
        mbSuccess = ReadValue(pName, bFound, mScratchString);

        if(bFound)
        {
            // Strip leading and trailing space from mScratchString.
            mScratchString.erase(0, mScratchString.find_first_not_of(" \t\n\r"));
            mScratchString.resize(mScratchString.find_last_not_of(" \t\n\r") + 1);

            // The string is in numerical time_t format or in string format.
            time_t t;

            if(mScratchString.find(" ") < mScratchString.length()) // If it looks like we have a string date...
                EA::Patch::DateStringToTimeT(mScratchString.c_str(), t);
            else
            {
                int64_t i64;
                EA::Patch::StringToInt64(mScratchString.c_str(), i64, 10);
                t = (time_t)i64;
            }

            // EA::StdC::DateTime uses seconds like time_t does, but it bases 
            // those seconds on a time earlier than 1970 and supports nanoseconds.
            // The real reason we use DateTime though is simply because it's portable,
            // while the C time functions in practice are not.
            value.SetSeconds(EA::StdC::TimeTSecondsSecondsToDateTime(t));
        }
    }

    return mbSuccess;
}


// Finds the next child iterator, or child list end.
// If bGetFirstChild == true, then currentChild is ignored and we look for the first such named child.
bool XMLReader::GetNextChild(EA::XML::DomNode* pParent, EA::XML::tNodeList::iterator currentChild, 
                              bool bGetFirstChild, const char8_t* pName, EA::XML::tNodeList::iterator& foundChild)
{
    if(mbSuccess)
    {
        if(bGetFirstChild)
            foundChild = pParent->mChildList.begin();
        else
        {
            foundChild = currentChild;

            if(foundChild != pParent->mChildList.begin())
                ++foundChild;
        }

        for(; (foundChild != pParent->mChildList.end()); ++foundChild)
        {
            if((*foundChild)->mName.comparei(pName) == 0) // If the name matches...
                break;
        }

        // At this point, foundChild is the named node or is list.end().
    }
    else
        foundChild = pParent->mChildList.end();

    return mbSuccess;
}


bool XMLReader::MoveToFirstChild(const char8_t* pName, bool& bFound)
{
    return MoveToChild(pName, bFound, true);
}


bool XMLReader::MoveToNextChild(const char8_t* pName, bool& bFound)
{
    return MoveToChild(pName, bFound, false);
}


bool XMLReader::MoveToChild(const char8_t* pName, bool& bFound, bool bMoveToFirstChild)
{
    bFound = false;

    if(mbSuccess)
    {
        NodeIterator& nodeIterator = mNodeIteratorStack.back();

        mbSuccess = GetNextChild(nodeIterator.mpParentNode, nodeIterator.mChildNode, 
                                 bMoveToFirstChild, pName, nodeIterator.mChildNode);

        bFound = (mbSuccess && (nodeIterator.mChildNode != nodeIterator.mpParentNode->mChildList.end()));

        // At this point nodeIterator.mChildNode refers to the named node or list.end.
    }

    return mbSuccess;
}


bool XMLReader::ReadAttribute(const char8_t* pAttributeName, bool& bFound, String& attributeValue)
{
    bFound = false;

    if(mbSuccess)
    {
        // We get the named attribute for the node at mNodeIteratorStack.back().mChildNode.
        EA::XML::DomNode* pNode = GetCurrentDomNode();
        EAPATCH_ASSERT(pNode && (pNode->mNodeType == EA::XML::XmlReader::Element));

        if(pNode->mNodeType == EA::XML::XmlReader::Element)
        {
            const EA::XML::DomElement* pElement = const_cast<EA::XML::DomNode*>(pNode)->AsDomElement();

            const char8_t* pAttributeValue = pElement->GetAttributeValue(pAttributeName); // Returns NULL if not present.

            if(pAttributeValue)
            {
                attributeValue = pAttributeValue;
                bFound = true;
            }
        }
    }
    else
        attributeValue.clear();

    return mbSuccess;
}

EA::XML::DomNode* XMLReader::GetCurrentDomNode()
{
    EAPATCH_ASSERT(mbDocumentReadActive && !mNodeIteratorStack.empty());
    NodeIterator& nodeIterator = mNodeIteratorStack.back();

    EA::XML::tNodeList::const_iterator it = nodeIterator.mChildNode;
    EAPATCH_ASSERT(it != nodeIterator.mpParentNode->mChildList.end());

    EA::XML::DomNode*  pNode = *it;

    return pNode;
}


String XMLReader::GetCurrentDomNodeName()
{
    // Get the name of the array; the name the user passed to MoveToFirstChild before calling 
    // this function. We need to get that name manually because we don't explicitly store it
    // upon the user calling MoveToFirstChild. To consider: Have such an mName member in NodeIterator.
    EA::XML::DomNode*        pNode = GetCurrentDomNode();
    const EA::XML::tString8& sNodeName = pNode->GetName();
    const String             sName(sNodeName.data(), sNodeName.length());  // sName here is the name the user passed to MoveToFirstChild.

    return sName;
}


String XMLReader::GetCurrentDomNodeText()
{
    // Get the name of the array; the name the user passed to MoveToFirstChild before calling 
    // this function. We need to get that name manually because we don't explicitly store it
    // upon the user calling MoveToFirstChild. To consider: Have such an mName member in NodeIterator.
    EA::XML::DomNode*        pNode = GetCurrentDomNode();
    EAPATCH_ASSERT(pNode && (pNode->mNodeType == EA::XML::XmlReader::Element)); // To consider: Do we really need to require that it be an Element node?

    EA::XML::tString8 sInnerText; // UTFXml should probably make GetValue be const.
    const_cast<EA::XML::DomNode*>(pNode)->GetInnerText(sInnerText);

    String sValue(sInnerText.data(), sInnerText.length());

    return sValue;
}




/////////////////////////////////////////////////////////////////////////////
// XMLWriter
/////////////////////////////////////////////////////////////////////////////

XMLWriter::XMLWriter()
  : mXmlWriter(true, EA::XML::kWriteEncodingUTF8)
  , mpStream(NULL)
  , mbDocumentWriteActive(false)
{
}

XMLWriter::~XMLWriter()
{
    EAPATCH_ASSERT(!mbDocumentWriteActive); // The user must match a BeginDocument with EndDocument, regardless of if there was an intervening error.
}

void XMLWriter::HandleXmlError(EA::XML::XmlReader::ResultCode /*resultCode*/)
{
    // To do.
    EAPATCH_FAIL_MESSAGE("XMLReader::HandleXmlError");
    // Decide what to do, based on resultCode.
    // kEAPatchErrorFileXMLError
    // kEAPatchErrorFileReadFailure
    // kEAPatchErrorIdMemoryFailure
    // ErrorBase::HandleError(kECFatal, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pErrorContext = NULL);
}

bool XMLWriter::BeginDocument(EA::IO::IStream* pStream)
{
    EAPATCH_ASSERT(!mbDocumentWriteActive);
    mbDocumentWriteActive = true;

    if(mbSuccess)
    {
        mXmlWriter.SetIndentSpaces(2);
        mXmlWriter.SetLineEnd(EA::XML::kLineEndUnix);
        mXmlWriter.SetOutputStream(pStream, true);  // To consider: Enable user control of pretty-printing.

        mpStream  = pStream;
        mbSuccess = mXmlWriter.WriteXmlHeader();
    }

    return mbSuccess;
}

bool XMLWriter::EndDocument()
{
    EAPATCH_ASSERT(mbDocumentWriteActive);
    mbDocumentWriteActive = false;

    // Nothing to do, as XML doesn't have any kind of file-ending sequence.
    // Do we get the error state from mXmlWriter here?

    if(!mbSuccess)
    {
        // To do.
        // This could have failed due to a stream IO error, memory error, or an XML encoding error.
        // The UTFXml enum ResultCode defines the errors as UTFXml defines them.
        // HandleXmlError(mXmlWriter.______());
    }

    return mbSuccess;
}

// Example: 
//     <SomeClass>
bool XMLWriter::BeginParent(const char8_t* pName)
{
    if(mbSuccess)
        mbSuccess = mXmlWriter.BeginElement(pName);
    return mbSuccess;
}

// Example: 
//     </SomeClass>
bool XMLWriter::EndParent(const char8_t* pName)
{
    if(mbSuccess)
        mbSuccess = mXmlWriter.EndElement(pName);
    return mbSuccess;
}

// Example: 
//     <Bool>0</Bool>
bool XMLWriter::WriteValue(const char8_t* pName, bool value)
{
    const String sValue(String::CtorSprintf(), "%s", value ? kXMLStringTrue : kXMLStringFalse);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Int8>123</Int8>
bool XMLWriter::WriteValue(const char8_t* pName, int8_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%x" : "%d", (int)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Uint8>123</Uint8>
bool XMLWriter::WriteValue(const char8_t* pName, uint8_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%x" : "%u", (unsigned)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Int16>1234</Int16>
bool XMLWriter::WriteValue(const char8_t* pName, int16_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%x" : "%d", (int)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Uint16>12345678</Uint16>
bool XMLWriter::WriteValue(const char8_t* pName, uint16_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%x" : "%u", (unsigned)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Int32>12345678</Int32>
bool XMLWriter::WriteValue(const char8_t* pName, int32_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "%x" : "%d", (int)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Uint32>12345678</Uint32>
bool XMLWriter::WriteValue(const char8_t* pName, uint32_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%x" : "%u", (unsigned)value);
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//     <Int64>1234567812345678</Int64>
bool XMLWriter::WriteValue(const char8_t* pName, int64_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%I64x" : "%I64d", value); // This assumes we are using EAStdC::Sprintf underneath.
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//      <Uint64>1234567812345678</Uint64>
bool XMLWriter::WriteValue(const char8_t* pName, uint64_t value, bool bHex)
{
    const String sValue(String::CtorSprintf(), bHex ? "0x%I64x" : "%I64u", value); // This assumes we are using EAStdC::Sprintf underneath.
    return WriteValue(pName, sValue.c_str(), sValue.length());
}

// Example: 
//      <Float>123.456</Float>
bool XMLWriter::WriteValue(const char8_t* pName, float value)
{   
    return WriteValue(pName, (double)value);
}

// Example: 
//      <Float>123.456</Float>
bool XMLWriter::WriteValue(const char8_t* pName, double value)
{
    char8_t buffer[64];
    EA::StdC::Sprintf(buffer, "%f", value);
    size_t length = EA::StdC::ReduceFloatString(buffer); // Remove superfluous characters from string.

    return WriteValue(pName, buffer, length);
}


// Example: 
//      <String>Hello world</String>
bool XMLWriter::WriteValue(const char8_t* pName, const char8_t* pValue, size_t valueLength)
{
    if(valueLength == (size_t)-1)
        valueLength = EA::StdC::Strlen(pValue);

    if(mbSuccess)
        mbSuccess = mXmlWriter.BeginElement(pName);
    if(mbSuccess)
        mbSuccess = mXmlWriter.WriteCharData(pValue, valueLength);
    if(mbSuccess)
        mbSuccess = mXmlWriter.EndElement(pName);

    return mbSuccess;
}


// Example: 
//      <String>Hello world</String>
bool XMLWriter::WriteValue(const char8_t* pName, const String& value)
{
    return WriteValue(pName, value.c_str(), value.length());
}


// Example: 
//      <String>Hello world</String>
//      <String>Hello world</String>
//      <String>Hello world</String>
bool XMLWriter::WriteValue(const char8_t* pName, const StringArray& value)
{
    String sTemp;

    for(eastl_size_t i = 0, iEnd = value.size(); i != iEnd; i++)
    {
        sTemp.sprintf("%s", pName);
        WriteValue(sTemp.c_str(), value[i].c_str(), value[i].length());
    }

    return mbSuccess;
}


// Example: 
//      <LocalizedString L="en_us">Hello</LocalizedString>
//      <LocalizedString L="es_es">Hola</LocalizedString>
bool XMLWriter::WriteValue(const char8_t* pName, const LocalizedString& value)
{
    if(mbSuccess)
    {
        const LocalizedString::LocaleStringMap& lsMap = value.GetLocaleStringMap();

        for(LocalizedString::LocaleStringMap::const_iterator it = lsMap.begin(); it != lsMap.end(); ++it)
        {
            const StringSmall& sLocale = it->first;
            const String&      sValue  = it->second;

            if(mbSuccess)
                mbSuccess = mXmlWriter.BeginElement(pName);
            if(mbSuccess)
                mbSuccess = mXmlWriter.AppendAttribute(kXMLLocaleAttributeName, sLocale.c_str());
            if(mbSuccess)
                mbSuccess = mXmlWriter.WriteCharData(sValue.c_str(), sValue.length());
            if(mbSuccess)
                mbSuccess = mXmlWriter.EndElement(pName);
        }
    }

    return mbSuccess;
}


// Example: 
//      <HashMD5>8343a4bc3f248abb30902d1bc754a3e4</HashMD5>
bool XMLWriter::WriteValue(const char8_t* pName, const HashValueMD5& value)
{
    EA_COMPILETIME_ASSERT(EAArrayCount(value.mData) == 16);
    char8_t buffer[36];

    EA::StdC::Sprintf(buffer, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                               value.mData[0], value.mData[1], value.mData[ 2], value.mData[ 3], value.mData[ 4], value.mData[ 5], value.mData[ 6], value.mData[ 7], 
                               value.mData[8], value.mData[9], value.mData[10], value.mData[11], value.mData[12], value.mData[13], value.mData[14], value.mData[15]);
    EAPATCH_ASSERT(EA::StdC::Strlen(buffer) == 32);

    return WriteValue(pName, buffer, 32);
}


// Example: 
//      <HashMD5Brief>4bc3f58343a4a3e4</HashMD5Brief>
bool XMLWriter::WriteValue(const char8_t* pName, const HashValueMD5Brief& value)
{
    EA_COMPILETIME_ASSERT(EAArrayCount(value.mData) == 8);
    char8_t buffer[20];

    EA::StdC::Sprintf(buffer, "%02x%02x%02x%02x%02x%02x%02x%02x", 
                               value.mData[0], value.mData[1], value.mData[ 2], value.mData[ 3], value.mData[ 4], value.mData[ 5], value.mData[ 6], value.mData[ 7]);
    EAPATCH_ASSERT(EA::StdC::Strlen(buffer) == 16);

    return WriteValue(pName, buffer, 16);
}


// Example: 
//      <FileFilterArray flags="CaseFold UnixPath">GameData/Plugins/</FileFilterArray>
//      <FileFilterArray flags="CaseFold UnixPath">GameData/Extras/*</FileFilterArray>
bool XMLWriter::WriteValue(const char8_t* pName, const FileFilterArray& value)
{
    if(mbSuccess)
    {
        for(eastl_size_t i = 0, iEnd = value.size(); i != iEnd; i++)
        {
            const String&  sFilterSpec = value[i].mFilterSpec;
            const uint32_t matchFlags  = value[i].mMatchFlags;

            if(mbSuccess)
                mbSuccess = mXmlWriter.BeginElement(pName);
            if(mbSuccess)
            {
                const String sMatchFlags = FnMatchFlagsToString(matchFlags);
                mbSuccess = mXmlWriter.AppendAttribute(kXMLFnMatchFlagsAttributeName, sMatchFlags.c_str());
            }
            if(mbSuccess)
                mbSuccess = mXmlWriter.WriteCharData(sFilterSpec.c_str(), sFilterSpec.length());
            if(mbSuccess)
                mbSuccess = mXmlWriter.EndElement(pName);
        }
    }

    return mbSuccess;
}


// Example: 
//      <DateTime>12345678</DateTime>
//      <DateTime>2012-06-15 23:59:59 GMT</DateTime>
bool XMLWriter::WriteValue(const char8_t* pName, const EA::StdC::DateTime& value, bool bStringForm)
{
    String sValue;

    if(bStringForm)
    {
        char8_t buffer[kMinDateStringCapacity];
        int64_t time64 = EA::StdC::DateTimeSecondsToTimeTSeconds(value.GetSeconds());
        time_t  time   = (time_t)time64; // Some platforms still have a 32 bit time_t, which will be a problem for huge dates.
        TimeTToDateString(time, buffer);
        sValue = buffer;
    }
    else
        sValue.sprintf("%I64u", value.GetSeconds());

    return WriteValue(pName, sValue.c_str(), sValue.length());
}




} // namespace Patch
} // namespace EA








