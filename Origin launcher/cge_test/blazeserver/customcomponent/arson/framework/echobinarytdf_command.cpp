/*************************************************************************************************/
/*!
    \file   echobinarytdf_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes

#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/echobinarytdf_stub.h"

#include "framework/protocol/shared/heat2encoder.h"
#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
#endif
namespace Blaze
{
namespace Arson
{
class EchoBinaryTDFCommand : public EchoBinaryTDFCommandStub
{
public:
    EchoBinaryTDFCommand(
        Message* message, EchoTdfRequestResponse* request, ArsonSlaveImpl* componentImpl)
        : EchoBinaryTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~EchoBinaryTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    // The command is for test case  NestedVariableTdf::head2encoderdecoder
    EchoBinaryTDFCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;
#ifdef EA_PLATFORM_WINDOWS
        EA::TDF::Tdf* tdf = mRequest.getTdf();
        if (tdf != nullptr)
        {
            EA::TDF::Tdf* newTdf = tdf->clone();
            Arson::EncodeDecodeTestClass testClass = *(static_cast<Arson::EncodeDecodeTestClass*>(newTdf));

            uint32_t wseq = 1;
            uint32_t wsize = 0;
            char8_t data[1024];
            memset(data, 0, sizeof(data));
            RawBuffer buffer((uint8_t*)data + sizeof(wseq) + sizeof(wsize), sizeof(data) - (sizeof(wseq) + sizeof(wsize)));
            Heat2Encoder encoder;
            if(encoder.encode(buffer, testClass))
            {
                wsize = buffer.datasize();
                wseq = htonl(wseq);
                memcpy(&(data[0]), &wseq, sizeof(wseq));
                wsize = htonl(wsize);
                memcpy(&(data[0]) + sizeof(wseq), &wsize, sizeof(wsize));
                // Change the binary file path if needed before run the command
                char8_t* binartFilePath = "E:\\temp\\binaries\\arson\\binary_input.log";
                FILE *outFile = fopen(binartFilePath, "wb");
                if (outFile != nullptr)
                {
                    fwrite(data, 1, sizeof(wseq) + sizeof(wsize) + buffer.datasize(), outFile);
                    fflush(outFile);
                    fclose(outFile);
                    outFile = nullptr;
                }

                // Read in the file we just wrote, and confirm that Blaze decodes it correctly

                FILE *inFile = fopen(binartFilePath, "r");
                while (inFile != nullptr && !feof(inFile))
                {
                    _setmode(_fileno(inFile), _O_BINARY);

                    uint32_t rseq = 0;
                    uint32_t rsize = 0;

                    // extract the sequence counter
                    fread(reinterpret_cast<char8_t*>(&rseq), 1, sizeof(uint32_t), inFile);
                    rseq = ntohl(rseq);

                    // extract the size
                    fread(reinterpret_cast<char8_t*>(&rsize), 1, sizeof(uint32_t), inFile);
                    rsize = ntohl(rsize);

                    if(rseq == 0 || rsize == 0)
                        continue;

                    Arson::EncodeDecodeTestClass* decodedClass = BLAZE_NEW Arson::EncodeDecodeTestClass;
                    uint8_t buf[1024];
                    memset(buf, 0, sizeof(buf));

                    RawBuffer raw(buf, sizeof(buf));
                    raw.put(rsize);

                    // use the size to extract the EncodeDecodeTestClass data
                    size_t rSize = fread(reinterpret_cast<char8_t*>(raw.data()), 1, rsize, inFile);

                    if (rSize != rsize)
                    {
                        ERR_LOG("Attempted to read " << rsize << " but only read " << static_cast<int32_t>(rSize));
                    }

                    Heat2Decoder decoder;
                    if (decoder.decode(raw, *decodedClass) == ERR_OK)
                    {
                        if (!decodedClass->equalsValue(testClass))
                        {
                            ERR_LOG("Decoded class does not equal encoded class!");
                        }             
                        mResponse.setTdf(*decodedClass);
                        err = ERR_OK;
                    }

                    fclose(inFile);
                    inFile = nullptr;
                }
            }

        }

        mRequest.getEchoList().copyInto(mResponse.getEchoList());
        mRequest.getVariableList().copyInto(mResponse.getVariableList());
        mRequest.getVariableMap().copyInto(mResponse.getVariableMap());
#endif
        return commandErrorFromBlazeError(err);
    }
};

DEFINE_ECHOBINARYTDF_CREATE()

} //Arson
} //Blaze
