#include "originslider.h"
#include <QMouseEvent>

namespace Origin {
namespace UIToolkit {
QString OriginSlider::mPrivateStyleSheet = QString("");

OriginSlider::OriginSlider(QWidget* parent)
: QSlider(parent)
{
    setOriginElementName("OriginSlider");
    m_children.append(this);
}


OriginSlider::~OriginSlider()
{ 
    this->removeElementFromChildList(this);
}


QString& OriginSlider::getElementStyleSheet()
{
    if(mPrivateStyleSheet.isEmpty() && !m_styleSheetPath.isEmpty())
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


bool OriginSlider::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QSlider::event(event);
}


void OriginSlider::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QSlider::paintEvent(event);
}


void OriginSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum()-minimum()) * (height()-event->y())) / height());
        else
            setValue(minimum() + ((maximum()-minimum()) * event->x()) / width());

        event->accept();
    }
    QSlider::mousePressEvent(event);
}


void OriginSlider::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

}
}
