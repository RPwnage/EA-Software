/////////////////////////////////////////////////////////////////////////////
// UpdateDebugView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef UPDATEDEBUGVIEW_H
#define UPDATEDEBUGVIEW_H

#include "LocalizedContentString.h"

#include <QWidget>
#include <QTableWidgetItem>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class UpdateDebugView;
}

namespace Origin 
{
namespace Client
{
/// \brief Helper class to correctly sort our file size column.
class ORIGIN_PLUGIN_API UpdateDebugViewItem : public QTableWidgetItem
{
public:
    /// \brief Constructor
    /// \param text Text to show for this item.
    UpdateDebugViewItem(const QString& text);

    /// \brief Overridden less-than operator
    /// \param other The object to compare to.
    /// \return True if this object is less than the passed object.
    bool operator < (const QTableWidgetItem& other) const;
};

class ORIGIN_PLUGIN_API UpdateDebugView : public QWidget
{
    Q_OBJECT

public:
    /// \brief Constructor
    UpdateDebugView(QWidget* parent = NULL);

    /// \brief Destructor
    ~UpdateDebugView();
    
    /// \brief Adds file information to the progress table for the given file.
    /// \param index The row in which to place the file.
    /// \param fileName The name of the file.
    /// \param size The total size of the file.
    /// \param installLocation The directory this file is being installed to.
    /// \param packageFileCrc The CRC of the file in the package
    /// \param diskFileCrc The CRC of the file on disk, or 0 if no file exists.
    void addFile(qint32 index, const QString& fileName, quint64 size, const QString& installLocation, quint32 packageFileCrc, quint32 diskFileCrc);

    /// \brief Sets the URL of this download.
    /// \param source The URL of this download.
    void setSource(const QString& source);
    
    /// \brief Sets the size of this download.
    /// \param size The size of this download.
    void setSize(quint64 size);

    /// \brief Sets the number of files to display.
    /// \param count The number of files to display.
    void setFileCount(quint32 count);

    /// \brief Clears all progress in the progress table.
    void clear();
    
    /// \brief Enables or disables sorting.
    /// \param enable True if sorting should be enabled.
    void setSortingEnabled(bool enable);

    /// \brief Lets the view know to display update data
    void showUpdateFiles(bool show);

private:
    enum ColumnHeaderValue
    {
        kColumnHeaderFileName = 0,       ///< File name
        kColumnHeaderFileSize,           ///< File size
        kColumnHeaderInstallLocation,    ///< Install location
        kColumnHeaderPackageCrcLocation, ///< Package CRC location
        kColumnHeaderFileCrcLocation,    ///< Disk CRC location
        kNumColumnHeaders                ///< Number of column headers
    };

private:
    Ui::UpdateDebugView* ui; ///< Qt UI object
    LocalizedContentString mLocalizer; ///< Locale object hardcoded to en_US for translation.  ODT is intentionally not localized.

};
}
}
#endif