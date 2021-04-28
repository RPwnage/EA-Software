#include "JavascriptCommunicationManager.h"

#include <QtCore/QPointer>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>

#include "WebWidget/FeatureRequest.h"
#include "WebWidget/NativeInterface.h"
#include "WebWidget/NativeInterfaceRegistry.h"
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Client
{
    
JavascriptCommunicationManager::JavascriptCommunicationManager(QWebPage *page) :
    QObject(page),
    mPage(page)
{
    ORIGIN_VERIFY_CONNECT(mPage->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));
}

JavascriptCommunicationManager::~JavascriptCommunicationManager()
{
    // Delete any interface objects we have ownership of
    for(NativeInterfaces::ConstIterator it = mNativeInterfaces.constBegin();
        it != mNativeInterfaces.constEnd();
        it++)
    {
        if (it->hasOwnership())
        {
            delete it->bridgeObject();
        }
    }
}

bool JavascriptCommunicationManager::addHelper(const QString &helperName, QObject *helper)
{
    if (!mHelpers.contains(helperName))
    {
        if (helper->objectName().isEmpty())
        {
            helper->setObjectName(helperName);
        }

        mHelpers[helperName] = QPointer<QObject>(helper);
        return true;
    }

    return false;
}



QObject* JavascriptCommunicationManager::helper(const QString &helperName) const
{
    return mHelpers[helperName];
}

void JavascriptCommunicationManager::addFeatureRequest(const WebWidget::FeatureRequest &req)
{
    QList<WebWidget::FeatureRequest> requests;
    requests.append(req);

    mNativeInterfaces.append(WebWidget::NativeInterfaceRegistry::interfacesForFeatureRequests(mPage->mainFrame(), requests));
}

void JavascriptCommunicationManager::setTrustedHostname(const QString &hostname)
{
    mTrustedHostname = hostname;
}

bool JavascriptCommunicationManager::isFrameTrusted() const
{
    const QString hostname = mPage->mainFrame()->url().host();

    return (hostname == mTrustedHostname) || 
        hostname.endsWith(QString(".ea.com"), Qt::CaseInsensitive) ||
        hostname.endsWith(QString(".secure.force.com"), Qt::CaseInsensitive) ||
        hostname.endsWith(QString(".origin.com"), Qt::CaseInsensitive) ||
        mPage->mainFrame()->url().toString().contains("login_successful");
}
    
void JavascriptCommunicationManager::populatePageJavaScriptWindowObject()
{
    if (!isFrameTrusted())
    {
        // Not trusted
        return;
    }
    
    QWebFrame* frame = mPage->mainFrame();

    // Add all the native interfaces on
    for(NativeInterfaces::const_iterator it = mNativeInterfaces.constBegin();
        it != mNativeInterfaces.constEnd();
        it++)
    {
        frame->addToJavaScriptWindowObject(it->name(), it->bridgeObject());
    }
    
    // Add our helpers 
    QMap< QString, QPointer<QObject> >::const_iterator iHelper = mHelpers.constBegin();
    while (iHelper != mHelpers.constEnd()) 
    {
        frame->addToJavaScriptWindowObject(iHelper.key(), iHelper.value().data());
        ++iHelper;
    }
}


}
}
