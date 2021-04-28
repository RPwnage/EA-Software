/*************************************************************************************************/
/*!
    \file   addpointstowallet_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"

// includes
#include "arson/rpc/arsonslave/clearassociationlistsforusers_stub.h"
#include "arsonslaveimpl.h"

#include "associationlists/associationlistsslaveimpl.h"

namespace Blaze
{
namespace Arson
{

////////////////////////////////////////////////////////
class ClearAssociationListsForUsersCommand : public ClearAssociationListsForUsersCommandStub
{
public:
    ClearAssociationListsForUsersCommand(Message* message, ClearAssociationListRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearAssociationListsForUsersCommandStub(message, request), mComponent(componentImpl)
    {
    }

    /* Private methods *******************************************************************************/
private:

    ClearAssociationListsForUsersCommandStub::Errors execute() override
    {
        BlazeRpcError result = ERR_OK;

        if (mRequest.getUsers().empty())
            return ERR_OK;

        Component* comp = gController->getComponent(Blaze::Association::AssociationListsSlave::COMPONENT_ID);
        if (comp == nullptr)
            return ERR_SYSTEM;

        Blaze::Association::AssociationListsSlaveImpl* assocComp = (Blaze::Association::AssociationListsSlaveImpl*) comp;

        //There's no reason to have a bulk "find all non-empty lists" commands for a set of users, so we have to build our own here
        DbConnPtr conn = gDbScheduler->getReadConnPtr(assocComp->getDbId());
        if (conn == nullptr)
        {
            FAIL_LOG("[ClearAssociationListsForUsers].execute: Cannot get DB conn");
            return commandErrorFromBlazeError(ERR_DB_SYSTEM);
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT blazeid, assoclisttype, listsize FROM user_association_list_size WHERE blazeid IN (");

        for (ClearAssociationListRequest::BlazeIdList::const_iterator itr = mRequest.getUsers().begin(), end = mRequest.getUsers().end(); itr != end; )
        {
            BlazeId id = *itr++;
            query->append("$D$s", id, itr != end ? "," : ")");
        }
       
        DbResultPtr dbResult;
        if ((result = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            FAIL_LOG("[ClearAssociationListsForUsers].execute: failed to query size table " << getDbErrorString(result) << ".");      
            return commandErrorFromBlazeError(result);
        }

        //Now iterate the results and clear out any non-zero tables
        for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr !=end; ++itr)
        {
            uint32_t size = (*itr)->getUInt(2);

            if (size > 0)
            {
                Association::ListIdentification listId;
                listId.setListType((*itr)->getUShort(1));
                Association::AssociationListRef list = assocComp->findOrCreateList(listId, (*itr)->getUInt64((uint32_t) 0));
                if (list != nullptr && !list->getInfo().getStatusFlags().getPaired())
                {
                    result = list->clearMembers(gCurrentUserSession != nullptr ? gCurrentUserSession->getClientPlatform() : ClientPlatformType::INVALID);
                }
            }           
        }

        return commandErrorFromBlazeError(result);
    }

private:
    // Not owned memory.
    ArsonSlaveImpl* mComponent;
};

// static factory method impl
DEFINE_CLEARASSOCIATIONLISTSFORUSERS_CREATE()


} // Arson
} // Blaze
