#include <QtWidgets>
#include "origindropdown.h"

namespace Origin {
namespace UIToolkit {
QString OriginDropdown::mPrivateStyleSheet = QString("");


OriginDropdown::OriginDropdown(QWidget *parent/*= 0*/)
: QComboBox(parent)
, mWheel(false)
{
    setOriginElementName("OriginDropdown");
    m_children.append(this);
    setFocusPolicy(Qt::StrongFocus);
}

OriginDropdown::~OriginDropdown()
{
    this->removeElementFromChildList(this);
}

QString & OriginDropdown::getElementStyleSheet()
{
    if (mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginDropdown::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Wheel:
        // Don't scroll through the options unless the dropdown view is visible.
        if(view()->isVisible() == false)
            return false;
        // Moved from ebisudropdown to make settingsdialog work: TODO: Do something better than this.
        mWheel = true;
        break;
    default:
        mWheel = false;
        break;
    }
    OriginUIBase::event(event);
    return QComboBox::event(event);
}

void OriginDropdown::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QComboBox::paintEvent(event);
}

void OriginDropdown::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
    
void OriginDropdown::showPopup()
{
    view()->setFixedWidth(this->width());
    QComboBox::showPopup();
}
}
}