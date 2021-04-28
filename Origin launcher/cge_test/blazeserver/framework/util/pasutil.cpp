/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/pasutil.h"
#include "framework/tdf/networkaddress.h"
#include "framework/controller/processcontroller.h"

namespace Blaze
{
static const char8_t* STR_PORT_FILE_EXTENSION = ".port";

PasUtil::PasUtil() :
 mResolver(nullptr),
 mPasSlave(nullptr)
{
}

PasUtil::~PasUtil()
{
    if (mPasSlave != nullptr)
        delete mPasSlave;

    if (mResolver != nullptr)
        delete mResolver;
}

bool PasUtil::initialize(const char8_t* serviceName, const char8_t* instanceName, const char8_t* pasAddr)
{
    if (mPasSlave != nullptr || mResolver != nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].initialize: Attempt to re-initialize PAS util.");
        return false;
    }

    int32_t portType = gProcessController->getBootConfigTdf().getPasPortType();
    if (!PAS::GetPortTypeEnumMap().exists(portType))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].initialize: The number " << portType << " does not correspond to a known PAS port type.");
        return false;
    }

    if (pasAddr != nullptr)
    {
        const char8_t* hostName = nullptr;
        bool secure = false;
        HttpProtocolUtil::getHostnameFromConfig(pasAddr, hostName, secure);
        InetAddress pasInetAddr;
        pasInetAddr.setHostnameAndPort(hostName);
        mResolver = BLAZE_NEW PasRpcProxyResolver(pasInetAddr, secure);
        mPasSlave = BLAZE_NEW PAS::PASSlave(*mResolver, pasInetAddr);

        mRequest.setHost(NameResolver::getInternalAddress().getHostname());
        mRequest.setService(serviceName);
        mRequest.setInstanceName(instanceName);
        mRequest.setPortType(static_cast<PAS::PortType>(portType));
        mFilename.append_sprintf("%s%s", instanceName, STR_PORT_FILE_EXTENSION);

        return true;
    }

    return false;
}

bool PasUtil::getBasePort(uint32_t numberOfPorts, uint16_t& basePort)
{
    if (mPasSlave == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].getBasePort: PAS util is not initialized.");
        return false;
    }
    PAS::PortAssignment resp;
    PAS::Error errResp;
    mRequest.setPorts(static_cast<uint16_t>(numberOfPorts));
    BlazeRpcError err = mPasSlave->createOrFetchPortAssignment(mRequest, resp, errResp);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].getBasePort: Error " << ErrorHelp::getErrorName(err) << " getting port assignment from PAS (error code: " << errResp.getCode()
            << ", error cause: \"" << errResp.getCause() << "\"). Will attempt to read last successful response from file " << mFilename);

        FILE* portFp = fopen(mFilename.c_str(), "r");
        if (portFp == nullptr)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].getBasePort: Failed to read port file (" << mFilename << "): " << strerror(errno));
            return false;
        }

        bool result = false;
        char8_t buf[32];
        fgets(buf, sizeof(buf), portFp);
        uint32_t savedNumOfPorts = 0;
        blaze_str2int(buf, &savedNumOfPorts);

        // we can only reuse the last saved successful port assignment
        // if we are requesting for the same or smaller number of ports
        if (numberOfPorts <= savedNumOfPorts)
        {
            fgets(buf, sizeof(buf), portFp);
            blaze_str2int(buf, &basePort);
            result = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].getBasePort: "
                "Cannot use stored ports in port file because requested number of ports (" << numberOfPorts << ") > saved range(" << savedNumOfPorts << ")");
        }
        fclose(portFp);
        return result;
    }

    // write the successful response to file to be used upon our next restart
    // in case the PAS is down when we do the restart
    FILE* portFp = fopen(mFilename.c_str(), "w+");
    if (portFp == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[PasUtil].getBasePort: Failed to write port file (" << mFilename << "): " << strerror(errno));
    }
    else
    {
        fprintf(portFp, "%d\n", numberOfPorts);
        fprintf(portFp, "%d\n", resp.getStart());
        fclose(portFp);
    }

    basePort = resp.getStart();
    return true;
}

} // Blaze

