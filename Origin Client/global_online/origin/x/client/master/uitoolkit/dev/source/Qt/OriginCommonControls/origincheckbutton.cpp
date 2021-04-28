#include <QtWidgets>
#include <QMessageBox>
#include "origincheckbutton.h"

namespace Origin {
    namespace UIToolkit {
QString OriginCheckButton::mPrivateStyleSheet = QString("");

OriginCheckButton::OriginCheckButton(QWidget *parent/*= 0*/) :
    QCheckBox(parent)
   ,mStatus(Normal)
{
    setOriginElementName("OriginCheckButton");
    m_children.append(this);
}

void OriginCheckButton::setValidationStatus(const OriginValidationStatus& status)
{
    mStatus = status;
    this->setStyleSheet(getElementStyleSheet());
}

OriginCheckButton::OriginValidationStatus OriginCheckButton::getValidationStatus() const
{
    return mStatus;
}

bool OriginCheckButton::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QCheckBox::event(event);
}

void OriginCheckButton::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QCheckBox::paintEvent(event);
}

//Must be done to get a static stylesheet string per sub class
QString & OriginCheckButton::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginCheckButton::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginCheckButton::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginCheckButton::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

    }
}