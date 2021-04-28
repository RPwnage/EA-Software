#ifndef _WEBWIDGET_UPDATEIDENTIFIER_H
#define _WEBWIDGET_UPDATEIDENTIFIER_H

#include <QUrl>
#include <QByteArray>

#include "WebWidgetPluginAPI.h"

class QDataStream;

namespace WebWidget
{
    ///
    /// Identifies a unique version of a widget update
    ///
    /// This is a combination of three identifying values:
    /// - The URL initially used to query for the update
    /// - The final URL the update was downloaded from
    /// - The ETag of the final URL
    ///
    /// Note that this means that otherwise identical updates fetched from different URLs will have unique identifiers.
    ///
    class WEBWIDGET_PLUGIN_API UpdateIdentifier
    {
    public:
        ///
        /// Creates a null UpdateIdentifier
        ///
        UpdateIdentifier() {}

        ///
        /// Creates a new UpdateIdentifier
        ///
        /// \param  updateUrl URL initially used to query for the update 
        /// \param  finalUrl  URL the update was downloaded from
        /// \param  etag      ETag for the downloaded update
        ///
        UpdateIdentifier(const QUrl &updateUrl, const QUrl &finalUrl, const QByteArray &etag) :
            mUpdateUrl(updateUrl),
            mFinalUrl(finalUrl),
            mEtag(etag) {}

        ///
        /// Returns the URL initially requested to download the update before redirects
        ///
        QUrl updateUrl() const { return mUpdateUrl; }
         
        ///
        /// Returns the URL the update was downloaded from after redirects
        ///
        QUrl finalUrl() const { return mFinalUrl; }

        ///
        /// Returns the ETag of the final URL
        ///
        QByteArray etag() const { return mEtag; }
        
        ///
        /// Returns if this is a null widget update
        ///
        bool isNull() const { return mEtag.isNull(); }

        ///
        /// Compares two UpdateIdentifier instances for equality
        ///
        bool operator==(const UpdateIdentifier &other) const;

        ///
        /// Compares two UpdateIdentifier instances for inequality
        ///
        bool operator!=(const UpdateIdentifier &other) const;

    private:
        QUrl mUpdateUrl;
        QUrl mFinalUrl;
        QByteArray mEtag;
    };
}

WEBWIDGET_PLUGIN_API QDataStream& operator<<(QDataStream &, const WebWidget::UpdateIdentifier &);
WEBWIDGET_PLUGIN_API QDataStream& operator>>(QDataStream &, WebWidget::UpdateIdentifier &);

#endif
