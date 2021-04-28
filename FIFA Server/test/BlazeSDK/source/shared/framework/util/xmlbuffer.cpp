/*************************************************************************************************/
/*!
    \file

    This code was derived from the RenderWare XML parser and writer classes.


    \attention
        (c) Electronic Arts. All Rights Reserved.
        (c) 2004-2005 Criterion Software Limited and its licensors. All Rights Reserved
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class XmlBuffer

    This class provides rudimentary generation and parsing of XML data.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include <string.h>
#include "framework/util/shared/xmlbuffer.h"
#include "EAStdC/EACType.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const char gWhiteSpace[] = { ' ', '\t', 10, 13, 0 };
static const char gAlphaNumeric[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.,%[]:\\-\'&+;";
static const char gAssignment = '=';
static const char gTagOpen = '<';
static const char gTagClose = '>';
static const char gQuot = '"';
static const char gApos = '\'';
static const char gComment = '!';
static const char gHyphen = '-';
static const char gSlash = '/';
static const char gCdataOpen[] = "![CDATA[";
static const char gCdataClose[] = "]]>";
static const char gProcessingInstruction = '?';

/*  Conversion between characters that XML treats specially.

    This class performs lookups between the special character and
    the string that replaces it.  Some characters need to be escaped
    with strings because they are treated as control characters by an
    XML interpreter.  Here is the list of characters and the string
    that replaces them:

    > ... &gt\n
    < ... &lt\n
    & ... &amp\n
    ' ... &apos\n
    " ... &quot\n
*/
class XmlEntity
{
public:
    enum
    {
        ENTITY_COUNT = 5        // number of pre-defined entities
    };

    struct Map
    {
        char ch;                // character
        const char *str;        // corresponding string
        unsigned int strsize;   // length of corresponding string
    };

    /*  \brief Find the corresponding string for this character.
        \param c The character to look up
        \return A nullptr-terminated string is returned, if the
        character does need to be treated speically.  If it doesn't,
        nullptr is returned.
    */
    static const char *find(char c);

    /*  \brief Find the corresponding charcter for this string.
        \param str The string to search for.
        \param len The length of the substring is returned via
        this parameter
        \return If the string matches a special string, the
        character to replace it is returned.  If not, 0 is returned
    */
    static char find(const char *str, size_t &len);

private:
    static const Map mMap[ENTITY_COUNT];
};

static const char8_t sMappedChars[] = "><&\'\"";

const XmlEntity::Map XmlEntity::mMap[XmlEntity::ENTITY_COUNT] =
{
    {'>', "gt;", 3},
    {'<', "lt;", 3},
    {'&', "amp;", 4},
    {'\'', "apos;", 5},
    {'"', "quot;", 5}
};

const char * XmlEntity::find(char c)
{
    for(unsigned int i=0; i < XmlEntity::ENTITY_COUNT; ++i)
    {
        if(c == mMap[i].ch)
        {
            return mMap[i].str;
        }
    }

    // wasn't a special character
    return nullptr;
}

