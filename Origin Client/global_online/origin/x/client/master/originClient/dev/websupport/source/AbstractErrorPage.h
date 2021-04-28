#ifndef _ABSTRACTERRORPAGE_H
#define _ABSTRACTERRORPAGE_H

#include <QString>
#include "PageErrorDetector.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

///
/// Abstract interface for generating error HTML for web pages
///
class ORIGIN_PLUGIN_API AbstractErrorPage
{
public:
    virtual ~AbstractErrorPage()
    {
    }

    ///
    /// \brief Called to generate error HTML when the user enters offline mode
    ///
    /// The default implementation calls errorHtml()
    ///
    /// \return Error page HTML to display or a null QString to ignore the error
    ///
    virtual QString offlineErrorHtml() const { return errorHtml(); }

    ///
    /// \brief Called to generate error HTML when the client is recovering from invalid auth token
    ///
    /// The default implementation calls errorHtml()
    ///
    /// \return Error page HTML to display or a null QString to ignore the error
    ///
    virtual QString recoveringErrorHtml() const { return errorHtml(); }

    ///
    /// \brief Called to generate error HTML when a page error has been detected
    ///
    /// The default implementation calls errorHtml()
    ///
    /// \param  type  The type of page error detected
    /// \param  reply The network reply generating the error if applicable
    /// \return Error page HTML to display or a null QString to ignore the error
    ///
    virtual QString pageErrorHtml(PageErrorDetector::ErrorType type, QNetworkReply *reply) const { return errorHtml(); }

    ///
    /// \brief Called to generate error HTML when an error has been detected
    ///
    /// The default implementation ignores all errors by return a null QString
    ///
    /// \return Error page HTML to display
    ///
    virtual QString errorHtml() const { return QString(); };
};

}
}

#endif
