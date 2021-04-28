// DcmtDownloadViewController.h
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef DCMT_DOWNLOAD_VIEW_CONTROLLER_H
#define DCMT_DOWNLOAD_VIEW_CONTROLLER_H

#include "Qt/originwindow.h"
#include "WebWidget/WidgetView.h"
#include "ShiftDirectDownload/DcmtFileDownloader.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

/// \brief Controller for download window.
class DcmtDownloadViewController : public QObject
{
	Q_OBJECT

public:
    /// \brief Constructor
    /// \param download The download for which to display a view.
    /// \param parent The parent of the widget - shouldn't be used.
    DcmtDownloadViewController(DcmtFileDownloader* download, QObject* parent = 0);

    /// \brief Destructor
	~DcmtDownloadViewController();

    /// \brief Destroys all DcmtDownloadViewControllers that have been created since startup.
    static void destroy();

    /// \brief Shows the window.
	void show();

public slots:
    /// \brief Closes the window.
    void close();

private slots:
    void onDownloadError();
    void onDownloadComplete();

private:
    UIToolkit::OriginWindow* mDownloadWindow;
    QPointer<DcmtFileDownloader> mDownload;

    // Need to track all created instances of DcmtDownloadViewController so we
    // can free the memory at shutdown.
    static QList<DcmtDownloadViewController*> sInstances;
};

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif
