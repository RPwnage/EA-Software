/////////////////////////////////////////////////////////////////////////////
// OriginCalloutIconTemplate.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "origincallouticontemplate.h"
#include "ui_OriginCalloutIconTemplate.h"
#include "ui_OriginCalloutIconTemplateContentLine.h"
#include <QVBoxLayout>

namespace Origin
{
namespace UIToolkit
{

QString OriginCalloutIconTemplate::mPrivateStyleSheet = QString("");

OriginCalloutIconTemplateContentLine::OriginCalloutIconTemplateContentLine(const QString& iconSrc, const QString& text)
: QWidget(NULL)
, ui(new Ui::OriginCalloutIconTemplateContentLine())
{
    ui->setupUi(this);
    ui->linetext->setText(text);
    if(iconSrc.count())
    {
        ui->lineIcon->setPixmap(QPixmap(iconSrc));
    }
    else
    {
        ui->lineIcon->hide();
    }
}


OriginCalloutIconTemplateContentLine::~OriginCalloutIconTemplateContentLine()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


OriginCalloutIconTemplate::OriginCalloutIconTemplate()
: QWidget(NULL)
, ui(new Ui::OriginCalloutIconTemplate)
, mContent(NULL)
{
	ui->setupUi(this);
    connect(ui->btnClose, SIGNAL(clicked()), this, SIGNAL(closeClicked()));
    setOriginElementName("OriginCalloutIconTemplate");
    m_children.append(this);
}


OriginCalloutIconTemplate::~OriginCalloutIconTemplate()
{
    if(mContent)
    {
        mContent->deleteLater();
        mContent = NULL;
    }
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void OriginCalloutIconTemplate::setContent(QWidget* content)
{
    removeContent();
    if(content)
    {
        mContent = content;
        ui->divline->show();
        ui->contentLayout->setContentsMargins(10, 7, 9, 7);
		ui->contentLayout->addWidget(mContent);
        ui->verticalLayout->setStretch(2, 3);
        setProperty("hasContent", "true");
        setStyleSheet(this->styleSheet());
    }
    else
    {
        ui->divline->hide();
        ui->contentLayout->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout->setStretch(2, 0);
    }
    adjustSize();
}


void OriginCalloutIconTemplate::setContent(QList<OriginCalloutIconTemplateContentLine*> contentLines)
{
    QWidget* content = new QWidget();
    QVBoxLayout* verticalLayout = new QVBoxLayout(content);
    verticalLayout->setSpacing(10);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    foreach(OriginCalloutIconTemplateContentLine* line, contentLines)
    {
        verticalLayout->addWidget(line);
    }
    setContent(content);
}


void OriginCalloutIconTemplate::setTitle(const QString& title)
{
    if(ui && ui->lblTitle)
    {
        ui->lblTitle->setText(title);
    }
}


void OriginCalloutIconTemplate::removeContent()
{
    if(mContent)
    {
        ui->contentLayout->removeWidget(mContent);
        mContent->deleteLater();
        setProperty("hasContent", QVariant::Invalid);
        setStyleSheet(this->styleSheet());
    }
}

QString& OriginCalloutIconTemplate::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


bool OriginCalloutIconTemplate::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}


void OriginCalloutIconTemplate::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


void OriginCalloutIconTemplate::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

}
}
