#include <QDebug>
#include <QFile>
#include <QDomDocument>
#include <QRegExp>

#include "WidgetConfiguration.h"
#include "WidgetPackageFile.h"

namespace
{
    const QString WidgetNamespaceURI("http://www.w3.org/ns/widgets");
    const QString OriginExtensionNamespaceURI("http://widgets.dm.origin.com/configuration");

    // Parses an attribute and simplifies whitespace
    // Implements http://www.w3.org/TR/widgets/#rule-for-getting-a-single-attribute-value-0
    QString parseAttribute(const QDomElement &element, const QString &attributeName, const QString &defaultValue = QString(), bool treatEmptyAsMissing = true)
    {
        if (!element.hasAttribute(attributeName))
        {
            return defaultValue;
        }

        QString attributeValue(element.attribute(attributeName));
        attributeValue = attributeValue.simplified();

        if (treatEmptyAsMissing && attributeValue.isEmpty())
        {
            return defaultValue;
        }

        return attributeValue;
    }
     
    // Parses a boolean attribute as per the widget spec
    bool parseBooleanAttribute(const QDomElement &element, const QString &attributeName, bool defaultValue = false)
    {
        // This works if the attribute isn't set as it'll return a null string
        const QString attributeValue(parseAttribute(element, attributeName));

        if (attributeValue == "true")
        {
            return true;
        }
        else if (attributeValue == "false")
        {
            return false;
        }

        return defaultValue;
    }

    // This isn't fully correct but should catch the most grievous cases
    // The important thing is we don't let path special characters through
    bool validateBcp47Tag(const QString &tag)
    {
        return QRegExp("[a-zA-Z0-9\\-]+").exactMatch(tag);
    }

    // Checks that an element is in the Or
}

namespace WebWidget
{
    WidgetConfiguration::WidgetConfiguration() :
        mValid(false)
    {
    }

    WidgetConfiguration::WidgetConfiguration(const WidgetPackageFile &configDoc) : 
        mValid(false)
    {
        const QString backingFilePath(configDoc.backingFileInfo().absoluteFilePath());
        QFile configFile(backingFilePath);

        if (!configFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "Unable to open widget configuration file:" << backingFilePath;
            return;
        }

        QDomDocument doc;

        if (!doc.setContent(&configFile, true))
        {
            qWarning() << "Unable to parse widget configuration XML";
            return;
        }

        QDomElement widgetElement(doc.documentElement());

        if ((widgetElement.tagName() != "widget") ||
            (widgetElement.namespaceURI() != WidgetNamespaceURI))
        {
            qWarning() << "Unexpected widget configuration XML structure";
            // Unexpected document element and namespace
            return;
        }

        // Handle the widget element itself
        handleWidgetElement(widgetElement);
        
        // We should only parse the first content element if it's valid or not
        bool seenContentElement = false;
        for(QDomElement element = widgetElement.firstChildElement();
            !element.isNull();
            element = element.nextSiblingElement())
        {
            if (element.namespaceURI() == WidgetNamespaceURI)
            {
                // Check for standardized tags
                if (element.tagName() == "access")
                {
                    handleAccessElement(element);
                }
                else if (!seenContentElement && (element.tagName() == "content"))
                {
                    handleContentElement(element);
                    seenContentElement = true;
                }
                else if (element.tagName() == "feature")
                {
                    handleFeatureElement(element);
                }
            }
            else if (element.namespaceURI() == OriginExtensionNamespaceURI)
            {
                if (element.tagName() == "link")
                {
                    handleLinkElement(element);
                }
            }
        }

        mValid = true;
    }
    
    void WidgetConfiguration::handleWidgetElement(const QDomElement &element)
    {
        if (element.hasAttribute("version"))
        {
            QString version = parseAttribute(element, "version");

            // We must ignore empty version attributes
            // In our case that means leaving them as null QStrings
            if (!version.isEmpty())
            {
                mVersion = version;
            }
        }

        if (element.hasAttribute("defaultlocale"))
        {
            // Normalize to lowercase
            // Widgets can't read this back so it doesn't really matter
            QString defaultLocale = parseAttribute(element, "defaultlocale").toLower();

            // Does this look vaguely valid?
            if (validateBcp47Tag(defaultLocale))
            {
                mDefaultLocale = defaultLocale;
            }
        }
    }

    void WidgetConfiguration::handleAccessElement(const QDomElement &element)
    {
        QString originString = parseAttribute(element, "origin");

        // Is this the all resources request?
        if (originString == "*")
        {
            mAccessRequests.append(AccessRequest(AccessRequest::AllResources));
            return;
        }

        // Figure out our security origin
        QUrl originUrl(originString, QUrl::StrictMode);
        
        // Make sure we only scheme and host and optionally a port
        if (!originUrl.isValid() || 
            originUrl.host(QUrl::FullyEncoded).isEmpty() ||
            !originUrl.userInfo(QUrl::FullyEncoded).isEmpty() ||
            !originUrl.path(QUrl::FullyEncoded).isEmpty() ||
            !originUrl.fragment(QUrl::FullyEncoded).isEmpty() ||
            !originUrl.query(QUrl::FullyEncoded).isEmpty())
        {
            qWarning() << "Invalid widget access origin:" << originString;
            return;
        }

        // Figure out our scope
        AccessRequest::AccessScope scope;
        
        if (parseBooleanAttribute(element, "subdomains"))
        {
            scope = AccessRequest::SecurityOriginAndSubdomains;
        }
        else
        {
            scope = AccessRequest::SecurityOriginOnly;
        }

        // Add the access request
        AccessRequest accessRequest(scope, SecurityOrigin(originUrl));
        mAccessRequests.append(accessRequest);
    }

    void WidgetConfiguration::handleContentElement(const QDomElement &element)
    {
        const QString src(parseAttribute(element, "src"));

        if (src.isEmpty())
        {
            // No path specified
            return;
        }

        mCustomStartFile = CustomStartFile(
            src,
            parseAttribute(element, "type", "text/html", true).toLatin1(),
            parseAttribute(element, "encoding", "UTF-8", true).toLatin1());
    }

    void WidgetConfiguration::handleFeatureElement(const QDomElement &element)
    {
        if (!element.hasAttribute("name"))
        {
            // Not valid
            return;
        }

        FeatureParameters parameters;

        // Parse the passed parameters
        for(QDomElement param = element.firstChildElement("param");
            !param.isNull();
            param = param.nextSiblingElement("param"))
        {
            if (param.namespaceURI() != WidgetNamespaceURI)
            {
                continue;
            }

            const QString paramName(parseAttribute(param, "name"));

            // Allow empty values but not empty names
            if (!paramName.isEmpty() && param.hasAttribute("value"))
            {
                // Add it to the parameters
                parameters.insert(paramName, parseAttribute(param, "value"));
            }
        }

        // Add the feature request to the list
        mFeatureRequests.append(FeatureRequest(parseAttribute(element, "name"),
            parseBooleanAttribute(element, "required", true),
            parameters));
    }

    void WidgetConfiguration::handleLinkElement(const QDomElement &element)
    {
        if (!element.hasAttribute("role") || !element.hasAttribute("template"))
        {
            // Not valid
            return;
        }

        mLinks.insert(parseAttribute(element, "role"), 
            UriTemplate(parseAttribute(element, "template")));
    }
}
