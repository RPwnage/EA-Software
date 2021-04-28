/////////////////////////////////////////////////////////////////////////////
// InstallSpaceInfo.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef INSTALLSPACEINFO_H
#define INSTALLSPACEINFO_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
	class InstallSpaceInfo;
}

namespace Origin
{
namespace UIToolkit
{
	class OriginCheckButton;
}
namespace Client
{
class ORIGIN_PLUGIN_API InstallSpaceInfo : public QWidget
{
    Q_OBJECT
public:
    InstallSpaceInfo(QWidget* parent = NULL);
    ~InstallSpaceInfo();
	
	void setShortcutsVisble(const bool& bytes);
    void setInstallSize(const qulonglong& bytes);
	void setDownloadSize(const qulonglong& bytes);
	void setAvailableSpace(const qulonglong& bytes);
    bool sufficientFreeSpace() const;
	void addCheckButton(const QString& desc, const QString& tooltip, const QString& cancelWarning);
	QList<UIToolkit::OriginCheckButton*> optionalCheckButtons() const {return mOptionalCheckButtons;}
	UIToolkit::OriginCheckButton* desktopShortcut();
    UIToolkit::OriginCheckButton* startmenuShortcut();


private:
	Ui::InstallSpaceInfo* mUI;
	QList<UIToolkit::OriginCheckButton*> mOptionalCheckButtons;
};
}
}
#endif