char XmlEntity::find(const char *str, size_t &len)
{
    for(size_t i=0; i<XmlEntity::ENTITY_COUNT; ++i)
    {
        if(strncmp(mMap[i].str, str, mMap[i].strsize) == 0)
        {
            len = mMap[i].strsize;
            return mMap[i].ch;
        }
    }

    // this wasn't a special entity string
    len = 0;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// State parser, this is an internal class, not part of the public API
//

// Internal XML Parser class.
class StateParser
{
public:
    // States the XML parser can be in.
    enum State
    {
        BEGIN,      // very start of the document
        ELEMENT,    // expecting an element
        END         // end of document
    };

    // Type of element currently being parsed.
    enum ElementType
    {
        OPEN,       // open element i.e. @<element@>
        CLOSE       // close element i.e. @</element@>
    };

    // advance XML data pointer past whitespace.
    inline void skipWhite()
    {
        while(mIsWhiteSpace[static_cast<unsigned char>(*mData)] && (*mData != '\0'))
            mData++;
    }

    // find a close tag ('>')
    bool findCloseTag()
    {
        while((*mData != gTagClose) && (*mData != '\0'))
            mData++;

        if(*mData == '\0')
            return false;

        mData++;
        return true;
    }

    // advance XML data pointer past whitespace and XML comments (<!-- ... -->)
    //
    // @return @c true: successfully skipped WS and no op, @c false: found unclosed no op tag
    bool skipWhiteAndNoOpTags()
    {
        bool skip = true;
        while(skip)
        {
            skipWhite();

            // ignore certain tags
            if(*mData == gTagOpen)
            {
                // look for a comment open tag '<!--'
                if((*(mData+1) == gComment) && (*(mData+2) == gHyphen) && (*(mData+3) == gHyphen))
                {
                    // look for the close tag
                    bool search = true;
                    while(search)
                    {
                        // find close tag '-->'
                        if(findCloseTag())
                        {
                            if((*(mData-2)==gHyphen) && (*(mData-3)==gHyphen))
                                search = false;
                        }
                        else
                            return false;
                    }
                }
                //  look for a PI open tag '<?'
                else if (*(mData+1) == gProcessingInstruction)
                {
                    // look for the close tag
                    bool search = true;
                    while(search)
                    {
                        // find close tag '?>'
                        if(findCloseTag())
                        {
                            if(*(mData-2)==gProcessingInstruction)
                                search = false;
                        }
                        else
                            return false;
                    }
                }
                else
                    skip = false;
            }
            else
                skip = false;
        }
        return true;
    }

    
    // @brief Push an element onto the element stack.
    //
    // Push an element onto the element stack, this is used to match
    // close elements with open elements.  If this is the very first 
    // tag, generate StartDocument() event.
    //
    // @param element element to push on stack.
    // @return @c true: element pushed on stack, @c false: stack full.
    bool pushElement(const XmlElement *element)
    {
        if(mStackTop == XmlBuffer::MAX_ELEMENTS || element == nullptr)
            return false;

        mStack[mStackTop].setName(element->name);   // Also sets the length
        ++mStackTop;
        if(mStackTop == 1)
            mCallback->startDocument();

        return true;
    }

    // @brief Pop an element from the stack.
    //
    // @return the element at the top of the stack or @c nullptr if the stack is empty.
    const XmlElement *popElement()
    {
        if(mStackTop > 0)
            return &mStack[--mStackTop];
        else
            return nullptr;
    }

    // @brief Add an XML attribute to the attribute list.
    //
    // @return @c true: attribute added, @c false: attribute list full.
    bool addAttribute(const XmlAttribute *attrib)
    {
        if(mAttribCount == XmlBuffer::MAX_ATTRIBUTES)
            return false;

        mAttributes[mAttribCount++] = *attrib;
        return true;
    }

    // @brief StateParser constructor.
    //
    // @param data pointer to XML data.
    // @param callback pointer to XmlCallback instance.
    StateParser(const char *data, XmlCallback *callback) :
        mData(data),
        mState(BEGIN),
        mCallback(callback),
        mStackTop(0),
        mAttribCount(0)
    {
        memset(mStack, 0, sizeof(mStack));
        memset(mAttributes, 0, sizeof(mAttributes));

        memset(mIsWhiteSpace, 0, sizeof(mIsWhiteSpace));
        memset(mIsAlphaNumeric, 0, sizeof(mIsAlphaNumeric));

        for (int i = 0; gWhiteSpace[i] != 0; ++i)
        {
            mIsWhiteSpace[static_cast<unsigned char>(gWhiteSpace[i])] = true;
        }
        mIsWhiteSpace[0] = true;

        for (int i = 0; gAlphaNumeric[i] != 0; ++i)
        {
            mIsAlphaNumeric[static_cast<unsigned char>(gAlphaNumeric[i])] = true;
        }
        mIsAlphaNumeric[0] = true;
    }

    // @brief Begin state parser.
    //
    // This is the very beginning of the document, deal with possible <?xml tag.
    //
    // @return @c true: parse successful, @c false: parse failed.
    bool parseBegin()
    {
        // do we have a start tag?
        if(strncmp(mData, "<?xml", 5) == 0)
        {
            // look for the '?>' close tag
            bool end = false;
            while(!end)
            {
                if(!findCloseTag())
                    return false;
                if(*(mData-2) == '?')
                    end = true;
            }
        }
        mState = ELEMENT;
        return true;
    }

    // @brief check for a CDATA section.
    //
    // If a CDATA section is found, it is parsed and the Cdata callback
    // is called.
    // @return @c true: CDATA section found and parsed, @c false: no CDATA section found.
    bool checkCdata()
    {
        bool ret = false;

        // check for a CDATA section
        if((*mData == gCdataOpen[0]) &&
            (strncmp(mData, gCdataOpen, sizeof(gCdataOpen)-1) == 0))
        {
            mData += (sizeof(gCdataOpen)-1);
            const char *data = mData;

            const size_t CdataEndLength = strlen(gCdataClose);
            size_t mDataRemainingLength = strlen(mData);

            while(mDataRemainingLength >= CdataEndLength)
            {
                if (strncmp(mData,gCdataClose, sizeof(gCdataClose)-1) == 0)
                {
                    size_t len = static_cast<size_t>(mData-data);
                    mCallback->cdata(data, len);
                    mData += (sizeof(gCdataClose)-1);
                    break;
                }
                ++mData;
                --mDataRemainingLength;
            }
            ret = true;
        }
        return ret;
    }

    bool parseAttributes()
    {
        bool ret = false;
        bool parse = true;
        XmlAttribute attrib;
        while(parse)
        {
            // assume the worst
            parse = false;

            // get the attribute name
            attrib.name = mData;
            while((*mData != '\0') && (mIsAlphaNumeric[static_cast<unsigned char>(*mData)]))
                mData++;
            attrib.nameLen = static_cast<unsigned int>(mData-attrib.name);
            // name found, now find '='
            skipWhite();
            if(*mData == gAssignment)
            {
                mData++;
                skipWhite();
                // need quote/apos next
                if((*mData == gQuot) || (*mData == gApos))
                {
                    // search for corresponding closing quote/apos
                    char quot = *mData;

                    attrib.value = ++mData;
                    // find end quote for value
                    while((*mData != quot) && (*mData != '\0'))
                        mData++;

                    if(*mData == quot)
                    {
                        attrib.valueLen = static_cast<unsigned int>(mData - attrib.value);
                        mData++;

                        if(addAttribute(&attrib))
                        {
                            skipWhite();

                            if((*mData == gTagClose) || (*mData == gSlash))
                                parse = false;
                            else // no '>' follows attribute, so another attribute then
                                parse = true;

                            ret = true;
                        }
                    }
                }
            }
        }
        return ret;
    }

    bool handleCloseTag(ElementType type, const XmlElement &element)
    {
        bool ret = true;
        mData++;
        if(type == OPEN)
        {
            // this is an open element, push it on the stack and generate a StartElement event
            if(pushElement(&element))
                mCallback->startElement(element.name, element.len, mAttributes, mAttribCount);
            else
                ret = false;
        }
        else
        {
            // this is a close element, pop the stack and see if it matches.
            const XmlElement *oldElement = popElement();
            if( (oldElement == nullptr) ||
                (strncmp(element.name, oldElement->name, element.len) != 0))
            {
                ret = false;
            }
            else
            {
                mCallback->endElement(element.name, element.len);
                // if the stack is empty, the document is finished.
                if(mStackTop == 0)
                {
                    mCallback->endDocument();
                    mState = END;
                }
            }
        }
        return ret;
    }

    bool handlePCData()
    {
        bool ret = true;

        // character data lasts until a < or > character
        const char *data = mData;
        while((*mData != '\0') && (*mData != gTagOpen) && (*mData != gTagClose))
            mData++;

        if(*mData == gTagClose)
            ret = false;
        else
        {
            unsigned int len = static_cast<unsigned int>(mData-data);
            mCallback->characters(data, len);
        }
        return ret;
    }

    // @brief Element state parser.
    //
    // This is the main parsing function. It will parse one XML element
    // plus any attributes it may have. When it doesn't find an element,
    // it will generate a character data event.
    //
    // @return @c true: parse successful, @c false: parse failed.
    bool parseElement()
    {
        bool ret = true;

        if (skipWhiteAndNoOpTags() == false)
            return false;

        // we now either have a open tag or character data
        if(*mData == gTagOpen)
        {
            // tag open found ('<'), we have an element.
            ElementType type = OPEN;
            XmlElement element;

            mData++;

            if(!checkCdata())
            {
                // if the element starts with a slash (i.e. </element>) it is a close element
                if(*mData == gSlash)
                {
                    type = CLOSE;
                    mData++;
                }

                element.setName(mData);

                // search past the element, first whitespace or nonalpha character
                const char8_t* dataStart = mData;
                bool search = true;
                while((*mData != '\0') && (search))
                {
                    if((mIsWhiteSpace[static_cast<unsigned char>(*mData)]) || !(mIsAlphaNumeric[static_cast<unsigned char>(*mData)]))
                    {
                        search = false;
                    }
                    else
                    {
                        mData++;
                    }
                }
                element.setLen(static_cast<size_t>(mData-dataStart));

                skipWhite();

                // end of tag name, see what is next
                mAttribCount = 0;
                // past element name, if there is alnum, attributes follow
                if(mIsAlphaNumeric[static_cast<unsigned char>(*mData)])
                {
                    ret = false;
                    if(type == OPEN)
                    {
                        ret = parseAttributes();
                    }
                }

                // '/>' : tag end, so this is a element of type '<element/>'
                if((*mData == gSlash) && (*(mData+1) == gTagClose))
                {
                    mData+=2;
                    if(type == OPEN)
                    {
                        mCallback->startElement(element.name, element.len, mAttributes, mAttribCount);
                        mCallback->endElement(element.name, element.len);
                    }
                    else
                        ret = false;
                }
                else if(*mData == gTagClose)
                {
                    ret = handleCloseTag(type, element);
                }
                else
                    ret = false;
            }
        }
        else
        {
            ret = handlePCData();
        }
        return ret;
    }

    // State machine
    //
    // Based on the state, call the right function.
    // A true OO implementation would have objects for each state and
    // call virtual functions, etc. But for the sake of simplicity of implementation
    // it uses a good old fashioned switch statement :-)
    //
    // @return @c true: parse successful, @c false: parse failed.
    bool parseState()
    {
        switch(mState)
        {
        case BEGIN:
            return parseBegin();
        case ELEMENT:
            return parseElement();
        case END:
        default:
            return false;
        }
    }

    // @brief Parse the XML data.
    //
    // @return @c true: parse successful, @c false: parse failed.
    bool parse()
    {
        bool ret = true;
        while(ret && (*mData != '\0') && (mState != END))
            ret = parseState();
        return ret;
    }

private:
    const char *mData;                                   // XML data pointer.
    State mState;                                        // current state.
    XmlCallback *mCallback;                              // XmlCallback instance.
    unsigned int mStackTop;                              // Element stack index.
    XmlElement mStack[XmlBuffer::MAX_ELEMENTS];          // Element stack.
    size_t mAttribCount;                                 // Number of attributes in attribute list.
    XmlAttribute mAttributes[XmlBuffer::MAX_ATTRIBUTES]; // Attribute list.
    bool mIsWhiteSpace[256];
    bool mIsAlphaNumeric[256];
};

/*************************************************************************************************/
/*!
     \brief XmlBuffer::XmlBuffer

      Construct a new XmlBuffer instance.

     \param[in] buf - Buffer to use for input/output
     \param[in] len - Length of provided buffer
     \param[in] indent - Number of spaces to use for indenting

*/
/*************************************************************************************************/
XmlBuffer::XmlBuffer(uint8_t* buf, size_t len, uint32_t indent, const bool outputEmptyElements /*= true*/, bool sanitizeUTF8 /* = false */)
:   mOwnBuffer(true),
    mDocStarted(false),
    mOutputEmptyElements(outputEmptyElements),
    mStackTop(0),
    mIndent(indent),
    mNewLine(false),
    mCloseTag(false),
    mSanitizeUTF8(sanitizeUTF8)
{
    mRawBuffer = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "XmlRawBuffer") RawBuffer(buf, len);
}

