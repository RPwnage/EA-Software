
#include "NisSample.h"

#include "NisBase.h"

namespace NonInteractiveSamples
{

NisSample::NisSample(NisBase &nisBase)
    : mNisBase(nisBase)
{
}

Pyro::UiDriver &NisSample::getUiDriver()
{
    return *mNisBase.getUiDriver();
}

Blaze::BlazeHub *NisSample::getBlazeHub()
{
    return mNisBase.getBlazeHub();
}

void NisSample::done()
{
    mNisBase.killRunningNis();
}

void NisSample::reportBlazeError(Blaze::BlazeError error)
{
    mNisBase.getUiDriver()->addLog(Pyro::ErrorLevel::ERR, "Blaze error");
    this->done();
}

}
