/*************************************************************************************************/
/*!
    \file   removeandbanmember_commandbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoveAndBanMemberCommandBase

    Abstract class that defines the bulk of functionality for Remove/Ban/UnbanMemberCommand classes.

    \note
        Defines 3 steps used for member removal/banning/unbanning: 
            - removeMember() removes a member record from the clubs_members table if necessary 
              pre conditions are met (authorization level etc.), and also performs additional
              actions required in some situations (like club deletion if the last member gets 
              removed)
            - banMember() inserts/deletes a ban record into/from the clubs_bans table also based
              on necessary perconditions (this function also works for unbanning using a flag).
        Implements the Template Method design pattern in executeGeneric(). Subclasses
        (Remove/Ban/UnbanMemberCommand) need to simply override the pure virtual executeConcrete() 
        to call necessary steps from the 3 above - all DB houskeeping is taken care of in 
        executeGeneric().
*/
/*************************************************************************************************/

#ifndef BLAZE_REMOVEANDBANMEMBER_COMMANDBASE_H
#define BLAZE_REMOVEANDBANMEMBER_COMMANDBASE_H

/*** Include Files *******************************************************************************/

// clubs includes
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class RemoveAndBanMemberCommandBase : public ClubsCommandBase
    {
    public:
    
        ~RemoveAndBanMemberCommandBase() override
        {
        }

    protected:

        enum BanAction
        {
            BAN_ACTION_NONE   = 0,
            BAN_ACTION_BAN    = 1,
            BAN_ACTION_UNBAN  = 2
        };

        // This class is supposed to be used only as a base for subclasses.
        RemoveAndBanMemberCommandBase(ClubsSlaveImpl*   componentImpl,
                                      const char8_t*    cmdName);

        BlazeRpcError         executeGeneric(ClubId clubId);
        virtual BlazeRpcError executeConcrete(ClubsDbConnector* dbc) = 0;

        BlazeRpcError removeMember(ClubId               toremClubId, 
                                   BlazeId              toremBlazeId, 
                                   ClubsDbConnector*    dbc);

        BlazeRpcError banMember(ClubId              banClubId, 
                                BlazeId             banBlazeId, 
                                BanAction           banAction, 
                                ClubsDbConnector*   dbc);

    private:
        const char8_t*       mCmdName;
    };

} // Clubs
} // Blaze

#endif // BLAZE_REMOVEANDBANMEMBER_COMMANDBASE_H
