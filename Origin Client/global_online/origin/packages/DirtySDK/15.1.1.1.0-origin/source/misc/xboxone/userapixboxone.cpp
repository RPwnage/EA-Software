/*H********************************************************************************/
/*!
    \File userapixboxone.cpp

    \Description
        Implements the xboxone specific code to request UserProfiles, and parse the responses.

    \Copyright
        Copyright (c) Electronic Arts 2012

    \Version 1.0 25/04/2013 (amakoukji) First Version
*/
/********************************************************************************H*/

/*** Include files ***********************************************************/

#include "DirtySDK/platform.h"

#include <xdk.h>
#include <collection.h>

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/utf8.h"
#include "userapipriv.h"

#using "Microsoft.Xbox.Services.winmd"
using namespace Platform;
using namespace Windows::Data;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Xbox::System;
using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Social;
using namespace Microsoft::Xbox::Services::Presence;
using namespace Microsoft::Xbox::Services::RealTimeActivity;

/*** Defines ***************************************************************************/

#define USERAPI_MASK_NOTIFY_PRESENCE (0x10)
#define USERAPI_MASK_NOTIFY_TITLE    (0x20)
#define USERAPI_MASK_FRIENDS         (0x40)
#define USERAPI_MASK_POST            (0x80)

#define USERAPI_ASYNC_TIMOUT_MS      (20000) // 20 seconds

#define USERAPI_DISPLAY_PIC_SIZE_SMALL  "&w=64&h=64"
#define USERAPI_DISPLAY_PIC_SIZE_MEDIUM "&w=208&h=208"
#define USERAPI_DISPLAY_PIC_SIZE_LARGE  "&w=424&h=424"

#define HOME_MENU_TITLE_ID           (714681658) 

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef enum UserApiLevelXb1NotificationState
{
    NOTIFICATION_STATE_ERROR = -1,
    NOTIFICATION_STATE_IDLE = 0,
    NOTIFICATION_STATE_CONNECTING_TO_RTA,
    NOTIFICATION_STATE_CONNECTED_TO_RTA,
    NOTIFICATION_STATE_QUERY_FRIENDS,
    NOTIFICATION_STATE_FRIEND_RECEIVED,
    NOTIFICATION_STATE_SIGNUP,
    NOTIFICATION_STATE_SIGNUP_COMPLETE
} UserListApiLevelPS4E;

typedef struct UserApiPlatformUserContextT
{
    XboxLiveContext ^refXblUserContext;

    // Async operations
    IAsyncOperation<IVectorView<XboxUserProfile^>^> ^refAsyncOpProfile;
    IAsyncOperation<PresenceRecord^> ^refAsyncOpPresence;
    IAsyncAction ^refAsyncOpPostRichPresence;
    IAsyncOperation<RealTimeActivity::RealTimeActivityDevicePresenceChangeSubscription^> ^refAsyncOpNotifyPresence;
    IAsyncOperation<RealTimeActivity::RealTimeActivityTitlePresenceChangeSubscription^>  ^refAsyncOpNotifyTitle;

    // Results
    IVectorView<XboxUserProfile^> ^refResultsProfiles;
    PresenceRecord                ^refResultsPresence;
    UserApiEventErrorE            eResultRichPresencePost;

    // timers
    uint32_t uRtaConnectionTimer;
    uint32_t uOpProfileTimer;
    uint32_t uOpPresenceTimer;
    uint32_t uOpFriendTimer;
    uint32_t uOpPostRichPresenceTimer;
    uint32_t uOpNotifyPresenceTimer;
    uint32_t uOpNotifyTitleTimer;

    // variables for long version of notification signup
    IAsyncOperation<XboxSocialRelationshipResult^>  ^refAsyncOpFriend;
    XboxSocialRelationshipResult^ refFriendResult;
    UserApiLevelXb1NotificationState NotificationSignUpState;
    UserApiNotifyTypeE eType;
    bool bConnectedToRTA;
    uint32_t uExpectedNotificationResults;
    uint32_t uReceivedNotificationResults;
    bool     bSignUpAsyncFree;

} UserApiPlatformUserContextT;

struct UserApiPlatformDataT
{
    char pAvatarSize;
    UserApiPlatformUserContextT aUserContext[NETCONN_MAXLOCALUSERS];
    uint32_t bAsyncInProgress[NETCONN_MAXLOCALUSERS];
    uint32_t uNotificationTitle;
};

/*** Function Prototypes ***************************************************************/

static void _UserApiTitlePresenceEvent(UserApiRefT *pRef, uint32_t uUserIndex, RealTimeActivityTitlePresenceChangeEventArgs^ refArgs);
static void _UserApiConsolePresenceEvent(UserApiRefT *pRef, uint32_t uUserIndex, RealTimeActivityDevicePresenceChangeEventArgs^ refArgs);
static void _UserApiSignupForConsolePresence(UserApiRefT *pRef, uint32_t uUserIndex);
static void _UserApiSignupForTitlePresence(UserApiRefT *pRef, uint32_t uUserIndex);
static void _UserApiQueryFriends(UserApiRefT *pRef, uint32_t uUserIndex);
static void _UserApiConnectToRTA(UserApiRefT *pRef, uint32_t uUserIndex);
static void _UserApiRegisterProfileUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex);
uint8_t _UserApiInitializeContext(UserApiRefT *pRef, uint32_t uUserIndex);
PresenceTitleRecord^ _UserApiGetPriorityTitle(IVectorView<PresenceTitleRecord^>^ presenceTitleRecordList);

/*** Variables *************************************************************************/

/*** Private functions ******************************************************/

