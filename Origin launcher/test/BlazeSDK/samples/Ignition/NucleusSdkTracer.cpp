#include "Ignition/NucleusSdkTracer.h"

#if defined(BLAZE_USING_NUCLEUS_SDK) && defined(EA_DEBUG)

namespace Ignition
{
NucleusSdkTracer::NucleusSdkTracer()
    : mnRefCount(0), mUi(nullptr)
{
}

NucleusSdkTracer::~NucleusSdkTracer()
{
}

void NucleusSdkTracer::Init(Pyro::UiDriver& uiLogger)
{
    mUi = &uiLogger;
    if (EA::Trace::GetTracer() != this)
        EA::Trace::SetTracer(this);
}

// ITracer
EA::Trace::tAlertResult NucleusSdkTracer::Trace(const EA::Trace::TraceHelper& helper, const char* pText)
{
    if (!mUi)
        return EA::Trace::kAlertResultNone;
    mUi->addLog_va(Pyro::ErrorLevel::NONE, "Ignition [NucleusLoginHelper::Trace()] %s", pText);
    return EA::Trace::kAlertResultNone;
}

EA::Trace::tAlertResult NucleusSdkTracer::TraceV(const EA::Trace::TraceHelper& helper, const char* pFormat, va_list arglist)
{
    if (!mUi)
        return EA::Trace::kAlertResultNone;
    eastl::string8 msg;
    mUi->addLog_va(Pyro::ErrorLevel::NONE, "Ignition [NucleusLoginHelper::Trace()] %s", msg.sprintf_va_list(pFormat, arglist).c_str());
    return EA::Trace::kAlertResultNone;
}
bool NucleusSdkTracer::IsFiltered(const EA::Trace::TraceHelper& helper) const
{
    if (helper.GetLevel() < EA::Trace::kLevelPrivate) return true;
    return false;
}

int NucleusSdkTracer::AddRef()
{
    return ++mnRefCount;
}

int NucleusSdkTracer::Release()
{
    int32_t rc = --mnRefCount;
    if (rc > 0) return rc;
    //delete this;
    return 0;
}

void* NucleusSdkTracer::AsInterface(EA::Trace::InterfaceId iid)
{
    using namespace EA::Trace;
    switch ((InterfaceIdInt)iid)
    {
    case IUnknown32::kIID: // As of this writing the 32 bit IUnknown32 GUID doesn't have its own name.
        return static_cast<IUnknown32*>(static_cast<ITracer*>(this));
    case ITracer::kIID:
        return static_cast<ITracer*>(this);
    }
    return nullptr;
}

}

//ifdef EA_DEBUG
#endif