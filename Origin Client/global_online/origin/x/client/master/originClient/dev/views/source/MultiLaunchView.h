/////////////////////////////////////////////////////////////////////////////
// MultiLaunchView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef MULTILAUNCHVIEW_H
#define MULTILAUNCHVIEW_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Origin 
{
namespace Client
{
class ORIGIN_PLUGIN_API MultiLaunchView : public QWidget
{
    Q_OBJECT

public:
	MultiLaunchView(const QStringList& launchNameList, QWidget* parent = NULL);
    ~MultiLaunchView();

    /// \brief Returns the selected launch option.
    QString launchOptionSelected() {return mLaunchOptionSelected;}


protected slots:
    /// \brief Called when the launch option selected changes.
    void onRadioClicked();


private:
    QString mLaunchOptionSelected;
};
}
}
#endif