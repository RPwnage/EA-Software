/*************************************************************************************************/
/*!
    \file   gamebrowserserviceimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERSERVICEIMPL_H
#define BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERSERVICEIMPL_H

/*** Include files *******************************************************************************/
#include "gamebrowserservice/tdf/gamebrowserservice.h"
#include "gamebrowserservice/rpc/gamebrowserserviceslave_stub.h"
#include "gamemanager/tdf/gamebrowser.h"

#include "framework/controller/drainlistener.h"
#include "framework/system/fiberjobqueue.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameBrowserService
{
namespace v1
{

class GameBrowserServiceSlaveImpl : public GameBrowserServiceSlaveStub,
    private Blaze::DrainStatusCheckHandler
{
public:
    GameBrowserServiceSlaveImpl();
    virtual ~GameBrowserServiceSlaveImpl();

    void terminateList(const Blaze::GameManager::DestroyGameListRequest& request);
private:
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onResolve() override;

    void onShutdown() override;

    bool isDrainComplete() override;

    void terminateListInternal(const Blaze::GameManager::DestroyGameListRequest request);

public:
    void obfuscatePlatformInfo(Blaze::ClientPlatformType platform, EA::TDF::Tdf& tdf) const override;

private:
    FiberJobQueue mTerminateListQueue;
    bool mShutdown;
}; //class GameBrowserServiceSlaveImpl

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze

#endif // BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERSERVICEIMPL_H
