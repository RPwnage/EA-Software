/////////////////////////////////////////////////////////////////////////////
// OriginCalloutIconTemplate.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ORIGINCALLOUTICONTEMPLATE_H
#define ORIGINCALLOUTICONTEMPLATE_H

#include <QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Ui
{
    class OriginCalloutIconTemplate;
    class OriginCalloutIconTemplateContentLine;
}

namespace Origin
{
namespace UIToolkit
{

class UITOOLKIT_PLUGIN_API OriginCalloutIconTemplateContentLine : public QWidget
{
public:
    OriginCalloutIconTemplateContentLine(const QString& iconSrc = "", const QString& text = "");
    ~OriginCalloutIconTemplateContentLine();

private:
    Ui::OriginCalloutIconTemplateContentLine* ui;
};

class UITOOLKIT_PLUGIN_API OriginCalloutIconTemplate : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
    OriginCalloutIconTemplate();
    ~OriginCalloutIconTemplate();
	
    void setContent(QWidget* content);
    void setContent(QList<OriginCalloutIconTemplateContentLine*> contentLines);
    void setTitle(const QString& title);
    void removeContent();

signals:
    void closeClicked();


protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    QWidget* getSelf() {return this;}
    QString&  getElementStyleSheet();
    void prepForStyleReload();

private:
    static QString mPrivateStyleSheet;
    Ui::OriginCalloutIconTemplate* ui;
    QWidget* mContent;
};

}
}
#endif
