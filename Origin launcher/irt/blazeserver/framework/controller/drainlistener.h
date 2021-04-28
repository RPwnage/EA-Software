/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DRAIN_LISTENER_H
#define BLAZE_DRAIN_LISTENER_H


namespace Blaze
{

class DrainListener
{
public:
    virtual void onControllerDrain() = 0;
    virtual ~DrainListener() {}
};

class DrainStatusCheckHandler
{
public:
    virtual ~DrainStatusCheckHandler() {}
    virtual bool isDrainComplete() = 0;
};

} // Blaze

#endif // BLAZE_DRAIN_LISTENER_H

