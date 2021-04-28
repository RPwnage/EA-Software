#include "originclosebutton.h"
#include <QApplication>

namespace Origin
{
namespace UIToolkit
{

QString OriginCloseButton::mPrivateStyleSheet = QString("");

OriginCloseButton::OriginCloseButton(QWidget* parent)
: QPushButton(parent)
, mButtonSize(ButtonInvalid)
{
    setButtonSize(ButtonLarge);
    setOriginElementName("OriginCloseButton");
    m_children.append(this);
}


OriginCloseButton::~OriginCloseButton()
{
    removeElementFromChildList(this);
}


void OriginCloseButton::setButtonSize(const ButtonSize& buttonSize)
{
    mButtonSize = buttonSize;
    QVariant variant = QVariant::Invalid;
    switch(buttonSize)
    {
    case ButtonLarge:
        variant = "large";
        break;

    case ButtonSmall:
        variant = "small";
        break;

    case ButtonInvalid:
    default:
        break;
    }
    setProperty("style", variant);
    setStyle(QApplication::style());
}


QString& OriginCloseButton::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


bool OriginCloseButton::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QPushButton::event(event);
}


void OriginCloseButton::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QPushButton::paintEvent(event);
}


void OriginCloseButton::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
    
}
}