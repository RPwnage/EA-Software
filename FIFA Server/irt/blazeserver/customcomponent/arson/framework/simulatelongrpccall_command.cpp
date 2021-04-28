/*************************************************************************************************/
/*!
    \file   reconfigure.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/util/random.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/simulatelongrpccall_stub.h"

namespace Blaze
{
namespace Arson
{
class SimulateLongRPCCallCommand : public SimulateLongRPCCallCommandStub
{
public:
    SimulateLongRPCCallCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SimulateLongRPCCallCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SimulateLongRPCCallCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    SimulateLongRPCCallCommandStub::Errors execute() override
    {
        int count(0);
        do
        {
            const size_t bufSize = 1024;
            char8_t *buf = BLAZE_NEW_ARRAY(char8_t, bufSize);
            memset(buf, 0, bufSize);

            Random::initializeSeed();
            for (size_t i = 0; i < bufSize - 1; ++i)
            {
                // generate a random ASCII character (non-control character)
                int randNum = Random::getRandomNumber(93) + 33; // 33 - 126 are the valid non control characters
                memset(&buf[i], randNum, 1);
            }

            char8_t substr[100];
            int offset = Random::getRandomNumber(bufSize - sizeof(substr) - 1);
            blaze_strsubzcpy(substr, sizeof(substr), buf + offset, sizeof(substr));
            TRACE_LOG("[SimulateLongRPCCallCommand:].execute() - Random buffer snippet " << substr);

            delete [] buf;
            buf = nullptr;
            ++count;

            Blaze::BlazeRpcError err = gSelector->sleep(5);
            if (err != Blaze::ERR_OK)
            {
                return ERR_SYSTEM;
            }

        } while (count < 1000);

        return ERR_OK;
    }
};

DEFINE_SIMULATELONGRPCCALL_CREATE()

} //Arson
} //Blaze
