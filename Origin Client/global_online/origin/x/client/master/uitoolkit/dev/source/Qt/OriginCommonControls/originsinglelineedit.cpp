#include <QtWidgets>
#include "originsinglelineedit.h"

namespace Origin {
    namespace UIToolkit {
QString OriginSingleLineEdit::mPrivateStyleSheet = QString("");

#define SEARCH_PADDING 6
#define SEARCH_MARGIN 13

OriginSingleLineEdit::OriginSingleLineEdit(QWidget *parent/*= 0*/) :
    QLineEdit(parent)
   ,mStatus(Normal)
   ,mAccessory(None)
   ,mMouseDown(false)
   ,mOrigCursor(cursor())
{
    setOriginElementName("OriginSingleLineEdit");
    m_children.append(this);

    // Add our dynamic properties
    this->setProperty("password", false);
#if defined(Q_OS_MAC)
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
}


QString & OriginSingleLineEdit::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
		mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginSingleLineEdit::event(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::Move:
#if defined(Q_OS_MAC)
            // For some reason, this gets unset after the object has moved.
            setAttribute(Qt::WA_MacShowFocusRect, false);
#endif            
            break;
            
        default:
            break;
    }
    OriginUIBase::event(event);
    return QLineEdit::event(event);
}

void OriginSingleLineEdit::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QLineEdit::paintEvent(event);

    // if there is an accessory, do this:
    switch (mAccessory)
    {
    case Search:
        {
            QPainter painter;
            painter.begin(this);
            // the first time we come here, we need to load it
            if (mSearchPixmap.isNull())
            {
                mSearchPixmap = QPixmap(":/OriginCommonControls/OriginSingleLineEdit/search.png");
            }
            if (mSearchRect.isNull()) 
            {
                int searchY = contentsRect().y() + (contentsRect().height()/2 - mSearchPixmap.height()/2);
                mSearchRect = QRect(contentsRect().right() - mSearchPixmap.width() - SEARCH_PADDING, searchY, mSearchPixmap.width(), mSearchPixmap.height());
            }
            painter.drawPixmap(mSearchRect,mSearchPixmap);
            painter.end();
        }
        break;
    case None:
    default:
        break;
    };
}

void OriginSingleLineEdit::mousePressEvent(QMouseEvent* e)
{
    switch (mAccessory)
    {
        case Search:
            {
                // check if they moused down over the icon
                if (mSearchRect.contains(e->pos()))
                    mMouseDown = true;                    
            }
            break;
        case None:
        default:
            break;
    };
    QLineEdit::mousePressEvent(e);
} 

void OriginSingleLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    // are we in mMouseDown and over the accessory still?
    if (mMouseDown) 
    {
        switch (mAccessory)
        {
        case Search:
            {
                if (mSearchRect.contains(e->pos()))
                    emit accessoryClicked();                 
            }
            break;
        case None:
        default:
            break;
        };
    }
    mMouseDown = false;
    QLineEdit::mouseReleaseEvent(e);
} 

void OriginSingleLineEdit::mouseMoveEvent(QMouseEvent *e)
{    
    switch (mAccessory)
    {
    case Search:
        {
            // set the correct cursor
            if (mSearchRect.contains(e->pos()))
            {
                setCursor(QCursor(Qt::ArrowCursor));
            }
            else 
                setCursor(mOrigCursor);           
        };
        break;
    case None:
    default:
        break;
    }
    
    QLineEdit::mouseMoveEvent(e);
}

void OriginSingleLineEdit::focusOutEvent(QFocusEvent *e)
{
    mMouseDown = false;
    QLineEdit::focusOutEvent(e);
}

void OriginSingleLineEdit::resizeEvent(QResizeEvent* e)
{
    QLineEdit::resizeEvent(e);

    if (mAccessory != None) 
    {
        // redo the location
        int searchY = contentsRect().y() + (contentsRect().height()/2 - mSearchPixmap.height()/2);
        mSearchRect = QRect(contentsRect().right() - mSearchPixmap.width() - SEARCH_PADDING, searchY, mSearchPixmap.width(), mSearchPixmap.height());
    }
}

bool OriginSingleLineEdit::passwordView() const
{
    return m_passwordView;
}
void OriginSingleLineEdit::setPasswordView(bool enabled)
{
    m_passwordView = enabled;
    if (m_passwordView)
    {
        setEchoMode(QLineEdit::Password);
    }
    else
    {
        setEchoMode(QLineEdit::Normal);
    }
}

void OriginSingleLineEdit::setValidationStatus(const OriginValidationStatus& status)
{
    mStatus = status;
    this->setStyleSheet(getElementStyleSheet());
}

OriginSingleLineEdit::OriginValidationStatus OriginSingleLineEdit::getValidationStatus() const
{
    return mStatus;
}

void OriginSingleLineEdit::setInputAccessory(const OriginInputAccessory& accessory)
{
    mAccessory = accessory;
    switch (mAccessory)
    {
        case Search:
            // this is on top of the padding so that the editing stops short of the accessory
            setTextMargins(0, 0, SEARCH_MARGIN, 0);
            break;
        case None:
        default:
            setTextMargins(0, 0, 0, 0);
            break;
    };

    this->setStyleSheet(getElementStyleSheet());
}

OriginSingleLineEdit::OriginInputAccessory OriginSingleLineEdit::getInputAccessory() const
{
    return mAccessory;
}

void OriginSingleLineEdit::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginSingleLineEdit::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginSingleLineEdit::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

}
}