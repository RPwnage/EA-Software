/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EVENTXMLENCODER_H
#define BLAZE_EVENTXMLENCODER_H

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/tdfencoder.h"
#include "EATDF/codec/tdfxmlencoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class EventXmlEncoder : public Blaze::TdfEncoder
{
public:
    const char8_t* getName() const override { return EventXmlEncoder::getClassName(); }
    Encoder::Type getType() const override { return Encoder::EVENTXML; }

    static const char8_t* getClassName() { return "eventxml"; }
        
    bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        , bool onlyEncodeChanged = false
#endif
        ) override;
    void setAssociatedUserSession(const UserSession* session) { mEncoder.mAssociatedSession = session; }
    void setUseDefaultDiff(bool useDefaultDiff) { mEncoder.mDefaultDiff = useDefaultDiff; }
    
private:
    class EventEncoder : public EA::TDF::XmlEncoder
    {
    public:
        enum { ATTR_COMPONENT = 0, ATTR_TYPE, ATTR_DATE, ATTR_SESSID, ATTR_BLAZEID, ATTR_MAX };

        /// \brief XML attribute containing a name and a value.
        struct EventXmlAttribute
        {
            const char8_t * name;      ///< name of attribute
            size_t nameLen;         ///< length of attribute name.
            const char8_t * value;     ///< value of attribute
            size_t valueLen;        ///< length of attribute value
        };

        EventXmlAttribute mEventAttrs[ATTR_MAX];
        char8_t mEventAttrBufs[ATTR_MAX][64];
        bool mIsEvent;
        bool mIsSessionNotification;
        bool mDefaultDiff;
        const UserSession* mAssociatedSession;

        EventEncoder();

        bool encodeTdf(RawBuffer& buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyEncodeChanged = false
#endif
            );
        void putStartElement(const char8_t *name, const EventXmlAttribute *attributes, size_t attributeCount);

    };
    EventEncoder mEncoder;
};

} // Blaze

#endif // BLAZE_EVENTXMLENCODER_H

