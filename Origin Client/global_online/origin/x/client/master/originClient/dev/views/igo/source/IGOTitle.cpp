///////////////////////////////////////////////////////////////////////////////
// IGOTitle.cpp
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOTitle.h"
#include <QVBoxLayout>
#include <QPainter>
#include <qstyleoption.h>
#include "originlabel.h"

namespace Origin
{
namespace Client
{
const QString IGOTitleURL("http://widgets.dm.origin.com/linkroles/gametitleview");

IGOTitle::IGOTitle(QWidget* parent)
: Origin::Engine::IGOQWidget(parent)
, mLabel(NULL)
{
    setObjectName("IGOTitle");
    setLayout(new QVBoxLayout());
    QWidget::layout()->setMargin(0);
    QWidget::layout()->setSpacing(0);
    mLabel = new UIToolkit::OriginLabel();
    mLabel->setWordWrap(true);
    mLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    mLabel->setLabelType(Origin::UIToolkit::OriginLabel::DialogTitle);
    mLabel->setSpecialColorStyle(Origin::UIToolkit::OriginLabel::Color_White);
    mLabel->setElideMode(Qt::ElideRight);
    mLabel->setMaxNumLines(2);
    QWidget::layout()->addWidget(mLabel);
    ORIGIN_VERIFY_CONNECT_EX(
        Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameFocused(uint32_t)),
        this, SLOT(onGameFocused(const uint32_t&)), Qt::QueuedConnection);
}


IGOTitle::~IGOTitle()
{
    if (mLabel)
    {
        mLabel->deleteLater();
        mLabel = NULL;
    }
}


void IGOTitle::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void IGOTitle::onGameFocused(const uint32_t& gameId)
{
    const Engine::IGOGameTracker::GameInfo info = Engine::IGOController::instance()->gameTracker()->gameInfo(gameId);
    if (info.isValid())
    {
        mLabel->setText(info.mBase.mTitle);
        const QRect desktopWorkArea = Engine::IGOController::instance()->desktopWorkArea();
        if (desktopWorkArea.width() != 0)
        {
            onSizeChanged(desktopWorkArea.width(), desktopWorkArea.height());
        }
    }
}


void IGOTitle::onSizeChanged(uint32_t width, uint32_t height)
{
    const int marginOffset = 620;
    setMaximumWidth(width - marginOffset);
    adjustSize();

    Engine::IIGOWindowManager::WindowProperties settings;
    settings.setPosition(QPoint(0, 80), Engine::IIGOWindowManager::WindowDockingOrigin_X_CENTER, true);

    Engine::IGOController::instance()->igowm()->setWindowProperties(ivcView(), settings);

    emit igoResize(size());

}

} // Client
} // Origin