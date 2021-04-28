/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CHANNELHANDLER_H
#define BLAZE_CHANNELHANDLER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Channel;

class ChannelHandler
{
public:
    virtual ~ChannelHandler() { }

    virtual void onRead(Channel& channel) = 0;
    virtual void onWrite(Channel& channel) = 0;
    virtual void onError(Channel& channel) = 0;
    virtual void onClose(Channel& channel) = 0;
    
};

} // Blaze

#endif // BLAZE_CHANNELHANDLER_H

