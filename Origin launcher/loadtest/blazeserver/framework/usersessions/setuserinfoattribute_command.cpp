/*************************************************************************************************/
/*!
    \file   setuserinfoattribute_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetUserInfoAttributeCommand 

    Set the user info attribute value with the supplied mask on given user(s).

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/rpc/usersessionsslave/setuserinfoattribute_stub.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
    class SetUserInfoAttributeCommand : public SetUserInfoAttributeCommandStub
    {
    public:

        SetUserInfoAttributeCommand(Message* message, Blaze::SetUserInfoAttributeRequest* request, UserSessionsSlave* componentImpl)
            : SetUserInfoAttributeCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~SetUserInfoAttributeCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetUserInfoAttributeCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[SetUserInfoAttributeCommand].start()");

            SetUserInfoAttributeCommandStub::Errors err = ERR_OK;

            if (EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            ObjectIdList &blazeObjectIdList = mRequest.getBlazeObjectIdList();

            if (blazeObjectIdList.size() == 0)
            {
                err = USER_ERR_INVALID_PARAM;
            }
            else
            {
                BlazeIdVector blazeIdLookups(BlazeStlAllocator("UserInfoAttributeIds", UserSessionsSlave::COMPONENT_MEMORY_GROUP));
                blazeIdLookups.reserve(blazeObjectIdList.size());

                ObjectIdList::const_iterator idIter, idEnd;
                idIter = blazeObjectIdList.begin();
                idEnd = blazeObjectIdList.end();

                for (; idIter != idEnd; ++idIter)
                {
                    const EA::TDF::ObjectId blazeObjectId = *idIter;
                    blazeIdLookups.push_back(static_cast<BlazeId>(blazeObjectId.id));
                }
                
                bool needPermissionCheck = false;
                ClientPlatformType platform = ClientPlatformType::INVALID;
                
                if (blazeIdLookups.size() > 1)
                {
                    needPermissionCheck = true;
                }
                else if (blazeIdLookups.size() == 1)
                {
                    BlazeIdVector::const_iterator it = blazeIdLookups.begin();
                    
                    if (gCurrentUserSession->getBlazeId() == *it)
                    {
                        platform = gCurrentUserSession->getPlatformInfo().getClientPlatform();
                    }
                    else
                    {
                        needPermissionCheck = true;
                    }
                }

                if (needPermissionCheck)
                {
                    // check if the current user has the permission to update User's attribute
                    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_ATTRIBUTE))
                    {
                        BLAZE_WARN_LOG(Log::USER, "[SetUserInfoAttributeCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                         << " attempted to update other users' attribute, no permission!" );

                        return ERR_AUTHORIZATION_REQUIRED;
                    }
                }

                if (gUserSessionManager->updateUserInfoAttribute(blazeIdLookups, 
                    mRequest.getAttributeBits(), mRequest.getMaskBits(), platform) != Blaze::ERR_OK) 
                {
                    err = ERR_SYSTEM;
                }
                
                if (needPermissionCheck)
                {
                    BLAZE_INFO_LOG(Log::USER, "[SetUserInfoAttributeCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                        << " updated other users' attribute , had permission!" );
                }
            }

            return err;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_SETUSERINFOATTRIBUTE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
