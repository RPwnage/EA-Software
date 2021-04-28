/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Filename

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/eventxmlencoder.h"
#include "framework/util/shared/rawbufferistream.h"

namespace Blaze
{

EventXmlEncoder::EventEncoder::EventEncoder()
    : mIsEvent(false),
      mIsSessionNotification(false),
      mDefaultDiff(false),
      mAssociatedSession(nullptr)
{
    memset(mEventAttrBufs, 0, sizeof(mEventAttrBufs));

    mEventAttrs[ATTR_COMPONENT].name = "component";
    mEventAttrs[ATTR_COMPONENT].nameLen = strlen(mEventAttrs[ATTR_COMPONENT].name);
    mEventAttrs[ATTR_COMPONENT].value = mEventAttrBufs[ATTR_COMPONENT];
    mEventAttrs[ATTR_TYPE].name = "type";
    mEventAttrs[ATTR_TYPE].nameLen = strlen(mEventAttrs[ATTR_TYPE].name);
    mEventAttrs[ATTR_TYPE].value = mEventAttrBufs[ATTR_TYPE];
    mEventAttrs[ATTR_DATE].name = "date";
    mEventAttrs[ATTR_DATE].nameLen = strlen(mEventAttrs[ATTR_DATE].name);
    mEventAttrs[ATTR_DATE].value = mEventAttrBufs[ATTR_DATE];
    mEventAttrs[ATTR_SESSID].name = "sessionid";
    mEventAttrs[ATTR_SESSID].nameLen = strlen(mEventAttrs[ATTR_SESSID].name);
    mEventAttrs[ATTR_SESSID].value = mEventAttrBufs[ATTR_SESSID];
    mEventAttrs[ATTR_BLAZEID].name = "blazeid";
    mEventAttrs[ATTR_BLAZEID].nameLen = strlen(mEventAttrs[ATTR_BLAZEID].name);
    mEventAttrs[ATTR_BLAZEID].value = mEventAttrBufs[ATTR_BLAZEID];
}

bool EventXmlEncoder::encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                             , bool onlyEncodeChanged
#endif
                             )
{
    return mEncoder.encodeTdf(buffer, tdf, frame
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , onlyEncodeChanged
#endif
            );
}

