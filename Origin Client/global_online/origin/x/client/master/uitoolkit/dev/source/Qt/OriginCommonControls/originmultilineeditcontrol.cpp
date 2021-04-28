#include "originMultilineeditcontrol.h"
#include <QtWidgets>

namespace Origin {
    namespace UIToolkit {
QString OriginMultiLineEditControl::mPrivateStyleSheet = QString("");

OriginMultiLineEditControl::OriginMultiLineEditControl(QWidget *parent/*= 0*/) :
    QTextEdit(parent)
{
    setOriginElementName("OriginMultiLineEditControl");
    m_children.append(this);
}


QString & OriginMultiLineEditControl::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginMultiLineEditControl::event(QEvent* event)
{
    OriginUIBase::event(event);
    if ( event->type() == QEvent::FocusIn )
    {
        emit objectFocusChanged(true);
    }
    else if ( event->type() == QEvent::FocusOut)
    {
        emit objectFocusChanged(false);
    }
    return QTextEdit::event(event);
}

void OriginMultiLineEditControl::paintEvent(QPaintEvent *event)
{
    QTextEdit::paintEvent(event);
}


void OriginMultiLineEditControl::keyPressEvent(QKeyEvent* event)
{
    QTextEdit::keyPressEvent(event);
    if (parentWidget())
    {
       parentWidget()->update();
    }
}



void OriginMultiLineEditControl::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginMultiLineEditControl::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginMultiLineEditControl::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
    }
}