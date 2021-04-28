/*! ************************************************************************************************/
/*!
    \file utiljobs.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/internal/util/utiljobs.h"
#include "BlazeSDK/component/utilcomponent.h"

#ifdef EA_PLATFORM_XBOXONE
#include "BlazeSDK/blazesdkdefs.h"
#include "shared/framework/util/shared/stringbuilder.h"
#include "shared/framework/protocol/shared/jsondecoder.h"

#include "BlazeSDK/component/xblsystemconfigscomponent.h"
#include "DirtySDK/proto/protohttp.h"

#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "BlazeSDK/loginmanager/loginmanager.h"
#endif

namespace Blaze
{
namespace Util
{

#if !defined(EA_PLATFORM_XBOXONE)
    /*! ************************************************************************************************/
    /*! \brief copy the textlist in request into response, and set the result type as unprocessed
    ***************************************************************************************************/
    static void convert(const UserStringList& request, FilterUserTextResponse& response)
    {
        response.getFilteredTextList().reserve(request.getTextList().size());
        UserStringList::UserTextList::const_iterator i = request.getTextList().begin();
        UserStringList::UserTextList::const_iterator e = request.getTextList().end();
        for (; i!=e; ++i) 
        {
            FilteredUserText *filteredUserText = response.getFilteredTextList().pull_back();
            filteredUserText->setFilteredText((*i)->getText());
            filteredUserText->setResult(FILTER_RESULT_UNPROCESSED);
        }
    }
