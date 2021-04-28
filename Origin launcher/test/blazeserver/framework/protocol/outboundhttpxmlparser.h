/*************************************************************************************************/
/*!
    \file   OutboundHttpXmlParser.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDHTTPXMLPARSER_H
#define BLAZE_OUTBOUNDHTTPXMLPARSER_H

/*** Include files *******************************************************************************/

#include "framework/util/xmlbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class OutboundHttpResult;

class OutboundHttpXmlParser : public XmlCallback
{
public:

    OutboundHttpXmlParser(OutboundHttpResult& result);
    ~OutboundHttpXmlParser() override {};

    /**
      \brief Start Document event.

      This function gets called when the first XML element is encountered. Any whitespace,
      comments and the <?xml tag at the start of the document are skipped before this
      function is called.
    */
    void startDocument() override {};

    /**
      \brief End Document event.

      This function gets called when a close element matching the first open element
      is encountered. This marks the end of the XML document. Any data following this
      close element is ignored by the parser.
    */
    void endDocument() override {};


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
    void startElement(const char* name, size_t nameLen,
        const XmlAttribute* attributes, size_t attributeCount) override;


    /**
      \brief End Element event.

      This function gets called when a close element is encountered. (\</element\>)

      \param name name of the element. (Not null-terminated)
      \param nameLen length of the name of the element.
      \note The name pointer passed to this function will point directly to the
      name of the element in the actual XML data. This is why the string is not
      null-terminated and the length is provided instead.
    */
    void endElement(const char* name, size_t nameLen) override;

    /**
      \brief Characters event.

      This function gets called when XML characters are encountered.

      \param characters pointer to character data.
      \param charLen length of character data.
      \note The character pointer passed to this function will point directly to the
      character data in the actual XML data. This is why the data is not
      null-terminated and the length is provided instead.
    */
    void characters(const char* characters, size_t charLen) override;

    /**
      \brief CDATA event.

      This function gets called when a CDATA section is encountered.

      \param data pointer to the data.
      \param dataLen length of the data.
    */
    void cdata(const char* data, size_t dataLen) override;

    OutboundHttpResult& getResult() { return mResult; };

    const bool isError() { return mHasErrorResult; };
    const bool isXmlError() { return mHasXmlError; };

protected:

private:

    static const uint16_t FULLPATH_SIZE = 2048;

    bool mHasErrorResult;
    bool mHasXmlError;
    OutboundHttpResult& mResult;

    // Parsers are generally not going to do multithreaded parsing.  Okay to have an internal buffer.
    char8_t mFullPath[FULLPATH_SIZE];   
    char8_t* mPathEnd;
    size_t mPathSizeLeft;

    NON_COPYABLE(OutboundHttpXmlParser);

};

} // Blaze

#endif // BLAZE_OUTBOUNDHTTPXMLPARSER_H