/*************************************************************************************************/
/*!
     \brief XmlBuffer::XmlBuffer

      Construct a new XmlBuffer instance.

     \param[in] initialCapacity - Size to allocate buffer to
     \param[in] indent - Number of spaces to use for indenting

*/
/*************************************************************************************************/
XmlBuffer::XmlBuffer(size_t initialCapacity, uint32_t indent, const bool outputEmptyElements /*= true*/, bool sanitizeUTF8 /* = false */)
:   mOwnBuffer(true),
    mDocStarted(false),
    mOutputEmptyElements(outputEmptyElements),
    mStackTop(0),
    mIndent(indent),
    mNewLine(false),
    mCloseTag(false),
    mSanitizeUTF8(sanitizeUTF8)
{
    mRawBuffer = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "XmlRawBuffer") RawBuffer(initialCapacity);
}

/*************************************************************************************************/
/*!
     \brief XmlBuffer::XmlBuffer

      Construct a new XmlBuffer instance.

     \param[in] rawBuffer - Raw buffer to use as base storage
     \param[in] indent - Number of spaces to use for indenting

*/
/*************************************************************************************************/
XmlBuffer::XmlBuffer(RawBuffer* rawBuffer, uint32_t indent, const bool outputEmptyElements /*= true*/, bool sanitizeUTF8 /* = false */)
:  mSanitizeUTF8(sanitizeUTF8)

