#include <QtWidgets>
#include "originpushbutton.h"


namespace Origin {
    namespace UIToolkit {
QString OriginPushButton::mPrivateStyleSheetPrimary = QString("");
QString OriginPushButton::mPrivateStyleSheetSecondary = QString("");
QString OriginPushButton::mPrivateStyleSheetTertiary = QString("");
QString OriginPushButton::mPrivateStyleSheetAccept = QString("");
QString OriginPushButton::mPrivateStyleSheetDecline = QString("");
QString OriginPushButton::mPrivateStyleSheetDark = QString("");


OriginPushButton::OriginPushButton(QWidget *parent/*= 0*/) :
    QPushButton(parent)
   ,mButtonType(ButtonTertiary)
   ,mFocusRect(false)
{
    setOriginElementName("OriginPushButton", "ButtonTertiary");
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_children.append(this);
}

QString & OriginPushButton::getElementStyleSheet()
{
    QString* s = &mPrivateStyleSheetTertiary;
    QString path;

    if (isDefault())
    {
        setOriginElementName("OriginPushButton", "ButtonPrimary");
        path = ":/OriginCommonControls/OriginPushButton/styleButtonPrimary.qss";
        s = &mPrivateStyleSheetPrimary;
    }
    else
    {
        switch (mButtonType)
        {
        case ButtonPrimary:
            setOriginElementName("OriginPushButton", "ButtonPrimary");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonPrimary.qss";
            s = &mPrivateStyleSheetPrimary;
            break;
        case ButtonSecondary:
            setOriginElementName("OriginPushButton", "ButtonSecondary");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonSecondary.qss";
            s = &mPrivateStyleSheetSecondary;
            break;
		case ButtonAccept:
			setOriginElementName("OriginPushButton", "ButtonAccept");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonAccept.qss";
            s = &mPrivateStyleSheetAccept;
			break;
		case ButtonDecline:
			setOriginElementName("OriginPushButton", "ButtonDecline");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonDecline.qss";
            s = &mPrivateStyleSheetDecline;
			break;
        case ButtonDark:
            setOriginElementName("OriginPushButton", "ButtonDark");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonDark.qss";
            s = &mPrivateStyleSheetDark;
            break;
        default:
            mButtonType = ButtonTertiary;
            // fall through
        case ButtonTertiary:
            setOriginElementName("OriginPushButton", "ButtonTertiary");
            path = ":/OriginCommonControls/OriginPushButton/styleButtonTertiary.qss";
            s = &mPrivateStyleSheetTertiary;
            break;
        }
    }

    if (s->isEmpty())
    {
        // Load the appropriate style sheet template based on the labelType
        QFile inputFile(path);
        inputFile.open(QIODevice::ReadOnly);
        QTextStream in(&inputFile);
        *s = (in.readAll());
        inputFile.close();
    }

    mCurrentStyleSheet = *s;

    // Replace the style sheet template macros per language
    loadLangIntoStyleSheet(mCurrentStyleSheet, m_OriginElementName, m_OriginSubElementName);

    return mCurrentStyleSheet;

}

void OriginPushButton::setButtonType(ButtonType buttonType)
{
    mButtonType = buttonType;

    if (isDefault())
    {
        setOriginElementName("OriginPushButton", "ButtonPrimary");
    }
    else
    {
        switch (mButtonType) 
        {
        case ButtonPrimary:
            setOriginElementName("OriginPushButton", "ButtonPrimary");
            break;
        case ButtonSecondary:
            setOriginElementName("OriginPushButton", "ButtonSecondary");
            break;
		case ButtonAccept:
			setOriginElementName("OriginPushButton", "ButtonAccept");
			break;
		case ButtonDecline:
			setOriginElementName("OriginPushButton", "ButtonDecline");
			break;
        case ButtonDark:
            setOriginElementName("OriginPushButton", "ButtonDark");
            break;
        default:
            mButtonType = ButtonTertiary;
            // fall through
        case ButtonTertiary:
            setOriginElementName("OriginPushButton", "ButtonTertiary");
            break;
        };
    }

    this->setStyleSheet(getElementStyleSheet());
    adjustSize();
}

bool OriginPushButton::event(QEvent* event)
{
    OriginUIBase::event(event);
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
            setFocus(Qt::MouseFocusReason);
            break;
        case QEvent::KeyPress:
            {
                QKeyEvent * keyPressEvent = (QKeyEvent*)(event);
                if ( (keyPressEvent->key() == Qt::Key_Enter ||
                    keyPressEvent->key() == Qt::Key_Return)
                    && hasFocus() && !autoDefault())
                {
                    emit clicked();
                }
            }
            break;
        case QEvent::FocusIn:
            {
                QFocusEvent* fe = dynamic_cast<QFocusEvent*>(event);
                if (!mFocusRect &&
                    (fe->reason() == Qt::TabFocusReason ||
                    fe->reason() == Qt::BacktabFocusReason))
                {
                    mFocusRect = true;
                    /*
                    EBILOGACTION << "Tab/Mouse Focus in: " << text().toLatin1();
                    QString str = styleSheet();
                    str.append("OriginPushButton:focus { outline: 1px dotted #000; border: 0px 0px 0px 0px;}");
                    setStyleSheet(str);
                    */
                }
                emit focusIn(this); 
            }
            break;
        case QEvent::FocusOut:
            {
                emit focusOut(this);
            }
            break;
        default:
            break;
    };

