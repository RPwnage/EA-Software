#ifndef _WEBWIDGET_UPDATEINSTALLATION_H
#define _WEBWIDGET_UPDATEINSTALLATION_H

#include <QObject>
#include <QFutureWatcher>
#include <QTemporaryFile>

#include "WebWidgetPluginAPI.h"
#include "UpdateIdentifier.h"
#include "UpdateError.h"

class QNetworkReply;

namespace WebWidget
{
    class UpdateCache;

    ///
    /// Represents an asynchronous update installation
    ///
    /// \sa UpdateCache::installUpdate()
    ///
    class WEBWIDGET_PLUGIN_API UpdateInstallation : public QObject
    {
        Q_OBJECT
        friend class UpdateCache;
    public:
        ~UpdateInstallation();

        ///
        /// Returns if this installation is finished
        ///
        bool isFinished() const { return mFinished; }
        
        ///
        /// Returns the update identifier for this update
        ///
        UpdateIdentifier updateIdentifier() const { return mUpdateIdentifier; }

    signals:
        ///
        /// Emitted once the installation has completed
        ///
        /// \param error  Error code of the update if or UpdateNoError is no error occurred
        ///
        void finished(WebWidget::UpdateError error);

    private slots:
        void sourceReadyRead();
        void sourceFinished();
        void unpackFinished();

    private:
        UpdateInstallation(UpdateCache *cache, const UpdateIdentifier &identifier, QNetworkReply *source);
        void finish(UpdateError error);

        QFutureWatcher<UpdateError> mUnpackWatcher;

        UpdateCache *mCache;
        UpdateIdentifier mUpdateIdentifier;
        QNetworkReply *mSource;
        QTemporaryFile mTemporaryPackage;
        bool mFinished;
    };
}

#endif
