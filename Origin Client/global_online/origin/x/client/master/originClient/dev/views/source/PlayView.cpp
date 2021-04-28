/////////////////////////////////////////////////////////////////////////////
// PlayView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PlayView.h"
#include "origincheckbutton.h"
#include "origintooltip.h"
#include "origintransferbar.h"
#include "services/qt/QtUtil.h"
#include <QLayout>

namespace Origin
{
namespace Client
{

QJsonObject PlayView::commandLinkObj(const QString& option, const QString& icon, const QString& title, const QString& text, const QString& bottomText)
{
    QJsonObject ret;
    ret["option"] = option;
    ret["icon"] = icon;
    ret["title"] = title;
    ret["text"] = text;
    ret["bottomtext"] = bottomText;
    return ret;
}


OIGSettingsView::OIGSettingsView(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(4);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    UIToolkit::OriginCheckButton* checkBox = new UIToolkit::OriginCheckButton(this);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(checkBox->sizePolicy().hasHeightForWidth());
    checkBox->setSizePolicy(sizePolicy);
    checkBox->setText(tr("ebisu_client_igo_mac_warning_checkbox").arg(tr("ebisu_client_igo_name")));
    checkBox->setChecked(true);
    horizontalLayout->addWidget(checkBox);
    UIToolkit::OriginTooltip* tooltipBtn = new UIToolkit::OriginTooltip(this);
    tooltipBtn->setToolTip(Services::FormatToolTip(tr("ebisu_client_igo_mac_warning_tooltip")));
    horizontalLayout->addWidget(tooltipBtn);
    QSpacerItem* horizontalSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);
    adjustSize();
}

bool OIGSettingsView::isChecked() const
{
    UIToolkit::OriginCheckButton* checkBox = findChild<UIToolkit::OriginCheckButton*>();
    return checkBox ? checkBox->isChecked() : false;
}

}
}