{
    initialize(rawBuffer, indent, outputEmptyElements);
}

XmlBuffer::XmlBuffer(bool sanitizeUTF8 /* = false */)
: mSanitizeUTF8(sanitizeUTF8)
{
    initialize(nullptr, 0, false);
}

XmlBuffer::~XmlBuffer()
{
    if (mOwnBuffer) 
    {
        BLAZE_DELETE_MGID(DEFAULT_BLAZE_MEMGROUP, mRawBuffer);
    }

}
void XmlBuffer::initialize(RawBuffer* rawBuffer, uint32_t indent, const bool outputEmptyElements)
{
    mOwnBuffer = false;
    mDocStarted = false;
    mOutputEmptyElements = outputEmptyElements;
    mStackTop = 0;
    mIndent = indent;
    mNewLine = false;
    mCloseTag = false;
    mRawBuffer = rawBuffer;
}

/**
    \brief Start an XML document.

    This function starts an XML document, no other functions can be called
    before this one. It will write out the XML header.

    \param[in] encoding - Optional value for the encoding attribute.  If not specified, no encoding attribute will be set.
*/
void XmlBuffer::putStartDocument(const char8_t* encoding /*= nullptr*/)
{
    if (encoding == nullptr)
    {
        // No encoding
        printString("<?xml version=\"1.0\"?>");
    }
    else
    {
        char8_t str[128];
        blaze_snzprintf(str, sizeof(str), "<?xml version=\"1.0\" encoding=\"%s\"?>", encoding);
        printString(str);
    }

    mDocStarted = true;
    mNewLine = true;
    mCloseTag = false;
}

