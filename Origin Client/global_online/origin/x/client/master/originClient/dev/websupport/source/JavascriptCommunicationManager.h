#ifndef _JavascriptCommunicationManager_H
#define _JavascriptCommunicationManager_H

#include <QObject>
#include <QString>
#include <QMap>

#include "services/plugin/PluginAPI.h"

namespace WebWidget
{
    class FeatureRequest;
    class NativeInterface;
}

class QWebPage;

namespace Origin
{
namespace Client
{

///
/// \brief Class for managing adding JavaScript APIs to a QWebPage
///
/// Handles adding both JavaScript helpers and WebWidget-style feature requests to trusted web pages
///
class ORIGIN_PLUGIN_API JavascriptCommunicationManager : public QObject
{
    Q_OBJECT
public:
    ///
    /// \brief Creates an instance managing the JavaScript bridge for the passed QWebPage 
    ///
    /// \param  page Page to manager. This will also become the QObject parent for the instance. 
    ///
    explicit JavascriptCommunicationManager(QWebPage *page);

    virtual ~JavascriptCommunicationManager();

    ///
    /// \brief Adds a Javascript helper to the list of helpers to be included
    ///
    /// JavaScript helpers are normal QObjects that are attached to the JavaScript window object
    /// See the Qt Javascript Bridge documentation.
    ///
    /// JavaScript helpers are useful for small, page-specific APIs. Complex or common features should the feature
    /// request framework developed for Web Widgets. See addFeatureRequest for more information.
    ///
    /// \param  helperName  Name of the property of the window object to attach the helper to. This will also become
    ///                     the Qt object name of the helper if none is currently set
    /// \param  helper      QObject to expose under helperName. JavascriptCommunicationManager does not take ownership of the 
    ///                     helper
    /// \return True if the helper was added, false if another helper with that name already exists
    ///
    bool addHelper(const QString &helperName, QObject* helper);

    ///
    /// \brief Returns the JavaScript helper with the given name
    ///
    /// \param  helperName  Name of the JavaScript helper to look up
    /// \return QObject instance of the helper or NULL if no helper was found
    ///
    QObject* helper(const QString &helperName) const;

    ///
    /// \brief Adds a WebWidget feature request to the list of features to be included
    ///
    /// This behaves the same as &lt;feature/&gt; element do in a web widget's configuration. A list of features can be
    /// found at http://opqa-online.rws.ad.ea.com/OriginAPIDocs/bridgedocgen/index.xhtml 
    ///
    /// \param  req Feature request to add
    ///
    void addFeatureRequest(const WebWidget::FeatureRequest &req);

    ///
    /// \brief Sets the hostname trusted to have access to the JavaScript bridge
    ///
    /// If the user navigates away from the trusted host the JavaScript bridge will be deactivated until they
    /// return. Certain hostnames including .ea.com and .origin.com are implicitly trusted
    ///
    /// \param  hostname Hostname to trust
    ///
    void setTrustedHostname(const QString &hostname);
    
    ///
    /// \return  True if the current frame is trusted, false otherwise
    ///
    bool isFrameTrusted() const;

private slots:
    void populatePageJavaScriptWindowObject();

    
private:

    QWebPage *mPage;

    QString mTrustedHostname;
    
    typedef QMap< QString, QPointer<QObject> > Helpers;
    Helpers mHelpers;
    
    typedef QList<WebWidget::NativeInterface> NativeInterfaces;
    NativeInterfaces mNativeInterfaces;
};

}
}

#endif
