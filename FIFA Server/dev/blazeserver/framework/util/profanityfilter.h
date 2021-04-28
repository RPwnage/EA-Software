/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROFANITYFILTER_H
#define BLAZE_PROFANITYFILTER_H

/*** Include files *******************************************************************************/

#include "framework/lobby/profan.h"
#include "eathread/eathread_storage.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ProfanityServiceConfig;

class ProfanityFilter
{
    NON_COPYABLE(ProfanityFilter);

public:
    ProfanityFilter();
    ~ProfanityFilter();

    bool onValidateConfig(const ProfanityServiceConfig& config, ConfigureValidationErrors& validationErrors);
    void onConfigure(const ProfanityServiceConfig& config);
    void onReconfigure(const ProfanityServiceConfig& config);

    // Filters a line of text, puts it in the output buffer:
    int32_t isProfane(const char8_t* stringToCheck);
    int32_t inplaceFilter(EA::TDF::TdfString& stringToFilter);

    // Batch (inplace) filtering command:
    // If a filter id is provided, then the filter service will be used (regardless of the config)
    int32_t batchFilter(const eastl::vector<EA::TDF::TdfString*>& filterBatch, bool replaceInplace, const char8_t* filterId = nullptr);

    bool usingProfanityService();
    void getStatusInfo(ComponentStatus& status) const;

protected:
    // Used in onValidateConfig(). Builds a new pending ProfanRef that will replace the
    // existing ProfanRef on successful (re)configuration.
    int32_t buildValidatedFilter(const ProfanityServiceConfig& config);

    // Completes (re)configuration of the profanity filter.
    // Replaces the existing ProfanRef with the pending ProfanRef created
    // by buildValidatedFilter().
    void completeConfiguration();

    // DEPRECATED - Old functionality: 
    // convert a line of text
    int32_t conv(char8_t* line)
    {
        return ProfanConv(mProfan, line);
    }

    // let user set their own substitution table
    void subst(const char8_t* sublst)
    {
        return ProfanSubst(mProfan, sublst);
    }

    // chop leading/trailing/extra spaces from line
    int32_t chop(char8_t *line)
    {
        return ProfanChop(mProfan, line);
    }

    int32_t sanitize(char8_t *line)
    {
        ProfanChop(mProfan, line);
        return ProfanConv(mProfan, line);
    }

    const char8_t * profanLastMatchedRule()
    {
        return ProfanLastMatchedRule(mProfan);
    }

    int32_t profanSave(int32_t iFd)
    {
        return ProfanSave(mProfan, iFd);
    }

    int32_t profanLoad(int32_t iFd)
    {
        return ProfanLoad(mProfan, iFd);
    }
    
private:
    int32_t batchFilterInternal(const eastl::vector<EA::TDF::TdfString*>& filterBatch, bool replaceInplace, const char8_t* filterId = nullptr);

    ProfanRef* mProfan;
    ProfanRef* mPendingProfan;
    const ProfanityServiceConfig* mConfig;

    Metrics::Counter mProfanityFilterServiceStringsChecked;
    Metrics::Counter mProfanityFilterServiceBatchStringChecks;
    Metrics::Counter mProfanityFilterServiceStringsFoundProfane;
    Metrics::Counter mProfanityFilterServiceErrors;

    Metrics::Counter mProfanityFilterServiceAttempts;               //count of the total attempts/tries to use profanity service
    Metrics::Counter mProfanityFilterServiceRetries;                //count of the total retries to use profanity service
    Metrics::Counter mProfanityFilterServiceSuccesses;              //count of the total attempts that succeeded
    Metrics::Counter mProfanityFilterServiceFailsAfterRetry;        //count of the total attempts that failed after retry (number of retries is configurable)
    Metrics::Counter mProfanityFilterServiceFailsWithoutRetry;      //count of the total attempts that failed without retry (depending on error status, some attempts are not retried at all)
};

extern EA_THREAD_LOCAL ProfanityFilter* gProfanityFilter;

} // Blaze

#endif // BLAZE_PROFANITYFILTER_H