/**
  \brief End an XML document.

  This function ends an XML document. No other functions other than StartDocument
  can be called after this one.
*/
void XmlBuffer::putEndDocument()
{
    mDocStarted = false;

    // write out the remainder of the buffer
    printChar('\n');
}

/**
  \brief Start an XML element.
  \param name name of the element, it has to be null-terminated.
  \param attributes attribute list for this element. (can be nullptr)
  \param attributeCount number of attributes for this element.
*/
void XmlBuffer::putStartElement(const char *name, bool writeIfEmpty)
{
    putStartElement(name, nullptr, 0, writeIfEmpty);
}

/**
  \brief Start an XML element.
  \param name name of the element.
  \param attributes attribute list for this element. (can be nullptr)
  \param attributeCount number of attributes for this element.
*/
void XmlBuffer::putStartElement(const char *name, const XmlAttribute *attributes, size_t attributeCount, bool writeIfEmpty)
{
    // close the tag if necessary
    closeTag();

    pushElement(name, attributeCount, writeIfEmpty);

    indent(mStackTop-1);
    printChar('<');
    printString(name);

    for(size_t i=0; i<attributeCount; ++i)
    {
        // print element values enclosed in double quotes
        // putCharacters will take care of any special characters
        printChar(' ');
        putCharacters(attributes[i].name);
        printChar('=');
        printChar('"');
        putCharacters(attributes[i].value);
        printChar('"');
    }

    mCloseTag = true;
    mNewLine = true;
}
/**
  \brief End an XML element.
  \param name name of the element to end.  If specified it has to be null-terminated.  
              If nullptr, the name used to open the element will be used to close it.
*/
void XmlBuffer::putEndElement(const char *name /*= nullptr*/)
{
    const XmlElement* element = popElement();
    if (element == nullptr)
    {
        return;
    }

    if (mCloseTag)
    {
        // no characters or other elements were written in between.
        if (mOutputEmptyElements || element->numAttribs > 0 || element->writeIfEmpty)
        {
            // empty or attribute-only elements are okay, so we close this as a single line tag
            printString("/>");
        }
        else
        {
            // remove the empty element from the document
            removeEmptyElement();
        }

        mCloseTag=false;
    }
    else
    {
        indent(mStackTop);
        printString("</");
        if (name == nullptr)
        {
            // Get the name from the element stack
            printString(element->name);
        }
        else
        {
            EA_ASSERT(strcmp(name, element->name) == 0);
            printString(name);
        }
        printChar('>');
    }

    mNewLine = true;
}

