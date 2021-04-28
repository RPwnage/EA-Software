#include <QtWidgets>
#include "originradiobutton.h"

namespace Origin {
    namespace UIToolkit {
QString OriginRadioButton::mPrivateStyleSheet = QString("");


OriginRadioButton::OriginRadioButton(QWidget *parent/*=0*/) :
    QRadioButton(parent)
   ,mStatus(Normal)
{
    setOriginElementName("OriginRadioButton");
    m_children.append(this);
}

void OriginRadioButton::setValidationStatus(const OriginValidationStatus& status)
{
     mStatus = status;
     this->setStyleSheet(getElementStyleSheet());
}

OriginRadioButton::OriginValidationStatus OriginRadioButton::getValidationStatus() const
{
     return mStatus;
}

QString & OriginRadioButton::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginRadioButton::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Wheel:
        event->ignore();
        break;
    default:
        break;
    }
    OriginUIBase::event(event);
    return QRadioButton::event(event);
}

void OriginRadioButton::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QRadioButton::paintEvent(event);
}

void OriginRadioButton::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginRadioButton::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginRadioButton::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
    }
}