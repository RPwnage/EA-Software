#include "origindivider.h"
#include <QApplication>

namespace Origin
{
namespace UIToolkit
{

QString OriginDivider::mPrivateStyleSheet = QString("");

OriginDivider::OriginDivider(QWidget* parent)
: QWidget(parent)
, mLineStyle(Invalid)
, mLineOrientation(Horizontal)
{
    setLineOrientation(Horizontal);
    setLineStyle(LightGrey_White);
    setOriginElementName("OriginDivider");
    m_children.append(this);
}


OriginDivider::~OriginDivider()
{
    removeElementFromChildList(this);
}


void OriginDivider::setLineStyle(const LineStyle& lineStyle)
{
    mLineStyle = lineStyle;
    QVariant variant = QVariant::Invalid;
    switch(mLineStyle)
    {
    case DarkGrey_White:
        variant = "DarkGrey_White";
        break;

    case DarkGrey_LightGrey:
        variant = "DarkGrey_LightGrey";
        break;

    case LightGrey_White:
        variant = "LightGrey_White";
        break;

    case DarkGreen_LightGreen:
        variant = "DarkGreen_LightGreen";
        break;

    case Dotted_DarkGrey:
        variant = "Dotted_DarkGrey";
        break;

    case Dotted_LightGrey:
        variant = "Dotted_LightGrey";
        break;

    case Invalid:
    default:
        break;
    }
    setProperty("style", variant);
    setStyle(QApplication::style());
}


void OriginDivider::setLineOrientation(const LineOrientation& lineOrientation)
{
    mLineOrientation = lineOrientation;
    QVariant variant = QVariant::Invalid;
    switch(mLineOrientation)
    {
    case Horizontal:
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
        variant = "horizontal";
        break;

    case Vertical:
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        variant = "vertical";
        break;

    default:
        break;
    }
    setProperty("orientation", variant);
    setStyle(QApplication::style());
}


QString& OriginDivider::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


bool OriginDivider::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}


void OriginDivider::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


void OriginDivider::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
    
}
}