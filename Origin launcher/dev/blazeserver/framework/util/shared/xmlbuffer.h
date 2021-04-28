/*************************************************************************************************/
/*!
    \file

    This code was derived from the RenderWare XML parser and writer classes.


    \attention
        (c) Electronic Arts. All Rights Reserved.
        (c) 2004-2005 Criterion Software Limited and its licensors. All Rights Reserved
*/
/*************************************************************************************************/

#ifndef BLAZE_XMLBUFFER_H
#define BLAZE_XMLBUFFER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/outputstream.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

// XML element
struct BLAZESDK_API XmlElement
{
    XmlElement() : len(0), numAttribs(0) { name[0] = '\0'; }
    XmlElement(const char *_name, size_t _len, size_t attribs = 0) : len(_len), numAttribs(attribs)
    {
        if ( _name != nullptr )
        {
            blaze_strnzcpy(name, _name, sizeof(name)); // Force nullptr termination
        }
    }

    void setName(const char8_t* newName)
    {
        len = 0;
        name[0] = '\0';
        if ( newName != nullptr )
        {
            blaze_strnzcpy(name, newName, sizeof(name)); // Ensure nullptr terminated
            len = strlen(name);
        }
    }

    void setLen(const size_t newLen)
    {
        if (newLen < sizeof(name))
        {
            len = newLen;
            name[len] = '\0';   // nullptr terminate for good measure.
        }
    }

    void setWriteIfEmpty(bool newWriteIfEmpty) { writeIfEmpty = newWriteIfEmpty; }

    char8_t name[128];  // copy of the element name
    size_t len;         // length of element name
    size_t numAttribs;  // Number of attributes associated with the element
    bool writeIfEmpty;
};

/// \brief XML attribute containing a name and a value.
struct BLAZESDK_API XmlAttribute
{
    const char * name;      ///< name of attribute (not null terminated).
    size_t nameLen;         ///< length of attribute name.
    const char * value;     ///< value of attribute (not null terminated).
    size_t valueLen;        ///< length of attribute value

    /// \brief Assign a value to an attribute
    void set(const char *name, const char *value);
};

/**
  \brief Callback interface used by the XML buffer.

  This interface is used by the parser portion of the XML buffer to call back client code when the
  XML is parsed. The client is responsible for providing an implementation.
*/
class BLAZESDK_API XmlCallback
{
public:

    virtual ~XmlCallback()
    {
    }

    /**
      \brief Start Document event.

      This function gets called when the first XML element is encountered. Any whitespace,
      comments and the <?xml tag at the start of the document are skipped before this
      function is called.
    */
    virtual void startDocument() = 0;

    /**
      \brief End Document event.

      This function gets called when a close element matching the first open element
      is encountered. This marks the end of the XML document. Any data following this
      close element is ignored by the parser.
    */
    virtual void endDocument()   = 0;


    /**
      \brief Start Element event.

      This function gets called when an open element is encountered. (\<element\>)

      \param name name of the element. (Not null-terminated)
      \param nameLen length of the name of the element.
      \param attributes list of attributes for this element (can be nullptr if attributeCount = 0)
      \param attributeCount number of attributes.
      \note The name pointers of the element and any attributes passed to this function
      will point directly to the actual XML data. This is why the strings are not
      null-terminated and the length is provided instead.
    */
    virtual void startElement(const char *name, size_t nameLen,
        const XmlAttribute *attributes, size_t attributeCount) = 0;


    /**
      \brief End Element event.

      This function gets called when a close element is encountered. (\</element\>)

      \param name name of the element. (Not null-terminated)
      \param nameLen length of the name of the element.
      \note The name pointer passed to this function will point directly to the
      name of the element in the actual XML data. This is why the string is not
      null-terminated and the length is provided instead.
    */
    virtual void endElement(const char *name, size_t nameLen) = 0;

    /**
      \brief Characters event.

      This function gets called when XML characters are encountered.

      \param characters pointer to character data.
      \param charLen length of character data.
      \note The character pointer passed to this function will point directly to the
      character data in the actual XML data. This is why the data is not
      null-terminated and the length is provided instead.
    */
    virtual void characters(const char *characters, size_t charLen) = 0;

    /**
      \brief CDATA event.

      This function gets called when a CDATA section is encountered.

      \param data pointer to the data.
      \param dataLen length of the data.
    */
    virtual void cdata(const char *data, size_t dataLen) = 0;

};

