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

#include <np/np_webapi.h>

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

#if defined(EA_PLATFORM_PS4_CROSSGEN)
const char8_t* LoginStateInitConsole::HEADER_STRING_XGEN = "ps5_online_ticket: ";
#else
const char8_t* LoginStateInitConsole::HEADER_STRING = "ps4_accountId: %\" PRIu64 \"\r\nps4_ticket: %s";
#endif

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

            // We got the PS4 ticket from Sony. Now, lets send it to Nucleus for authentication.
            SceNpAccountId userAccountId;
            NetConnStatus('acct', mSession.mDirtySockUserIndex, &userAccountId, sizeof(SceNpAccountId));

#if defined(EA_PLATFORM_PS4_CROSSGEN)
            size_t headerSize = strlen(HEADER_STRING_XGEN) + ticketSize + 1;
            char8_t *header = BLAZE_NEW_ARRAY(char8_t, headerSize, MEM_GROUP_LOGINMANAGER_TEMP, "header");
            blaze_snzprintf(header, headerSize, "%s%s", HEADER_STRING_XGEN, ticketData);
#else
            size_t headerSize = HEADER_STRING_SIZE + ticketSize + EA::StdC::kUint64MinCapacity + 1;

            char8_t *header = BLAZE_NEW_ARRAY(char8_t, headerSize, MEM_GROUP_LOGINMANAGER_TEMP, "header");
            blaze_snzprintf(header, headerSize, "ps4_accountId: %" PRIu64 "\r\nps4_ticket: %s", userAccountId, ticketData);
#endif
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
        // If we ended up here, something is very wrong. PS4 login flow should only
        // require a ticket while doing the Nucleus authentication step (LOGIN_MODE_NUCLEUS_AUTH_CODE)
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::onRequestTicketDone()] The LoginMode is not LOGIN_MODE_NUCLEUS_AUTH_CODE.  This should never happen on PS4.\n");
    }
}

BlazeError LoginStateInitConsole::setExternalLoginData(Authentication::LoginRequest &req)
{
    SceNpAccountId accountId;
    if (NetConnStatus('acct', mSession.mDirtySockUserIndex, &accountId, sizeof(accountId)) < 0)
    {
        // we were able to successfully obtain the ticket before we got here but if we are not able
        // to get the soid associated with the ticket, the ticket must have been invalid
        BLAZE_SDK_DEBUGF("ERROR: onRequestTicketDone() failed on NetConnStatus('acct') after having successfully obtained the psn ticket.  This should not happen.");
        return SDK_ERR_INVALID_NP_TICKET;
    }

    req.getPlatformInfo().getExternalIds().setPsnAccountId(accountId);

    return ERR_OK;
}

} // LoginManager
} // Blaze
