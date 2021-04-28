/*************************************************************************************************/
/*!
\file connapivoipmanager.h


\attention
(c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_CONNAPI_VOIP_MANAGER_H
#define BLAZE_GAMEMANAGER_CONNAPI_VOIP_MANAGER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"

#include "EABase/eabase.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/idler.h"

#include "BlazeSDK/networkmeshadapter.h" 
#include "BlazeSDK/networkmeshadapterlistener.h"

#include "BlazeSDK/blaze_eastl/vector.h"

//  forward declare VoipTunnelRefT from voiptunnel.h
struct VoipTunnelRefT;
struct VoipTunnelClientT;

namespace Blaze
{
    class BlazeHub;

    /*! **************************************************************************************************************/
    /*! \class ConnApiVoipManager
        \brief Manages client-server VoIP for rebroadcaster/dedicated server based games via VoipTunnel
    ******************************************************************************************************************/
    class BLAZESDK_API ConnApiVoipManager : public Idler
    {
    public:
        /*! ****************************************************************************/
        /*! \brief Constructor. 

        Private as all construction should happen via the factory method.
        ********************************************************************************/
        ConnApiVoipManager(BlazeHub *blazeHub, const Mesh *mesh, uint32_t voipPort, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
        virtual ~ConnApiVoipManager();

        void connectToEndpoint(const Blaze::MeshEndpoint *pEndpoint);
        void disconnectFromEndpoint(const Blaze::MeshEndpoint *pEndpoint);
    private:
        NON_COPYABLE(ConnApiVoipManager);

        //  Idler
        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
        uint32_t GetClientIdFromEndpoint(const Blaze::MeshEndpoint *pEndpoint);

    private:
        void refreshSendLists( const Blaze::Mesh *mesh );

        const Mesh *mMesh;
        BlazeHub *mBlazeHub;
        VoipTunnelRefT *mVoipTunnelRef;
        uint32_t mVoipPort;
        int32_t mMaxClients;

        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
    };

} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_VOIP_MANAGER_H
