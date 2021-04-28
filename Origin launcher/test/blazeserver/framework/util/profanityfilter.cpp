/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/util/profanityfilter.h"

#include "framework/grpc/outboundgrpcmanager.h"
#include "eadp/profanity/profanity_filter.grpc.pb.h"

namespace Blaze
{

ProfanityFilter::ProfanityFilter()
    : mProfan(ProfanCreate()), mPendingProfan(nullptr), mConfig(nullptr)
    , mProfanityFilterServiceStringsChecked(*Metrics::gFrameworkCollection, "profanityFilterServiceStringsChecked")
    , mProfanityFilterServiceBatchStringChecks(*Metrics::gFrameworkCollection, "profanityFilterServiceBatchStringChecks")
    , mProfanityFilterServiceStringsFoundProfane(*Metrics::gFrameworkCollection, "profanityFilterServiceStringsFoundProfane")
    , mProfanityFilterServiceErrors(*Metrics::gFrameworkCollection, "profanityFilterServiceErrors")
    , mProfanityFilterServiceAttempts(*Metrics::gFrameworkCollection, "profanityFilterServiceAttempts")
    , mProfanityFilterServiceRetries(*Metrics::gFrameworkCollection, "profanityFilterServiceRetries")
    , mProfanityFilterServiceSuccesses(*Metrics::gFrameworkCollection, "profanityFilterServiceSuccesses")
    , mProfanityFilterServiceFailsAfterRetry(*Metrics::gFrameworkCollection, "profanityFilterServiceFailsAfterRetry")
    , mProfanityFilterServiceFailsWithoutRetry(*Metrics::gFrameworkCollection, "profanityFilterServiceFailsWithoutRetry")
{
}

ProfanityFilter::~ProfanityFilter()
{
    ProfanDestroy(mProfan);
    if (mPendingProfan != nullptr)
        ProfanDestroy(mPendingProfan);
}

void ProfanityFilter::onConfigure(const ProfanityServiceConfig& config)
{
    // onValidateConfig(config);

    mConfig = &config;

    completeConfiguration();
}

void ProfanityFilter::onReconfigure(const ProfanityServiceConfig& config)
{
    mConfig = &config;

    completeConfiguration();
}

bool ProfanityFilter::onValidateConfig(const ProfanityServiceConfig& config, ConfigureValidationErrors& validationErrors)
{
    if (config.getUseProfanityService())
    {
        if (config.getProfanityServiceFilterId()[0] == '\0')
        {
            StringBuilder msg;
            msg << "[ProfanityFilter.onValidateConfig]: Configuration failure. Trying to use profanity service with a null or empty filter id for service.";
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
    else
    {
        int32_t entryCount = buildValidatedFilter(config);
        if (entryCount < 0)
        {
            eastl::string msg;
            msg.sprintf("[Controller].onValidateConfig: Invalid profanity filter list. List may contain unsupported characters or have too many unique characters or too many equivalent character definitions.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            BLAZE_TRACE_LOG(Log::SYSTEM, "[Controller].onValidateConfig: ProfanityFilter validated with " << entryCount << " entries.");
        }
    }


    return validationErrors.getErrorMessages().empty();
}


int32_t ProfanityFilter::buildValidatedFilter(const ProfanityServiceConfig& config)
{
    if (mPendingProfan != nullptr)
    {
        BLAZE_WARN_LOG(Log::UTIL, "[ProfanityFilter].buildValidatedFilter: deleting existing pending ProfanRef");
        ProfanDestroy(mPendingProfan);
    }
    mPendingProfan = ProfanCreate();

    const char8_t* list = config.getProfanityFilter();
    int32_t count = ProfanImport(mPendingProfan, list);
    if (count < 0)
    {
        ProfanDestroy(mPendingProfan);
        mPendingProfan = nullptr;
    }

    return count;
}

void ProfanityFilter::completeConfiguration()
{
    ProfanDestroy(mProfan);
    mProfan = mPendingProfan;
    mPendingProfan = nullptr;
}

bool ProfanityFilter::usingProfanityService()
{
    return (mConfig && mConfig->getUseProfanityService());
}

int32_t ProfanityFilter::isProfane(const char8_t* stringToCheck)
{
    EA::TDF::TdfString tempString(stringToCheck);
    eastl::vector<EA::TDF::TdfString*> filterBatch;
    filterBatch.push_back(&tempString);
    return batchFilter(filterBatch, false);
}

int32_t ProfanityFilter::inplaceFilter(EA::TDF::TdfString& stringToFilter)
{
    eastl::vector<EA::TDF::TdfString*> filterBatch;
    filterBatch.push_back(&stringToFilter);
    return batchFilter(filterBatch, true);
}

int32_t ProfanityFilter::batchFilter(const eastl::vector<EA::TDF::TdfString*>& filterBatch, bool replaceInplace, const char8_t* filterId)
{
    mProfanityFilterServiceStringsChecked.increment(filterBatch.size());
    mProfanityFilterServiceBatchStringChecks.increment();

    int32_t results = batchFilterInternal(filterBatch, replaceInplace, filterId);

    if (results < 0)
        mProfanityFilterServiceErrors.increment();
    else
        mProfanityFilterServiceStringsFoundProfane.increment(results);

    return results;
}

int32_t ProfanityFilter::batchFilterInternal(const eastl::vector<EA::TDF::TdfString*>& filterBatch, bool replaceInplace, const char8_t* filterId)
{
    if (!usingProfanityService() && filterId == nullptr)
    {
        int32_t result = 0;
        for (EA::TDF::TdfString* curString : filterBatch)
        {
            if (replaceInplace)
                result += conv(const_cast<char8_t*>(curString->c_str()));
            else
                result += ProfanConv(mProfan, const_cast<char8_t*>(curString->c_str()), true);
        }

        return result;
    }

    int tryCount = 0;
    while (tryCount <= mConfig->getProfanityServiceRetryLimit())
    {
        ++tryCount;

        if (tryCount > 1)
        {
            TimeValue sleepTime;
            sleepTime.setMillis((1U << (tryCount - 2)) * mConfig->getProfanityServiceRetryDelay());
            Fiber::sleep(sleepTime, "ProfanityFilter::batchFilterInternal");

            mProfanityFilterServiceRetries.increment();
        }

        auto profanityFilterRpc = CREATE_GRPC_UNARY_RPC("profanity", eadp::profanity::ProfanityFilter, AsyncBatchFilterContent);
        if (profanityFilterRpc == nullptr)
        {
            BLAZE_ERR_LOG(Log::UTIL, "[ProfanityFilter].filter: Failed to create gRPC for string batch");
            break;  //no retry
        }

        mProfanityFilterServiceAttempts.increment();

        eadp::profanity::BatchFilterRequest request;
        request.mutable_filter_config()->set_filter_id(filterId == nullptr ? mConfig->getProfanityServiceFilterId() : filterId);
        request.mutable_filter_config()->set_include_original_content(false);
        // Add all the strings that are going to be filtered: 
        for (EA::TDF::TdfString* curString : filterBatch)
            request.add_content()->set_original_text(curString->c_str());

        Blaze::Grpc::ResponsePtr responsePtr;
        // No token to attach here as per Social folks. The security is provided by the the SSL client certs.
        Blaze::BlazeRpcError error = profanityFilterRpc->sendRequest(&request, responsePtr);

        //when Profanity Service closes gRPC channel every few minutes, error 14 UNAVAILABLE with message "Socket closed" could happen
        if (profanityFilterRpc->shouldRetry())
        {
            continue;   //retry
        }

        auto response = static_cast<eadp::profanity::BatchFilterResponse*>(responsePtr.get());
        if (error != ERR_OK || response == nullptr)
        {
            BLAZE_ERR_LOG(Log::UTIL, "[ProfanityFilter].filter: System level error - failed to filter request: " << request.DebugString().c_str() << "\n reason: " << ErrorHelp::getErrorName(error));
            break;  //no retry
        }
        if (response->status().code() != 0)
        {
            BLAZE_ERR_LOG(Log::UTIL, "[ProfanityFilter].filter: Response level error - failed to filter request: " << request.DebugString().c_str() << "\n" << "response: " << response->DebugString().c_str());
            break;  //no retry
        }
        if (request.content().size() != response->filtered_content().size())
        {
            BLAZE_ERR_LOG(Log::UTIL, "[ProfanityFilter].filter: The number of entries in request and response do not match. Skip processing. request: " << request.DebugString().c_str() << "\n" << "response: " << response->DebugString().c_str());
            break;  //no retry
        }

        mProfanityFilterServiceSuccesses.increment();

        int32_t filteredTextCount = 0;
        int32_t index = 0;
        for (EA::TDF::TdfString* curString : filterBatch)
        {
            if (response->filtered_content(index).has_profanity())
            {
                if (replaceInplace)
                {
                    curString->set(response->filtered_content(index).filtered_text().c_str());
                }
                ++filteredTextCount;
            }
            ++index;
        }

        return filteredTextCount;
    }

    if (tryCount > 1)
    {
        mProfanityFilterServiceFailsAfterRetry.increment();
        BLAZE_WARN_LOG(Log::UTIL, "[ProfanityFilter].filter: Profanity filter service failed after " << (tryCount - 1) << " retry(ies) for string batch");
    }
    else
    {
        mProfanityFilterServiceFailsWithoutRetry.increment();
        BLAZE_WARN_LOG(Log::UTIL, "[ProfanityFilter].filter: Profanity filter service failed without retry for string batch");
    }
    return -1;
}

void ProfanityFilter::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();
    char8_t buf[32];

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mProfanityFilterServiceAttempts.getTotal());
    map["TotalProfanityFilterServiceAttempts"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mProfanityFilterServiceRetries.getTotal());
    map["TotalProfanityFilterServiceRetries"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mProfanityFilterServiceSuccesses.getTotal());
    map["TotalProfanityFilterServiceSuccesses"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mProfanityFilterServiceFailsAfterRetry.getTotal());
    map["TotalProfanityFilterServiceFailsAfterRetry"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mProfanityFilterServiceFailsWithoutRetry.getTotal());
    map["TotalProfanityFilterServiceFailsWithoutRetry"] = buf;
}

} // Blaze

