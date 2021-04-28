#include "origintooltip.h"
#include <QHelpEvent>
#include <QApplication>

namespace Origin {
namespace UIToolkit {
QString OriginTooltip::mPrivateStyleSheet = QString("");

OriginTooltip::OriginTooltip(QWidget* parent)
: QWidget(parent)
{
    setOriginElementName("OriginTooltip");
    m_children.append(this);
}


OriginTooltip::~OriginTooltip()
{
    this->removeElementFromChildList(this);
}


bool OriginTooltip::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}


void OriginTooltip::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


void OriginTooltip::enterEvent(QEvent* event)
{
    QHelpEvent tooltipEvent(QHelpEvent(QEvent::ToolTip, pos(), mapToGlobal(QPoint(0,0))));
    QApplication::sendEvent(this, &tooltipEvent);
    QWidget::enterEvent(event);
}


QString& OriginTooltip::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

}
}