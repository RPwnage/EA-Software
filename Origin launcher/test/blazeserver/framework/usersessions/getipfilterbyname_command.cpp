/*************************************************************************************************/
/*!
    \file   getipfilterbyname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetIpFilterByNameCommand 
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "blazerpcerrors.h"
#include "framework/blaze.h"
#include "framework/logger.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getipfilterbyname_stub.h"

#include "framework/connection/socketutil.h"
#include "framework/dynamicinetfilter/dynamicinetfilter.h"

namespace Blaze
{

    class GetIpFilterByNameCommand : public GetIpFilterByNameCommandStub
    {
    public:

        GetIpFilterByNameCommand(Message* message, Blaze::GetIpFilterByNameRequest* request, UserSessionsSlave* componentImpl)
            : GetIpFilterByNameCommandStub(message, request),
              mComponent(componentImpl)
        {

        }

        ~GetIpFilterByNameCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetIpFilterByNameCommandStub::Errors execute() override
        {
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return ERR_SYSTEM;
            }

            // check if the current user has the permission to access InetFilter
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
            {
                BLAZE_WARN_LOG(Log::USER, "[GetIpFilterByNameCommand].execute: " << UserSessionManager::formatAdminActionContext(mMessage)
                    << ". Attempted to get IP white list info, no permission!");
                return ERR_AUTHORIZATION_REQUIRED;
            }

            InetFilter* filter = gUserSessionManager->getGroupManager()->getIpFilterByName(mRequest.getIpWhiteListName());
            if (filter)
            {
                GetIpFilterByNameResponse::CidrBlockList& subNets = mResponse.getSubNets();

                const InetFilter::FilterList& staticFilters = filter->getFilters();
                for (InetFilter::FilterList::const_iterator staticFilter = staticFilters.begin(),
                    staticFilterEnd = staticFilters.end();
                    staticFilter != staticFilterEnd;
                ++staticFilter)
                {
                    populateCidrBlock(*staticFilter, subNets.pull_back());
                }

                const InetFilter::DynamicFilterGroupList& dynamicFilterGroups = filter->getDynamicFilterGroups();
                if (!dynamicFilterGroups.empty() && gDynamicInetFilter)
                {
                    for (InetFilter::DynamicFilterGroupList::const_iterator group = dynamicFilterGroups.begin(),
                        groupEnd = dynamicFilterGroups.end();
                        group != groupEnd;
                    ++group)
                    {
                        const InetFilter::FilterList* dynamicFilterList = gDynamicInetFilter->getGroupFilter(group->c_str());
                        if (dynamicFilterList)
                        {
                            for (InetFilter::FilterList::const_iterator dynamicFilter = dynamicFilterList->begin(),
                                dynamicFilterEnd = dynamicFilterList->end();
                                dynamicFilter != dynamicFilterEnd;
                            ++dynamicFilter)
                            {
                                populateCidrBlock(*dynamicFilter, subNets.pull_back());
                            }
                        }
                    }
                }
            }

            return commandErrorFromBlazeError(Blaze::ERR_OK);
        }

        void populateCidrBlock(const InetFilter::Filter& ipWhiteListFilter, CidrBlock* ipBlock)
        {
            char8_t ipStr[Blaze::MAX_IP_LENGTH+1] = {0};
            blaze_snzprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", NIPQUAD(ipWhiteListFilter.ip));
            ipBlock->setIp(ipStr);

            uint32_t prefixLength = ipWhiteListFilter.mask;
            if (prefixLength != 0)
            {
                prefixLength = ntohl(ipWhiteListFilter.mask);
                prefixLength = (prefixLength^0xffffffffU) + 1;
                prefixLength = Blaze::Log2(prefixLength);
                prefixLength = 32 - prefixLength;
            }

            ipBlock->setPrefixLength(prefixLength);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETIPFILTERBYNAME_CREATE_COMPNAME(UserSessionManager)

} // Blaze
