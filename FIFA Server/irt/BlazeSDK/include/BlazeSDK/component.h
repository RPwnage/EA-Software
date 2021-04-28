/*! **********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_COMPONENT_H
#define BLAZE_COMPONENT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/******* Include files *******************************************************************************/

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeerrortype.h"


/******* Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ComponentManager;
class FunctorBase;
struct RestResourceInfo;

/*! ***************************************************************/
/*! \class Component
    \brief Base class for all component types.
*******************************************************************/
class BLAZESDK_API Component
{
public:
    
    Component(ComponentManager* manager, uint16_t componentId) 
    :   mManager(manager), mComponentId(componentId) {}
    virtual ~Component() {}

    /*! ***************************************************************/
    /*! \brief Returns the id of this component.
    *******************************************************************/
    uint16_t getComponentId() const { return mComponentId; }

    virtual const char8_t* getComponentName() const = 0; 
    virtual const char8_t* getCommandName(uint16_t commandId) const = 0; 
    virtual const char8_t* getNotificationName(uint16_t notificationId) const = 0; 
    virtual const char8_t* getErrorName(BlazeError errorCode) const = 0;
    virtual const RestResourceInfo* getRestInfo(uint16_t commandId) const = 0;

#ifdef BLAZE_ENABLE_TDF_CREATION_BY_ID    
    // tdf decoder helpers for external fireshark package
    virtual EA::TDF::Tdf* createRequestTdf(uint16_t commandId) = 0;
    virtual EA::TDF::Tdf* createResponseTdf(uint16_t commandId) = 0;
    virtual EA::TDF::Tdf* createErrorTdf(uint16_t commandId) = 0;
    virtual EA::TDF::Tdf* createNotificationTdf(uint16_t notificationId) = 0;
#endif

/*! \cond INTERNAL_DOCS */
protected:
    friend class ComponentManager;

    /*! ***************************************************************/
    /*! \brief Internal function for handling a notification from the server.
    *******************************************************************/
    virtual void handleNotification(uint16_t command, const uint8_t* buf, size_t bufSize, uint32_t userIndex) = 0;

    ComponentManager* mManager;
    uint16_t mComponentId;

    NON_COPYABLE(Component);
/*! \endcond */
};

} //Blaze

#endif //BLAZE_COMPONENT_H
