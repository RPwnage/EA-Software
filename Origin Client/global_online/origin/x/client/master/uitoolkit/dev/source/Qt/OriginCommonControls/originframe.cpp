#include <QtWidgets>
#include "originframe.h"
#include <QMessageBox>

namespace Origin {
    namespace UIToolkit {

QString OriginFrame::mPrivateStyleSheet = QString("");


OriginFrame::OriginFrame(QWidget *parent/*= 0*/) :
    QFrame(parent)
{
    setProperty("background", true);
    QVBoxLayout * layout = new QVBoxLayout(this);
    m_text = new OriginMultiLineEdit();
    m_text->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(m_text);
    //set no margins around item
    setProperty("childFocus", true);
    layout->setContentsMargins(0,0,0,0);
    connect(m_text, SIGNAL(objectFocusChanged(bool)), this, SLOT(onChildFocusChanged(bool)));


}

QString& OriginFrame::getPrivateStyleSheet()
{
    if (mPrivateStyleSheet.length() == 0)
    {
        QFile inputFile(":/OriginCommonControls/OriginFrame/style.qss");
        inputFile.open(QIODevice::ReadOnly);
        QTextStream in(&inputFile);
        mPrivateStyleSheet = in.readAll();
        inputFile.close();
    }
    return mPrivateStyleSheet;
}

bool OriginFrame::event(QEvent* event)
{
    if (event->type() == QEvent::Polish)
    {
        this->setStyleSheet(OriginFrame::getPrivateStyleSheet());
    }
    return QFrame::event(event);
}

void OriginFrame::paintEvent(QPaintEvent *event)
{
    QStyleOption styleOption;
    styleOption.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);

    QFrame::paintEvent(event);
}


bool OriginFrame::isBackgroundEnabled() const
{
    return m_backgroundEnabled;
}

void OriginFrame::setBackgroundEnabled(bool enabled)
{
    m_backgroundEnabled = enabled;
}

QString OriginFrame::getHTML() const
{
   return m_text->getHtml();
}
void OriginFrame::setHTML(QString htmlText)
{
    m_text->setHtml(htmlText);
}


void OriginFrame::onChildFocusChanged(bool focusIn)
{
    this->setProperty("childFocus", focusIn);
    event(new QEvent(QEvent::Polish));
}

    }
}