/**
  \brief Write XML characters (PCDATA).

  This method can for efficiency purposes simply write out the data verbatim.

  If requested (by setting the checkSpecialChars arg to true), this method will
  replace invalid control characters with a question mark, and perform the necessary
  escaping for ASCII characters that require it (e.g. \& is replaced by \&amp;).

  In addition, if both the checkSpecialChars setting is true and the mSanitizeUTF8
  member variable is set, bytes with the high-bit set will be validated to ensure they
  are valid UTF-8 sequences, each non-conforming byte will be replaced by a '?'.
  
    UTF8 encoding
    00-7F   0-127       US-ASCII (single byte)
    80-BF   128-191     Second, third, or fourth byte of a multi-byte sequence
    C0-C1   192-193     invalid: Overlong encoding: start of a 2-byte sequence, but code point <= 127
    C2-DF   194-223     Start of 2-byte sequence
    E0-EF   224-239     Start of 3-byte sequence
    F0-F4   240-244     Start of 4-byte sequence (F4 is partially restricted for codepoints above 10FFFF)
    F5-F7   245-247     Restricted by RFC 3629: start of 4-byte sequence for codepoint above 10FFFF
    F8-FB   248-251     Restricted by RFC 3629: start of 5-byte sequence
    FC-FD   252-253     Restricted by RFC 3629: start of 6-byte sequence
    FE-FF   254-255     Invalid: not defined by original UTF-8 specification

  \param characters        characters to write.
  \param checkSpecialChars whether to check for (and replace) control characters, perform escape sequences, and
                           (optionally, depending on the sanitize member variable) validate UTF-8 correctness
*/
void XmlBuffer::putCharacters(const char *chars, bool checkSpecialChars /* = true */)
{
    // close the tag if necessary
    closeTag();

    mNewLine = false; // no newline after characters

    // If we're not doing any form of validation, we trust the caller knows what they are doing
    // and they had better be providing a clean string that is valid verbatim as XML
    // (for example printing the string representation of an int) - specifically note that
    // the mSanitizeUTF8 member is not considered at all, if the caller for efficiency is
    // claiming that the input does not require inspection for control characters and/or
    // chars requiring escaping, then we equally assume they are most likely not providing
    // any bytes with the high bit set, or do not require validation of such bytes if they are
    if ( !checkSpecialChars )
    {
        printString(chars);
        return;
    }

    while (*chars != '\0')
    {
        // For efficiency, print runs of chars that don't require special handling all at once,
        // scan through and find batches that can be printed verbatim
        const char8_t* pch = chars;
        while ((*pch >= 0x20) && (*pch < 0x7f) && (strchr(sMappedChars, *pch) == nullptr))
        {
            ++pch;
        }

        if (pch != chars)
        {
            printString(chars, pch-chars);
            chars = pch;
            if (*chars == '\0')
                return;
        }

        // At this point we've got a char that requires special handling, either control characters, Ascii chars that need escaping,
        // or UTF-8 chars to validate
        uint8_t ch = static_cast<uint8_t>(*chars);

        // Escape the Ascii chars that require XML escaping...
        const char *s = XmlEntity::find(ch);
        if (s != nullptr)
        {
            // was a special character
            printChar('&');
            printString(s);
            ++chars;
            continue;
        }

        // The following whitespace characters should be relatively rare, so rather than filtering them above,
        // we'll handle them here, anything other than these at this point is a control character we don't support
        if ((ch & 0x80) == 0)
        {
            if (ch == '\n' || ch == '\r' || ch == '\t')
                printChar(ch);
            else
                printChar('?');
            ++chars;
            continue;
        }

        // If we're not doing UTF8 validation, just print the high-bit char now
        if (!mSanitizeUTF8)
        {
            printChar(ch);
            ++chars;
            continue;
        }

        // Now we get into the meat of UTF-8 validation, check the validity of the first char
        if (ch < 194 || ch > 244)
        {
            printChar('?');
            ++chars;
            continue;
        }

        int32_t count = 0;
        if (ch > 193 && ch < 224)
            count = 2;
        else if (ch > 223 && ch < 240)
            count = 3;
        else if (ch > 239 && ch < 245)
            count = 4;

        // And check the validity of the remaining chars...
        bool valid = true;
        for (int32_t j = 1; j < count; ++j)
        {
            uint8_t ch2 = static_cast<uint8_t>(chars[j]);
            if (ch2 < 128 || ch2 > 191)
            {
                valid = false;
                break;
            }

            // Additional special case, for four byte UTF-8 sequences only some values are valid when the first byte is 244,
            if (ch == 244 && j == 1 && ch2 > 143)
            {
                valid = false;
                break;
            }
        }

        // If we've found a fully valid UTF-8 representation of a char, print all the bytes now
        if (valid)
        {
            printString(chars, count);
            chars += count;
        }
        else
        {
            // Otherwise replace just the first char with a question mark, we'll try next loop around to see
            // if the subsequent chars form a valid UTF-8 char
            printChar('?');
            ++chars;
        }
    } // for
}
       

