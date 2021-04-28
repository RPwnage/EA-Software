#include <QtWidgets>
#include <QtCore>
#include <QUrl>
#include "originlabel.h"

namespace Origin {
    namespace UIToolkit {

QString OriginLabel::mPrivateStyleSheetDialogLabel = QString("");
QString OriginLabel::mPrivateStyleSheetDialogDescription = QString("");
QString OriginLabel::mPrivateStyleSheetDialogTitle = QString("");
QString OriginLabel::mPrivateStyleSheetDialogSmall = QString("");
QString OriginLabel::mPrivateStyleSheetDialogFieldName = QString("");
QString OriginLabel::mPrivateStyleSheetDialogStatus = QString("");


OriginLabel::OriginLabel(QWidget *parent/*= 0*/)
: QLabel(parent)
, mLabelType(DialogUninitialized)
, mStatus(Normal)
, mElideMode(Qt::ElideNone)
, mHyperlinkStyle(Uninitialized)
, mSpecialColorStyle(Color_Normal)
, mHyperlink(false)
, mMaxNumLines(1)
, mHovered(false)
, mPressed(false)
, mFocusRect(false)
, mHyperLinkArrowWidget(NULL)
, ARROW_OFFSET_Y(0)
, mIsShowingElide(false)
{
    setLabelType(DialogLabel);
    setHyperlinkStyle(Blue);
    setHyperlink(false);
    setSpecialColorStyle(Color_Normal);
    m_children.append(this);
}

OriginLabel::~OriginLabel()
{
    this->removeElementFromChildList(this);
    if(mHyperLinkArrowWidget)
    {
        mHyperLinkArrowWidget->deleteLater();
        mHyperLinkArrowWidget = NULL;
    }
}


QString& OriginLabel::getElementStyleSheet()
{
    QString* s = &mPrivateStyleSheetDialogLabel;
    QString path;

    switch (mLabelType)
    {
        default:
        case DialogLabel:
            path = ":/OriginCommonControls/OriginLabel/styleDialogLabel.qss";
            s = &mPrivateStyleSheetDialogLabel;
            break;
        case DialogDescription:
            path = ":/OriginCommonControls/OriginLabel/styleDialogDescription.qss";
            s = &mPrivateStyleSheetDialogDescription;
            break;
        case DialogTitle:
            path = ":/OriginCommonControls/OriginLabel/styleDialogTitle.qss";
            s = &mPrivateStyleSheetDialogTitle;
            break;
        case DialogSmall:
            path = ":/OriginCommonControls/OriginLabel/styleDialogSmall.qss";
            s = &mPrivateStyleSheetDialogSmall;
            break;
        case DialogFieldName:
            path = ":/OriginCommonControls/OriginLabel/styleDialogFieldName.qss";
            s = &mPrivateStyleSheetDialogFieldName;
            break;
        case DialogStatus:
            path = ":/OriginCommonControls/OriginLabel/styleDialogStatus.qss";
            s = &mPrivateStyleSheetDialogStatus;
            break;
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

bool OriginLabel::event(QEvent* event)
{
    OriginUIBase::event(event);

    switch(event->type())
    {
    case QEvent::Polish:
        updateHyperlinkState(Plain);
        break;
    case QEvent::EnabledChange:
        if (getHyperlink())
        {
            if (isEnabled())
            {
                updateHyperlinkState(Plain);
                setCursor(Qt::PointingHandCursor);
            }
            else
            {
                updateHyperlinkState(mPressed ? Active : (mHovered ? Hover : Plain));
                setCursor(Qt::ArrowCursor);
            }
        }
        break;
    case QEvent::MouseButtonDblClick:
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if(mouseEvent && mouseEvent->button() == Qt::LeftButton && (textInteractionFlags() & Qt::TextSelectableByMouse))
            {
                setSelection(0, text().length());
            }
        }
        break;
    default:
        break;
    }


    if (isEnabled() && getHyperlink())
    {
        switch(event->type())
        {
        case QEvent::Enter:
            mHovered = true;
            updateHyperlinkState(Hover);
            break;
        case QEvent::Leave:
            mHovered = false;
            updateHyperlinkState(Plain);
            break;
        case QEvent::MouseButtonPress:
            if ( ((QMouseEvent*)event)->button() == Qt::LeftButton )
            {
                mPressed = true;
                updateHyperlinkState(Active);
            }
            break;
        case QEvent::MouseMove:
            {
                QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
                if (this->rect().contains(mouse_event->pos()))
                {
                    mHovered = true;
                    updateHyperlinkState(Active);
                }
                else
                {
                    mHovered = false;
                    updateHyperlinkState(Plain);
                }
            }
            break;
        case QEvent::MouseButtonRelease:
            if ( mPressed )
            {
                mPressed = false;
                updateHyperlinkState(mHovered ? Hover : Plain);

                if ( mHovered )
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
                }
            }
        default:
            break;
        }
    }

    if (isEnabled() && !getHyperlink())
    {
        if(event->type() == QEvent::MouseButtonRelease)
        {
            event->ignore();
        }
    }

    // This might be a little over the top but the arrow needs to stay with the text
    if(mHyperLinkArrowWidget)
    {
        const QRect r = fontMetrics().boundingRect(rect(), alignment(), text());
        mHyperLinkArrowWidget->move(r.right() + ARROW_OFFSET_X, r.top() + ARROW_OFFSET_Y);
    }

    return QLabel::event(event);
}

void OriginLabel::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);

