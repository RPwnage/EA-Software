#include <QtWidgets>
#include "originiconbutton.h"

namespace Origin {
    namespace UIToolkit {
QString OriginIconButton::mPrivateStyleSheet = QString("");

static const QColor colorNormal = QColor(103,103,103);
static const QColor colorHovered = QColor(255,255,255);
static const QColor colorDisabled = QColor(103,103,103,128);


OriginIconButton::OriginIconButton(QWidget *parent/*=0*/) :
    QPushButton(parent),
    mHovered(false),
    mPressed(false)
{
    setOriginElementName("OriginIconButton");
    m_children.append(this);
    setAttribute(Qt::WA_TranslucentBackground);

    mMask = NULL;
}

OriginIconButton::~OriginIconButton()
{
    if (mMask) delete mMask;
    this->removeElementFromChildList(this);
}

QString & OriginIconButton::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

void OriginIconButton::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QPushButton::paintEvent(event);

    if (mMask)
    {
        // Create a color mask from the icon

        QPainter painterMask;
        painterMask.begin(mMask);

        painterMask.setCompositionMode(QPainter::CompositionMode_Clear);
        painterMask.eraseRect(0,0,mMask->width(),mMask->height());

        painterMask.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painterMask.drawPixmap(0,0,icon.width(),icon.height(), icon);

        painterMask.setCompositionMode(QPainter::CompositionMode_SourceIn);
        QRect colorRect(0,0,mMask->width(),mMask->height());
        if (isEnabled())
        {
            painterMask.setBrush((mPressed || mHovered) ? colorHovered : colorNormal);
        }
        else
        {
            painterMask.setBrush(colorDisabled);
        }
        painterMask.drawRect(colorRect);

        painterMask.end();


        // Blit the backbuffer

        QRect frameRect = QRect(0,0, this->width(), this->height());
        int cx = (frameRect.width()/2) - (icon.width()/2);
        int cy = (frameRect.height()/2) - (icon.height()/2);

        // Offset for the button shadow in the lower right - this keeps our image centered on the button
        --cy;

        if (mPressed) cy += 1; // offset image 1px if pressed

        QPainter painter;
        painter.begin(this);

        painter.drawPixmap(cx,cy,mMask->width(),mMask->height(), *mMask);

        painter.end();
    }
}

void OriginIconButton::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginIconButton::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginIconButton::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

void OriginIconButton::setIcon(const QPixmap& pixmap)
{
    icon = pixmap;

    // Update our mask image
    if (mMask)
    {
        delete mMask;
        mMask = NULL;
    }

    if (!icon.isNull())
    {
        mMask = new QPixmap(icon.width(), icon.height());

        // Do some stupid voodoo to make our backbuffer has an alpha channel
        QBitmap* bmMask = new QBitmap(mMask->width(), mMask->height());
        bmMask->clear();
        mMask->setMask(*bmMask);
        delete bmMask;
    }

    update();
}

void OriginIconButton::updateState()
{
    this->setStyleSheet(getElementStyleSheet());
    update(); // queue a paint
}

bool OriginIconButton::event(QEvent* event)
{
    OriginUIBase::event(event);

    if (event->type() == QEvent::Polish)
    {
        updateState();
    }

    else if ( event->type() == QEvent::EnabledChange )
    {
        updateState();
    }

    if (isEnabled())
    {
        if ( event->type() == QEvent::Enter )
        {
            mHovered = true;
            updateState();
        }
        else if ( event->type() == QEvent::Leave )
        {
            mHovered = false;
            updateState();
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            if ( ((QMouseEvent*)event)->button() == Qt::LeftButton )
            {
                mPressed = true;
                updateState();
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
            if (this->rect().contains(mouse_event->pos()))
            {
                mHovered = true;
                updateState();
            }
            else
            {
                mHovered = false;
                updateState();
            }
        }
        else if ( event->type() == QEvent::MouseButtonRelease )
        {
            if ( mPressed )
            {
                mPressed = false;
                updateState();
            }
        }
        else if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
            if (key_event && (key_event->key() == Qt::Key_Space))
            {
                mPressed = true;
                updateState();
            }
        }
        else if ( event->type() == QEvent::KeyRelease )
        {
            QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
            if (key_event && (key_event->key() == Qt::Key_Space))
            {
                if ( mPressed )
                {
                    mPressed = false;
                    updateState();
                }
            }
        }

    }

    return QPushButton::event(event);
}

}
}
