
#ifndef NISGEOIP_H
#define NISGEOIP_H

#include "NisSample.h"

namespace NonInteractiveSamples
{

class NisGeoIp 
    : public NisSample
{
    public:
        NisGeoIp(NisBase &nisBase);
        virtual ~NisGeoIp();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            LookupGeoCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::GeoLocationData* data);
};

}

#endif // NISGEOIP_H
