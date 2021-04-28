
#ifndef NISGAMEREPORTING_H
#define NISGAMEREPORTING_H

#include "NisSample.h"

#include "BlazeSDK/component/gamereportingcomponent.h"

namespace NonInteractiveSamples
{

class NisGameReporting
    : public NisSample
{
    public:
        NisGameReporting(NisBase &nisBase);
        virtual ~NisGameReporting();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            AppSubmitOfflineGameReport(void);
        void            ResultNotificationCb(const Blaze::GameReporting::ResultNotification * notification, uint32_t userIndex);
        void            SubmitOfflineGameReportCb(Blaze::BlazeError err, Blaze::JobId);
        void            AppGetGameReports(void);
        void            GetGameReportsCb(const Blaze::GameReporting::GameReportsList * gameReportsList, Blaze::BlazeError errorCode, Blaze::JobId jobId);

        Blaze::GameReporting::GameReportingComponent * mGameReportingComponent;
};

}

#endif // NISGAMEREPORTING_H
