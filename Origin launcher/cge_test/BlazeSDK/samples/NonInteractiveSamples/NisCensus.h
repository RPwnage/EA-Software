
#ifndef NISCENSUS_H
#define NISCENSUS_H

#include "NisSample.h"

#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"

namespace NonInteractiveSamples
{

class NisCensus
    : public NisSample,
      protected Blaze::CensusData::CensusDataAPIListener
{
    public:
        NisCensus(NisBase &nisBase);
        virtual ~NisCensus();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    protected:
        void SubscribeToCensusDataCb(Blaze::BlazeError error);
        void UnsubscribeFromCensusDataCb(Blaze::BlazeError error);

        void onNotifyCensusData();

    private:
        int32_t mNotificationCount;
};

}

#endif
