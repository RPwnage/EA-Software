#ifndef IGNITION_NUCLEUS_SDK_TRACER_H
#define IGNITION_NUCLEUS_SDK_TRACER_H

#include "Ignition/NucleusLoginHelper.h"

#if defined(BLAZE_USING_NUCLEUS_SDK) && defined(EA_DEBUG)
#include <EATrace/EATrace_imp.h>

namespace Pyro
{
    class UiDriver;
}

namespace Ignition
{

// NucleusSDK logging tracer
class NucleusSdkTracer : public EA::Trace::ITracer
{
public:
    NucleusSdkTracer();
    virtual ~NucleusSdkTracer();
    void Init(Pyro::UiDriver& uiLogger);

    EA::Trace::tAlertResult Trace(const EA::Trace::TraceHelper&, const char* pText) override;
    EA::Trace::tAlertResult TraceV(const EA::Trace::TraceHelper& helper, const char* pFormat, va_list arglist) override;
    bool IsFiltered(const EA::Trace::TraceHelper& helper) const override;
    int AddRef() override;
    int Release() override;
    void* AsInterface(EA::Trace::InterfaceId iid) override;

private:
    int mnRefCount;
    Pyro::UiDriver* mUi;
};

}

//#ifdef EA_DEBUG
#endif

#endif