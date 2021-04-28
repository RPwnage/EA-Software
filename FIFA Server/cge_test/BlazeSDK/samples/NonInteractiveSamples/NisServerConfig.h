
#ifndef NISSERVERCONFIG_H
#define NISSERVERCONFIG_H

#include "NisSample.h"

#include "BlazeSDK/component/utilcomponent.h"

namespace NonInteractiveSamples
{

class NisServerConfig : 
    public NisSample
{
    public:
        NisServerConfig(NisBase &nisBase);
        virtual ~NisServerConfig();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            FetchClientConfigCb(const Blaze::Util::FetchConfigResponse * response, Blaze::BlazeError error, Blaze::JobId jobId);
};

}

#endif // NISSERVERCONFIG_H