/**
  \brief Write characters without translation.

  \param characters characters to write. This has to be null-terminated.
*/
void XmlBuffer::putCharactersRaw(const char *chars)
{
    putCharactersRaw(chars, strlen(chars));
}
/**
  \brief Write characters without translation.

  \param characters characters to write. (optionally null-terminated)
  \param charLen number of characters to write.
*/
void XmlBuffer::putCharactersRaw(const char *chars, size_t charLen)
{
    closeTag();

    printString(chars, charLen);
}

/**
  \brief Write CDATA
  \param data data to write, it has to be null-terminated
*/
void XmlBuffer::putCdata(const char *data)
{
    putCdata(data, strlen(data));
}

/**
  \brief Write CDATA

  \param data data to write.
  \param dataLen number of characters to write.
*/
void XmlBuffer::putCdata(const char *data, size_t dataLen)
{
    closeTag();
    printString("<![CDATA[");
    printString(data, dataLen);
    printString("]]>");
    mNewLine = false;
}

/**
  \brief Write an integer entity value

  \param value Integer value to write.
*/
void XmlBuffer::putInteger(int32_t value)
{
    char num[16];
    blaze_snzprintf(num, sizeof(num), "%d", value);
    printString(num);
}

/**
  \brief Returns the currently open element name, if any

  Returns the name of the innermost element currently open.

  \return Ptr to nullptr terminated string containing current element name, or nullptr if no elements are open
*/
const char8_t* XmlBuffer::getCurrentElementName() const
{
    if (mStackTop == 0)
    {
        return nullptr;
    }

    return mElementStack[mStackTop-1].name;
}

//
// Private functions
//

/**
  \brief Push an element onto the element stack.

  Push an element onto the element stack, this is used to match
  close elements with open elements.

  \param element element to push on stack.
  \return \e  true: element pushed on stack, \e  false: stack full.
*/
bool XmlBuffer::pushElement(const char *name, size_t numAttribs, bool writeIfEmpty)
{
    if(mStackTop == XmlBuffer::MAX_ELEMENTS)
        return false;

    mElementStack[mStackTop].setName(name);
    mElementStack[mStackTop].numAttribs = numAttribs;
    mElementStack[mStackTop].setWriteIfEmpty(writeIfEmpty);

    mStackTop++;

    return true;
}

/**
  \brief Pop an element from the stack.

  \return the element at the top of the stack or \e  nullptr if the stack is empty.
*/
const XmlElement *XmlBuffer::popElement()
{
    if ( mStackTop > 0 )
    {
        --mStackTop;
        const XmlElement* elem = &(mElementStack[mStackTop]);
        return elem;
    }

    return nullptr;
}

