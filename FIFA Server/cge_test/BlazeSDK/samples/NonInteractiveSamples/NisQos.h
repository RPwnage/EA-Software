
#ifndef NISQOS_H
#define NISQOS_H

#include "NisSample.h"

#include "BlazeSDK/connectionmanager/connectionmanager.h"

namespace NonInteractiveSamples
{

class NisQos : 
    public NisSample,
    protected Blaze::ConnectionManager::ConnectionManagerListener
{
    public:
        NisQos(NisBase &nisBase);
        virtual ~NisQos();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    protected:
        // From ConnectionManagerListener:
        virtual void    onQosPingSiteLatencyRetrieved(bool success);

    private:
        void            LogQosData(void);

        Blaze::ConnectionManager::ConnectionManager * mConnectionManager;
};

}

#endif // NISQOS_H
