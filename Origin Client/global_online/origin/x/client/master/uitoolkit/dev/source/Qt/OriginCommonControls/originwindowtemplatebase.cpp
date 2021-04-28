#include "originwindowtemplatebase.h"
namespace Origin {
namespace UIToolkit {

QString OriginWindowTemplateBase::mPrivateStyleSheet = QString("");

OriginWindowTemplateBase::OriginWindowTemplateBase(QWidget* parent)
: QWidget(parent)
{
    setOriginElementName("OriginWindowTemplate");
    m_children.append(this);
}


OriginWindowTemplateBase::~OriginWindowTemplateBase()
{
    removeElementFromChildList(this);
}


QString& OriginWindowTemplateBase::getElementStyleSheet()
{
    // While isn't this being called?
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginWindowTemplateBase::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


void OriginWindowTemplateBase::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
}


bool OriginWindowTemplateBase::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}
}
}