    // if the elide mode is none then paint (but make sure that wrap is on for multiline)
	if(mElideMode == Qt::ElideNone)
	{
		QLabel::paintEvent(event);
	}
    // if the elide mode is not none but there is only one line, then do the simple
    // elide calculation
	else if (mMaxNumLines <= 1)
	{
		QPainter painter(this);
		int iLeft, iTop, iRight, iBottom;
		this->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
		int iWidth = this->width() - iLeft - iRight;
		QString  elidedText=this->fontMetrics().elidedText( this->text(), mElideMode, iWidth );
		painter.drawText(this->rect(), this->alignment(),elidedText);
	}
    // this is a multiline job with elided text
    else 
    {
        // get the label's text
        QString text = this->text();
        // get the label's width
        int labelValidWidth = this->width();
        // get the valid width to use
        labelValidWidth -= 2*this->margin();

        // init the max line can be shown
        int maxline = mMaxNumLines;

        // create a textlayout to draw
        QTextLayout *textLayout= new QTextLayout( text, this->font());

        // get the label's font inforamtion
        QFontMetrics fontMetrics( this->font() );
        int leading = fontMetrics.leading();

        qreal height = 0;
        // this variable is used to restore the text width before maxline
        int textWidthBeforeLastLine = 0;

        textLayout->clearLayout();

        textLayout->beginLayout();
        for (int i = 0; i < maxline; i++) {
            QTextLine line = textLayout->createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(labelValidWidth);
            height += leading;
            line.setPosition(QPointF(0, height));
            height += line.height();
            // get every line's natural text width
            if (line.naturalTextWidth() > labelValidWidth)
            {
                // This text has no spaces and can't be broken mid word
                // skip adding this lines width so the text gets properly 
                // elided. This fixes problems with labels used in QGraphicsScenes (IGO)
                // https://developer.origin.com/support/browse/EBIBUGS-26565
                break;
            }
            if(i < maxline - 1)
            {
                textWidthBeforeLastLine += line.naturalTextWidth();
            }
        }
        textLayout->endLayout();

        // get a new string, add elide property
        QString newMessage =  this->fontMetrics().elidedText( text, Qt::ElideRight, textWidthBeforeLastLine + labelValidWidth);

        // reset the textlayout information
        textLayout->clearLayout();
        textLayout->setText(newMessage);

        // init the text layout's height 
        height = 0;

        textLayout->beginLayout();
        while (1) {
            QTextLine line = textLayout->createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(labelValidWidth);
            height += leading;
            line.setPosition(QPointF(0, height));
            height += line.height();
        }
        textLayout->endLayout();

        // draw the layout to the label
        QPainter painter(this);
        textLayout->draw(&painter, QPoint(0, 0));
        delete textLayout;

    }