    return QPushButton::event(event);
}

void OriginPushButton::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QPushButton::paintEvent(event);
    
    if (hasFocus() && mFocusRect) {
        QPainter p(this);

        QRect r = this->rect();
        p.save();
        
        p.setBackgroundMode(Qt::TransparentMode);
        QColor bg_col = p.background().color();
        // Create an "XOR" color.
        QColor patternCol((bg_col.red() ^ 0xff) & 0xff,
            (bg_col.green() ^ 0xff) & 0xff,
            (bg_col.blue() ^ 0xff) & 0xff);

        p.setBrush(QBrush(patternCol, Qt::Dense4Pattern));
        p.setBrushOrigin(r.topLeft());
        p.setPen(Qt::NoPen);
        
        //r = QRect(rect().x()+2, rect().y()+4, rect().width()-5, rect().height()-12);

        // this is one pixel in
        r = QRect(rect().x()+3, rect().y()+6, rect().width()-8, rect().height()-15);

        r = QRect(contentsRect());

        if (isDefault() || mButtonType == ButtonPrimary || mButtonType == ButtonSecondary)
        {
            p.drawRect(r.left() + 4, r.top() + 4, 1, r.height() - 12);   // Left
            p.drawRect(r.left() + 4, r.top() + 4, r.width() - 11, 1);    // Top
            p.drawRect(r.right() - 5, r.top() + 4, 1, r.height() - 11);  // Right
            p.drawRect(r.left() + 4, r.bottom() - 7, r.width() - 10, 1); // Bottom

        }
        else 
        {
            p.drawRect(r.left() + 3, r.top() + 5, 1, r.height() - 13);   // Left
            p.drawRect(r.left() + 3, r.top() + 5, r.width() - 8, 1);    // Top
            p.drawRect(r.right() - 4, r.top() + 5, 1, r.height() - 14);  // Right
            p.drawRect(r.left() + 3, r.bottom() - 9, r.width() - 7, 1); // Bottom
        }
        
        p.restore();

    } 
    
}

void OriginPushButton::setDefault(bool aDefault)
{
    QPushButton::setDefault(aDefault);
    this->setStyleSheet(getElementStyleSheet());
}

void OriginPushButton::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginPushButton::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginPushButton::prepForStyleReload()
{
    mCurrentStyleSheet = QString("");
}

}
}