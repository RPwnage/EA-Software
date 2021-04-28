
#ifndef NISSMALLUSERSTORAGE_H
#define NISSMALLUSERSTORAGE_H

#include "NisSample.h"

#include "BlazeSDK/component/utilcomponent.h"

namespace NonInteractiveSamples
{

class NisSmallUserStorage
    : public NisSample
{
    public:
        NisSmallUserStorage(NisBase &nisBase);
        virtual ~NisSmallUserStorage();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            UserSettingsSaveCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void            UserSettingsLoadCb(const Blaze::Util::UserSettingsResponse * response, Blaze::BlazeError error, Blaze::JobId jobId);
};

}

#endif // NISSMALLUSERSTORAGE_H