/*F*************************************************************************************************/
/*!
    \Function _UserApiTitlePresenceEvent

    \Description
        Handler for title presence notification from Microsoft.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request
    \Input refArgs    - details about the event


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiTitlePresenceEvent(UserApiRefT *pRef, uint32_t uUserIndex, RealTimeActivityTitlePresenceChangeEventArgs^ refArgs)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    UserApiNotificationT (*pList)[USERAPI_NOTIFY_LIST_MAX_SIZE] = &pRef->TitleNotification;
    int32_t i = 0;
    UserApiNotifyDataT data;
    Platform::String ^stringXuid = refArgs->Subscription->XboxUserId;
    uint64_t uXuid;

    NetPrintf(("userapixboxone: user %S presence notification\n", refArgs->Subscription->XboxUserId->Data()));
    uXuid = _wcstoui64(stringXuid->Data(), NULL, 10);
    DirtyUserFromNativeUser(&data.TitleData.DirtyUser, &uXuid);
    data.TitleData.uTitleId = refArgs->Subscription->TitleId;
    data.TitleData.uUserIndex = uUserIndex;
    
    // trigger the callback with the appropriate type but NULL data to indicate an error
    for (i = 0; i < USERAPI_NOTIFY_LIST_MAX_SIZE; ++i)
    {
        if ((*pList)[i].pCallback != NULL)
        {
            (*pList)[i].pCallback(pRef, pUserContext->eType, &data, (*pList)[i].pUserData);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiConsolePresenceEvent

    \Description
        Handler for console presence notification from Microsoft.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request
    \Input refArgs    - details about the event


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiConsolePresenceEvent(UserApiRefT *pRef, uint32_t uUserIndex, RealTimeActivityDevicePresenceChangeEventArgs^ refArgs)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    UserApiNotificationT (*pList)[USERAPI_NOTIFY_LIST_MAX_SIZE] = &pRef->PresenceNotification;
    int32_t i = 0;
    UserApiNotifyDataT data;
    Platform::String ^stringXuid = refArgs->Subscription->XboxUserId;
    uint64_t uXuid;

    NetPrintf(("userapixboxone: user %S presence notification\n", refArgs->Subscription->XboxUserId->Data()));
    uXuid = _wcstoui64(stringXuid->Data(), NULL, 10);
    DirtyUserFromNativeUser(&data.PresenceData.DirtyUser, &uXuid);
    data.PresenceData.uUserIndex = uUserIndex;
    

    // trigger the callback with the appropriate type but NULL data to indicate an error
    for (i = 0; i < USERAPI_NOTIFY_LIST_MAX_SIZE; ++i)
    {
        if ((*pList)[i].pCallback != NULL)
        {
            (*pList)[i].pCallback(pRef, pUserContext->eType, &data, (*pList)[i].pUserData);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiSignupForConsolePresence

    \Description
        Signup for Microsoft console presence.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiSignupForConsolePresence(UserApiRefT *pRef, uint32_t uUserIndex)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];

    if (pUserContext->bSignUpAsyncFree)
    {
        pUserContext->bSignUpAsyncFree = false;

        XboxSocialRelationship^ refCurrentFriend = pUserContext->refFriendResult->Items->GetAt(pUserContext->uReceivedNotificationResults);
        pUserContext->uOpNotifyPresenceTimer = NetTick();
        pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_NOTIFY_PRESENCE;
        pUserContext->refAsyncOpNotifyPresence = pRef->pPlatformData->aUserContext[uUserIndex].refXblUserContext->RealTimeActivityService->SubscribeToDevicePresenceChangeAsync(refCurrentFriend->XboxUserId);
        pUserContext->refAsyncOpNotifyPresence->Completed = ref new AsyncOperationCompletedHandler<RealTimeActivity::RealTimeActivityDevicePresenceChangeSubscription^>
        ([pRef, pUserContext, uUserIndex] (IAsyncOperation<RealTimeActivity::RealTimeActivityDevicePresenceChangeSubscription^> ^resultTask, Windows::Foundation::AsyncStatus status) 
        {
            if (pRef->bShuttingDown != TRUE)
            {
                switch (status)
                {
                    case Windows::Foundation::AsyncStatus::Completed:
                    {
                        NetPrintf(("userapixboxone: Success UserApiPlatRegisterUpdateEvent() (RealTimeActivityDevicePresenceChangeSubscription)\n"));
                        RealTimeActivity::RealTimeActivityDevicePresenceChangeSubscription ^refResult = nullptr;
                        try
                        {
                            refResult = resultTask->GetResults();
                        }
                        catch (Platform::Exception ^e)
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestProfile() Exception thrown (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                            pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                            break;
                        }

                        ++(pUserContext->uReceivedNotificationResults);

                        if (pUserContext->uReceivedNotificationResults == pUserContext->uExpectedNotificationResults)
                        {
                            pUserContext->NotificationSignUpState = NOTIFICATION_STATE_SIGNUP_COMPLETE;
                        }

                        break;
                    }
                    case  Windows::Foundation::AsyncStatus::Error:
                    {
                        NetPrintf(("userapixboxone: An error occured in call to UserApiPlatRegisterUpdateEvent() (RealTimeActivityDevicePresenceChangeSubscription), ErrorCode=%S (0x%08x)\n", resultTask->ErrorCode.ToString()->Data(), resultTask->ErrorCode)); 
                        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                        break;
                    }

                    case Windows::Foundation::AsyncStatus::Canceled:
                    {
                        NetPrintf(("userapixboxone: UserApiPlatRegisterUpdateEvent(), async operation (RealTimeActivityDevicePresenceChangeSubscription) was cancelled\n"));
                        break;
                    }
                }
                pUserContext->bSignUpAsyncFree = true;
            }

            pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_NOTIFY_PRESENCE;
            resultTask->Close();
        });
    }
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiSignupForTitlePresence

    \Description
        Signup for Microsoft title presence.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiSignupForTitlePresence(UserApiRefT *pRef, uint32_t uUserIndex)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];

    if (pUserContext->bSignUpAsyncFree)
    {
        pUserContext->bSignUpAsyncFree = false;

        // if the title id has not been overwritten with UserApiControl('nttl'), use the current title id
        if (pRef->pPlatformData->uNotificationTitle == 0)
        {
            pRef->pPlatformData->uNotificationTitle = _wcstoui64(Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), NULL, 10);
        }

        XboxSocialRelationship^ refCurrentFriend = pUserContext->refFriendResult->Items->GetAt(pUserContext->uReceivedNotificationResults);
        pUserContext->uOpNotifyTitleTimer = NetTick();
        pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_NOTIFY_TITLE;
        pUserContext->refAsyncOpNotifyTitle = pRef->pPlatformData->aUserContext[uUserIndex].refXblUserContext->RealTimeActivityService->SubscribeToTitlePresenceChangeAsync(refCurrentFriend->XboxUserId, pRef->pPlatformData->uNotificationTitle);
        pUserContext->refAsyncOpNotifyTitle->Completed = ref new AsyncOperationCompletedHandler<RealTimeActivity::RealTimeActivityTitlePresenceChangeSubscription^>
        ([pRef, pUserContext, uUserIndex] (IAsyncOperation<RealTimeActivity::RealTimeActivityTitlePresenceChangeSubscription^> ^resultTask, Windows::Foundation::AsyncStatus status) 
        {
            if (pRef->bShuttingDown != TRUE)
            {
                switch (status)
                {
                    case Windows::Foundation::AsyncStatus::Completed:
                    {
                        NetPrintf(("userapixboxone: Success UserApiPlatRegisterUpdateEvent() (RealTimeActivityTitlePresenceChangeSubscription)\n"));
                        RealTimeActivity::RealTimeActivityTitlePresenceChangeSubscription ^refResult = nullptr;
                        try
                        {
                            refResult = resultTask->GetResults();
                        }
                        catch (Platform::Exception ^e)
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestProfile() Exception thrown (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                            pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                            break;
                        }

                        ++(pUserContext->uReceivedNotificationResults);

                        if (pUserContext->uReceivedNotificationResults == pUserContext->uExpectedNotificationResults)
                        {
                            pUserContext->NotificationSignUpState = NOTIFICATION_STATE_SIGNUP_COMPLETE;
                        }

                        break;
                    }
                    case  Windows::Foundation::AsyncStatus::Error:
                    {
                        NetPrintf(("userapixboxone: An error occured in call to UserApiPlatRegisterUpdateEvent() (RealTimeActivityTitlePresenceChangeSubscription), ErrorCode=%S (0x%08x)\n", resultTask->ErrorCode.ToString()->Data(), resultTask->ErrorCode)); 
                        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                        break;
                    }

                    case Windows::Foundation::AsyncStatus::Canceled:
                    {
                        NetPrintf(("userapixboxone: UserApiPlatRegisterUpdateEvent(), async operation (RealTimeActivityDevicePresenceChangeSubscription) was cancelled\n"));
                        break;
                    }
                }
                pUserContext->bSignUpAsyncFree = true;
            }

            pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_NOTIFY_TITLE;
            resultTask->Close();
        });
    }
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiQueryFriends

    \Description
        Query Microsoft friends list for signing up to presence notifications.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiQueryFriends(UserApiRefT *pRef, uint32_t uUserIndex)
{
    pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_FRIENDS;
    pRef->pPlatformData->aUserContext[uUserIndex].refAsyncOpFriend = pRef->pPlatformData->aUserContext[uUserIndex].refXblUserContext->SocialService->GetSocialRelationshipsAsync();
    pRef->pPlatformData->aUserContext[uUserIndex].uOpFriendTimer = NetTick();
    pRef->pPlatformData->aUserContext[uUserIndex].refAsyncOpFriend->Completed = ref new AsyncOperationCompletedHandler<XboxSocialRelationshipResult^>
    ( [pRef, uUserIndex] (IAsyncOperation<XboxSocialRelationshipResult^> ^resultTask, Windows::Foundation::AsyncStatus status) 
    {
        if (pRef->bShuttingDown != TRUE)
        {
            switch (status)
            {
                case Windows::Foundation::AsyncStatus::Completed:
                {
                    XboxSocialRelationshipResult^ refResult = nullptr;
                    try
                    {
                        pRef->pPlatformData->aUserContext[uUserIndex].refFriendResult = resultTask->GetResults();  
                        pRef->pPlatformData->aUserContext[uUserIndex].uExpectedNotificationResults = pRef->pPlatformData->aUserContext[uUserIndex].refFriendResult->TotalCount;
                        pRef->pPlatformData->aUserContext[uUserIndex].uReceivedNotificationResults = 0;
                        pRef->pPlatformData->aUserContext[uUserIndex].bSignUpAsyncFree = true;
                        pRef->pPlatformData->aUserContext[uUserIndex].NotificationSignUpState = NOTIFICATION_STATE_FRIEND_RECEIVED;
                    }
                    catch (Platform::Exception^ ex)
                    {
                        // no callback occurs in the case of an exception
                        // this is because the pRef could no longer exist
                        NetPrintf(("userapixboxone: error fetching people list, error code 0x%0.8x\n", ex->HResult));
                        pRef->pPlatformData->aUserContext[uUserIndex].refFriendResult = nullptr;
                        pRef->pPlatformData->aUserContext[uUserIndex].NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                    }
                    break;
                }
                case Windows::Foundation::AsyncStatus::Error:
                {
                    NetPrintf(("userapixboxone: An error occured in call to UserListApiGetListAsync() (friend), ErrorCode=%S (0x%08x)\n", resultTask->ErrorCode.ToString()->Data(), resultTask->ErrorCode));
                    pRef->pPlatformData->aUserContext[uUserIndex].refFriendResult = nullptr;
                    pRef->pPlatformData->aUserContext[uUserIndex].NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                    break;
                }
                case Windows::Foundation::AsyncStatus::Canceled:
                {
                    NetPrintf(("userapixboxone: friend query was cancelled\n"));
                    break;
                }
            }
        }

        pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_FRIENDS;
        resultTask->Close();
    });
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiConnectToRTA

    \Description
        Connects a specified user's context to the RealTimeActivity functionality of Microsoft.

    \Input pRef       - pointer to the UserApiRefT module
    \Input uUserIndex - The index of the user making the request


    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
void _UserApiConnectToRTA(UserApiRefT *pRef, uint32_t uUserIndex)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];

    pUserContext->refXblUserContext->RealTimeActivityService->TitlePresenceChanged += 
            ref new Windows::Foundation::EventHandler<RealTimeActivityTitlePresenceChangeEventArgs^>( 
            [pRef, uUserIndex]( Platform::Object^, RealTimeActivityTitlePresenceChangeEventArgs^ refArgs )
    {
        _UserApiTitlePresenceEvent(pRef, uUserIndex, refArgs);
    });

   pUserContext->refXblUserContext->RealTimeActivityService->DevicePresenceChanged += 
            ref new Windows::Foundation::EventHandler<RealTimeActivityDevicePresenceChangeEventArgs^>( 
            [pRef, uUserIndex]( Platform::Object^, RealTimeActivityDevicePresenceChangeEventArgs^ refArgs )
    {
        _UserApiConsolePresenceEvent(pRef, uUserIndex, refArgs);
    });

    Windows::Foundation::IAsyncAction ^action= pRef->pPlatformData->aUserContext[uUserIndex].refXblUserContext->RealTimeActivityService->ConnectAsync();
    pUserContext->uRtaConnectionTimer = NetTick();
    action->Completed = ref new AsyncActionCompletedHandler([pRef, pUserContext, uUserIndex] (IAsyncAction ^action, Windows::Foundation::AsyncStatus status)
    {
        if (status == Windows::Foundation::AsyncStatus::Completed)
        {
            NetPrintf(("userapixboxone: [p%] local user %u connected to RealTimeActivity \n", pRef, uUserIndex));
            pUserContext->bConnectedToRTA = true;
            if (pUserContext->NotificationSignUpState == NOTIFICATION_STATE_CONNECTING_TO_RTA)
            {
                pUserContext->NotificationSignUpState = NOTIFICATION_STATE_CONNECTED_TO_RTA;
            }
        }
        else
        {
            NetPrintf(("userapixboxone: [p%] local user %u failed to connect to RealTimeActivity \n", pRef, uUserIndex));
            pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
        }
    });
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiInitializeContext

    \Description
        Initialize a user context.

    \Input pRef      - pointer to the UserApiRefT module
    \Input uUserIndex          - The index of the user making the request

    \Output
        uint8_t  - TRUE for success, FALSE otherwise.

    \Version 02/14/2014 (amakoukji)
*/
/*************************************************************************************************F*/
uint8_t _UserApiInitializeContext(UserApiRefT *pRef, uint32_t uUserIndex)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    User ^user = nullptr;

    if (NetConnStatus('xusr', (int32_t)uUserIndex, &user, sizeof(user)) < 0)
    {
        return(FALSE);
    }

    try
    {
        pUserContext->refXblUserContext = ref new XboxLiveContext(user);
        pUserContext->refXblUserContext->Settings->DiagnosticsTraceLevel = XboxServicesDiagnosticsTraceLevel::Verbose;
        pUserContext->refXblUserContext->Settings->EnableServiceCallRoutedEvents = true;
        TimeSpan ts;
        ts.Duration = 100000000; // 10 seconds
        pUserContext->refXblUserContext->Settings->HttpTimeoutWindow = ts; // The total window for the request(s) to execute
        ts.Duration = 100000000; // 10 seconds
        pUserContext->refXblUserContext->Settings->HttpTimeout = ts; // The timeout for a single request
        ts.Duration = 20000000; // 2 seconds
        pUserContext->refXblUserContext->Settings->HttpRetryDelay = ts; // the delay, after a request fails before the next request.
    
        pUserContext->refXblUserContext->Settings->ServiceCallRouted += ref new EventHandler<XboxServiceCallRoutedEventArgs^>(
            [=](Platform::Object^, XboxServiceCallRoutedEventArgs^ refArgs)
            {
                // Display HTTP errors to screen for easy debugging
                NetPrintf(("userapixboxone: [XboxLiveServices] %S %S\n", refArgs->HttpMethod->Data(), refArgs->Url->AbsoluteUri->Data()));
                NetPrintf(("userapixboxone: [XboxLiveServices] %S\n", refArgs->RequestHeaders->Data()));
                NetPrintf(("userapixboxone: [XboxLiveServices] \n"));
                NetPrintf(("userapixboxone: [XboxLiveServices] %S\n", refArgs->RequestBody->Data()));
    
                NetPrintf(("userapixboxone: [XboxLiveServices] Response Status: %S\n", refArgs->HttpStatus.ToString()->Data()));
                NetPrintf(("userapixboxone: [XboxLiveServices] %S\n", refArgs->ResponseHeaders->Data()));
                NetPrintf(("userapixboxone: [XboxLiveServices] \n"));
                NetPrintf(("userapixboxone: [XboxLiveServices] %S\n", refArgs->ResponseBody->Data()));
            });
    }
    catch (Exception ^ e)
    {
        NetPrintf(("userapixboxone: (Error) unable to instantiate XboxLiveContext (%S/0x%08x)\n", e->Message->Data(), e->HResult));
        NetPrintf((
            "userlistapixboxone: WARNING!!! Microsoft.Xbox.Services.dll is likely not loaded. Please add "
            "a reference to 'XBox Services API' in your top-level project file to make this feature useable.\n"));
        return(FALSE);
    }

    return(TRUE);
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiGetPriorityTitle

    \Description
        Choose the most relevant title from the list provided.

    \Input presenceTitleRecordList - list of active title presence data

    \Output
        PresenceTitleRecord^ - Highest priority title, or nullptr if failed

    \Version 04/24/2014 (amakoukji)
