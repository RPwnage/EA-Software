/////////////////////////////////////////////////////////////////////////////
// InstallSpaceInfo.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "InstallSpaceInfo.h"
#include "ui_InstallSpaceInfo.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originmsgbox.h"
#include "origintooltip.h"

namespace Origin
{
namespace Client
{

InstallSpaceInfo::InstallSpaceInfo(QWidget* parent)
: QWidget(parent)
, mUI(new Ui::InstallSpaceInfo)
{
	mUI->setupUi(this);
    mUI->desktopShortcut->setChecked(Services::readSetting(Services::SETTING_CREATEDESKTOPSHORTCUT, Services::Session::SessionService::currentSession()));
    mUI->startmenuShortcut->setChecked(Services::readSetting(Services::SETTING_CREATESTARTMENUSHORTCUT, Services::Session::SessionService::currentSession()));
}


InstallSpaceInfo::~InstallSpaceInfo()
{
	if(mUI)
	{
		delete mUI;
		mUI = NULL;
	}
}


void InstallSpaceInfo::setShortcutsVisble(const bool& visible)
{
	mUI->desktopShortcut->setVisible(visible);
	mUI->startmenuShortcut->setVisible(visible);
}


void InstallSpaceInfo::setInstallSize(const qulonglong& bytes)
{
    mUI->spaceInfo->setInstallSize(bytes);
}


void InstallSpaceInfo::setDownloadSize(const qulonglong& bytes)
{
    mUI->spaceInfo->setDownloadSize(bytes);
}


void InstallSpaceInfo::setAvailableSpace(const qulonglong& bytes)
{
	mUI->spaceInfo->setAvailableSpace(bytes);
}


bool InstallSpaceInfo::sufficientFreeSpace() const
{
    return mUI->spaceInfo->sufficientFreeSpace();
}


void InstallSpaceInfo::addCheckButton(const QString& desc, const QString& tooltip, const QString& cancelWarning)
{
	QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(9);

	UIToolkit::OriginCheckButton* optionalEula = new UIToolkit::OriginCheckButton();
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(optionalEula->sizePolicy().hasHeightForWidth());
    optionalEula->setSizePolicy(sizePolicy);

    // OFM-9537: If they are Canadian, checkboxes should be unchecked
    if(Services::readSetting(Services::SETTING_GeoCountry).toString().compare("CA", Qt::CaseInsensitive) == 0)
    {
        optionalEula->setChecked(false);
    }
    else
    {
        optionalEula->setChecked(true);
    }
	optionalEula->setText(desc);
	mOptionalCheckButtons.append(optionalEula);
	horizontalLayout->addWidget(optionalEula);

	// Add a (?) resource next to any checkbutton that has a tooltip
	if(!tooltip.isEmpty())
	{
        UIToolkit::OriginTooltip* tooltipBtn = new UIToolkit::OriginTooltip();
        tooltipBtn->setToolTip(tooltip);
        horizontalLayout->addWidget(tooltipBtn);
	}
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
	mUI->checkboxLayout->addLayout(horizontalLayout);
}


UIToolkit::OriginCheckButton* InstallSpaceInfo::desktopShortcut()
{
	return mUI->desktopShortcut;
}


UIToolkit::OriginCheckButton* InstallSpaceInfo::startmenuShortcut()
{
	return mUI->startmenuShortcut;
}

}
}