    if (hasFocus() && mFocusRect) 
    {
        QPainter p(this);

        QRect r = this->rect();
        p.save();
        p.setBackgroundMode(Qt::TransparentMode);
        p.setBrush(QBrush(QColor().black(), Qt::Dense4Pattern));
        p.setBrushOrigin(r.topLeft());
        p.setPen(Qt::NoPen);

        r = fontMetrics().boundingRect(rect(), alignment(), text());

        if(mHyperLinkArrowWidget)
            r.setWidth(r.width() + mHyperLinkArrowWidget->width() + ARROW_OFFSET_X);

        p.drawRect(r.left(), r.top(), 1, r.height());   // Left
        p.drawRect(r.left(), r.top(), r.width(), 1);    // Top
        p.drawRect(r.right(), r.top(), 1, r.height());  // Right
        p.drawRect(r.left(), r.bottom(), r.width(), 1); // Bottom

        p.restore();
    }
}

bool OriginLabel::isShowingElide() const
{
    if(mElideMode != Qt::ElideNone)
    {
        return fontMetrics().boundingRect(text()).width() > width();
    }
    return false;
}

void OriginLabel::setLabelType(LabelType aLabelType)
{
    if (aLabelType != mLabelType)
    {
        mLabelType = aLabelType;

        if (mHyperlink)
        {
            this->setKerning(true);
        }
        else
        {
            switch (mLabelType)
            {
                default:
                case DialogLabel:
                    this->setKerning(true);
                    break;
                case DialogDescription:
                    this->setKerning(false);
                    break;
                case DialogTitle:
                    this->setKerning(true);
                    break;
                case DialogSmall:
                    this->setKerning(true);
                    break;
            }
        }

        switch (mLabelType)
        {
            default:
            case DialogLabel:
                setOriginElementName("OriginLabel", "DialogLabel");
                ARROW_OFFSET_Y = 5;
                break;
            case DialogDescription:
                setOriginElementName("OriginLabel", "DialogDescription");
                ARROW_OFFSET_Y = 5;
                break;
            case DialogTitle:
                setOriginElementName("OriginLabel", "DialogTitle");
                ARROW_OFFSET_Y = 8;
                break;
            case DialogSmall:
                setOriginElementName("OriginLabel", "DialogSmall");
                ARROW_OFFSET_Y = 2;
                break;
            case DialogFieldName:
                setOriginElementName("OriginLabel", "DialogFieldName");
                ARROW_OFFSET_Y = 5;
                break;
            case DialogStatus:
                setOriginElementName("OriginLabel", "DialogStatus");
                ARROW_OFFSET_Y = 4;
                break;
                
        }


        this->setStyleSheet(getElementStyleSheet());

        adjustSize();
    }
}

void OriginLabel::setHyperlink(bool aHyperlink)
{
    mHyperlink = aHyperlink;

    mHovered = false;
    mPressed = false;

    updateHyperlinkState(Plain);

    setCursor(mHyperlink ? (Qt::PointingHandCursor) : (Qt::ArrowCursor));
    this->setFocusPolicy(mHyperlink ? (Qt::TabFocus) : (Qt::NoFocus));

    adjustSize();
}

void OriginLabel::setValidationStatus(const OriginValidationStatus& status)
{
    mStatus = status;
    switch(mStatus)
    {
    case Valid:
        setSpecialColorStyle(Color_Green);
        break;
    case Invalid:
        setSpecialColorStyle(Color_Red);
        break;
    default:
        break;
    }
}

OriginLabel::OriginValidationStatus OriginLabel::getValidationStatus() const
{
    return mStatus;
}