*/
/*************************************************************************************************F*/
PresenceTitleRecord^ _UserApiGetPriorityTitle(IVectorView<PresenceTitleRecord^>^ presenceTitleRecordList)
{
    // Priority is chosen based on the following order, 
    // Preference for the LAST item in the list;
    // Same console, same title
    // Same console different title (not home menu)
    // Same console
    // Different console
    // Inactive
    // Note that originally players playing the same title on different consoles ranked #2 but this comparison is currently impossible without 
    // foreknowledge of the particular title's equivalent id on other platforms (they are the same title id on XB1 and X360 for example)

    uint32_t uCurrentTitleId = _wcstoui64(Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), NULL, 10);
    PresenceTitleRecord ^refPriority1 = nullptr;
    PresenceTitleRecord ^refPriority2 = nullptr;
    PresenceTitleRecord ^refPriority3 = nullptr;
    PresenceTitleRecord ^refPriority4 = nullptr;
    PresenceTitleRecord ^refPriority5 = nullptr;
    IIterator<PresenceTitleRecord^> ^refIter = nullptr;
    
    if (presenceTitleRecordList == nullptr)
    {
        return(nullptr);
    }

    refIter = presenceTitleRecordList->First();

    // rank by priorities
    while (refIter->HasCurrent)
    {
        PresenceTitleRecord ^refRecord = refIter->Current;
        if (refRecord != nullptr)
        {
            if (refRecord->IsTitleActive && refRecord->DeviceType == Microsoft::Xbox::Services::Presence::PresenceDeviceType::XboxOne && refRecord->TitleId == uCurrentTitleId )
            {
                refPriority1 = refRecord;
                break;
            }
            else if (refRecord->IsTitleActive && refRecord->DeviceType == Microsoft::Xbox::Services::Presence::PresenceDeviceType::XboxOne && refRecord->TitleId != HOME_MENU_TITLE_ID)
            {
                refPriority2 = refRecord;
            }
            else if (refRecord->IsTitleActive && refRecord->DeviceType == Microsoft::Xbox::Services::Presence::PresenceDeviceType::XboxOne)
            {
                refPriority3 = refRecord;
            }
            else if (refRecord->IsTitleActive)
            {
                refPriority4 = refRecord;
            }
            else
            {
                refPriority5 = refRecord;
            }
        }
        refIter->MoveNext();
    }


    return(refPriority1 ? refPriority1 :
           refPriority2 ? refPriority2 :
           refPriority3 ? refPriority3 :
           refPriority4 ? refPriority4 :
           refPriority5);
}

