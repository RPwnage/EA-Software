/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"

// associationlists includes
#include "associationlists/tdf/associationlists.h"
#include "associationlistsslaveimpl.h"
#include "associationlists/rpc/associationlistsslave/getlists_stub.h"


namespace Blaze
{
namespace Association
{

class GetListsCommand : public GetListsCommandStub
{
public:
    GetListsCommand(Message* message, Blaze::Association::GetListsRequest* request, AssociationListsSlaveImpl* componentImpl)
        : GetListsCommandStub(message, request), 
        mComponent(componentImpl)
    {

    }

    ~GetListsCommand() override
    {
        // we clear this vector so that the EA::TDF::Tdf object doesn't try to delete/free the references it contains
        // which are references to persitent data on the server
        for (ListMembersVector::iterator itor = mResponse.getListMembersVector().begin(); itor != mResponse.getListMembersVector().end(); itor++)
            (*itor)->getListMemberInfoVector().clear();
    }

/* Private methods *******************************************************************************/
private:
    GetListsCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetListsCommandStub].execute()");
        BlazeRpcError result = Blaze::ERR_OK;

        // given the incoming ListInfoVector, make a ListIdentificationVector to be used later
        ListIdentificationVector requestedLists;        
        for (ListInfoVector::const_iterator it = mRequest.getListInfoVector().begin(),
             itend =  mRequest.getListInfoVector().end(); it != itend; it++)
        {
            ListIdentification *listIdentification = requestedLists.pull_back();
            (*it)->getId().copyInto(*listIdentification);
        }
        
        // load the requested lists, and push each one into the responses ListMembersVector
        AssociationListVector tempListsVector;
        result = mComponent->loadLists(requestedLists, gCurrentUserSession->getBlazeId(), tempListsVector, mRequest.getMaxResultCount(), mRequest.getOffset()); 
        if (result == Blaze::ERR_OK)
        {
            for (AssociationListVector::iterator itor = tempListsVector.begin(); result == ERR_OK && itor != tempListsVector.end(); ++itor)
            {
                AssociationListRef listRef = *itor;
                ListMembers* rspListMembers = mResponse.getListMembersVector().pull_back();
                listRef->getInfo().copyInto(rspListMembers->getInfo());
                rspListMembers->setTotalCount(listRef->getTotalCount());  
                rspListMembers->setOffset(mRequest.getOffset());    // just echo back offset
                result = listRef->fillOutMemberInfoVector(rspListMembers->getListMemberInfoVector(), gCurrentUserSession->getClientPlatform(), mRequest.getOffset(), mRequest.getMaxResultCount());
            }

            // all of the requested lists have been loaded, iterate through them to un/subscribe as requested by the user
            ListInfoVector::const_iterator it = mRequest.getListInfoVector().begin();
            ListInfoVector::const_iterator itend = mRequest.getListInfoVector().end();
            for ( ; it != itend; ++it)
            {
                const ListInfo *listInfo = *it;
                AssociationListRef list = mComponent->getList(listInfo->getId(), gCurrentUserSession->getBlazeId());
                if (list != nullptr)
                {
                    if (listInfo->getStatusFlags().getSubscribed())
                    {
                        list->subscribe();
                        TRACE_LOG("[GetListsCommandStub].execute() Subscribed to association list [" << listInfo->getId().getListName() 
                            << ":" << listInfo->getId().getListType() << "] owned by user [" << gCurrentUserSession->getBlazeId() << "]");
                    }
                    else
                    {
                        list->unsubscribe();
                        TRACE_LOG("[GetListsCommandStub].execute() Unsubscribed from association list [" << listInfo->getId().getListName() 
                            << ":" << listInfo->getId().getListType() << "] owned by user [" << gCurrentUserSession->getBlazeId() << "]");
                    }

                    // Update the List info after we've changed subscription:
                    ListMembersVector::const_iterator respListIter = mResponse.getListMembersVector().begin();
                    ListMembersVector::const_iterator respListIterEnd = mResponse.getListMembersVector().end();
                    for ( ; respListIter != respListIterEnd; ++respListIter)
                    {
                        if ((*respListIter)->getInfo().getId().getListType() == list->getInfo().getId().getListType() || 
                            blaze_stricmp((*respListIter)->getInfo().getId().getListName(), list->getInfo().getId().getListName()) == 0)
                        {
                            list->getInfo().copyInto((*respListIter)->getInfo());
                            break;
                        }
                    }
                }
                else
                {
                    // this should never happen
                    ERR_LOG("[GetListsCommandStub].execute() An association list that should have been loaded was not found: BlazeId [" 
                        << gCurrentUserSession->getBlazeId() << "], ListType [" << listInfo->getId().getListType() << "]");
                }
            }
        }
        else
        {
            ERR_LOG("[GetListsCommandStub].execute() Failed to load one or all requested association lists for user [" 
                << gCurrentUserSession->getBlazeId() << "]: ERROR [" << (ErrorHelp::getErrorName(result)) << "]");
        }

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_GETLISTS_CREATE()

} // Association
} // Blaze