#endif

    /*! ************************************************************************************************/
    /*! \brief store titleCb in functor directory and cache off api pointer
    ***************************************************************************************************/
    FilterUserTextJobBase::FilterUserTextJobBase(const UtilAPI::FilterUserTextCb &titleCb, Blaze::Util::UtilAPI *utilApi) :
        mTitleCb(titleCb), mUtilApi(utilApi)
    {
        setAssociatedTitleCb(mTitleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief cleanup the titleCb (in the functor dir) if the job is removed
    ***************************************************************************************************/
    FilterUserTextJobBase::~FilterUserTextJobBase()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief dispatch the title callback (does not delete the job)
    ***************************************************************************************************/
    void FilterUserTextJobBase::dispatchTitleCallback(BlazeError blazeError, const FilterUserTextResponse *response)
    {
        mTitleCb(blazeError, getId(), response);

        // clearing title callback since it should only be dispatched once
        mTitleCb.clear();
        setAssociatedTitleCb(mTitleCb);
    }


#if defined (EA_PLATFORM_XBOXONE)
        FilterUserTextJobXboxOne::FilterUserTextJobXboxOne(const UtilAPI::FilterUserTextCb &titleCb, Blaze::Util::UtilAPI *utilApi,
                const XboxOneValidateString &xboxOneValidateString) :
        FilterUserTextJobBase(titleCb, utilApi)
    {
        BlazeAssert(xboxOneValidateString.getId() != INVALID_JOB_ID);
        mXboxLiveServicesJobId = xboxOneValidateString.getId();
    }
    
    FilterUserTextJobXboxOne::~FilterUserTextJobXboxOne()
    {
        // Need to clean up the XboxOneValidateString job
        Job* job = mUtilApi->getBlazeHub()->getScheduler()->getJob(mXboxLiveServicesJobId);
        if (job != nullptr)
        {
            mUtilApi->getBlazeHub()->getScheduler()->removeJob(job, true);   
            mXboxLiveServicesJobId = INVALID_JOB_ID;
        }
    }
    
    void FilterUserTextJobXboxOne::execute()
    {
        Job::setExecuting(true);
        dispatchTitleCallback(SDK_ERR_RPC_TIMEOUT, nullptr);
        if (mXboxLiveServicesJobId != INVALID_JOB_ID)
        {
            mUtilApi->getBlazeHub()->getScheduler()->removeJob(mXboxLiveServicesJobId);   
            mXboxLiveServicesJobId = INVALID_JOB_ID;
        }
        Job::setExecuting(false);
    }

    void FilterUserTextJobXboxOne::cancel(BlazeError err)
    {
        Job::setExecuting(true);
        dispatchTitleCallback(err, nullptr);
        if (mXboxLiveServicesJobId != INVALID_JOB_ID)
        {
            mUtilApi->getBlazeHub()->getScheduler()->cancelJob(mXboxLiveServicesJobId);
            mXboxLiveServicesJobId = INVALID_JOB_ID;
        }
        Job::setExecuting(false);
    }
#else

    // not EA_PLATFORM_XBOXONE

    /*! ************************************************************************************************/
    /*! \brief a FilterUserTextJob
    ***************************************************************************************************/
    FilterUserTextJob::FilterUserTextJob(const UserStringList& userStringList, const UtilAPI::FilterUserTextCb &titleCb, 
                Blaze::Util::UtilAPI *utilApi, bool isClientOnly) :
        FilterUserTextJobBase(titleCb, utilApi), mIsClientOnly(isClientOnly), mResponse(MEM_GROUP_FRAMEWORK_TEMP)
    {
        convert(userStringList, mResponse);
    }

    FilterUserTextJob::~FilterUserTextJob()
    {
    }

    void FilterUserTextJob::execute()
    {
        Job::setExecuting(true);            //  inform the scheduler the job is executing.
        if (mIsClientOnly)
        {
            // non-XOne client-only filtering : execute means success
            //   dispatch unfiltered input text as the result
            dispatchTitleCallback(ERR_OK, &mResponse);
        }
        else
        {
            // non-XOne server filtering : execute means the job timed out
            //  dispatch timeout and nuke the RPC (if it's still out)
            dispatchTitleCallback(SDK_ERR_RPC_TIMEOUT, nullptr);
        }
        Job::setExecuting(false);
    }

    void FilterUserTextJob::cancel(BlazeError err)
    {
        Job::setExecuting(true);            //  inform the scheduler the job is executing.
        dispatchTitleCallback(err, nullptr);
        Job::setExecuting(false);
    }
#endif

#ifdef EA_PLATFORM_XBOXONE
    XboxOneValidateString::XboxOneValidateString( JobId filterUserTextJobId, Blaze::Util::UtilComponent* component, Blaze::Util::UtilAPI* api,
        bool clientOnly, MemoryGroupId memGroupId /*= MEM_GROUP_FRAMEWORK_TEMP*/, uint32_t timeoutMS /*=PROFANITY_FILTER_TIMEOUT_MS*/ )
        :   
        mFilterUserTextJobId(filterUserTextJobId),
        mUtilApi(api),
        mUtilComponent(component), 
        mClientOnly(clientOnly),
        mResponse(memGroupId),
        mMemGroup(memGroupId),
        mValidateStringUri(nullptr),
        mJobId(INVALID_JOB_ID),
        mTimeout(timeoutMS)
    {
        mValidateStringReq = ProtoHttpCreate(PROTO_HTTP_BUFFER_SIZE);

        //use config overrides found in util.cfg
        mUtilApi->OverrideConfigs(mValidateStringReq, "ValidateString");
    }

    XboxOneValidateString::~XboxOneValidateString()
    {
        ProtoHttpDestroy(mValidateStringReq);
        mUtilApi->getBlazeHub()->removeIdler(this);
        if ((mUtilApi->getBlazeHub()->getScheduler() != nullptr) && (mJobId != INVALID_JOB_ID))
        {
            mUtilApi->getBlazeHub()->getScheduler()->removeJob(mJobId);
            mJobId = INVALID_JOB_ID;
        }
    }

    void XboxOneValidateString::execute()
    {
        Job::setExecuting(true);            //  inform the scheduler the job is executing.
        //Execute means we timed out, so make the callback with an error
        if (mJobId != INVALID_JOB_ID)
        {
            mUtilApi->getBlazeHub()->getScheduler()->removeJob(mJobId);
            mJobId = INVALID_JOB_ID;
        }
        BLAZE_SDK_DEBUGF("XboxOneValidateString: Timed out\n");
        dispatchTitleCallback(SDK_ERR_RPC_TIMEOUT, nullptr, false);
        Job::setExecuting(false);
    }

    void XboxOneValidateString::cancel(BlazeError err)
    {
        Job::setExecuting(true);            //  inform the scheduler the job is executing.
        if (mJobId != INVALID_JOB_ID)
        {
            mUtilApi->getBlazeHub()->getScheduler()->cancelJob(mJobId);
            mJobId = INVALID_JOB_ID;
        }
        BLAZE_SDK_DEBUGF("XboxOneValidateString: Cancelled\n");
        dispatchTitleCallback(err, nullptr, false);
        Job::setExecuting(false);
    }

    bool XboxOneValidateString::start(const UserStringList& userStringList)
    {
        mUtilApi->getBlazeHub()->getScheduler()->scheduleJob("XboxValidateStringJob", this, mUtilApi, mTimeout);

        if (userStringList.getTextList().empty())
        {
            dispatchTitleCallback(ERR_OK, nullptr, true);
            return true;
        }

        for (UserStringList::UserTextList::const_iterator begin = userStringList.getTextList().begin(),
                end = userStringList.getTextList().end(); begin != end; ++begin)
        {
            mReq.getPayload().getStringstoVerify().push_back((*begin)->getText());
        }

        mUtilApi->getBlazeHub()->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_XBOXONE_STRING_VALIDATION_URI, &mValidateStringUri);
        NetConnControl('aurn', 0, 0, const_cast<char8_t*>(mValidateStringUri), nullptr);

        mUtilApi->getBlazeHub()->addIdler(this, "XboxOneValidateString");

        return true;
    }

    void XboxOneValidateString::idle(const uint32_t currentTime, const uint32_t elapsedTime)
    {
        // check if the job is still active
        FilterUserTextJobBase *job = (FilterUserTextJobBase*) mUtilApi->getBlazeHub()->getScheduler()->getJob(mFilterUserTextJobId);
        if (job == nullptr)
        {
            BLAZE_SDK_DEBUGF("Dropping XboxOneValidateString result for canceled/removed job\n");
            mUtilApi->mBlazeHub->removeIdler(this);
            return;
        }

        uint32_t userIndex = mUtilApi->getBlazeHub()->getPrimaryLocalUserIndex();
        uint32_t dirtySockUserIndex = mUtilApi->getBlazeHub()->getLoginManager(userIndex)->getDirtySockUserIndex();
        int32_t ticketSize = NetConnStatus('tick', dirtySockUserIndex, nullptr, 0);
        if (ticketSize != 0)
        {
            mUtilApi->mBlazeHub->removeIdler(this);

            if (ticketSize < 0)
            {
                BLAZE_SDK_DEBUGF("[XboxOneValidateString::idle()] Error initiating first party ticket request for user at dirty sock index %u (Blaze user index %u).\n", dirtySockUserIndex, userIndex);
                dispatchTitleCallback(ERR_SYSTEM, nullptr, true);
                return;
            }
            else
            {
                uint8_t *ticketData = BLAZE_NEW_ARRAY(uint8_t, ticketSize, mMemGroup, "XboxOneValidateString::idle");
                NetConnStatus('tick', dirtySockUserIndex, ticketData, ticketSize);

                BlazeSender::ServerConnectionInfo serverInfo(mValidateStringUri, 443, true);

                XBLSystem::XBLSystemConfigsComponent::createComponent(mUtilApi->getBlazeHub(), serverInfo);
                XBLSystem::XBLSystemConfigsComponent* comp = mUtilApi->getBlazeHub()->getComponentManager(userIndex)->getXBLSystemConfigsComponent();

                mReq.setAuthToken((char8_t*)ticketData);
                mJobId = comp->validateStrings(mReq, Blaze::MakeFunctor(this, &XboxOneValidateString::onVerifyStringResult), INVALID_JOB_ID, mTimeout);

                BLAZE_DELETE_ARRAY(mMemGroup, ticketData);
            }
        }
    }

    void XboxOneValidateString::onVerifyStringResult(const XBLSystem::ValidateStringsResponse* response, BlazeError error, JobId jobId)
    {
        if (error != ERR_OK)
        {
            dispatchTitleCallback(error, nullptr, false);
            return;
        }

        mJobId = INVALID_JOB_ID;

        uint32_t idx = 0;
        for (XBLSystem::ResultList::const_iterator iter = response->getVerifyStringResult().begin(),
             end = response->getVerifyStringResult().end(); iter != end; ++iter, ++idx)
        {
            FilteredUserText* text = mResponse.getFilteredTextList().pull_back();
            uint16_t resultCode = (*iter)->getResultCode();
            switch (resultCode)
            {
            case 0:
                text->setResult(FILTER_RESULT_PASSED);
                text->setFilteredText(mReq.getPayload().getStringstoVerify()[idx]);
                break;
            case 1:
                text->setResult(FILTER_RESULT_OFFENSIVE);
                break;
            case 2:
                text->setResult(FILTER_RESULT_STRING_TOO_LONG);
                break;
            case 3:
                text->setResult(FILTER_RESULT_OTHER);
                break;
            }

            if (resultCode != FILTER_RESULT_PASSED)
            {
                char8_t tempbuf[FilteredUserText::MAX_FILTEREDTEXT_LEN];
                tempbuf[FilteredUserText::MAX_FILTEREDTEXT_LEN-1] = '\0';
                char8_t* dest;               
                dest = tempbuf;
                //fill the whole thing with asterisk
                for (const char8_t* offendingString = mReq.getPayload().getStringstoVerify()[idx];
                     *offendingString != '\0' && (dest - tempbuf) < (sizeof(tempbuf) - 1);
                     ++offendingString, ++dest)
                {
                    *dest='*';
                }
                *dest='\0';
                text->setFilteredText(tempbuf);
            }
        }

        // This was a client only check, call the callback
        if (mClientOnly)
        {                               
            dispatchTitleCallback(ERR_OK, &mResponse, true);
        }
        // Process server filtering
        else
        {   
            mUtilComponent->filterForProfanity(mResponse, MakeFunctor(mUtilApi, &UtilAPI::internalFilterUserTextCb), mFilterUserTextJobId);
        }
    }

    void XboxOneValidateString::dispatchTitleCallback(BlazeError blazeError, const FilterUserTextResponse *response, bool removeOwningJob)
    {        
        JobScheduler *scheduler = mUtilApi->getBlazeHub()->getScheduler();
        FilterUserTextJobBase *job = static_cast<FilterUserTextJobBase*>(scheduler->getJob(mFilterUserTextJobId));
        if (job != nullptr && !job->isExecuting())
        {
            job->dispatchTitleCallback(blazeError, response);
            if (removeOwningJob)
            {
                mFilterUserTextJobId = INVALID_JOB_ID;
                scheduler->removeJob(job);
            }
        }
    }
 
#endif // EA_PLATFORM_XBOXONE

} // Util
} // Blaze
