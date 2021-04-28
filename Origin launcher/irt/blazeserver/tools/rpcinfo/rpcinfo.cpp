/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    Simple program to dump the component and command names and ids.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/component/blazerpc.h"

namespace rpcinfo
{
using namespace Blaze;

/*** Private functions ***************************************************************************/

static const struct
{
    RpcComponentType type;
    const char8_t* name;
} sComponentTypes[] =
{
    { RPC_TYPE_COMPONENT_CORE, "core" },
    { RPC_TYPE_COMPONENT_CUST, "custom" },
    { RPC_TYPE_FRAMEWORK,      "framework" }
};

static const char8_t* getComponentTypeName(RpcComponentType type)
{
    for(size_t i = 0; i < EAArrayCount(sComponentTypes); i++)
    {
        if (sComponentTypes[i].type == type)
            return sComponentTypes[i].name;
    }
    return "<unknown>";
}

static int usage(const char8_t* exe)
{
    fprintf(stderr, "Usage: %s [-c] [-n] [componentId]\n", exe);
    return 1;
}

/*** Public functions ****************************************************************************/

int rpcinfo_main(int argc, char** argv)
{
    uint16_t componentId = 0;
    bool printCommands = false;
    bool printNotifications = false;

    BlazeRpcComponentDb::initialize();

    for(int arg = 1; arg < argc; arg++)
    {
        if (strcmp(argv[arg], "-c") == 0)
        {
            printCommands = true;
        }
        else if (strcmp(argv[arg], "-n") == 0)
        {
            printNotifications = true;
        }
        else
        {
            if (componentId != 0)
                exit(usage(argv[0]));

            if ((argv[arg][0] == '0') && ((argv[arg][1] == 'x') || (argv[arg][1] == 'X')))
                componentId = (uint16_t)strtoul(argv[arg], nullptr, 16);
            else
                componentId = (uint16_t)strtoul(argv[arg], nullptr, 10);
        }
    }

    printf("%-22s %s\n", "Id", "Name");    
    printf("--------------------------------------------------------------------------------\n");
    BlazeRpcComponentDb::NamesById* components = BlazeRpcComponentDb::getComponents();
    BlazeRpcComponentDb::NamesById::iterator compItr = components->begin();
    BlazeRpcComponentDb::NamesById::iterator compEnd = components->end();
    for(; compItr != compEnd; ++compItr)
    {
        if ((componentId == 0) || (componentId == compItr->first))
        {
            char8_t compStr[256];
            blaze_snzprintf(compStr, sizeof(compStr), "0x%04x (%s/%d)", compItr->first,
                    getComponentTypeName(RpcGetComponentType(compItr->first)),
                    RpcComponentId(compItr->first));
            printf("%-22s %s\n", compStr, compItr->second);
            if (printCommands)
            {
                BlazeRpcComponentDb::NamesById* cmds
                    = BlazeRpcComponentDb::getCommandsByComponent(compItr->first);
                if (cmds->size() > 0)
                {
                    printf("    Commands:\n");
                    BlazeRpcComponentDb::NamesById::iterator cmdItr = cmds->begin();
                    BlazeRpcComponentDb::NamesById::iterator cmdEnd = cmds->end();
                    for(; cmdItr != cmdEnd; ++cmdItr)
                    {
                        printf("     %5d %s\n", cmdItr->first, cmdItr->second);
                    }
                }
                delete cmds;
            }
            if (printNotifications)
            {
                BlazeRpcComponentDb::NamesById* notifications
                    = BlazeRpcComponentDb::getNotificationsByComponent(compItr->first);
                if (notifications->size() > 0)
                {
                    printf("    Notifications:\n");
                    BlazeRpcComponentDb::NamesById::iterator notItr = notifications->begin();
                    BlazeRpcComponentDb::NamesById::iterator notEnd = notifications->end();
                    for(; notItr != notEnd; ++notItr)
                    {
                        printf("     %5d %s\n", notItr->first, notItr->second);
                    }
                }
                delete notifications;
            }
        }
    }
    delete components;
    return 0;
}

}
