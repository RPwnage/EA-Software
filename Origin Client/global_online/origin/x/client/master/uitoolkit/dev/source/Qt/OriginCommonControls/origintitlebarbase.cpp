#include "origintitlebarbase.h"

#include <QEvent>
#include <QApplication>

namespace Origin {
namespace UIToolkit {

QString OriginTitlebarBase::mPrivateStyleSheet = QString("");

OriginTitlebarBase::OriginTitlebarBase(QWidget* parent, OriginWindow* partner)
: QWidget(parent)
, mTitle("")
, mWindowPartner(partner)
, mSystemMenuEnabled(true)
, mCustomContent(NULL)
{
    setOriginElementName("OriginTitlebarBase");
    m_children.append(this);
}


OriginTitlebarBase::~OriginTitlebarBase()
{
    removeElementFromChildList(this);
}


QString& OriginTitlebarBase::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    if(parentWidget())
        parentWidget()->setStyleSheet(mPrivateStyleSheet);
    return mPrivateStyleSheet;
}


void OriginTitlebarBase::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


void OriginTitlebarBase::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


bool OriginTitlebarBase::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}

}
}