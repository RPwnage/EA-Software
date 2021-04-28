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

#include <np.h>

#include <np/np_webapi2.h>

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

const char8_t* LoginStateInitConsole::HEADER_STRING = "ps5_online_ticket: ";

/*! ***********************************************************************************************/
/*! \name LoginStateInitConsole methods
***************************************************************************************************/
void LoginStateInitConsole::onRequestTicketDone(const uint8_t *ticketData, int32_t ticketSize)//Note: This file doesn't work using regular NucleusSDK based flow and is deprecated. Will likely be deleted going forward.
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

            // We got the PS5 ticket from Sony. Now, lets send it to Nucleus for authentication.
            SceNpAccountId userAccountId;
            NetConnStatus('acct', mSession.mDirtySockUserIndex, &userAccountId, sizeof(SceNpAccountId));
            
            size_t headerSize = strlen(HEADER_STRING) + ticketSize + 1;

            char8_t *header = BLAZE_NEW_ARRAY(char8_t, headerSize, MEM_GROUP_LOGINMANAGER_TEMP, "header");
            blaze_snzprintf(header, headerSize, "%s%s", HEADER_STRING, ticketData);
            header[headerSize - 1] = '\0';

            doNucleusLogin(header);

            BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, header);
        }
        else
        {
            failLogin(SDK_ERR_INVALID_NP_TICKET);
        }
    }
    else
    {
        // If we ended up here, something is very wrong. PS5 login flow should only
        // require a ticket while doing the Nucleus authentication step (LOGIN_MODE_NUCLEUS_AUTH_CODE)
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::onRequestTicketDone()] The LoginMode is not LOGIN_MODE_NUCLEUS_AUTH_CODE.  This should never happen on PS5.\n");
    }
}

BlazeError LoginStateInitConsole::setExternalLoginData(Authentication::LoginRequest &req)
{
    SceNpAccountId accountId;
    if (NetConnStatus('acct', mSession.mDirtySockUserIndex, &accountId, sizeof(accountId)) < 0)
    {
        // we were able to successfully obtain the ticket before we got here but if we are not able
        // to get the soid associated with the ticket, the ticket must have been invalid
        BLAZE_SDK_DEBUGF("ERROR: setExternalLoginData() failed on NetConnStatus('acct') after having successfully obtained the psn ticket.  This should not happen.");
        return SDK_ERR_INVALID_NP_TICKET;
    }

    req.getPlatformInfo().getExternalIds().setPsnAccountId(accountId);

    return ERR_OK;
}

} // LoginManager
} // Blaze
