// Include files
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "DirtySDK/dirtysock/netconn.h"

#ifdef EA_PLATFORM_XBOXONE
#include "BlazeSDK/loginmanager/loginmanager.h"

namespace Blaze
{

namespace ConnectionManager
{

void QosManager::initLocalAddress()
{
    // since the SecureDeviceAddr fits in the DirtyAddrT, it is safe to use a buffer as large as a DirtyAddrT
    uint8_t aSecureDeviceAddressBlob[DIRTYADDR_MACHINEADDR_MAXLEN];
    int32_t iBlobSize;
    uint64_t aXuid;

    // the base address is the same for all local users.
    iBlobSize = NetConnStatus('xadd', 0, (void *)aSecureDeviceAddressBlob, sizeof(aSecureDeviceAddressBlob));

    if (iBlobSize <= 0)
    {
        BLAZE_SDK_DEBUGF("[qosmanager] WARNING: initLocalAddress() failed - returned blob size is negative.\n");
        return;
    }

    // obtain each indexes xuid
    for (uint32_t ui=0; ui < mBlazeHub.getNumUsers(); ++ui)
    {
        uint32_t dirtySockUserIndex = mBlazeHub.getLoginManager(ui)->getDirtySockUserIndex();
        NetConnStatus('xuid', dirtySockUserIndex, &aXuid, sizeof(aXuid));
        mLocalXUIDVector[ui] = aXuid;
    }

    XboxClientAddress xboxaddr;
    // the base network info will contain the primary user index by default.
    // Our getter sets the value as needed.
    xboxaddr.setXuid(mLocalXUIDVector[mBlazeHub.getPrimaryLocalUserIndex()]);
    xboxaddr.setMachineId(NetConnMachineId());

    // our address.  same for all users.
    xboxaddr.getXnAddr().setData((const uint8_t*)&aSecureDeviceAddressBlob, iBlobSize);

    // store it all off on the network info.
    mNetworkInfo.getAddress().setXboxClientAddress(&xboxaddr);
    BLAZE_SDK_DEBUGF("[qosmanager] Set local Xbox client address: aXuid()\n");
}

} // Connection

} // Blaze

#endif // EA_PLATFORM_XBOXONE
