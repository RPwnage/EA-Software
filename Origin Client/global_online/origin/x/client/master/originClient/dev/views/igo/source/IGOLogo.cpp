///////////////////////////////////////////////////////////////////////////////
// IGOLogo.cpp
// 
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#include "IGOLogo.h"
#include <QPainter>
#include <qstyleoption.h>

namespace Origin
{
namespace Client
{

IGOLogo::IGOLogo(QWidget* parent)
: Origin::Engine::IGOQWidget(parent)
{
    setObjectName("oigLogo");
}


IGOLogo::~IGOLogo()
{
}


void IGOLogo::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

} // Client
} // Origin