/**
  \brief Write a null-terminated string to the output buffer.
  \param s string to write.
*/
void XmlBuffer::printString(const char *s)
{
    printString(s, strlen(s));
}

/**
  \brief Write a string to the output buffer.
  \param s string to write. (optionally null-terminated)
  \param len length of string to write.
*/
void XmlBuffer::printString(const char *s, size_t len)
{
    char8_t* t = (char8_t*)mRawBuffer->acquire(len+1);
    if (t != nullptr)
    {
        if (s != nullptr)
            memcpy(t, s, len);
        t[len] = '\0';
        mRawBuffer->put(len);
    }
}

/**
  \brief Write indentation to the output buffer.
  \param count depth of indentation to write.
*/
void XmlBuffer::indent(uint32_t count)
{
    if(mNewLine)
    {
        size_t len = (size_t)(mIndent*count);
        uint8_t* t = mRawBuffer->acquire(len + 2); // account for '\n' and '\0'
        if (t != nullptr)
        {
            *t++ = '\n';
            memset(t, ' ', len);
            t[len] = '\0';

            mRawBuffer->put(len + 1);
        }
    }
}

/**
  \brief Closes the element start tag.
*/
void XmlBuffer::closeTag()
{
    if (mCloseTag)
    {
        printChar('>');
        mCloseTag=false;
    }
}

/**
  \brief Removes an empty element from the buffer.
*/
void XmlBuffer::removeEmptyElement()
{
    if (mRawBuffer == nullptr)
    {
        return;
    }

    // Work our way back from the tail to find the last \n, or to the front
    // of the buffer if this is the first element;
    const uint8_t* head = mRawBuffer->head();
    const uint8_t* tail = mRawBuffer->tail();
    if (head == nullptr || tail == nullptr)
        return;

    size_t numCharsToTrim = 0;
    while (tail > head && *tail != '\n')
    {
        --tail;
        ++numCharsToTrim;
    }

    if (*tail == '\n')
    {
        mNewLine = true;
    }

    // Update the raw buffer
    mRawBuffer->trim(numCharsToTrim);
}

/**
  \brief Set the values of this attribute.
  \param nameToSet name of the attribute.
  \param valueToSet value of the attribute.
*/
void XmlAttribute::set(const char *nameToSet, const char *valueToSet)
{
    name = nameToSet;
    nameLen = strlen(nameToSet);
    value = valueToSet;
    valueLen = strlen(valueToSet);
}

/**
  \brief Parse the XML data.
  \param data Pointer to XML data to parse.
  \return \c true: parse successful, \c false: parse failed.
*/
bool XmlReader::parse(XmlCallback* callback) const
{
    StateParser state(mData, callback);
    return state.parse();
}

/**
  \brief Copy XML character data.

  Copy data from source to destination, translating any standard entities.
  (&quot, &apos, &amp, &lt, &gt). Since all pointers provided by the parser
  to the callback functions point directly to the XML
  data, the client will need to copy these strings out of the XML data,
  translating any & commands at that point. This function can be used to
  do the copying, it will null-terminate the destination string as well.

  \param dest pointer to destination buffer.
  \param source pointer to source buffer.
  \param destLen length of destination buffer.
  \param sourceLen length of source buffer.
  \return the amount of characters written to the destination buffer.
*/
size_t XmlBuffer::copyChars(char *dest, const char *source, size_t destLen, size_t sourceLen)
{
    size_t written = 0;
    while((sourceLen-- != 0) && (written < (destLen-1)))
    {
        char c = *source++;

        // the XML standard states that all end-of-lines become a 0x0a
        if(c == 0x0d)
        {
            if(*source == 0x0a)
            {
                source++;
                sourceLen--;
            }
            c = 0x0a;
        }
        else if(c == '&')
        {
            size_t len;
            c = XmlEntity::find(source, len);
            if (c == 0)
            {
                c = '&';
            }
            else
            {
                sourceLen -= len;
                source += len;
            }    
        }
        *dest++ = c;
        written++;
    }
    *dest = '\0';
    return written;
}

} // namespace Blaze

