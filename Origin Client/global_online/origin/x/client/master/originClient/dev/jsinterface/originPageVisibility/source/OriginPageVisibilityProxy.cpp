#include "services/debug/DebugService.h"
#include "OriginPageVisibilityProxy.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

OriginPageVisibilityProxy::OriginPageVisibilityProxy(ExposedWidgetDetector *detector, QObject *parent) :
    QObject(parent),
    mDetector(detector)
{
    // Detector might be NULL
    // If so we just always return visible and never fire signals
    if (mDetector)
    {
        ORIGIN_VERIFY_CONNECT(mDetector, SIGNAL(exposureChanged(bool)),
                this, SIGNAL(visibilityChanged()));
    }
}

QString OriginPageVisibilityProxy::visibilityState()
{
    // We don't support any of the other states
    // They're not very meaningful to us anyway
    return hidden() ? "hidden" : "visible";
}

bool OriginPageVisibilityProxy::hidden()
{
    if (mDetector)
    {
        return !mDetector->isExposed();
    }

    // Assume always visible
    return false;
}

}
}
}
