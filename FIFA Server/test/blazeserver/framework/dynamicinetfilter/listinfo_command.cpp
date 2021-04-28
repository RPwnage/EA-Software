/*************************************************************************************************/
/*!
    \file   listinfo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListInfoCommand

    Retrieves a list of some or all entries in the DynamicInetFilter.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave/listinfo_stub.h"

#include "framework/dynamicinetfilter/dynamicinetfilterslaveimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{

class ListInfoCommand : public ListInfoCommandStub
{
public:
    ListInfoCommand(Message* message, ListRequest* request, DynamicInetFilterSlaveImpl* componentImpl);
    ~ListInfoCommand() override {}
    
private:
    // Not owned memory.
    DynamicInetFilterSlaveImpl* mComponent;

    // States
    ListInfoCommandStub::Errors execute() override;
};

DEFINE_LISTINFO_CREATE();

ListInfoCommand::ListInfoCommand(
    Message* message, ListRequest* request, DynamicInetFilterSlaveImpl* componentImpl)
    : ListInfoCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/    

ListInfoCommandStub::Errors ListInfoCommand::execute()
{
    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[ListInfoCommand].execute: Attempted-" 
        << UserSessionManager::formatAdminActionContext(mMessage) << ", req = \n" << mRequest);

    // check if the current user has the permission to access DynamicInetFilter
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::DYNAMIC_INET, "[ListInfoCommand].execute: " << UserSessionManager::formatAdminActionContext(mMessage)
            << ". Attempted to get list info, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    // validate inputs: subnet, if specified, must be valid, and strings cannot contain arbitrary characters
    if (!DynamicInetFilterImpl::validateString(mRequest.getGroup(), true))
        return DYNAMICINETFILTER_ERR_INVALID_GROUP;
    if (!DynamicInetFilterImpl::validateString(mRequest.getOwner(), true))
        return DYNAMICINETFILTER_ERR_INVALID_OWNER;
    if (!DynamicInetFilterImpl::validateSubNet(mRequest.getSubNet(), true))
        return DYNAMICINETFILTER_ERR_INVALID_SUBNET;

    bool hasRowId = (mRequest.getRowId() != INVALID_ROW_ID);
    bool hasGroup = (mRequest.getGroup()[0] != 0);
    bool hasOwner = (mRequest.getOwner()[0] != 0);
    bool hasSubNet = (mRequest.getSubNet().getPrefixLength() != INVALID_PREFIX_LENGTH);

    InetFilter::Filter filter;
    DynamicInetFilterImpl::generateFilter(mRequest.getSubNet(), filter);

    // Iterate through all groups, and queue up all entries which match the specified list criteria
    for (DynamicInetFilterImpl::InetFilterGroups::const_iterator group = mComponent->getInetFilterGroups().begin(),
        end = mComponent->getInetFilterGroups().end();
        group != end;
        ++group)
    {
        // Is a group filter specified, and this is not the right group?
        if (hasGroup && mRequest.getGroup() != group->mGroup)
            continue;

        for (uint32_t i = 0, endIt = group->mEntries.size(); i != endIt; ++i)
        {
            const Entry& entry = *group->mEntries[i];

            // Is a row filter specified, and this is not the right row?
            if (hasRowId && mRequest.getRowId() != entry.getRowId())
                continue;
            // Is an owner filter specified, and this is not the right owner?
            if (hasOwner && strcmp(mRequest.getOwner(), entry.getOwner()))
                continue;
            // Is a subnet filter specified, and this filter does not overlap with the specified subnet filter?
            if (hasSubNet)
            {
                const InetFilter::Filter& entryFilter = group->mFilters[i];
                uint32_t combinedMask = filter.mask & entryFilter.mask;
                if ((filter.ip & combinedMask) != (entryFilter.ip & combinedMask))
                    continue;
            }

            // Enqueue entry in response
            mResponse.getEntries().push_back(entry.clone());
        }
    }

    return ERR_OK;
}

}
} // Blaze
