
#include "NisEntitlements.h"

#include "EAAssert/eaassert.h"

#include "BlazeSDK/component/authenticationcomponent.h"

using namespace Blaze::Authentication;

namespace NonInteractiveSamples
{

static const char8_t * SAMPLE_ENTITLEMENT_TAG           = "SampleEntitlementTag";
static const char8_t * SAMPLE_ENTITLEMENT_TAG_GROUPNAME = "SPORECORE";

NisEntitlements::NisEntitlements(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisEntitlements::~NisEntitlements()
{
}

void NisEntitlements::onCreateApis()
{
    getUiDriver().addDiagnosticFunction();

    // Store a pointer the the Auth component, for convenience
    mAuthComponent = getBlazeHub()->getComponentManager(0)->getAuthenticationComponent();
}

void NisEntitlements::onCleanup()
{
}

//------------------------------------------------------------------------------
// Base class calls this. We'll query for a particular entitlement.
void NisEntitlements::onRun()
{
    getUiDriver().addDiagnosticFunction();

    AppGrantEntitlement();
}

//------------------------------------------------------------------------------
// Grant the entitlement we queried
void NisEntitlements::AppGrantEntitlement(void)
{
    getUiDriver().addDiagnosticFunction();

    PostEntitlementRequest request;
    request.setEntitlementTag(SAMPLE_ENTITLEMENT_TAG); // REQUIRED
    request.setGroupName(SAMPLE_ENTITLEMENT_TAG_GROUPNAME); // REQUIRED
    request.setWithPersona(false); // do not link to the persona

    getUiDriver().addDiagnosticCallFunc(mAuthComponent->grantEntitlement(request, MakeFunctor(this, &NisEntitlements::GrantEntitlementCb)));
}

//------------------------------------------------------------------------------
// CB from grantEntitlement. If it succeeded, we'll list entitlements.
void NisEntitlements::GrantEntitlementCb(Blaze::BlazeError err, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        AppListEntitlements();
    }
    else
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// List all linked and unlinked entitlements in two groups
void NisEntitlements::AppListEntitlements(void)
{
    getUiDriver().addDiagnosticFunction();

    ListEntitlementsRequest request;  
    request.getGroupNameList().push_back("SPORE");
    request.getGroupNameList().push_back("SPORECORE");
    request.getEntitlementSearchFlag().setIncludeUnlinked();
    request.getEntitlementSearchFlag().setIncludePersonaLink();  

    getUiDriver().addDiagnosticCallFunc(mAuthComponent->listEntitlements(request, MakeFunctor(this, &NisEntitlements::ListEntitlementsCb)));
}

//------------------------------------------------------------------------------
// CB from listEntitlements.
void NisEntitlements::ListEntitlementsCb(const Entitlements *entitlements, Blaze::BlazeError err, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        const Entitlements::EntitlementList & list = entitlements->getEntitlements();
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Entitlements: %d", list.size());

        Entitlements::EntitlementList::const_iterator itr       = list.begin();
        Entitlements::EntitlementList::const_iterator itrEnd    = list.end();
        for (; itr != itrEnd; ++itr)
        {
            const Entitlement * entitlement = *itr;

            char buf[1024] = "";
            entitlement->print(buf, sizeof(buf));
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, buf);
        }

        done();
    }
    else
    {
        reportBlazeError(err);
    }
}

}
