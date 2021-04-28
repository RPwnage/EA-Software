/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef REDIRECTOR_REDIRECTORUTIL_H
#define REDIRECTOR_REDIRECTORUTIL_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class Endpoint;

namespace Grpc
{
class GrpcEndpoint;
}
namespace Redirector
{

class ServerInstanceStaticData;
class RedirectorSettingsConfig;

class RedirectorUtil
{
public:
    static void populateServerInstanceStaticData(ServerInstanceStaticData& staticData);
    static void copyServerInstanceToStaticServerInstance(const ServerInstance& instance, StaticServerInstance& staticInstance);
    static void copyStaticServerInstanceToServerInstance(const StaticServerInstance& staticInstance, ServerInstance& instance);
    static void populateServerInstanceDynamicData(ServerInstanceDynamicData& dynamicData);
    static uint32_t getMaxConnectionsForEndpoint(const Endpoint& endpoint);
    static uint32_t getMaxConnectionsForEndpoint(const Grpc::GrpcEndpoint& endpoint);
};

} // Redirector
} // Blaze

#endif // REDIRECTOR_REDIRECTORUTIL_H

