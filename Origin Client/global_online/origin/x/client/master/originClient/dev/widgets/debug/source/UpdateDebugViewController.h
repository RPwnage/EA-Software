/////////////////////////////////////////////////////////////////////////////
// UpdateDebugViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef _UPDATEDEBUGVIEWCONTROLLER_H_
#define _UPDATEDEBUGVIEWCONTROLLER_H_

#include <QObject>
#include <QString>
#include <QPointer>

#include "engine/content/Entitlement.h"
#include "engine/debug/DownloadDebugDataManager.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Engine
{
namespace Debug
{
class CalculateUpdateDetailsHelper;
}
}

namespace Client
{
class UpdateDebugView;

class ORIGIN_PLUGIN_API UpdateDebugViewController : public QObject
{
	Q_OBJECT
public:
    /// \brief The UpdateDebugViewController constructor.
    /// \param entitlement Reference to the entitlement being updated.
    /// \param parent This object's parent object, if any.
	UpdateDebugViewController(Engine::Content::EntitlementRef entitlement, UpdateDebugView* view, QObject* parent = NULL);

    /// \brief The UpdateDebugViewController destructor.
    ~UpdateDebugViewController();

private slots:
    /// \brief Called when the user clicks Export.
    void onExportRequested();

    /// \brief Called when tracking for a download has started.
    /// \param productId The productId of the tracked download.
    void onDownloadAdded(const QString& productId);

    /// \brief Called when tracking for a download has ended.
    /// \param productId The productId of the tracked download.
    void onDownloadRemoved(const QString& productId);
    
    /// \brief Called when the downloader has calculated the update.
    void onUpdateCalculated();

    void onUrlFetchFinished();

private:
    void calculateUpdateForUrl(const QString& url);

private:
    Engine::Debug::CalculateUpdateDetailsHelper* mCalcHelper;

    UpdateDebugView* mView; ///< The update debug view.
    Engine::Content::EntitlementRef mEntitlement; ///< The entitlement to monitor.
    QPointer<Engine::Debug::DownloadDebugDataCollector> mCollector; ///< Object that collects file-by-file information.
};
}
}

#endif
