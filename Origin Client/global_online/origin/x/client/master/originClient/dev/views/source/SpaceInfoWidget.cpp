/////////////////////////////////////////////////////////////////////////////
// SpaceInfoWidget.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SpaceInfoWidget.h"
#include "ui_SpaceInfoWidget.h"
#include "utilities/LocalizedContentString.h"
namespace Origin
{
namespace Client
{

SpaceInfoWidget::SpaceInfoWidget(QWidget* parent)
: QWidget(parent)
, mUI(new Ui::SpaceInfoWidget())
{
    mUI->setupUi(this);
    mUI->lblTooltip->setVisible(false);
    setAvailableSpace(0);
    setInstallSize(0);
    setDownloadSize(0);
}


SpaceInfoWidget::~SpaceInfoWidget()
{
    if(mUI)
    {
        delete mUI;
        mUI = NULL;
    }
}


void SpaceInfoWidget::setRequiredLabel(const QString& label, const QString& tooltip)
{
    mUI->lblInstall->setText(label);

    mUI->lblTooltip->setToolTip(tooltip);
    mUI->lblTooltip->setVisible(tooltip != "");
}


void SpaceInfoWidget::setDownloadSize(const qulonglong& bytes)
{
    mUI->lblDownload->setVisible(bytes != 0);
    mUI->lblDownloadSize->setVisible(bytes != 0);
    mUI->originDivider1->setVisible(bytes != 0);
    mDownloadBytes = bytes;
    mUI->lblDownloadSize->setText(LocalizedContentString().formatBytes(bytes));
}


void SpaceInfoWidget::setInstallSize(const qulonglong& bytes)
{
    mInstallBytes = bytes;
    mUI->lblInstallSize->setText(LocalizedContentString().formatBytes(bytes));
}


void SpaceInfoWidget::setAvailableSpace(const qulonglong& bytes)
{
    mUI->lblAvailable->setVisible(bytes != 0);
    mUI->lblAvailableSpace->setVisible(bytes != 0);
    mUI->originDivider->setVisible(bytes != 0);
    mAvailableBytes = bytes;
    mUI->lblAvailableSpace->setText(LocalizedContentString().formatBytes(bytes));
}

}
}