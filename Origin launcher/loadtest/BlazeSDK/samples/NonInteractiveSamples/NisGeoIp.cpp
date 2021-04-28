#include "NisGeoIp.h"

#include "EAAssert/eaassert.h"

namespace NonInteractiveSamples
{

NisGeoIp::NisGeoIp(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisGeoIp::~NisGeoIp()
{
}

void NisGeoIp::onCreateApis()
{
}

void NisGeoIp::onCleanup()
{
}

void NisGeoIp::onRun()
{
    getUiDriver().addDiagnosticFunction();

    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Note:");
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  1. Make sure useGeoipData = true in framework.cfg");
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  2. Run SampleGeoIP from outside of WAN and against a remote blaze server, so that a proper IP is seen.");

    const Blaze::BlazeId blazeId = getBlazeHub()->getUserManager()->getLocalUser(0)->getId();
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getUserManager()->lookupUsersGeoIPData(blazeId,
        MakeFunctor(this, &NisGeoIp::LookupGeoCb)));
}

void NisGeoIp::LookupGeoCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::GeoLocationData* data)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        char buf[1024] = "";
        data->print(buf, sizeof(buf));
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, buf);
        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

}
