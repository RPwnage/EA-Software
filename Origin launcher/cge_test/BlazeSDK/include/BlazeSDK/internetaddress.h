/*************************************************************************************************/
/*!
    \file internetaddress.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/


#ifndef BLAZE_INTERNET_ADDRESS_H
#define BLAZE_INTERNET_ADDRESS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"

namespace Blaze
{
    /*! **********************************************************************************************************/
    /*! \class InternetAddress
        \brief This class represents an IPv4 address in host byte order
    **************************************************************************************************************/
    class BLAZESDK_API InternetAddress
    {
    public:

        /*! **********************************************************************************************************/
        /*! \name Constants
        **************************************************************************************************************/
        static const int MAX_LENGTH = 32;

    public:

        /*! **********************************************************************************************************/
        /*! \brief Sets the IP address and port number.
            \param[in] ip an IP address string (dotted octet notation).  Ex: "127.0.0.1".  
            \param[in] port the port number (in host order).
        **************************************************************************************************************/
        void setIpAndPort(const char *ip, uint16_t port);

        /*! ************************************************************************************************/
        /*!
            \brief Sets the ip address and port number.
        
            \param[in] ip address packed into an unsigned int in host byte order (see GetIp()).
            \param[in] port port number in host byte order
        ***************************************************************************************************/
        void setIpAndPort(uint32_t ip, uint16_t port);

        /*! **********************************************************************************************************/
        /*! \brief Converts the IP address to a string in dotted, four-octet format.
            \param[out] buf Stores the IP address in dotted, four-octet format.
            \param      len Size of \b buf, must be greater than 15.
        **************************************************************************************************************/
        char* asString(char *buf, size_t len) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the IP address as a non-negative integer in host byte order.
        **************************************************************************************************************/
        uint32_t getIp() const { return mIP; }

        /*! **********************************************************************************************************/
        /*! \brief Sets the IP address.
            \param ip A non-negative integer representing an IP address in host byte order.
        **************************************************************************************************************/
        void setIP(uint32_t ip) { mIP = ip; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the port number in host byte order.
        **************************************************************************************************************/
        uint16_t getPort() const { return mPort; }

        /*! **********************************************************************************************************/
        /*! \brief Sets the port number.
            \param port A port number in host byte order.
        **************************************************************************************************************/
        void setPort(uint16_t port) { mPort = port; }

        /*! **********************************************************************************************************/
        /*! \brief Determines whether the IP address is valid (non-zero).
            \return Returns True if the IP address is valid (non-zero), else returns False.
        **************************************************************************************************************/
        bool isValid() const { return mIP != 0; }


        /*! **********************************************************************************************************/
        /*! \name Destructor
        **************************************************************************************************************/
        ~InternetAddress() {}

        /*! **********************************************************************************************************/
        /*! \name Constructors
        **************************************************************************************************************/
        InternetAddress() :
             mIP(0),
             mPort(0)
         {
         }

        /*! **********************************************************************************************************/
        /*! \brief Construct a new instance
            \param ip   An IP address in host byte order
            \param port A port number in host byte order.
        **************************************************************************************************************/
        InternetAddress(uint32_t ip, uint16_t port) : 
            mIP(ip),
            mPort(port)
         {
         }


    private:
        uint32_t   mIP;
        uint16_t mPort;
    };

} // namespace Blaze

#endif // BLAZE_INTERNET_ADDRESS_H
