/*************************************************************************************************/
/*!
    \file accountxmlparser.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OutboundHttpXmlParser

    Is a very generic parser that calls a account result to set "Attribute" or "Values" using a
    "full name".  The full name will basically be an Xpath representation of the xml tag.

    e.g.

    ---
    <root>
        <user id="12345">
           <code>ABCDE</code>
           <name>John Doe</name>
        </user>
    </root>
    ---

    "/root/user/code" has a value of "ABCDE".

    \notes
        It's a simple parser so that classes that need to consume this XML doesn't need to
        implement all the functions or to keep track of the state of the parser.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/protocol/outboundhttpxmlparser.h"
#include "framework/protocol/outboundhttpresult.h"

#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief <MethodName>

    <Description of the method>

    \param[in]  <varname> - <Description of variable>
    \param[out] <varname> - <Description of variable>

    \return - <Description of return value - Delete this line if returning void>

    \notes
        <Any additional detail including references.  Delete this section if
        there are no notes.>
*/
/*************************************************************************************************/
OutboundHttpXmlParser::OutboundHttpXmlParser(OutboundHttpResult& result) :
        mHasErrorResult(false), mHasXmlError(false), mResult(result)

{
    mFullPath[0] = 0;
    mPathEnd = mFullPath;
    mPathSizeLeft = FULLPATH_SIZE - 1; // to account for string termination.

} // constructor OutboundHttpXmlParser


void OutboundHttpXmlParser::startElement(const char* name, size_t nameLen,
    const XmlAttribute* attributes, size_t attributeCount)
{
    // update full path.

    // If there is enough space left in the fullpath.
    if (mPathSizeLeft > (nameLen + 1))
    {
        mResult.startElement(name, nameLen);

        *mPathEnd++ = '/';
        mPathSizeLeft--;
        size_t charsAdded = blaze_strsubzcpy(mPathEnd, mPathSizeLeft, name, nameLen);

        // Advance mPathEnd pointer.
        mPathEnd += charsAdded;
        mPathSizeLeft -= charsAdded;

        // Call result's setAttributes.
        if (!mResult.checkSetError(mFullPath, attributes, attributeCount))
            mResult.setAttributes(mFullPath, attributes, attributeCount);
    }
    else
    {
        eastl::string tagName(name, nameLen);
        BLAZE_FATAL_LOG(Log::HTTP, "[OutboundHttpXmlParser] Cannot append tagName (" << tagName.c_str() << ") to FullPath at startElement.");

        // Should I crash?
    } // if
    // set result attributes.
} // startElement

void OutboundHttpXmlParser::endElement(const char* name, size_t nameLen)
{
    // shorten full path
    char8_t* checkPath = mPathEnd - (nameLen + 1);

    // Check to make sure it has not gone past the beginning of the buffer.
    if (mFullPath <= checkPath)
    {
        mPathEnd = checkPath;
        *mPathEnd = 0;
        mPathSizeLeft += nameLen + 1;
        mResult.endElement(name, nameLen);
    }
    else
    {
        eastl::string tagName(name, nameLen);
        BLAZE_FATAL_LOG(Log::HTTP, "[OutboundHttpXmlParser] Removing endElement (" << tagName.c_str() 
                        << ") pushes FullPath buffer before beginning!");

        // Should I crash?
    } // if

} // endElement

void OutboundHttpXmlParser::characters(const char* characters, size_t charLen)
{
    // Call result's setValue;
    if (!mResult.checkSetError(mFullPath, characters, charLen))
    {
        char8_t* buf  = BLAZE_NEW_ARRAY(char8_t, charLen + 1);
        memset(buf, 0, charLen + 1);
        XmlBuffer::copyChars(buf, characters, charLen + 1, charLen);
        mResult.setValue(mFullPath, buf, strlen(buf));
        delete[] buf;
    }
} // characters

void OutboundHttpXmlParser::cdata(const char* data, size_t dataLen)
{
    // Call result's setValue;
    if (!mResult.checkSetError(mFullPath, data, dataLen))
        mResult.setValue(mFullPath, data, dataLen);
} // cdata


/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

