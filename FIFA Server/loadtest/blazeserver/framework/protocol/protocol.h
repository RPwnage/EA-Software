/*************************************************************************************************/
/*!
    \file protocol.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROTOCOL_H
#define BLAZE_PROTOCOL_H



/*** Include files *******************************************************************************/
// for client type definition
#include "framework/tdf/userdefines.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Channel;
class RawBuffer;
class Request;

class Protocol
{
public:
    enum Type
    {
        FIRE = 0,
        HTTPXML,
        EVENTXML,
        OUTBOUNDHTTP,
        WALHTTPXML,
        XMLHTTP,
        REST,
        FIRE2,
        INVALID,
        MAX = INVALID
    };

public:
    Protocol() { }

    /*********************************************************************************************/
    /*! \rief Returns the type of protocol this is.
    *********************************************************************************************/
    virtual Type getType() const = 0;

    virtual void setMaxFrameSize(uint32_t maxFrameSize) = 0;

    /*********************************************************************************************/
    /*!
        \brief receive

        Determine if enough data is available in the given buffer to compose an entire frame.
        If not, return the number of bytes still required to complete the frame.  When the frame
        is complete, the data pointer in the raw buffer should be pointing to the start of the
        frame.

        \param[in]  buffer - Buffer to examine.
        \param[out] needed - The number of bytes needed to complete the frame

        \return - true if the frame is complete; false if a terminal error occured and the
                  connection should be closed.
    */
    /*********************************************************************************************/
    virtual bool receive(RawBuffer& buffer, size_t& needed) = 0;

    
    /*********************************************************************************************/
    /*!
         \brief Sets up a buffer with the correct amount of overhead.
         \param buffer The buffer to set up.
    *********************************************************************************************/
    virtual void setupBuffer(RawBuffer& buffer) const;
    

    /*********************************************************************************************/
    /*!
         \brief Returns the overhead to reserve for the framespace of the protocol
    */
    /*********************************************************************************************/
    virtual uint32_t getFrameOverhead() const { return 0; }


     /*********************************************************************************************/
    /*!
         \brief getName

         Get the name of this protocol instance.
    */
    /*********************************************************************************************/
    virtual const char8_t* getName() const = 0;

    /*! ***************************************************************************/
    /*! \brief Gets the type of connection this protocol represents.
    
        \return True if the connection is persistent (HEAT, TCP, etc) or 
                if the session can span multiple connections (HTTP).
    *******************************************************************************/
    virtual bool isPersistent() const = 0;

    /*! ***************************************************************************/
    /*! \brief Gets the default client type for this protocol

    \return the default ClientType 
    *******************************************************************************/
    virtual ClientType getDefaultClientType() const = 0;

    virtual ~Protocol() { }

};

} // Blaze

#endif // BLAZE_PROTOCOL_H