class XmlReader
{
public:
    XmlReader(const char8_t* data) { mData = data; }
    XmlReader(const RawBuffer& buf) { mData = reinterpret_cast<const char8_t*>(buf.data()); }
    bool parse(XmlCallback* callback) const;

private:
    const char8_t* mData;
};

class BLAZESDK_API XmlBuffer : public OutputStream
{
public:
    enum
    {
        MAX_ATTRIBUTES = 64,    // maximum number of attributes per element
        MAX_ELEMENTS = 64       // maximum number of nested elements
    };
    XmlBuffer(bool sanitizeUTF8 = false);
    XmlBuffer(uint8_t* buf, size_t len, uint32_t indent, const bool outputEmptyElements = true, bool sanitizeUTF8 = false);
    XmlBuffer(size_t initialCapacity, uint32_t indent, const bool outputEmptyElements = true, bool sanitizeUTF8 = false);
    XmlBuffer(RawBuffer* rawBuffer, uint32_t indent, const bool outputEmptyElements = true, bool sanitizeUTF8 = false);
    void initialize(RawBuffer* rawBuffer, uint32_t indent, const bool outputEmptyElements);
    ~XmlBuffer() override;

    void putStartDocument(const char8_t* encoding = nullptr);
    void putEndDocument();

    void putStartElement(const char *name, bool writeIfEmpty = false);
    void putStartElement(const char *name, const XmlAttribute *attributes, size_t attributeCount, bool writeIfEmpty = false);

    void putEndElement(const char *name = nullptr);

    void putCharacters(const char *chars, bool checkSpecialChars = true);

    void putCharactersRaw(const char *chars, size_t charLen);
    void putCharactersRaw(const char *chars);

    void putCdata(const char *data, size_t dataLen);
    void putCdata(const char *data);

    void putInteger(int32_t value);

    const char8_t* getCurrentElementName() const;
    uint32_t getOpenElementCount() const { return mStackTop; }

    // OutputStream interface
    uint32_t write(uint8_t data) override
    {
        putCharactersRaw((const char8_t*)&data, 1);
        return 1;
    }

    uint32_t write(uint8_t* data, uint32_t length) override
    {
        putCharactersRaw((const char8_t*)data, length);
        return length;
    }

    uint32_t write(uint8_t* data, uint32_t offset, uint32_t length) override
    {
        putCharactersRaw((const char8_t*)&data[offset], length);
        return length;
    }

    static size_t copyChars(char *dest, const char *source, size_t destLen, size_t sourceLen);

    void setOutputEmptyElements( bool outputEmptyElements ) { mOutputEmptyElements = outputEmptyElements; } 
    bool getOutputEmptyElements() const { return mOutputEmptyElements; }

private:
    void printBuffer();
    inline void printChar(char c);
    void printString(const char *s);
    void printString(const char *s, size_t len);
    void indent(uint32_t count);
    void closeTag();
    bool pushElement(const char *name, size_t numAttribs, bool writeIfEmpty);
    const XmlElement *popElement();
    void removeEmptyElement();

    RawBuffer *mRawBuffer;          // buffer to receive output data
    bool mOwnBuffer;                // whether this object owns the raw buffer
    bool mDocStarted;               // document started or not
    bool mOutputEmptyElements;      // true if empty elements are output, false if they are suppressed

    XmlElement mElementStack[XmlBuffer::MAX_ELEMENTS];
    uint32_t mStackTop;             // element stack index

    uint32_t mIndent;               // indentation value
    bool mNewLine;                  // output a newline as next character
    bool mCloseTag;                 // output the closing tag next

    bool mSanitizeUTF8;             // check if characters are valid UTF8 symbols and replace with '?' otherwise

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor 
    XmlBuffer(const XmlBuffer& otherObj);

    // Assignment operator
    XmlBuffer& operator=(const XmlBuffer& otherObj);
};

/**
  \brief Write one character to the output buffer.
  \param c character to write.
*/
inline void XmlBuffer::printChar(char c)
{
    uint8_t* t = mRawBuffer->acquire(2);
    if (t != nullptr)
    {
        t[0] = c;
        t[1] = '\0';

        mRawBuffer->put(1);
    }
}

} // namespace Blaze

#endif // BLAZE_XMLBUFFER_H

