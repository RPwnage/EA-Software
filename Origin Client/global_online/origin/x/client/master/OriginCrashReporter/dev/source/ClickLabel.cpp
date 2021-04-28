// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "ClickLabel.h"


ClickLabel::ClickLabel(QWidget * parent):QLabel(parent)
{
}

void ClickLabel::mousePressEvent(QMouseEvent *)
{
	emit clicked();
}