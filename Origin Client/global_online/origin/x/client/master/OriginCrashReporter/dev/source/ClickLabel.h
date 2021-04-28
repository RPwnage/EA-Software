// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#ifndef CLICKLABEL_H
#define CLICKLABEL_H

#include <QWidget>
#include <QLabel>

class ClickLabel :
	public QLabel
{
	Q_OBJECT
public:
	ClickLabel(QWidget *parent =0);

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent *);
};

#endif // CLICKLABEL_H