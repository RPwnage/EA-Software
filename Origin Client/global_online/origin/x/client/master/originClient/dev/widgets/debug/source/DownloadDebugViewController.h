/////////////////////////////////////////////////////////////////////////////
// DownloadDebugViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef _DOWNLOADDEBUGVIEWCONTROLLER_H_
#define _DOWNLOADDEBUGVIEWCONTROLLER_H_

#include <QObject>
#include <QString>
#include <QPointer>

#include "engine/content/Entitlement.h"
#include "engine/debug/DownloadDebugDataManager.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}

namespace Client
{
class DownloadDebugView;

class ORIGIN_PLUGIN_API DownloadDebugViewController : public QObject
{
	Q_OBJECT
public:
    /// \brief The DownloadDebugViewController constructor.
    /// \param entitlement Reference to the entitlement being downloaded.
    /// \param parent This object's parent object, if any.
	DownloadDebugViewController(Engine::Content::EntitlementRef entitlement, DownloadDebugView* view, QObject* parent = NULL);

    /// \brief The DownloadDebugViewController destructor.
    ~DownloadDebugViewController();


private slots:
    /// \brief Called when the view's table selection has changed.
    /// \param installLocation Value from the installLocation column.  Used as a unique identifier.
    void onSelectionChanged(const QString& installLocation);

    /// \brief Called when the user clicks Export.
    void onExportRequested();
            
    /// \brief Called when tracking for a download has started.
    /// \param productId The productId of the tracked download.
    void onDownloadAdded(const QString& productId);

    /// \brief Called when tracking for a download has ended.
    /// \param productId The productId of the tracked download.
    void onDownloadRemoved(const QString& productId);

    /// \brief Called when the downloader makes progress on a tracked download.
    /// \param fileNames The list of file names that have made progress.
    void onFileProgress(QList<QString> fileNames);

private:
    DownloadDebugView* mView; ///< The download debug view.
    Engine::Content::EntitlementRef mEntitlement; ///< The entitlement to monitor.
    QPointer<Engine::Debug::DownloadDebugDataCollector> mCollector; ///< Object that collects file-by-file progress information.
};
}
}

#endif
