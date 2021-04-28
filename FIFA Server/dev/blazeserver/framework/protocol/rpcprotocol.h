/*************************************************************************************************/
/*!
    \file rpcprotocol.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RPC_PROTOCOL_H
#define BLAZE_RPC_PROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/protocol.h"
#include "framework/protocol/shared/encoder.h"
#include "framework/protocol/shared/decoder.h"
#include "framework/logger.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Channel;
class RawBuffer;
class Request;
class TdfEncoder;
class TdfDecoder;

class RpcProtocol : public Protocol
{
public:
    typedef enum
    {
        MESSAGE,
        REPLY,
        NOTIFICATION,
        ERROR_REPLY,
        PING,
        PING_REPLY
    } MessageType;

    typedef enum
    {
        CONNECTION_NONE,
        CONNECTION_CLOSE
    } ConnectionTransform;

    typedef enum
    {
        FORMAT_INVALID,
        FORMAT_USE_TDF_NAMES,
        FORMAT_USE_TDF_RAW_NAMES,
        FORMAT_USE_TDF_TAGS,
        MAX_RESPONSE_FORMAT
    } ResponseFormat;

    typedef enum
    {
        ENUM_FORMAT_INVALID,
        ENUM_FORMAT_IDENTIFIER,
        ENUM_FORMAT_VALUE
    } EnumResponseFormat;

    typedef enum
    {
        XML_RESPONSE_FORMAT_INVALID,
        XML_RESPONSE_FORMAT_ELEMENTS,
        XML_RESPONSE_FORMAT_ATTRIBUTES
    } XmlResponseFormat;

    struct Frame
    {
        Frame(uint32_t _locale = 0, uint32_t _country = 0, Encoder::Type encoderType = Encoder::INVALID, Decoder::Type decoderType = Decoder::INVALID)
            : componentId(0),
            commandId(0),
            msgNum(0),
            userIndex(0),
            errorCode(0),
            messageType(MESSAGE),
            context(0),
            locale(_locale),
            country(_country),
            immediate(false),
            transform(CONNECTION_NONE),
            format(FORMAT_USE_TDF_NAMES),
            enumFormat(ENUM_FORMAT_IDENTIFIER),
            xmlResponseFormat(XML_RESPONSE_FORMAT_ELEMENTS),
            voidRpcResponse(false),
            useCommonListEntryName(false),
            int64AsString(false),
            responseEncodingType(encoderType),
            requestEncodingType(decoderType),
            useLegacyJsonEncoding(false),
            isFaviconRequest(false),
            movedTo(INVALID_INSTANCE_ID),
            sliverIdentity(INVALID_SLIVER_IDENTITY),
            superUserPrivilege(false),
            requestIsSecure(false),
            sessionKeyProvided(false)
        {
            *sessionKey = '\0';
            *serviceName = '\0';
        }

        uint16_t componentId;
        uint16_t commandId;
        uint32_t msgNum;
        uint32_t userIndex;
        uint32_t errorCode;
        MessageType messageType;
        uint64_t context;
        uint32_t locale;
        uint32_t country;
        bool immediate;
        char8_t serviceName[MAX_SERVICENAME_LENGTH];
        ConnectionTransform transform;
        ResponseFormat format;
        EnumResponseFormat enumFormat;
        XmlResponseFormat xmlResponseFormat;
        bool voidRpcResponse;
        bool useCommonListEntryName;
        bool int64AsString;
        Encoder::Type responseEncodingType;
        Decoder::Type requestEncodingType;
        bool useLegacyJsonEncoding;
        bool isFaviconRequest;
        InetAddress clientAddr;
        InstanceId movedTo;
        InetAddress movedToAddr;
        SliverIdentity sliverIdentity;
        bool superUserPrivilege;
        bool requestIsSecure;
        eastl::string requestUrl;
        LogContextWrapper logContext;
        

        inline const char8_t* getSessionKey() const { return (sessionKeyProvided ? sessionKey : nullptr); }
        inline void setSessionKey(const char8_t* _sessionKey)
        {
            sessionKeyProvided = (_sessionKey != nullptr);
            if (sessionKeyProvided)
                blaze_strnzcpy(sessionKey, _sessionKey, sizeof(sessionKey));
            else
                *sessionKey = '\0';
        }
    private:
        char8_t sessionKey[MAX_SESSION_KEY_LENGTH];
        bool sessionKeyProvided;
    };


public:
    RpcProtocol() { }

    /*********************************************************************************************/
    /*!
        \brief process

        Process the complete frame in the provided buffer and build a request object.  The
        call will own the request object on return.

        \param[in]  buffer - RawBuffer to read from.
        \param[in]  frame - Frame settings to fill out.
        
    */
    /*********************************************************************************************/
    virtual void process(RawBuffer& buffer, RpcProtocol::Frame& frame, TdfDecoder& decoder) = 0;


    /*********************************************************************************************/
    /*!
         \brief framePayload

         Setup the frame header in the given buffer using the provided frame struct.  The
         buffer's data point on input will be set to point to the frame's payload and the payload
         will be present.

         On exit, the given buffer's data pointer must be pointing to the head of the frame.
               
         \param[in] buffer - Buffer to encode frame header into.
         \param[in] frame - Frame data to use.
         \param[in] dataSize - Size of the payload.  <0 indicates payload size is unknown
    */
    /*********************************************************************************************/
    virtual void framePayload(RawBuffer& buffer, const RpcProtocol::Frame& frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) = 0;


    /*! ***************************************************************************/
    /*! \brief Gets the notification support for the protocol.
    
        \return True if the protocol supports notifications, false if it doesn't.
    *******************************************************************************/
    virtual bool supportsNotifications() const = 0;

    /*! ***************************************************************************/
    /*! \brief Gets whether notifications are compressed for the protocol.

    \return True if notifications will be compressed, false otherwise.
    *******************************************************************************/
    virtual bool compressNotifications() const { return false; }

    /*! ***************************************************************************/
    /*! \brief Gets if this protocol supports raw passthrough requests (e.g. Fire and Fire2).
    
        \return True if raw passthrough requests are supported.
    *******************************************************************************/
    virtual bool supportsRaw() const { return false; }

    /*! ***************************************************************************/
    /*! \brief Gets if this protocol supports PING messages being sent from the server
               to the client (e.g. Fire2).
    
        \return True if PING requests are supported.
    *******************************************************************************/
    virtual bool supportsClientPing() const { return false; }

    /*! ***************************************************************************/
    /*! \brief Returns the next available sequence number for use
    *******************************************************************************/
    virtual uint32_t getNextSeqno() = 0;

    ~RpcProtocol() override { }
};

} // Blaze

#endif // BLAZE_RPC_PROTOCOL_H
