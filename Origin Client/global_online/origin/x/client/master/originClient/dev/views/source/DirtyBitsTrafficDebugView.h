/////////////////////////////////////////////////////////////////////////////
// DirtyBitsTrafficDebugView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef DIRTYBITSTRAFFICDEBUGVIEW_H
#define DIRTYBITSTRAFFICDEBUGVIEW_H

#include "engine/dirtybits/DirtyBitsClient.h"
#include "services/plugin/PluginAPI.h"
#include <QWidget>

namespace Ui
{
    class DirtyBitsTrafficDebugView;
}

namespace Origin 
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Client
{

class ORIGIN_PLUGIN_API DirtyBitsTrafficDebugView : public QObject, Origin::Engine::DirtyBitsClientDebugListener
{
    Q_OBJECT

public:
    /// \brief Constructor
    DirtyBitsTrafficDebugView(QObject* parent = NULL);

    /// \brief Destructor
    ~DirtyBitsTrafficDebugView();
    
    /// \brief Adds the new dirty bit event to table
    /// \param context The context of the event
    /// \param timeStamp The timestamp of the event.
    /// \param payload If available, the JSON payload of the event.
    /// \param isError If the event is an error event.
    void onNewDirtyBitEvent(const QString& context, long long timeStamp, const QByteArray& payload, bool isError);

    void showWindow();

    UIToolkit::OriginWindow* window() {return mWindow;}

signals:
    void finished();


private slots:
    /// \brief Clears the table and notification breakdown area
    void clear();

    /// \brief Called when the user selects a new row.
    void onSelectionChanged();
    
    void onClose();

private:
    enum ColumnHeaderValue
    {
        kColumnHeaderEvent = 0,      ///< File name
        kColumnHeaderTimeReceived,   ///< File size
        kColumnHeaderStatus,         ///< Download status
        kNumColumnHeaders            ///< Number of column headers
    };

private:
    UIToolkit::OriginWindow* mWindow;
    Ui::DirtyBitsTrafficDebugView* ui; ///< Qt UI object

};
}
}
#endif
