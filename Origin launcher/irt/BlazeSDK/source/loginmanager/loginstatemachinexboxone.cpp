/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internal/loginmanager/loginstatemachineconsole.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "BlazeSDK/shared/framework/locales.h"
#include "BlazeSDK/blazesdkdefs.h"

#include "DirtySDK/proto/protohttp.h"

#include "um/sysinfoapi.h"

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

/*! ***********************************************************************************************/
/*! \name LoginStateInitConsole methods
***************************************************************************************************/
void LoginStateInitConsole::onRequestTicketDone(const uint8_t *ticketData, int32_t ticketSize)
{
    if (getLoginMode() == LOGIN_MODE_NUCLEUS_AUTH_CODE)
    {
        if (ticketData != nullptr)
        {
            BlazeError err = setExternalLoginData(mLoginRequest);
            if (err != ERR_OK)
            {
                failLogin(err, true);
                return;
            }

            // We got the STS token from MS. Now, lets send it to Nucleus for authentication.
            const char8_t *headerName = "x720token: ";
            size_t headerNameSize = strlen(headerName);

            char8_t *header = BLAZE_NEW_ARRAY(char8_t, headerNameSize + ticketSize + 1, MEM_GROUP_LOGINMANAGER_TEMP, "header");
            strcpy(header, headerName);
            strncat(header, (const char8_t *)ticketData, ticketSize);
            header[headerNameSize + ticketSize] = '\0';

            doNucleusLogin(header);

            BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, header);
        }
        else
        {
            // We failed to get an STS token from MS.
            failLogin(SDK_ERR_INVALID_XBL_TICKET);
        }
    }
    else
    {
        // If we ended up here, something is very wrong. Xbox One login flow should only
        // require a ticket while doing the Nucleus authentication step (LOGIN_MODE_NUCLEUS_AUTH_CODE)
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::onRequestTicketDone()] The LoginMode is not LOGIN_MODE_NUCLEUS_AUTH_CODE.  This should never happen on Xbox One.\n");
    }
}

BlazeError LoginStateInitConsole::setExternalLoginData(Authentication::LoginRequest &req)
{
    XUID xuid;

    if (NetConnStatus('xuid', mSession.mDirtySockUserIndex, &xuid, sizeof(xuid)) != 0)
    {
        return SDK_ERR_NO_CONSOLE_ID;
    }
    req.getPlatformInfo().getExternalIds().setXblAccountId(xuid);
    return ERR_OK;
}

} // LoginManager
} // Blaze