void OriginLabel::setHyperlinkStyle(HyperlinkStyle aHyperlinkStyle)
{
    mHyperlinkStyle = aHyperlinkStyle;
    switch(mHyperlinkStyle)
    {
    case Blue:
        setSpecialColorStyle(Color_Blue);
        break;
    case Black:
        setSpecialColorStyle(Color_Black);
        break;
    case LightBlue:
        setSpecialColorStyle(Color_LightBlue);
        break;
    default:
        break;
    }
    this->setStyleSheet(getElementStyleSheet());
}

void OriginLabel::setSpecialColorStyle(SpecialColorStyle aColorStyle)
{
    mSpecialColorStyle = aColorStyle;
    this->setStyleSheet(getElementStyleSheet());
}

void OriginLabel::setHyperlinkArrow(bool aHyperlinkArrow)
{
    if(aHyperlinkArrow && mHyperLinkArrowWidget == NULL)
    {
        mHyperLinkArrowWidget = new QLabel(this);
        mHyperLinkArrowWidget->setFixedSize(4,7);
        mHyperLinkArrowWidget->setPixmap(QPixmap(":/OriginCommonControls/OriginLabel/arrow.png"));
        mHyperLinkArrowWidget->setObjectName("hyperlinkArrow");
        const QRect r = fontMetrics().boundingRect(rect(), alignment(), text());
        mHyperLinkArrowWidget->move(r.right() + ARROW_OFFSET_X, r.top() + ARROW_OFFSET_Y);
    }
    else
    {
        if(mHyperLinkArrowWidget)
        {
            mHyperLinkArrowWidget->deleteLater();
            mHyperLinkArrowWidget = NULL;
        }
    }
}

void OriginLabel::setElideMode(Qt::TextElideMode aElideMode)
{
	mElideMode = aElideMode;
    this->setStyleSheet(getElementStyleSheet());
}

void OriginLabel::setMaxNumLines(const int& numLines)
{
    mMaxNumLines = numLines > 0 ? numLines : 1;
    setWordWrap(mMaxNumLines > 1);
}

void OriginLabel::updateHyperlinkState(HyperlinkState state)
{
    switch (state)
    {
        case Normal:
            setProperty("State", "Plain");
            break;
        case Hover:
            setProperty("State", "Hover");
            break;
        case Active:
            setProperty("State", "Active");
            break;
        default:
            setProperty("State", "");
            break;
    }

    this->setStyleSheet(getElementStyleSheet());
}

/*
 // PDH - I'm still playing with this piece of code
QSize OriginLabel::sizeHint () const
{
    if (hyperlink)
    {
        QFontMetrics fontMetrics(this->font());
        QSize size = fontMetrics.size(this->wordWrap() ? (Qt::TextWordWrap) : (Qt::TextSingleLine), this->text());
        int arrowWidth = (this->property("arrow-off").toString() == "1") ? 0 : 18;
        size.setWidth(size.width()+arrowWidth);
        return size;
    }
    else return QLabel::sizeHint();
}
*/

void OriginLabel::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginLabel::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginLabel::prepForStyleReload()
{
    mCurrentStyleSheet = QString("");
}

void OriginLabel::setKerning(bool enable)
{
    QFont font = this->font();
    font.setKerning(enable);
    this->setFont(font);
}

void OriginLabel::setText(const QString& text)
{
    QLabel::setText(text);
    if(mElideMode != Qt::ElideNone && this->sizePolicy().verticalPolicy() != QSizePolicy::Expanding)
    {
        // Since the elide code happens in the paint event this label
        // and it's parent widget need to know what the max size should be.
        QFontMetrics fontMetrics(font());
        int height = fontMetrics.height();
        int leading = fontMetrics.leading();
        int totalHeight = (mMaxNumLines * height) + ((mMaxNumLines - 1) *leading);// + height + leading + height;
        setMinimumHeight(height);
        setMaximumHeight(totalHeight);
    }
}

}
}

