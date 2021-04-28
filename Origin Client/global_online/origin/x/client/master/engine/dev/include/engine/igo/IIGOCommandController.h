///////////////////////////////////////////////////////////////////////////////
// IIGOCommandController.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef engine_IIGOCommandController_h
#define engine_IIGOCommandController_h

#if defined(ORIGIN_PC)
#define NOMINMAX
#endif

#include <QObject>

namespace Origin
{
namespace Engine
{

    // Interfaces to controller that handle igoCommand's
    class IIGOCommandController
    {
    public:

        // Set of IGO commands
        enum IGO_CMD
        {
            CMD_UNKNOWN,
            CMD_NEW_BROWSER,
            CMD_SHOW_FRIENDS,
            CMD_SHOW_SETTINGS,
            CMD_SHOW_CS,
            CMD_CLOSE_IGO,
            CMD_LOGO_CLICKED,
            CMD_START_BROADCAST,
            CMD_SHOW_BROADCAST,
            CMD_START_AUTOBROADCAST,
            CMD_SHOW_ACHIEVEMENTS
        };

    public:
        virtual ~IIGOCommandController() { };

        enum CallOrigin
        {
            CallOrigin_CLIENT = 0,
            CallOrigin_SDK,
            CallOrigin_UNDEFINED
        };

        virtual bool igoCommand(int cmd, CallOrigin callOrigin) = 0;    // return true if the command is handled, otherwise false
    };

    
} // Engine
} // Origin

#endif    // engine_IIGOCommandController_h
