/////////////////////////////////////////////////////////////////////////////
// TelemetryDebugView.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef TELEMETRYDEBUGVIEW_H
#define TELEMETRYDEBUGVIEW_H

#include "services/plugin/PluginAPI.h"
#include "TelemetryAPIDLL.h"

#include <QWidget>

namespace Ui
{
    class TelemetryDebugView;
}

namespace Origin 
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Plugins
{
namespace DeveloperTool
{

class TelemetryDebugView : public QObject, public TelemetryEventListener
{
    Q_OBJECT

public:
    /// \brief Constructor
    TelemetryDebugView(QObject* parent = NULL);

    /// \brief Destructor
    ~TelemetryDebugView();
    
    /// \brief Adds the new telemetry event to table
    /// \param timestamp The timestamp of the event.
    /// \param module The module of the event, i.e. BOOT.
    /// \param group The group of the event, i.e. SESS.
    /// \param string The string of the event, i.e. ENDS.
    /// \param data Map of attributes to values that makes up the rest of the telemetry event.
    virtual void processEvent(const TYPE_U64 timestamp, const TYPE_S8 module, const TYPE_S8 group, const TYPE_S8 string, const eastl::map<eastl::string8, TelemetryFileAttributItem*>& data);

    /// \brief Shows the window, or raises and activates it if it already exists.
    void showWindow();

private slots:
    /// \brief Clears the table and event breakdown area
    void clear();

    /// \brief Called when the user selects a new row.
    void onSelectionChanged();

    /// \brief Called when user clicks the Apply Filters button.
    void onApplyFilters();

    /// \brief Called when user clicks the Clear Filters button.
    void onClearFilters();

    /// \brief Called when user clicks the View XML button.
    void onViewXML();
    
    /// \brief Called when user closes the dialog.
    void onClose();

private:
    enum ColumnHeaderValue
    {
        kColumnHeaderTimeReceived = 0,  ///< Event timestamp
        kColumnHeaderModule,            ///< Telemetry module, i.e. BOOT
        kColumnHeaderGroup,             ///< Telemetry group, i.e. SESS
        kColumnHeaderString,            ///< Telemetry string, i.e. ENDS
        kNumColumnHeaders               ///< Number of column headers
    };

    enum FilterBehavior
    {
        kFilterBehaviorMatchAll = 0,    ///< Match all of the current filters
        kFilterBehaviorMatchAny         ///< Match any of the current filters
    };

private:
    /// \brief Updates the given row's visibility based on current filters.
    /// \param rowNumber The number of the row to update.
    Q_INVOKABLE void updateRowVisibility(int rowNumber);

    UIToolkit::OriginWindow* mWindow;   ///< Containing window
    Ui::TelemetryDebugView* ui;         ///< Qt UI object

    QString mCurrentModuleFilter;           ///< Current Module filter
    QString mCurrentGroupFilter;            ///< Current Group filter
    QString mCurrentStringFilter;           ///< Current String filter
    FilterBehavior mCurrentFilterBehavior;  ///< Current filter behavior
};
}
}
}
#endif