/*** Public functions ********************************************************/

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatCreateData

    \Description
        Allocates and initializes resources needed for the UserApi on Xbox One.

    \Input pRef      - pointer to the UserApiRefT module

    \Output
        UserApiPlatformDataT* - pointer to the Xbox One specific UserApiPlatformDataT, or NULL on failure.

    \Version 04/17/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
UserApiPlatformDataT *UserApiPlatCreateData(UserApiRefT *pRef)
{
    UserApiPlatformDataT *pPlatformData;
    if ((pPlatformData = (UserApiPlatformDataT*)DirtyMemAlloc(sizeof(*pPlatformData), USERAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("userapixboxone: could not allocate module state\n"));
        return(NULL);
    }

    memset(pPlatformData, 0, sizeof(*pPlatformData));
    pRef->pPlatformData = pPlatformData;

    // set the default avatar size to large (we always retrurn large before)
    pRef->pPlatformData->pAvatarSize = 'l';

    // initialize existing user contexts
    for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; ++iUser)
    {
        User ^user = nullptr;
        if (NetConnStatus('xusr', iUser, &user, sizeof(user)) < 0)
        {
            // no user at iUser
            continue;
        }

        _UserApiInitializeContext(pRef, (uint32_t)iUser);
    }

    return pPlatformData;
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatDestroyData

    \Description
        Destroys the xboxone specific resources used by the UserApi module.
        
    \Input *pRef           - pointer to UserApiT module reference.
    \Input *pPlatformData  - pointer to the UserApiPlatformDataT struct to be destroyed.

    \Output 
        int32_t         - 0 for success, negative for error

    \Version 04/17/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatDestroyData(UserApiRefT *pRef, UserApiPlatformDataT *pPlatformData)
{
    uint32_t i = 0;

    if (pPlatformData == NULL)
    {
        NetPrintf(("userapixboxone: UserApiPlatDestroyData() called while module is not active\n"));
        return(0);
    }
    else 
    {
        // check if no async operations are in progress, if they are defer shutdown
        for (; i < NETCONN_MAXLOCALUSERS; ++i)
        {
            if (pRef->pPlatformData->bAsyncInProgress[i] != 0)
            {
                return(-1);
            }
        }
    }

    pPlatformData->aUserContext->refXblUserContext = nullptr;
    pPlatformData->aUserContext->refAsyncOpProfile = nullptr;
    pPlatformData->aUserContext->refAsyncOpPresence = nullptr;
    pPlatformData->aUserContext->refAsyncOpPostRichPresence = nullptr;
    pPlatformData->aUserContext->refAsyncOpNotifyPresence = nullptr;
    pPlatformData->aUserContext->refAsyncOpNotifyTitle = nullptr;
    pPlatformData->aUserContext->refResultsProfiles = nullptr;
    pPlatformData->aUserContext->refResultsPresence = nullptr;
    pPlatformData->aUserContext->refAsyncOpFriend = nullptr;
    pPlatformData->aUserContext->refFriendResult = nullptr;

    NetPrintf(("userapixboxone: [%p] deferred destroy complete.\n", pRef));
    DirtyMemFree(pPlatformData, USERAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatRequestProfile

    \Description
        Initiates the profile request to Xbox LIVE Services
        
    \Input *pRef               - module ref
    \Input uUserIndex          - The index of the user making the request
    \Input pLookupUsers        - Array of DirtyUserT struct containing the users to lookup
    \Input iLookupUsersLength  - maximum users to return per query

    \Output 
        int32_t            - 0 for success, negative for error
        
    \Version 04/17/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRequestProfile(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers, int32_t iLookupUsersLength)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    User ^user = nullptr;

    if (pRef->pPlatformData->bAsyncInProgress[uUserIndex] & USERAPI_MASK_PROFILE)
    {
        NetPrintf(("userapixboxone: [%p] request for user %d already in progress.\n", pRef, uUserIndex));
        return(-1);
    }

    Platform::Collections::Vector<String^> ^users = ref new Platform::Collections::Vector<String^>(iLookupUsersLength);
    for (int32_t iIndex = 0; iIndex < iLookupUsersLength; ++iIndex)
    {
        uint64_t uXuid;
        DirtyUserToNativeUser(&uXuid, sizeof(uXuid), pLookupUsers + iIndex);
        users->SetAt(iIndex, uXuid.ToString());
    }

    if (NetConnStatus('xusr', (int32_t)uUserIndex, &user, sizeof(user)) < 0)
    {
        // no user at uUserIndex
        return(-2);
    }

    if (user->IsSignedIn && !user->IsGuest)
    {
        uint32_t currentId = 0;
        if (pUserContext->refXblUserContext != nullptr)
        {
            currentId = pUserContext->refXblUserContext->User->Id;
        }

        if (pUserContext->refXblUserContext == nullptr || user->Id != currentId)
        {
            _UserApiInitializeContext(pRef, uUserIndex);
        }

        pRef->iLookupsSent[uUserIndex] = iLookupUsersLength;
        pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_PROFILE;
        pUserContext->refAsyncOpProfile = pUserContext->refXblUserContext->ProfileService->GetUserProfilesAsync(users->GetView());
        pUserContext->uOpProfileTimer = NetTick();
        pUserContext->refAsyncOpProfile->Completed = ref new AsyncOperationCompletedHandler<IVectorView<XboxUserProfile^>^>
        ([pRef, uUserIndex](IAsyncOperation<IVectorView<XboxUserProfile^>^>^ refAsyncOp, Windows::Foundation::AsyncStatus status)
            {
                if (pRef->bShuttingDown != TRUE)
                {
                    switch (status)
                    {
                        case Windows::Foundation::AsyncStatus::Completed:
                        {
                            try
                            {
                                pRef->pPlatformData->aUserContext[uUserIndex].refResultsProfiles = refAsyncOp->GetResults();
                            }
                            catch (Platform::Exception ^e)
                            {
                                NetPrintf(("userapixboxone: UserApiPlatRequestProfile() Exception thrown (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                                break;
                            }

                            pRef->bAvailableDataIndex[uUserIndex] = TRUE;

                            break;
                        }
                        case Windows::Foundation::AsyncStatus::Error:
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestProfile() An error occured in call to GetUserProfilesAsync(), ErrorCode=%S (0x%08x)\n", refAsyncOp->ErrorCode.ToString()->Data(), refAsyncOp->ErrorCode));
                            pRef->pPlatformData->aUserContext[uUserIndex].refResultsProfiles = nullptr;
                            pRef->bAvailableDataIndex[uUserIndex] = TRUE;
                            break;
                        }
                        case Windows::Foundation::AsyncStatus::Canceled:
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestProfile() The async operation was cancelled\n"));
                            pRef->pPlatformData->aUserContext[uUserIndex].refResultsProfiles = nullptr;
                            pRef->bAvailableDataIndex[uUserIndex] = TRUE;
                            break;
                        }
                    }
                }

                refAsyncOp->Close();
                pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_PROFILE;
            }
        );
    }
    else if (user->IsGuest)
    {
        NetPrintf(("userapixboxone: Profile request can not be performed with a guest. Ignoring the request.\n"));
        return(-3);
    }
    else if (!user->IsSignedIn)
    {
        NetPrintf(("userapixboxone: Profile request can not be performed with a user that is not signed in. Ignoring the request.\n"));
        return(-4);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatAbortRequests

    \Description
        Aborts all request to Xbox LIVE Services for a user
        
    \Input *pRef               - module ref
    \Input uUserIndex          - The index of the user making the request

    \Output 
        int32_t            - 0 for success, negative for error
        
    \Version 04/17/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatAbortRequests(UserApiRefT *pRef, uint32_t uUserIndex)
{
    // intentionally left blank
    // do not cancel the operations but allow them to finish, cancel behavior from MS is undefined
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatRequestPresence

    \Description
        Requests presence information from Microsoft
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pLookupUsers       - User to lookup

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRequestPresence(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers)
{
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    wchar_t  wTargetXuid[22];
    char     strTargetXuid[22];
    uint64_t uTargetXuid;
    Platform::String ^XUID = nullptr;
    User ^user = nullptr;

    if (pRef->pPlatformData->bAsyncInProgress[uUserIndex] & USERAPI_MASK_PRESENCE)
    {
        NetPrintf(("userapixboxone: [%p] request for user %d already in progress.\n", pRef, uUserIndex));
        return(-1);
    }

    DirtyUserToNativeUser(&uTargetXuid, sizeof(uTargetXuid), pLookupUsers);
    ds_snzprintf(strTargetXuid, 22, "%llu", uTargetXuid);
    mbstowcs(wTargetXuid, strTargetXuid, sizeof(wTargetXuid));
    XUID = ref new Platform::String(wTargetXuid);

    if (NetConnStatus('xusr', (int32_t)uUserIndex, &user, sizeof(user)) < 0)
    {
        // no user at uUserIndex
        return(-2);
    }

    if (user->IsSignedIn)
    {
        uint32_t currentId = 0;
        if (pUserContext->refXblUserContext != nullptr)
        {
            currentId = pUserContext->refXblUserContext->User->Id;
        }

        if (pUserContext->refXblUserContext == nullptr || user->Id != currentId)
        {
            _UserApiInitializeContext(pRef, uUserIndex);
        }

        pRef->iLookupsSent[uUserIndex] = 1;
        pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_PRESENCE;
        pUserContext->refAsyncOpPresence = pUserContext->refXblUserContext->PresenceService->GetPresenceAsync(XUID);
        pUserContext->uOpPresenceTimer = NetTick();
        pUserContext->refAsyncOpPresence->Completed = ref new AsyncOperationCompletedHandler<PresenceRecord^>
        ([pRef, uUserIndex](IAsyncOperation<PresenceRecord^>^ refAsyncOp, Windows::Foundation::AsyncStatus status)
            {
                if (pRef->bShuttingDown != TRUE)
                {
                    switch (status)
                    {
                        case Windows::Foundation::AsyncStatus::Completed:
                        {
                            try
                            {
                                pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence = refAsyncOp->GetResults();
                            }
                            catch (Platform::Exception ^e)
                            {
                                NetPrintf(("userapixboxone: UserApiPlatRequestPresence() Exception thrown (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                                break;
                            }

                            pRef->bAvailableDataIndexPresence[uUserIndex] = TRUE;
                            pRef->bAvailableDataIndexRichPresence[uUserIndex] = TRUE;

                            break;
                        }
                        case Windows::Foundation::AsyncStatus::Error:
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestPresence() An error occured in call to GetUserPresenceAsync(), ErrorCode=%S (0x%08x)\n", refAsyncOp->ErrorCode.ToString()->Data(), refAsyncOp->ErrorCode));
                            pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence = nullptr;
                            pRef->bAvailableDataIndexPresence[uUserIndex] = TRUE;
                            pRef->bAvailableDataIndexRichPresence[uUserIndex] = TRUE;
                            break;
                        }
                        case Windows::Foundation::AsyncStatus::Canceled:
                        {
                            NetPrintf(("userapixboxone: UserApiPlatRequestProfile() The async operation was cancelled\n"));
                            pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence = nullptr;
                            pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence = nullptr;
                            pRef->bAvailableDataIndexPresence[uUserIndex] = TRUE;
                            pRef->bAvailableDataIndexRichPresence[uUserIndex] = TRUE;
                            break;
                        }
                    }
                }

                refAsyncOp->Close();
                pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_PRESENCE;
            }
        );
    }
    else
    {
        NetPrintf(("userapixboxone: Presence request can not be performed with a user that is not signed in. Ignoring the request.\n"));
        return(-3);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatRequestRichPresence

    \Description
        Requests rich presence information from Microsoft
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pLookupUser        - User to lookup

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRequestRichPresence(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser)
{
    // On XB1 Rich presence comes with presence. If presence is specified ignore this, otherwise  call the presence request
    if (pRef->uUserDataMask[uUserIndex]  & USERAPI_MASK_PRESENCE)
    {
        return(0);
    }
    else
    {
        return(UserApiPlatRequestPresence(pRef, uUserIndex, pLookupUser));
    }
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatRequestRecentlyMet

    \Description
        Unused for XB1
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pPlayerMet         - User to match

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRequestRecentlyMet(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pPlayerMet)
{
    // Recently met players concept does not exist on XB1. Return an error
    return(USERAPI_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function UserApiPlatRequestPostRichPresence

    \Description
        Submit rich presence data to Microsoft for a local player
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pData              - Rich presence data

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRequestPostRichPresence(UserApiRefT *pRef, uint32_t uUserIndex, UserApiRichPresenceT *pData)
{
    wchar_t wRichPresence[USERAPI_RICH_PRES_MAXLEN+1];
    mbstowcs(wRichPresence, pData->strData, sizeof(wRichPresence));
    Platform::String ^richPresenceString = ref new Platform::String(wRichPresence);
    PresenceData ^presenceData = ref new PresenceData(Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId, richPresenceString);
    User ^user = nullptr;
    UserApiPlatformUserContextT *pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];

    // todo: extend this functionality to be able to take arguments. 
    //       See the MS docs on Rich Presence;
    //       https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/rich_presence_strings_overview_feb14.aspx
    if (NetConnStatus('xusr', (int32_t)uUserIndex, &user, sizeof(user)) < 0)
    {
        // no user at uUserIndex

        return(-1);
    }

    if (pRef->pPlatformData->bAsyncInProgress[uUserIndex] & USERAPI_MASK_POST)
    {
        NetPrintf(("userapixboxone: [%p] request for user %d already in progress.\n", pRef, uUserIndex));
        return(-1);
    }

    if (user->IsSignedIn)
    {
        uint32_t currentId = 0;
        if (pUserContext->refXblUserContext != nullptr)
        {
            currentId = pUserContext->refXblUserContext->User->Id;
        }

        if (pUserContext->refXblUserContext == nullptr || user->Id != currentId)
        {
            _UserApiInitializeContext(pRef, uUserIndex);
        }

        pUserContext->eResultRichPresencePost = USERAPI_ERROR_OK;
        pRef->pPlatformData->bAsyncInProgress[uUserIndex] |= USERAPI_MASK_POST;
        pUserContext->refAsyncOpPostRichPresence = pUserContext->refXblUserContext->PresenceService->SetPresenceAsync(true, presenceData);
        pUserContext->uOpPostRichPresenceTimer = NetTick();
        pUserContext->refAsyncOpPostRichPresence->Completed = ref new AsyncActionCompletedHandler([pRef, uUserIndex] (IAsyncAction ^resultTask, Windows::Foundation::AsyncStatus status) 
        {
            if (pRef->bShuttingDown != TRUE)
            {
                switch (status)
                {
                    case Windows::Foundation::AsyncStatus::Completed:
                    {
                        pRef->pPlatformData->aUserContext[uUserIndex].eResultRichPresencePost = USERAPI_ERROR_OK;
                        pRef->UserRmpList[uUserIndex].iTotalReceived += 1;
                        break;
                    }
                    case  Windows::Foundation::AsyncStatus::Error:
                    {
                        NetPrintf(("userapixboxone: An error occured in call to UserApiPostRichPresenceAsync() (profile), ErrorCode=%S (0x%08x)\n", resultTask->ErrorCode.ToString()->Data(), resultTask->ErrorCode)); 
                        pRef->pPlatformData->aUserContext[uUserIndex].eResultRichPresencePost = USERAPI_ERROR_REQUEST_FAILED;
                        pRef->UserRmpList[uUserIndex].iTotalErrors += 1;
                        break;
                    }

                    case Windows::Foundation::AsyncStatus::Canceled:
                    {
                        NetPrintf(("userapixboxone: UserApiPostRichPresenceAsync(), async operation (post rich presence) was cancelled\n"));
                        pRef->UserRmpList[uUserIndex].iTotalErrors += 1;
                        break;
                    }
                }
            }

            pRef->pPlatformData->bAsyncInProgress[uUserIndex] &= !USERAPI_MASK_POST;
            pRef->bAvailableDataIndexRMP[uUserIndex] = TRUE;
        });
    
    }
    else
    {
        NetPrintf(("userapixboxone: Post rich presence request can not be performed with a user that is not signed in. Ignoring the request.\n"));
        return(-3);
    }

    return(0);
}


/*F********************************************************************************/
/*!
    \Function UserApiPlatControl

    \Description
        Control behavior of module.

    \Input *pRef    - pointer to module state
    \Input iControl - status selector
    \Input iValue   - control value
    \Input iValue2  - control value
    \Input *pValue  - control value

    \Output
        int32_t     - Return 0 if successful. Otherwise a selector specific error

    \Notes
        iControl can be one of the following:

        \verbatim
            'avsz' - Set which avatar size will be retrieved. iValue = 's' for small (64x64), 'm' for medium (208x208) and 'l' for big (424x424). The default value is 'l' on xboxone
        \endverbatim

    \Version 05/10/2013 (mcorcoran) - First version
*/
/********************************************************************************F*/
int32_t UserApiPlatControl(UserApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // set the avatar sizes
    if (iControl == 'avsz')
    {
        switch (iValue)
        {
            case 's':
            case 'm':
            case 'l':
                pRef->pPlatformData->pAvatarSize = iValue;
                return(0);

            default:
                NetPrintf(("userapixboxone: [%p] invalid avatar size\n", pRef));
                return(-1);
        }
    }
    // set the title id to monitor for title presence notifications
    else if (iControl == 'nttl')
    {
        pRef->pPlatformData->uNotificationTitle = (uint32_t)iValue;
        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function UserApiPlatUpdate

    \Description
        Control behavior of module.

    \Input *pRef    - pointer to module state

    \Output
        int32_t     - Return 0 if successful. Otherwise a selector specific error

    \Version 08/08/2013 (amakoukji) - First version
*/
/********************************************************************************F*/
int32_t UserApiPlatUpdate(UserApiRefT *pRef)
{
    // update the notification signup state for each user

    UserApiPlatformUserContextT *pUserContext = NULL;
    uint32_t uUserIndex = 0;
    uint32_t uCurrentTime = NetTick();

    // check for timeouts
    for (uUserIndex = 0; uUserIndex < NETCONN_MAXLOCALUSERS; ++uUserIndex)
    {
        pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
        if ((pRef->pPlatformData->bAsyncInProgress[uUserIndex] & (USERAPI_MASK_PRESENCE | USERAPI_MASK_RICH_PRESENCE)) && (NetTickDiff(uCurrentTime, pUserContext->uOpPresenceTimer) > USERAPI_ASYNC_TIMOUT_MS))
        {
            NetPrintf(("userapixboxone: The presence request async operation timed out\n"));    
            pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence = nullptr;
            pRef->bAvailableDataIndex[uUserIndex] = TRUE;
            pRef->pPlatformData->bAsyncInProgress[uUserIndex] = 0;
        }

        if ((pRef->pPlatformData->bAsyncInProgress[uUserIndex] & USERAPI_MASK_PROFILE) && (NetTickDiff(uCurrentTime, pUserContext->uOpProfileTimer) > USERAPI_ASYNC_TIMOUT_MS))
        {
            NetPrintf(("userapixboxone: The profile request async operation timed out\n"));    
            pRef->pPlatformData->aUserContext[uUserIndex].refResultsProfiles = nullptr;
            pRef->bAvailableDataIndex[uUserIndex] = TRUE;
            pRef->pPlatformData->bAsyncInProgress[uUserIndex] = 0;
        }

        if ((pRef->pPlatformData->bAsyncInProgress[uUserIndex] & USERAPI_MASK_POST) && (NetTickDiff(uCurrentTime, pUserContext->uOpPostRichPresenceTimer) > USERAPI_ASYNC_TIMOUT_MS))
        {
            NetPrintf(("userapixboxone: The rich presence post async operation timed out\n"));    
            pRef->UserRmpList[uUserIndex].iTotalErrors += 1;
            pRef->pPlatformData->bAsyncInProgress[uUserIndex] = 0;
            pRef->bAvailableDataIndexRMP[uUserIndex] = TRUE;
        }
    }

    // process Rta
    for (uUserIndex = 0; uUserIndex < NETCONN_MAXLOCALUSERS; ++uUserIndex)
    {
        pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];

        switch (pUserContext->NotificationSignUpState)
        {
            case NOTIFICATION_STATE_ERROR:
            {
                UserApiNotificationT (*pList)[USERAPI_NOTIFY_LIST_MAX_SIZE] = NULL;
                int32_t i = 0;

                NetPrintf(("userapixboxone: an error occured signing up for presence notification\n"));
                if (pUserContext->eType == USERAPI_NOTIFY_PRESENCE_UPDATE)
                {
                    pList = &pRef->PresenceNotification;
                    pRef->bPresenceNotificationStarted = FALSE;
                }
                else if (pUserContext->eType == USERAPI_NOTIFY_TITLE_UPDATE)
                {
                    pList = &pRef->TitleNotification;
                    pRef->bTitleNotificationStarted = FALSE;
                }
                else if (pUserContext->eType == USERAPI_NOTIFY_RICH_PRESENCE_UPDATE)
                {
                    pList = &pRef->RichPresenceNotification;
                    pRef->bRichPresenceNotificationStarted = FALSE;
                }

                // trigger the callback with the appropriate type but NULL data to indicate an error
                for (i = 0; i < USERAPI_NOTIFY_LIST_MAX_SIZE; ++i)
                {
                    if ((*pList)[i].pCallback != NULL)
                    {
                        (*pList)[i].pCallback(pRef, pUserContext->eType, NULL, (*pList)[i].pUserData);
                    }
                }

                // reset the state
                pUserContext->NotificationSignUpState = NOTIFICATION_STATE_IDLE;
                break;
            }

            case NOTIFICATION_STATE_CONNECTED_TO_RTA:
            {
                _UserApiQueryFriends(pRef, uUserIndex);
                pUserContext->NotificationSignUpState = NOTIFICATION_STATE_QUERY_FRIENDS;
                break;
            }

            case NOTIFICATION_STATE_FRIEND_RECEIVED:
            {
                pUserContext->NotificationSignUpState = NOTIFICATION_STATE_SIGNUP;
                break;
            }

            case NOTIFICATION_STATE_SIGNUP:
            {
                if (pUserContext->eType == USERAPI_NOTIFY_PRESENCE_UPDATE)
                {
                    _UserApiSignupForConsolePresence(pRef, uUserIndex);
                    // check timeout
                    if (NetTickDiff(uCurrentTime, pUserContext->uOpNotifyPresenceTimer) > USERAPI_ASYNC_TIMOUT_MS)
                    {
                        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                    }
                }
                else if (pUserContext->eType == USERAPI_NOTIFY_TITLE_UPDATE)
                {
                    _UserApiSignupForTitlePresence(pRef, uUserIndex);
                    // check timeout
                    if (NetTickDiff(uCurrentTime, pUserContext->uOpNotifyTitleTimer) > USERAPI_ASYNC_TIMOUT_MS)
                    {
                        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                    }
                }
                break;
            }

            case NOTIFICATION_STATE_SIGNUP_COMPLETE:
            {
                // done! go back to idle
                pUserContext->NotificationSignUpState = NOTIFICATION_STATE_IDLE;
                break;
            }

            case NOTIFICATION_STATE_CONNECTING_TO_RTA:
            {
                // check timeout
                if (NetTickDiff(uCurrentTime, pUserContext->uRtaConnectionTimer) > USERAPI_ASYNC_TIMOUT_MS)
                {
                    pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                }
                break;
            }

            case NOTIFICATION_STATE_QUERY_FRIENDS:
            {
                // check timeout
                if (NetTickDiff(uCurrentTime, pUserContext->uOpFriendTimer) > USERAPI_ASYNC_TIMOUT_MS)
                {
                    pUserContext->NotificationSignUpState = NOTIFICATION_STATE_ERROR;
                }
                break;
            }
        }
    }



    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiProcessProfileResponse

    \Description
        Process profile response from first party
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input  bBatch             - flag to indicate multiple responses are to be processed
    \Input *ProfileData        - response structure to populate
    \Input *pUserData          - user defined structure to pass

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
/*************************************************************************************************F*/
int32_t _UserApiProcessProfileResponse(UserApiRefT *pRef, int32_t uUserIndex, uint8_t bBatch, UserApiProfileT *ProfileData, UserApiUserDataT *pUserData)
{
    IVectorView<XboxUserProfile^> ^refResult = pRef->pPlatformData->aUserContext[uUserIndex].refResultsProfiles;
    UserApiEventDataT data;

    if (refResult != nullptr)
    {
        for (uint32_t i = 0; i < refResult->Size; ++i)
        {
            XboxUserProfile ^refProfile = refResult->GetAt(i);
            uint64_t uXuid;
            memset(&data, 0, sizeof(UserApiEventDataT));
            data.eEventType = USERAPI_EVENT_DATA;
            data.uUserIndex = uUserIndex;
            data.eError     = USERAPI_ERROR_OK;

            uXuid = _wcstoui64(refProfile->XboxUserId->Data(), NULL, 10);   
            DirtyUserFromNativeUser(&pUserData->DirtyUser, &uXuid);
            wcstombs(ProfileData->strAvatarUrl, refProfile->GameDisplayPictureResizeUri->AbsoluteUri->ToString()->Data(), sizeof(ProfileData->strAvatarUrl));

            // append the avatar size info to the avatar url
            if (pRef->pPlatformData->pAvatarSize == 's')
            {
                ds_strnzcat(ProfileData->strAvatarUrl, USERAPI_DISPLAY_PIC_SIZE_SMALL, sizeof(ProfileData->strAvatarUrl));
            }
            else if (pRef->pPlatformData->pAvatarSize == 'm')
            {
                ds_strnzcat(ProfileData->strAvatarUrl, USERAPI_DISPLAY_PIC_SIZE_MEDIUM, sizeof(ProfileData->strAvatarUrl));
            }
            else
            {
                // always assume large just incase
                ds_strnzcat(ProfileData->strAvatarUrl, USERAPI_DISPLAY_PIC_SIZE_LARGE, sizeof(ProfileData->strAvatarUrl));
            }
            
            Utf8EncodeFromUCS2(ProfileData->strGamertag, sizeof(ProfileData->strGamertag), (uint16 *)refProfile->Gamertag->Data());
            Utf8EncodeFromUCS2(ProfileData->strDisplayName, sizeof(ProfileData->strDisplayName), (uint16 *)refProfile->GameDisplayName->Data());
            ProfileData->uLocale = 0; // this info is hidden
            ProfileData->pRawData = &refProfile;

            if (bBatch)
            {
                _UserApiTriggerCallback(pRef, uUserIndex, USERAPI_ERROR_OK, USERAPI_EVENT_DATA, pUserData);
            }

            ++(pRef->UserContextList[uUserIndex].iTotalReceived);
        }
    }
    else
    {
        pRef->UserContextList[uUserIndex].iTotalErrors += pRef->iLookupsSent[uUserIndex];
        return(USERAPI_ERROR_REQUEST_FAILED);
    }
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiProcessPresenceResponse

    \Description
        Process presence response from first party
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pPresenceData      - response structure to populate
    \Input *pUserData          - user defined structure to pass

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
/*************************************************************************************************F*/
int32_t _UserApiProcessPresenceResponse(UserApiRefT *pRef, int32_t uUserIndex, UserApiPresenceT *pPresenceData, UserApiUserDataT *pUserData)
{
    PresenceRecord ^refResult = pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence;
    uint64_t uXuid = 0;

    // For presence take only the first console that is online
    if (refResult != nullptr)
    {
        uXuid = _wcstoui64(refResult->XboxUserId->Data(), NULL, 10);   
        DirtyUserFromNativeUser(&pUserData->DirtyUser, &uXuid);

        switch (refResult->UserState)
        {
            case UserPresenceState::Away:
                pPresenceData->ePresenceStatus = USERAPI_PRESENCE_AWAY;
            break;

            case UserPresenceState::Offline:
                pPresenceData->ePresenceStatus = USERAPI_PRESENCE_OFFLINE;
            break;

            case UserPresenceState::Online:
                pPresenceData->ePresenceStatus = USERAPI_PRESENCE_ONLINE;
            break;

            default:
            case UserPresenceState::Unknown:
                pPresenceData->ePresenceStatus = USERAPI_PRESENCE_UNKNOWN;
            break;
        }

        pPresenceData->bIsPlayingSameTitle = FALSE;

        if (refResult->PresenceTitleRecords != nullptr)
        {
            PresenceTitleRecord ^refRecord = _UserApiGetPriorityTitle(refResult->PresenceTitleRecords);
            uint32_t uCurrentTitleId = _wcstoui64(Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), NULL, 10);
            uint32_t uTargetTitleId = 0;

            if (refRecord != nullptr)
            {
                wcstombs (pPresenceData->strTitleName, refRecord->TitleName->ToString()->Data(), sizeof(pPresenceData->strTitleName));
                ds_snzprintf(pPresenceData->strTitleId, sizeof(pPresenceData->strTitleId), "%u", refRecord->TitleId);
                ds_snzprintf(pPresenceData->strPlatform, sizeof(pPresenceData->strPlatform), "%i", refRecord->DeviceType);
                pPresenceData->pRawData = &refResult;

                uTargetTitleId = refRecord->TitleId;
                if (uCurrentTitleId == uTargetTitleId)
                {
                    pPresenceData->bIsPlayingSameTitle = TRUE;
                }
            }
        }
    }
    else
    {
        pRef->UserContextList[uUserIndex].iTotalErrors += pRef->iLookupsSent[uUserIndex];
        return(USERAPI_ERROR_REQUEST_FAILED);
    }
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _UserApiProcessRichPresenceResponse

    \Description
        Process rich presence response from first party
        
    \Input *pRef               - module ref
    \Input  uUserIndex         - The index of the user making the request
    \Input *pRichPresenceData  - response structure to populate
    \Input *pUserData          - user defined structure to pass

    \Output 
        int32_t                - 0 for success, negative for error
        
    \Version 01/21/2013 (amakoukji)
/*************************************************************************************************F*/
int32_t _UserApiProcessRichPresenceResponse(UserApiRefT *pRef, int32_t uUserIndex, UserApiRichPresenceT *pRichPresenceData, UserApiUserDataT *pUserData)
{
    PresenceRecord ^refResult = pRef->pPlatformData->aUserContext[uUserIndex].refResultsPresence;
    uint64_t uXuid = 0;

    // For presence, take only XB1 data
    if (refResult != nullptr)
    {
        uXuid = _wcstoui64(refResult->XboxUserId->Data(), NULL, 10);   
        DirtyUserFromNativeUser(&pUserData->DirtyUser, &uXuid);
        pRichPresenceData->pRawData = &refResult;

        if (refResult->PresenceTitleRecords != nullptr)
        {
            PresenceTitleRecord ^refRecord = _UserApiGetPriorityTitle(refResult->PresenceTitleRecords);
            if (refRecord != nullptr)
            {
                wcstombs (pRichPresenceData->strData, refRecord->Presence->ToString()->Data(), sizeof(pRichPresenceData->strData));
            }
        }
    }
    else
    {
        pRef->UserContextList[uUserIndex].iTotalErrors += pRef->iLookupsSent[uUserIndex];
        return(USERAPI_ERROR_REQUEST_FAILED);
    }
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _UserApiProcessRmpResponse

    \Description
        Called by update function in the platform specific UserApi modules when a profile
        or error is ready. Intentionally left empty for XB1

    \Input *pRef                 - pointer to UserApiT module reference.
    \Input *uUserIndex           - The index of the user associated with this profile request.

    \Output
        void

    \Version 02/14/2014 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t _UserApiProcessRmpResponse(UserApiRefT *pRef, uint32_t uUserIndex)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _UserApiTriggerPostCallback

    \Description
        Called by update function in the platform specific UserApi modules when a profile
        or error is ready. 

    \Input *pRef                 - pointer to UserApiT module reference.
    \Input *uUserIndex           - The index of the user associated with this profile request.

    \Version 02/14/2014 (amakoukji) - First version
*/
/*************************************************************************************************F*/
void _UserApiTriggerPostCallback(UserApiRefT *pRef, uint32_t uUserIndex)
{
    UserApiPostResponseT PostResponseData;

    PostResponseData.pMessage = NULL;
    PostResponseData.eError   = pRef->pPlatformData->aUserContext[uUserIndex].eResultRichPresencePost;
    PostResponseData.uUserIndex = uUserIndex;

    NetCritEnter(&pRef->postCrit);

    if (pRef->pPostCallback[uUserIndex] != NULL)
    {
        pRef->pPostCallback[uUserIndex](pRef, &PostResponseData, pRef->pUserDataPost[uUserIndex]);
    }

    NetCritLeave(&pRef->postCrit);
}

/*F*************************************************************************************************/
/*!
    \Function    _UserApiRegisterProfileUpdateEvent

    \Description
        Helper function for registering Profile Update Event
        or error is ready. 

    \Input *pRef                 - pointer to UserApiT module reference.
    \Input *uUserIndex           - The index of the user associated with this profile request.

    \Version 10/31/2014 (tcho) - First version
*/
/*************************************************************************************************F*/
void _UserApiRegisterProfileUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex)
{
    User::UserDisplayInfoChanged += ref new EventHandler<UserDisplayInfoChangedEventArgs^>([pRef, uUserIndex]( Platform::Object^, UserDisplayInfoChangedEventArgs^ refArgs )
    {
        UserApiNotificationT (*pList)[USERAPI_NOTIFY_LIST_MAX_SIZE] = &pRef->ProfileUpdateNotification;
        UserApiNotifyDataT profileUpdateResponse;
        uint32_t iIndex = 0;
        User ^updatedUser = nullptr;
        uint64_t uXuid;
            
        for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (NetConnStatus('xusr', iIndex, &updatedUser, sizeof(updatedUser)) >= 0)
            {
                if (refArgs->User->XboxUserId == updatedUser->XboxUserId)
                {
                    Platform::String ^stringXuid = refArgs->User->XboxUserId;
                    uXuid = _wcstoui64(stringXuid->Data(), NULL, 10);
                       
                    profileUpdateResponse.ProfileUpdateData.uUserIndex = iIndex;
                    DirtyUserFromNativeUser(&profileUpdateResponse.ProfileUpdateData.DirtyUser, &uXuid);

                    break;
                }
            }
        }

        // trigger the callback with the appropriate type but NULL data to indicate an error
        for (iIndex = 0; iIndex < USERAPI_NOTIFY_LIST_MAX_SIZE; ++iIndex)
        {
            if ((*pList)[iIndex].pCallback != NULL)
            {
                (*pList)[iIndex].pCallback(pRef, USERAPI_NOTIFY_PROFILE_UPDATE , &profileUpdateResponse, (*pList)[iIndex].pUserData);
            }
        }
    });
}


/*F*************************************************************************************************/
/*!
    \Function    UserApiPlatRegisterUpdateEvent

    \Description
        Register for notifications from first part

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input  eType               - type of event to register for

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 06/16/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiPlatRegisterUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex, UserApiNotifyTypeE eType)
{  
    // Only presence update is supported on XB1
    UserApiPlatformUserContextT *pUserContext = NULL;
    uint32_t currentId = 0;
    User ^user = nullptr;

    // handle unsupported types
    if (eType == USERAPI_NOTIFY_RICH_PRESENCE_UPDATE)
    {
        return(USERAPI_ERROR_UNSUPPORTED);
    }

    // register the DisplayInfoChanged Event
    if (eType == USERAPI_NOTIFY_PROFILE_UPDATE)
    {
        _UserApiRegisterProfileUpdateEvent(pRef, uUserIndex);
    }

    if (NetConnStatus('xusr', uUserIndex, &user, sizeof(user)) < 0)
    {
        // no user at uUserIndex
        NetPrintf(("userapixboxone: attempt to register for notifications for user that doesn't exist at index u\n", uUserIndex));
        return(USERAPI_ERROR_NO_USER);
    }
    
    // setup user context if not already done
    pUserContext = &pRef->pPlatformData->aUserContext[uUserIndex];
    if (pUserContext->refXblUserContext != nullptr)
    {
        currentId = pUserContext->refXblUserContext->User->Id;
    }

    if (pRef->pPlatformData->aUserContext[uUserIndex].refXblUserContext == nullptr)
    {
        if (pUserContext->refXblUserContext == nullptr || user->Id != currentId)
        {
            _UserApiInitializeContext(pRef, uUserIndex);
        }
    }

    if  (pUserContext->NotificationSignUpState != NOTIFICATION_STATE_IDLE)
    {
        return(USERAPI_ERROR_INPROGRESS);
    }

    // start the process
    pUserContext->eType = eType;
    if (pUserContext->bConnectedToRTA)
    {
        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_QUERY_FRIENDS;
        _UserApiQueryFriends(pRef, uUserIndex);
    }
    else if (pUserContext->NotificationSignUpState != NOTIFICATION_STATE_CONNECTING_TO_RTA)
    {
        pUserContext->NotificationSignUpState = NOTIFICATION_STATE_CONNECTING_TO_RTA;
        _UserApiConnectToRTA(pRef, uUserIndex);
    }

    return(0);
}

