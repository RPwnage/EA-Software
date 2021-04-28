/////////////////////////////////////////////////////////////////////////////
// DownloadDebugView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef DOWNLOADDEBUGVIEW_H
#define DOWNLOADDEBUGVIEW_H

#include "engine/debug/DownloadFileMetadata.h"
#include "services/plugin/PluginAPI.h"
#include "LocalizedContentString.h"

#include <QWidget>
#include <QTableWidgetItem>

namespace Ui
{
    class DownloadDebugView;
}

namespace Origin 
{
namespace UIToolkit
{
    class OriginPushButton;
}

namespace Client
{
/// \brief Helper class to correctly sort our file size column.
class ORIGIN_PLUGIN_API DownloadDebugViewItem : public QTableWidgetItem
{
public:
    /// \brief Constructor
    /// \param text Text to show for this item.
    DownloadDebugViewItem(const QString& text);

    /// \brief Overridden less-than operator
    /// \param other The object to compare to.
    /// \return True if this object is less than the passed object.
    bool operator < (const QTableWidgetItem& other) const;
};

class ORIGIN_PLUGIN_API DownloadDebugView : public QWidget
{
    Q_OBJECT

public:
    /// \brief Constructor
    DownloadDebugView(QWidget* parent = NULL);

    /// \brief Destructor
    ~DownloadDebugView();
    
    /// \brief Adds file information to the progress table for the given file.
    /// \param index The row in which to place the file.
    /// \param fileName The name of the file.
    /// \param size The total size of the file.
    /// \param installLocation The directory this file is being installed to.
    /// \param status The status of the file.
    void addFile(qint32 index, const QString& fileName, qint64 size, const QString& installLocation, Origin::Engine::Debug::FileStatus status);

    /// \brief Updates the "Status" cell for the given file.
    /// \param fileName TBD.
    /// \param status The status of the file.
    void updateStatus(const QString& fileName, Origin::Engine::Debug::FileStatus status);

    /// \brief Sets the URL of this download.
    /// \param source The URL of this download.
    void setSource(const QString& source);
    
    /// \brief Sets the destination of this download.
    /// \param destination The destination of this download.
    void setDestination(const QString& destination);

    /// \brief Sets the number of files to display.
    /// \param count The number of files to display.
    void setFileCount(qint32 count);
    
    /// \brief Adds file information to the file info table for the given file.
    ///        If file data already exists, this will replace it.
    /// \param fileName The name of the file
    /// \param errorCode The numerical error code.
    /// \param errorDescription The detailed description of the error.
    void setFileInfo(const QString& fileName, quint64 errorCode, const QString& errorDescription);

    /// \brief Clears all progress in the progress table.
    void clear();
    
    /// \brief Enables or disables sorting.
    /// \param enable True if sorting should be enabled.
    void setSortingEnabled(bool enable);

signals:
    /// \brief Emitted when the user clicks a row in the progress table.
    /// \param installLocation Value from the installLocation column.  Used as a unique identifier.
    void selectionChanged(const QString& installLocation);

private slots:
    /// \brief Called when the user selects a new row.
    void onSelectionChanged();

private:
    enum ColumnHeaderValue
    {
        kColumnHeaderFileName = 0,      ///< File name
        kColumnHeaderFileSize,          ///< File size
        kColumnHeaderInstallLocation,   ///< Install location
        kColumnHeaderStatus,            ///< Download status
        kNumColumnHeaders               ///< Number of column headers
    };

    enum RowHeaderValue
    {
        kRowHeaderFile = 0, ///< File
        kRowHeaderError,    ///< Error code
        kRowHeaderInfo,     ///< Detailed error description
        kNumRowHeaders      ///< Number of row headers
    };

private:
    Ui::DownloadDebugView* ui; ///< Qt UI object
    LocalizedContentString mLocalizer; ///< Locale object hardcoded to en_US for translation.  ODT is intentionally not localized.

};
}
}
#endif