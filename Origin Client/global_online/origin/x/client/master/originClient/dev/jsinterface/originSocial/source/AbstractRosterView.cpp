#include "AbstractRosterView.h"
#include "RemoteUserProxy.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

AbstractRosterView::AbstractRosterView(QObject *parent) : 
    QObject(parent),
    m_delayedChangedQueued(false)
{
}

QObjectList AbstractRosterView::contacts()
{
    QObjectList ret; 

    for(QSet<RemoteUserProxy*>::const_iterator it = m_currentMembers.constBegin();
        it != m_currentMembers.constEnd();
        it++)
    {
        ret.append(*it);
    }

    return ret;
}

void AbstractRosterView::markViewChanged()
{
    // Simple event compression
    if (!m_delayedChangedQueued)
    {
        m_delayedChangedQueued = true;
        QMetaObject::invokeMethod(this, "emitDelayedChanged", Qt::QueuedConnection);
    }
}

void AbstractRosterView::emitDelayedChanged()
{
    m_delayedChangedQueued = false;
    emit changed();
}

// Functions called from OriginSocialProxy
void AbstractRosterView::unfilteredContactAddedOrChanged(RemoteUserProxy *proxy, bool initialAdd)
{
    // Call our filter function
    bool shouldInclude = contactIncludedInView(proxy->proxied()); 
    bool alreadyIncluded = m_currentMembers.contains(proxy);
 
    if (shouldInclude == alreadyIncluded)
    {
        return;
    }

    if (shouldInclude)
    {
        m_currentMembers.insert(proxy);
        emit contactAdded(proxy);
    }
    else
    {
        m_currentMembers.remove(proxy);
        emit contactRemoved(proxy);
    }
        
    if (!initialAdd)
    {
        markViewChanged();
    }
}

void AbstractRosterView::unfilteredContactRemoved(RemoteUserProxy *proxy)
{
    if (m_currentMembers.remove(proxy))
    {
        emit contactRemoved(proxy);    
        markViewChanged();
    }
}

}
}
}
