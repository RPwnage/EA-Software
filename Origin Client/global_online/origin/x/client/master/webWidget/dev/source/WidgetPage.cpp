#include <QDebug>
#include <QtWebKitWidgets/QWebFrame>

#include "WidgetPage.h"
#include "WidgetRepository.h"
#include "WidgetPackageReader.h"
#include "WidgetNetworkAccessManager.h"
#include "NativeInterfaceInjector.h"
#include "NativeInterfaceRegistry.h"

namespace WebWidget
{
    WidgetPage::WidgetPage(QObject *parent) :
        QWebPage(parent),
        mExternalNetworkAccessManager(NULL),
        mMainFrameInjector(NULL)
    {
        connect(this, SIGNAL(frameCreated(QWebFrame *)), 
                this, SLOT(onFrameCreated(QWebFrame *)));
    }

    WidgetPage::~WidgetPage()
    {
        // Delete any page specific interfaces we have ownership of
        for(QList<NativeInterface>::const_iterator it = mPageSpecificInterfaces.constBegin();
            it != mPageSpecificInterfaces.constEnd();
            it++)
        {
            if (it->hasOwnership())
            {
                delete it->bridgeObject();
            }
        }
    }

    bool WidgetPage::loadWidget(WidgetRepository *repository, const QString &widgetName, const WidgetLink &link, const QLocale &locale)
    {
        return loadWidget(repository->installedWidget(widgetName), link, locale);
    }

    bool WidgetPage::loadWidget(const InstalledWidget &widget, const WidgetLink &link, const QLocale &locale)
    {
        resetWidgetContext();

        if (widget.isNull())
        {
            qWarning() << "Attempted to load a null widget";
            return false;
        }

        // Create a new widget package reader for our locale
        WidgetPackageReader packageReader(widget.package(), locale);

        // Check if this is a valid widget package
        if (!packageReader.isValidPackage())
        {
            qWarning() << "Attempted to load an invalid widget";
            return false;
        }

        // Store our configuration
        mWidgetConfiguration = packageReader.configuration();

        // Find our widget's authority
        // widget:// doesn't allow usernames or ports so the host is the entire authority
        const QString authority(widget.securityOrigin().host());

        // Set our network access manager
        setNetworkAccessManager(new WidgetNetworkAccessManager(authority, packageReader, mExternalNetworkAccessManager, this));
        
        // Set our security origin
        // This is used to confine our native interfaces and links to the widget's security origin
        // This must be set before resolving links
        mSecurityOrigin = widget.securityOrigin();
        
        // Create an injector for our feature requests
        mMainFrameInjector = createInterfaceInjectorForFrame(mainFrame());
                
        if (mMainFrameInjector == NULL)
        {
            resetWidgetContext();
            return false;
        }

        // Start building our start URL
		mStartUrl.clear();
        mStartUrl.setScheme("widget");
        mStartUrl.setAuthority(authority);

        if (link.isNull())
        {
            // Create the proper widget URL for the start file
            const QString widgetPath = packageReader.startFile().widgetPath();
            mStartUrl.setPath("/" + widgetPath);
        }
        else
        {
            // Resolve the link versus the start URL
            mStartUrl = resolveWidgetLink(link, mStartUrl);

            if (!mStartUrl.isValid())
            {
                qWarning() << "Unable to resolve initial widget link to" << link.roleUri();
                resetWidgetContext();
                return false;
            }
        }

        // mainFrame()->load(mStartUrl)
        // -- Stupid alert --
        // Don't use mainFrame()->load(mStartUrl) because for some reason it doesn't set the .url() property (!) which then causes loadLink() to fail
        // Maybe a Qt 5 bug??
         mainFrame()->setUrl(mStartUrl);

        return true;
    }

	QUrl WidgetPage::startUrl() const
	{
		return mStartUrl;
	}


    bool WidgetPage::loadLink( const WidgetLink &link, bool isForceReload /*= false*/ )
    {
        const QUrl resolvedUrl(resolveWidgetLink(link, mainFrame()->url()));
        
        if (!resolvedUrl.isValid())
        {
            qWarning() << "Unable to load widget link to" << link.roleUri();
            return false;
        }

        if (isForceReload)
        {
            mainFrame()->load(resolvedUrl);
        } 
        else
        {
            if (mainFrame()->url() != resolvedUrl)
            {
                mainFrame()->load(resolvedUrl);
            }
        }

        return true;
    }

    QUrl WidgetPage::resolveWidgetLink(const WidgetLink &link, const QUrl &baseUrl) const
    {
        if (!mWidgetConfiguration.isValid() || link.isNull())
        {
            return QUrl();
        }

        UriTemplate linkTemplate = mWidgetConfiguration.links()[link.roleUri()];

        if (linkTemplate.isNull())
        {
            return QUrl();
        }

        // Replace the template variables
        const QUrl expandedUrl(linkTemplate.expand(link.variables()));
        
        // Don't allow accidental or malicious links to outside our security origin
        const QUrl resolvedUrl(baseUrl.resolved(expandedUrl));

        if (mSecurityOrigin.isNull() || (SecurityOrigin(resolvedUrl) != mSecurityOrigin))
        {
            return QUrl();
        }

        return resolvedUrl;
    }

    void WidgetPage::resetWidgetContext()
    {
        // Clear all native interfaces and our security origin
        delete mMainFrameInjector;
        mMainFrameInjector = NULL;

        mSecurityOrigin = SecurityOrigin();
        mWidgetConfiguration = WidgetConfiguration();
    }
        
    bool WidgetPage::shouldInterruptJavaScript()
    {
#ifdef _DEBUG
        qWarning() << "WebWidget JavaScript for" << mSecurityOrigin.host() << "is taking excessive main loop time";
#endif
        return false;
    }
        
    void WidgetPage::onFrameCreated(QWebFrame *frame)
    {
        createInterfaceInjectorForFrame(frame);
    }
    
    NativeInterfaceInjector* WidgetPage::createInterfaceInjectorForFrame(QWebFrame *frame)
    {
        QList<FeatureRequest> featureRequests = mWidgetConfiguration.featureRequests();
        QList<NativeInterface> nativeInterfaces = NativeInterfaceRegistry::interfacesForFeatureRequests(frame, featureRequests);
        
        // Add interfaces the widget explicitly requested
        for(QList<FeatureRequest>::const_iterator it = featureRequests.constBegin();
            it != featureRequests.constEnd();
            it++)
        {
            if (it->required())
            {
                qWarning() << "Unable to satisfy required feature request" << it->uri();
                return NULL;
            }
        }

        // Add page-specific interfaces
        for(QList<NativeInterface>::const_iterator it = mPageSpecificInterfaces.constBegin();
            it != mPageSpecificInterfaces.constEnd();
            it++)
        {
            nativeInterfaces << it->clone();
        }

        return new NativeInterfaceInjector(frame, mSecurityOrigin, nativeInterfaces);
    }
        
    void WidgetPage::setExternalNetworkAccessManager(QNetworkAccessManager *manager)
    {
        mExternalNetworkAccessManager = manager;
    }

    QNetworkAccessManager* WidgetPage::externalNetworkAccessManager() const
    {
        return mExternalNetworkAccessManager;
    }
        
    WidgetConfiguration WidgetPage::widgetConfiguration() const
    {
        return mWidgetConfiguration;
    }
        
    void WidgetPage::addPageSpecificInterface(const NativeInterface &interface)
    {
        mPageSpecificInterfaces.append(interface);
    } 

}
