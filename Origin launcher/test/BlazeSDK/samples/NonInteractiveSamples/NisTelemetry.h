
#ifndef NISTELEMETRY_H
#define NISTELEMETRY_H

#include "NisSample.h"

#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/apidefinitions.h"
#include "BlazeSDK/util/telemetryapi.h"

namespace NonInteractiveSamples
{

class NisTelemetry : 
    public NisSample
{
    public:
        NisTelemetry(NisBase &nisBase);
        virtual ~NisTelemetry();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

        virtual void    Update(void);
        virtual void    PreLogin(void);

    private:

        // Wrapper for submitting a telemetry 3.0 event
        int32_t SubmitEvent(TelemetryApiRefT *);

        // Telemetry API handle
        TelemetryApiRefT *mTelemetryApiRef;

        // Disabled countries list
        char mDisabledCountries[640];

        bool mAutoLogin;
};

}
#endif // NISTELEMETRY_H
