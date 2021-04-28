#include <QtWidgets>
#include "originmultilineedit.h"
#include <QMessageBox>

const int MIN_HEIGTH_SIZE = 30;
const QString FONT_FAMILY = "Arial";

namespace Origin {
    namespace UIToolkit {

QString OriginMultiLineEdit::mPrivateStyleSheet = QString("");


OriginMultiLineEdit::OriginMultiLineEdit(QWidget *parent/*= 0*/) :
    QFrame(parent), prevLineCount(1), m_autoExpanding(false)
{
    setOriginElementName("OriginMultiLineEdit");
    m_children.append(this);
    setProperty("background", true);
    setProperty("autoExpand", false);
    setProperty("maxLines", 10);

    //add layout to frame
    QVBoxLayout * layout = new QVBoxLayout(this);
    m_text = new OriginMultiLineEditControl();
    m_text->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(m_text);

    //set no margins around item
    layout->setContentsMargins(0,0,0,0);

    m_text->setTabChangesFocus(true);
    connect(m_text, SIGNAL(objectFocusChanged(bool)), this, SLOT(onChildFocusChanged(bool)));
}

void OriginMultiLineEdit::keyPressEvent(QKeyEvent* event)
{
    this->getMultiLineWidget()->keyPressEvent(event);
    getMultiLineWidget()->update();
    if (m_autoExpanding)
    {
        adjustWidgetSize();
    }
}

QString & OriginMultiLineEdit::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginMultiLineEdit::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QFrame::event(event);
}

void OriginMultiLineEdit::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QFrame::paintEvent(event);
    if (m_autoExpanding)
    {
        adjustWidgetSize();
    }

}


bool OriginMultiLineEdit::isBackgroundEnabled() const
{
    return m_backgroundEnabled;
}

void OriginMultiLineEdit::setBackgroundEnabled(bool enabled)
{
    m_backgroundEnabled = enabled;
}

QString OriginMultiLineEdit::getHtml() const
{
   return m_text->toHtml();
}
void OriginMultiLineEdit::setHtml(QString htmlText)
{
    m_text->setHtml(htmlText);
}

QString OriginMultiLineEdit::getPlainText() const
{
    return m_text->toPlainText();
}


void OriginMultiLineEdit::onChildFocusChanged(bool focusIn)
{
    this->setProperty("childFocus", focusIn);
    QEvent * tempPolishEvent = new QEvent(QEvent::Polish);
    event(tempPolishEvent);
    delete tempPolishEvent;
}


OriginMultiLineEditControl * OriginMultiLineEdit::getMultiLineWidget()
{
    return m_text;
}



void OriginMultiLineEdit::adjustWidgetSize(bool resizeEvent)
{
    int lineCount = getTotalLineCount();
    if(prevLineCount == lineCount && !resizeEvent)
    {
        //no new line has been inserted
        return;
    }
    prevLineCount = lineCount;

    QMargins textMargins = this->contentsMargins();
    int vertMargins = textMargins.bottom() + textMargins.top();

    if(lineCount <= 1)
    {
        //set min size
        this->setFixedHeight(MIN_HEIGTH_SIZE);

    }else if(lineCount <= m_maxLines)
    {
        QSizeF docSize = this->getMultiLineWidget()->document()->documentLayout()->documentSize();
        this->setFixedHeight(qCeil(docSize.height()) + vertMargins);
    }else
    {

        //set height for MAX_LINES_SHOWN lines
        QTextLayout* textLayout = this->getMultiLineWidget()->document()->firstBlock().layout();
        int lineHeight = textLayout->lineAt(0).height();
        int maxHeight = lineHeight*m_maxLines + vertMargins;
        this->setFixedHeight(maxHeight);
        this->getMultiLineWidget()->ensureCursorVisible();
    }
}


int OriginMultiLineEdit::getTotalLineCount( )
{
    // NOTE: using a QTextCursor for the line count can potentially trigger a
    // crash in Qt 4.7.1 when using a Japanese input system (e.g. Hiragana).
    int lines = 0;
    int blockCount = getMultiLineWidget()->document()->blockCount();
    for(int x = 0; x < blockCount; ++x)
    {
            QTextLayout* layout = getMultiLineWidget()->document()->findBlockByNumber(x).layout();
            lines += layout->lineCount();
    }
    if (lines > 1)
    {
        //QMessageBox::information(NULL, "lines", QString("lines = %1").arg(lines), QMessageBox::Ok);
    }

    return lines;
}

void OriginMultiLineEdit::resizeEvent(QResizeEvent *e)
{

    if (m_autoExpanding)
    {
        adjustWidgetSize(true);
    }
    QFrame::resizeEvent(e);
}


bool OriginMultiLineEdit::isAutoExpanding() const
{
    return m_autoExpanding;
}

void OriginMultiLineEdit::setAutoExpanding(bool expanding)
{
    m_autoExpanding = expanding;
    if ( !m_autoExpanding)
    {
        this->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    else
    {
        adjustWidgetSize(true);
    }
}

int OriginMultiLineEdit::getMaxLines() const
{
    return m_maxLines;
}

void OriginMultiLineEdit::setMaxLines(int lines)
{
    if ( lines < 1)
    {
        lines =1;
    }
    m_maxLines = lines;
}

void OriginMultiLineEdit::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginMultiLineEdit::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginMultiLineEdit::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
}
}