bool EventXmlEncoder::EventEncoder::encodeTdf(RawBuffer& buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                             , bool onlyEncodeChanged
#endif
                             )
{
    RawBufferIStream istream(buffer);
    mWriter.SetOutputStream(&istream);
    mWriter.SetLineEnd(EA::XML::kLineEndUnix); // we want /n as line end
    if ((frame != nullptr) && (frame->messageType == RpcProtocol::NOTIFICATION))
    {
        bool found = false;
        const char8_t* eventName = BlazeRpcComponentDb::getEventNameById(
                frame->componentId, frame->commandId, &found);

        if (!found)
        {
            eventName = BlazeRpcComponentDb::getNotificationNameById(
                        frame->componentId, frame->commandId);

            if (blaze_strcmp("<UNKNOWN>", eventName) == 0)  
            {
                // Not a recognized event so just eat it
                return true;
            }
            else
            {
                mIsSessionNotification = true;
            }
        }

        mIsEvent = true;
        uint32_t year, month, day, hour, min, sec, msec;
        TimeValue::getGmTimeComponents(TimeValue::getTimeOfDay(),
                &year, &month, &day, &hour, &min, &sec, &msec);

        int32_t len = blaze_snzprintf(
                mEventAttrBufs[ATTR_COMPONENT], sizeof(mEventAttrBufs[ATTR_COMPONENT]),
                "%s", BlazeRpcComponentDb::getComponentNameById(frame->componentId));
        mEventAttrs[ATTR_COMPONENT].valueLen = len;

        size_t idx;
        for(idx = 0; (eventName[idx] != '\0') && (idx < (sizeof(mEventAttrBufs[ATTR_TYPE]) - 1));
                ++idx)
        {
            mEventAttrBufs[ATTR_TYPE][idx] = (char8_t)tolower(eventName[idx]);
        }
        mEventAttrBufs[ATTR_TYPE][idx] = '\0';
        mEventAttrs[ATTR_TYPE].valueLen = idx;

        len = blaze_snzprintf(mEventAttrBufs[ATTR_DATE], sizeof(mEventAttrBufs[ATTR_DATE]),
                "%d-%02d-%02dT%02d:%02d:%02d.%dZ", year, month, day, hour, min, sec, msec);
        mEventAttrs[ATTR_DATE].valueLen = len;

        len = blaze_snzprintf(mEventAttrBufs[ATTR_SESSID], sizeof(mEventAttrBufs[ATTR_SESSID]),
                   "%" PRIu64, mAssociatedSession ? mAssociatedSession->getId() : 0);
        mEventAttrs[ATTR_SESSID].valueLen = len;

        len = blaze_snzprintf(mEventAttrBufs[ATTR_BLAZEID], sizeof(mEventAttrBufs[ATTR_BLAZEID]),
                   "%" PRId64, mAssociatedSession ? mAssociatedSession->getBlazeId() : 0);
        mEventAttrs[ATTR_BLAZEID].valueLen = len;

        putStartElement("event", mEventAttrs, EAArrayCount(mEventAttrs));
    }
    else
    {
        mIsEvent = false;
    }

    EA::TDF::EncodeOptions encOptions;
    encOptions.useCommonEntryName = frame->useCommonListEntryName;
    switch (frame->format)
    {
    case RpcProtocol::FORMAT_USE_TDF_NAMES:
        encOptions.format = EA::TDF::EncodeOptions::FORMAT_USE_TDF_NAMES;
        break;
    case RpcProtocol::FORMAT_USE_TDF_RAW_NAMES:
        encOptions.format = EA::TDF::EncodeOptions::FORMAT_USE_TDF_RAW_NAMES;
        break;
    case RpcProtocol::FORMAT_USE_TDF_TAGS:
        {
            EA_FAIL_MSG("FORMAT_USE_TDF_TAGS XML encoding option is not supported!");
            return false;
        }
    default:
        {
            EA_FAIL_MSG("Unknown frame format!");
            return false;
        }
    }

    switch (frame->enumFormat)
    {
    case RpcProtocol::ENUM_FORMAT_IDENTIFIER:
        encOptions.enumFormat = EA::TDF::EncodeOptions::ENUM_FORMAT_IDENTIFIER;
        break;
    case RpcProtocol::ENUM_FORMAT_VALUE:
        encOptions.enumFormat = EA::TDF::EncodeOptions::ENUM_FORMAT_VALUE;
        break;
    default:
        break;
    }

    mEncOptions = &encOptions;
    EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    visitOpt.onlyIfSet = onlyEncodeChanged;
#endif
    visitOpt.onlyIfNotDefault = mDefaultDiff;
    if (!mIsEvent)
        mWriter.WriteXmlHeader();
    char8_t elementName[EA::TDF::MAX_XML_ELEMENT_LENGTH];
    const char8_t* className = getSanitizedMemberName(tdf.getClassName(), elementName); // convert to lower case
    mWriter.customBeginElement(className);
    bool result = tdf.visit(*this, visitOpt);
    mWriter.customEndElement(className);
    if (mIsEvent)
        mWriter.customEndElement("event");
    mWriter.WriteCharData("\n", 1); // add /n to the end of document
    mWriter.SetOutputStream(nullptr);
    mWriter.setIndentLevel(0);

    return result;
}

void EventXmlEncoder::EventEncoder::putStartElement(const char8_t *name, const EventXmlAttribute *attributes, size_t attributeCount)
{
    mWriter.customBeginElement(name);

    for (size_t i=0; i<attributeCount; ++i)
    {
        mWriter.customAppendAttribute(attributes[i].name, attributes[i].value);
    }
}

} // Blaze

