/*************************************************************************************************/
/*!
    \file   Localizestring_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/localizestring_stub.h"


#include "framework/util/localization.h"


namespace Blaze
{
namespace Arson
{
class LocalizeStringCommand : public LocalizeStringCommandStub
{
public:
    LocalizeStringCommand(
        Message* message, Blaze::Arson::LocalizeStringReq* request, ArsonSlaveImpl* componentImpl)
        : LocalizeStringCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~LocalizeStringCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    LocalizeStringCommandStub::Errors execute() override
    {
        const uint32_t MAX_NUM_OF_PARAMS = 3;
        const uint32_t ARG_SIZE = 1024;

        uint32_t const NUM_OF_PARAMS = mRequest.getParams().size();

        Blaze::Arson::StringIdParamList::const_iterator it = mRequest.getParams().begin();
        Blaze::Arson::StringIdParamList::const_iterator itEnd = mRequest.getParams().end();

        char localizedString[ARG_SIZE];
        char arg1[ARG_SIZE];
        char arg2[ARG_SIZE];
        char arg3[ARG_SIZE];

        if (NUM_OF_PARAMS > 0 && NUM_OF_PARAMS <= MAX_NUM_OF_PARAMS)
        {
            //1st param
            if (it != itEnd)
            {               
                blaze_strnzcpy(arg1, it->c_str(), ARG_SIZE);
                it++;
            }

            //2nd param
            if (it != itEnd)
            {               
                blaze_strnzcpy(arg2, it->c_str(), ARG_SIZE);
                it++;
            }

            //3rd param
            if (it != itEnd)
            {               
                blaze_strnzcpy(arg3, it->c_str(), ARG_SIZE);
                it++;
            }
        }

        localizedString[0] = '\0';
        switch (NUM_OF_PARAMS)
        {
            case 1:
                gLocalization->localize(mRequest.getStringId(), mRequest.getLocale(), localizedString, ARG_SIZE, &arg1);
                break;

            case 2:
                gLocalization->localize(mRequest.getStringId(), mRequest.getLocale(), localizedString, ARG_SIZE, &arg1, &arg2);
                break;

            case 3:
                gLocalization->localize(mRequest.getStringId(), mRequest.getLocale(), localizedString, ARG_SIZE, &arg1, &arg2, &arg3);
                break;
        }

        mResponse.setLocalizedString(localizedString);
    
        if (localizedString[0] != '\0')
        {
            return ERR_OK;
        }
        else            
        {
            return ERR_SYSTEM;
        }
    }

    static Errors arsonError(BlazeRpcError error);
};

LocalizeStringCommandStub* LocalizeStringCommandStub::create(Message *msg, Blaze::Arson::LocalizeStringReq* request, ArsonSlave *component)
{
    return BLAZE_NEW LocalizeStringCommand(msg, request, static_cast<ArsonSlaveImpl*>(component));
}

LocalizeStringCommandStub::Errors LocalizeStringCommand::arsonError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
  //      case Blaze::STATS_ERR_UNKNOWN_CATEGORY: result = ARSON_ERR_STATS_UNKNOWN_CATEGORY; break;
        //case Blaze::STATS_ERR_BAD_PERIOD_TYPE: result = ARSON_ERR_STATS_BAD_PERIOD_TYPE; break;
  //      case Blaze::STATS_ERR_BAD_SCOPE_INFO: result = ARSON_ERR_STATS_BAD_SCOPE_INFO; break;
  //      case Blaze::STATS_ERR_STAT_NOT_FOUND: result = ARSON_ERR_STAT_NOT_FOUND; break;
  //      case Blaze::STATS_ERR_INVALID_UPDATE_TYPE: result = ARSON_ERR_STATS_INVALID_UPDATE_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[LocalizeStringCommand].arsonErrorFromStatsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
