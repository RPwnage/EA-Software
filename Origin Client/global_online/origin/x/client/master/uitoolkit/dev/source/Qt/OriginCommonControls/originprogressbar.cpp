#include <QtWidgets>
#include <QPainter>
#include "originprogressbar.h"
#include "origindivider.h"
#include "OriginCommonUIUtils.h"
namespace Origin {
    namespace UIToolkit  {
    
QString OriginProgressBar::mPrivateStyleSheet = QString("");

QPixmap* OriginProgressBar::mPixmapGutter = NULL;
QPixmap* OriginProgressBar::mPixmapBlueFillBar = NULL;
QPixmap* OriginProgressBar::mPixmapGreenFillBar = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarPaused = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarShadow = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarMask = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarIndeterminateMask = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarBarberPoleOverlay = NULL;

QPixmap* OriginProgressBar::mPixmapGutterSmall = NULL;
QPixmap* OriginProgressBar::mPixmapBlueFillBarSmall = NULL;
QPixmap* OriginProgressBar::mPixmapGreenFillBarSmall = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarPausedSmall = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarMaskSmall = NULL;
QPixmap* OriginProgressBar::mPixmapFillBarIndeterminateMaskSmall = NULL;


QPixmap* OriginProgressBar::mPixmapCompleted = NULL;

QFont* OriginProgressBar::mLabelFont = NULL;

long OriginProgressBar::mPixmapRefCount = 0;


OriginProgressBar::OriginProgressBar(QWidget *parent/*=0*/) :
    QProgressBar(parent),
    mFillBarBackbuffer(NULL),
    mAnimationXPos(0),
    paused(false),
    animated(true),
    fixed(false),
    completed(false),
    labelVisible(true),
    tiny(false),
    indeterminate(false),
    value(0),
	precision(0),
    progressIndicatorValue(0.0),
    color(Color_Blue),
    mIndicator(NULL)
{
    format = QString("%p %%");
    mPercentage = 0.0f;
    mLabelValue = "";

    mAnimationTimer = NULL;

    setOriginElementName("OriginProgressBar");
    m_children.append(this);
    setAttribute(Qt::WA_TranslucentBackground);

    ++mPixmapRefCount;
    setMinimumWidth(100);
}

OriginProgressBar::~OriginProgressBar()
{
    this->removeElementFromChildList(this);

    animated = false;
    updateAnimation();

    if (mFillBarBackbuffer) delete mFillBarBackbuffer;

    // how to clean up shared static pixmaps? Probably need a refcount
    if (--mPixmapRefCount <= 0)
    {
        if (mPixmapGutter) { delete mPixmapGutter; mPixmapGutter = NULL; }
        if (mPixmapBlueFillBar) { delete mPixmapBlueFillBar; mPixmapBlueFillBar = NULL; }
        if (mPixmapGreenFillBar) { delete mPixmapGreenFillBar; mPixmapGreenFillBar = NULL; }
        if (mPixmapFillBarPaused) { delete mPixmapFillBarPaused; mPixmapFillBarPaused = NULL; }
        if (mPixmapFillBarShadow) { delete mPixmapFillBarShadow; mPixmapFillBarShadow = NULL; }
        if (mPixmapFillBarMask) { delete mPixmapFillBarMask; mPixmapFillBarMask = NULL; }
        if (mPixmapFillBarIndeterminateMask) { delete mPixmapFillBarIndeterminateMask; mPixmapFillBarIndeterminateMask = NULL; }
        if (mPixmapFillBarBarberPoleOverlay) { delete mPixmapFillBarBarberPoleOverlay; mPixmapFillBarBarberPoleOverlay = NULL; }

        if (mPixmapGutterSmall) { delete mPixmapGutterSmall; mPixmapGutterSmall = NULL; }
        if (mPixmapBlueFillBarSmall) { delete mPixmapBlueFillBarSmall; mPixmapBlueFillBarSmall = NULL; }
        if (mPixmapGreenFillBarSmall) { delete mPixmapGreenFillBarSmall; mPixmapGreenFillBarSmall = NULL; }
        if (mPixmapFillBarPausedSmall) { delete mPixmapFillBarPausedSmall; mPixmapFillBarPausedSmall = NULL; }
        if (mPixmapFillBarMaskSmall) { delete mPixmapFillBarMaskSmall; mPixmapFillBarMaskSmall = NULL; }
        if (mPixmapFillBarIndeterminateMaskSmall) { delete mPixmapFillBarIndeterminateMaskSmall; mPixmapFillBarIndeterminateMaskSmall = NULL; }

        if (mPixmapCompleted) { delete mPixmapCompleted; mPixmapCompleted = NULL; }

        if (mLabelFont) { delete mLabelFont; mLabelFont = NULL; }
        if (mIndicator) { mIndicator->deleteLater(); mIndicator = NULL; }
    }
}

QString & OriginProgressBar::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginProgressBar::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QProgressBar::event(event);
}

void OriginProgressBar::resizeEvent(QResizeEvent* e)
{
    QProgressBar::resizeEvent(e);

    // Widget has been resized - need to create a new backbuffer
    if (mFillBarBackbuffer)
    {
        delete mFillBarBackbuffer;
        mFillBarBackbuffer = NULL;
    }
}

void OriginProgressBar::showEvent(QShowEvent *pEvent)
{
    QProgressBar::showEvent(pEvent);
    updateAnimation();
}

void OriginProgressBar::hideEvent(QHideEvent *pEvent)
{
    QProgressBar::hideEvent(pEvent);
    updateAnimation();
}

#ifdef Q_OS_WIN
        
void OriginProgressBar::paintEvent(QPaintEvent *e)
{
    (void)e;
    
    ensureBuffers();
    
    QRect frameRect = QRect(0,0, this->width(), this->height());
    
    QPainter painter;
    painter.begin(this);
    
    if (tiny)
    {
        // Draw the background image
        OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapGutterSmall, frameRect, QMargins(3,0,3,0));
        
        // Draw the fill bar
        int range = this->maximum() - this->minimum();
        if ((range > 0) || indeterminate)
        {
            // Begin compositing to our backbuffer
            QPainter backbufferPainter;
            backbufferPainter.begin(mFillBarBackbuffer);
            
            QPainter::CompositionMode oldCompMode = backbufferPainter.compositionMode();
            
            // Clear out the old contents of the backbuffer before we begin painting
            backbufferPainter.setCompositionMode(QPainter::CompositionMode_Clear);
            backbufferPainter.eraseRect(0,0,mFillBarBackbuffer->width(), mFillBarBackbuffer->height());
            
            backbufferPainter.setCompositionMode(oldCompMode);
            
            float percentage = indeterminate ? 1.0f : mPercentage;
            if (percentage > 1.0f) percentage = 1.0f;
            if (percentage > 0.0f)
            {
                int barWidth = frameRect.width() * percentage;
                if (barWidth > 0)
                {
                    QRect fillbarRect(0,0,barWidth,5);
                    if(color == Color_Green)
                    {
                        OriginCommonUIUtils::DrawImageSlices(backbufferPainter, paused ? (*mPixmapFillBarPausedSmall) : (*mPixmapGreenFillBarSmall), fillbarRect, QMargins(3,0,1,0));
                    }
                    else
                    {
                        OriginCommonUIUtils::DrawImageSlices(backbufferPainter, paused ? (*mPixmapFillBarPausedSmall) : (*mPixmapBlueFillBarSmall), fillbarRect, QMargins(3,0,1,0));
                    }
                    
                    if (!fixed || indeterminate)
                    {
                        // Overlay the barbershop pole stripes as a tiled image
                        backbufferPainter.setCompositionMode(QPainter::CompositionMode_Overlay);
                        backbufferPainter.drawTiledPixmap(QRect(0,0,barWidth-1,mPixmapFillBarBarberPoleOverlay->height()), *mPixmapFillBarBarberPoleOverlay, QPoint(mAnimationXPos,0));
                        backbufferPainter.setCompositionMode(oldCompMode);
                    }
                }
            }
            
            // Mask the backbuffer!
            backbufferPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            OriginCommonUIUtils::DrawImageSlices(backbufferPainter, indeterminate ? (*mPixmapFillBarIndeterminateMaskSmall) : (*mPixmapFillBarMask), frameRect, QMargins(6,0,6,0));
            backbufferPainter.setCompositionMode(oldCompMode);
            
            backbufferPainter.end();
            
            // Blit the backbuffer to our main buffer
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawPixmap(0,0, *mFillBarBackbuffer);
        }
        
    }
    else
    {
        if (completed)
        {
            // Draw the completed image
            OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapCompleted, frameRect, QMargins(6,0,6,0));
        }
        else
        {
            // Draw the background image
            OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapGutter, frameRect, QMargins(6,0,6,0));
            
            // Draw the fill bar
            int range = this->maximum() - this->minimum();
            if ((range > 0) || indeterminate)
            {
                // Begin compositing to our backbuffer
                QPainter backbufferPainter;
                backbufferPainter.begin(mFillBarBackbuffer);
                
                QPainter::CompositionMode oldCompMode = backbufferPainter.compositionMode();
                
                // Clear out the old contents of the backbuffer before we begin painting
                backbufferPainter.setCompositionMode(QPainter::CompositionMode_Clear);
                backbufferPainter.eraseRect(0,0,mFillBarBackbuffer->width(), mFillBarBackbuffer->height());
                
                backbufferPainter.setCompositionMode(oldCompMode);
                
                float percentage = indeterminate ? 1.0f : mPercentage;
                if (percentage > 1.0f) percentage = 1.0f;
                if (percentage > 0.0f)
                {
                    int barWidth = frameRect.width() * percentage;
                    if (barWidth > 0)
                    {
                        QRect fillbarRect(1,1,barWidth,19);
                        if(color == Color_Green)
                        {
                            OriginCommonUIUtils::DrawImageSlices(backbufferPainter, paused ? (*mPixmapFillBarPaused) : (*mPixmapGreenFillBar), fillbarRect, QMargins(6,0,1,0));
                        }
                        else
                        {
                            OriginCommonUIUtils::DrawImageSlices(backbufferPainter, paused ? (*mPixmapFillBarPaused) : (*mPixmapBlueFillBar), fillbarRect, QMargins(6,0,1,0));
                        }
                        
                        // Draw the shadow to the right of the fill bar
                        backbufferPainter.drawPixmap(1 + barWidth, 1, *mPixmapFillBarShadow);
                        
                        if (!fixed || indeterminate)
                        {
                            // Overlay the barbershop pole stripes as a tiled image
                            backbufferPainter.setCompositionMode(QPainter::CompositionMode_Overlay);
                            backbufferPainter.drawTiledPixmap(QRect(2,2,barWidth-1,mPixmapFillBarBarberPoleOverlay->height()), *mPixmapFillBarBarberPoleOverlay, QPoint(mAnimationXPos,0));
                            backbufferPainter.setCompositionMode(oldCompMode);
                        }
                    }
                }
                
                // Mask the backbuffer!
                backbufferPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                OriginCommonUIUtils::DrawImageSlices(backbufferPainter, indeterminate ? (*mPixmapFillBarIndeterminateMask) : (*mPixmapFillBarMask), frameRect, QMargins(6,0,6,0));
                backbufferPainter.setCompositionMode(oldCompMode);
                
                
                backbufferPainter.end();
                
                // Blit the backbuffer to our main buffer
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.drawPixmap(0,0, *mFillBarBackbuffer);
            }
        }
        
        if (labelVisible)
        {
            if (!mLabelFont)
            {
                mLabelFont = new QFont ("Arial", 8, QFont::Bold);
            }
            
            // Paint the label
            QRect labelRect(frameRect);
            
            painter.setFont(*mLabelFont);
            painter.setPen(QColor(66,66,66,255));
            painter.drawText(labelRect, mLabelValue, QTextOption(Qt::AlignCenter));
        }
        
    }
    
    painter.end();
    
    // Kick off our animation if we are supposed to be animated
    // We only begin animating on the first paint
    updateAnimation();
}
        
#else
        
void OriginProgressBar::paintEvent(QPaintEvent *e)
{
    (void)e;
    
    ensureBuffers();

    QRect frameRect = QRect(0,0, this->width(), this->height());

    QPainter painter;
    painter.begin(this);

    if (tiny)
    {
        // Draw the background image
        OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapGutterSmall, frameRect, QMargins(3,0,3,0));

        // Draw the fill bar
        int range = this->maximum() - this->minimum();
        if ((range > 0) || indeterminate)
        {
            QPainter::CompositionMode oldCompMode = painter.compositionMode();

            float percentage = indeterminate ? 1.0f : mPercentage;
            if (percentage > 1.0f) percentage = 1.0f;
            if (percentage > 0.0f)
            {
                int barWidth = frameRect.width() * percentage;
                if (barWidth > 0)
                {
                    QRect fillbarRect(0,0,barWidth,5);
                    if(color == Color_Green)
                    {
                        OriginCommonUIUtils::DrawImageSlices(painter, paused ? (*mPixmapFillBarPausedSmall) : (*mPixmapGreenFillBarSmall), fillbarRect, QMargins(3,0,1,0));
                    }
                    else
                    {
                        OriginCommonUIUtils::DrawImageSlices(painter, paused ? (*mPixmapFillBarPausedSmall) : (*mPixmapBlueFillBarSmall), fillbarRect, QMargins(3,0,1,0));
                    }

                    if (!fixed || indeterminate)
                    {
                        // Overlay the barbershop pole stripes as a tiled image
                        painter.setCompositionMode(QPainter::CompositionMode_Overlay);
                        painter.drawTiledPixmap(QRect(0,0,barWidth-1,mPixmapFillBarBarberPoleOverlay->height()), *mPixmapFillBarBarberPoleOverlay, QPoint(mAnimationXPos,0));
                        painter.setCompositionMode(oldCompMode);
                    }
                }
            }
        }

    }
    else
    {
        if (completed)
        {
            // Draw the completed image
            OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapCompleted, frameRect, QMargins(6,0,6,0));
        }
        else
        {
            // Draw the background image
            OriginCommonUIUtils::DrawImageSlices(painter, *mPixmapGutter, frameRect, QMargins(6,0,6,0));

            // Draw the fill bar
            int range = this->maximum() - this->minimum();
            if ((range > 0) || indeterminate)
            {
                QPainter::CompositionMode oldCompMode = painter.compositionMode();

                float percentage = indeterminate ? 1.0f : mPercentage;
                if (percentage > 1.0f) percentage = 1.0f;
                if (percentage > 0.0f)
                {
                    int barWidth = frameRect.width() * percentage;
                    if (barWidth > 0)
                    {
                        QRect fillbarRect(1,1,barWidth,19);
                        if(color == Color_Green)
                        {
                            OriginCommonUIUtils::DrawImageSlices(painter, paused ? (*mPixmapFillBarPaused) : (*mPixmapGreenFillBar), fillbarRect, QMargins(6,0,1,0));
                        }
                        else
                        {
                            OriginCommonUIUtils::DrawImageSlices(painter, paused ? (*mPixmapFillBarPaused) : (*mPixmapBlueFillBar), fillbarRect, QMargins(6,0,1,0));
                        }

                        // Draw the shadow to the right of the fill bar
                        painter.drawPixmap(1 + barWidth, 1, *mPixmapFillBarShadow);

                        if (!fixed || indeterminate)
                        {
                            // Overlay the barbershop pole stripes as a tiled image
                            painter.setCompositionMode(QPainter::CompositionMode_Overlay);
                            painter.drawTiledPixmap(QRect(2,2,barWidth-1,mPixmapFillBarBarberPoleOverlay->height()), *mPixmapFillBarBarberPoleOverlay, QPoint(mAnimationXPos,0));
                            painter.setCompositionMode(oldCompMode);
                        }
                    }
                }
            }
        }

        if (labelVisible)
        {
            if (!mLabelFont)
            {
                mLabelFont = new QFont ("Arial", 10, QFont::Bold);
            }

            // Paint the label
            QRect labelRect(frameRect);

            painter.setFont(*mLabelFont);
            painter.setPen(QColor(66,66,66,255));
            painter.drawText(labelRect, mLabelValue, QTextOption(Qt::AlignCenter));
        }

    }

    painter.end();
}
        
#endif
        
void OriginProgressBar::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginProgressBar::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginProgressBar::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

void OriginProgressBar::ensureBuffers()
{
    if (!mPixmapGutter)
        mPixmapGutter = new QPixmap(":/OriginCommonControls/OriginProgressBar/channel.png");
    if (!mPixmapBlueFillBar)
        mPixmapBlueFillBar = new QPixmap(":/OriginCommonControls/OriginProgressBar/fill_bar.png");
    if (!mPixmapGreenFillBar)
        mPixmapGreenFillBar = new QPixmap(":/OriginCommonControls/OriginProgressBar/green_fill_bar.png");
    if (!mPixmapFillBarPaused)
        mPixmapFillBarPaused = new QPixmap(":/OriginCommonControls/OriginProgressBar/fill_bar_paused.png");
    if (!mPixmapFillBarShadow)
        mPixmapFillBarShadow = new QPixmap(":/OriginCommonControls/OriginProgressBar/fill_bar_shadow.png");
    if (!mPixmapFillBarMask)
        mPixmapFillBarMask = new QPixmap(":/OriginCommonControls/OriginProgressBar/mask.png");
    if (!mPixmapFillBarIndeterminateMask)
        mPixmapFillBarIndeterminateMask = new QPixmap(":/OriginCommonControls/OriginProgressBar/mask_indeterminate.png");
    if (!mPixmapFillBarBarberPoleOverlay)
        mPixmapFillBarBarberPoleOverlay = new QPixmap(":/OriginCommonControls/OriginProgressBar/barber_pole_overlay.png");

    if (!mPixmapGutterSmall)
        mPixmapGutterSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/channel_small.png");
    if (!mPixmapBlueFillBarSmall)
        mPixmapBlueFillBarSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/fill_bar_small.png");
    if (!mPixmapGreenFillBarSmall)
        mPixmapGreenFillBarSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/green_fill_bar_small.png");
    if (!mPixmapFillBarPausedSmall)
        mPixmapFillBarPausedSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/fill_bar_paused_small.png");
    if (!mPixmapFillBarMaskSmall)
        mPixmapFillBarMaskSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/mask_small.png");
    if (!mPixmapFillBarIndeterminateMaskSmall)
        mPixmapFillBarIndeterminateMaskSmall = new QPixmap(":/OriginCommonControls/OriginProgressBar/mask_small_indeterminate.png");

    if (!mPixmapCompleted)
        mPixmapCompleted = new QPixmap(":/OriginCommonControls/OriginProgressBar/complete.png");

    if (!mFillBarBackbuffer)
    {
        mFillBarBackbuffer = new QPixmap(this->width(), this->height());

        // Do some stupid voodoo to make our backbuffer have an alpha channel

        QBitmap* bmMask = new QBitmap(this->width(), this->height());
        bmMask->clear();
        mFillBarBackbuffer->setMask(*bmMask);
        delete bmMask;
    }
}

void OriginProgressBar::onAnimationTimer()
{
    if (indeterminate)
    {
        ++mAnimationXPos;
    }
    else
    {
        --mAnimationXPos;
    }
    this->update();
}

void OriginProgressBar::setPaused(bool aPaused)
{
    if (aPaused != paused)
    {
        paused = aPaused;

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setLabelVisible(bool aVisible)
{
    if (aVisible != labelVisible)
    {
        labelVisible = aVisible;

        this->update();
    }
}

void OriginProgressBar::setAnimated(bool aAnimated)
{
    if (aAnimated != animated)
    {
        animated = aAnimated;

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setFixed(bool aFixed)
{
    if (aFixed != fixed)
    {
        fixed = aFixed;

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setCompleted(bool aCompleted)
{
    if (aCompleted != completed)
    {
        completed = aCompleted;

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setTiny(bool aTiny)
{
    if (aTiny != tiny)
    {
        tiny = aTiny;
        this->setStyleSheet(getElementStyleSheet());

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setIndeterminate(bool aIndeterminate)
{
    if (aIndeterminate != indeterminate)
    {
        indeterminate = aIndeterminate;

        this->updateAnimation();

        this->update();
    }
}

void OriginProgressBar::setValue(int aValue)
{
    this->setValue((double)aValue);
}

void OriginProgressBar::setValue(double aValue)
{
    QProgressBar::setValue((int)aValue);

    value = aValue;

    int range = this->maximum() - this->minimum();
    mPercentage = (value-minimum()) / (float)range;
    if (mPercentage < 0.0f) mPercentage = 0.0f;

    updateLabel();

    this->update();
}

void OriginProgressBar::setFormat( const QString &aFormat )
{
    // From the QProgressBar docs
    // %p - is replaced by the percentage completed.
    // %v - is replaced by the current value.
    // %m - is replaced by the total number of steps. (not supported here)
    // The default value is "%p%".

    format = aFormat;
    updateLabel();
    this->update();
}

void OriginProgressBar::setPrecision( int aPrecision )
{
	if(aPrecision < 0)
		aPrecision = 0;

    precision = aPrecision;
    updateLabel();
    this->update();
}

void OriginProgressBar::setColor(const Color& aColor)
{
    if(mIndicator)
    {
        if(aColor == Color_Green)
        {
            mIndicator->setLineStyle(OriginDivider::DarkGreen_LightGreen);
        }
        else
        {
            mIndicator->setLineStyle(OriginDivider::DarkGrey_LightGrey);
        }
    }
    color = aColor;
}


void OriginProgressBar::setProgressIndicatorValue(const double& aValue)
{
    if(aValue <= 0)
    {
        if(mIndicator)
        {
            mIndicator->deleteLater();
            mIndicator = NULL;
        }
    }
    else if(parentWidget())
    {
        if(mIndicator == NULL)
        {
            mIndicator = new OriginDivider(parentWidget());
            mIndicator->setLineStyle(OriginDivider::DarkGrey_LightGrey);
            mIndicator->setLineOrientation(OriginDivider::Vertical);
            mIndicator->setFixedHeight(this->height() + 6);
        }

        if(mIndicator && progressIndicatorValue != aValue)
        {
            const QPoint mapFromParent = this->mapTo(this->parentWidget(), QPoint(0,0));
            int pos = mapFromParent.x() + (width() * aValue);
            mIndicator->move(pos, mapFromParent.y() - 3);
            mIndicator->show();
        }
    }
    progressIndicatorValue = aValue;
}

void OriginProgressBar::updateLabel()
{
    // Use the default locale, which should be set during app startup and if the locale changes at runtime.
    // PDH - shouldn't this be read from the QProgressBar's "format" property?
    QLocale       locale;
    QString       sPercentage             = locale.toString(mPercentage*100.0f, 'f', precision);
    QString       sValue                  = locale.toString(value, 'f', precision);
    const QString sLocalePercentSign      = locale.percent();

    QString formattedString = format;
    formattedString.replace("%p", sPercentage);
    formattedString.replace("%v", sValue);
    formattedString.replace("%%", sLocalePercentSign);

    mLabelValue = formattedString;
}

void OriginProgressBar::updateAnimation()
{
    if (isVisible() && animated && !paused && !fixed && !completed)
    {
        if (!mAnimationTimer)
        {
            mAnimationTimer = new QTimer(this);
            mAnimationTimer->setInterval(indeterminate ? 15 : 50);
            connect(mAnimationTimer, SIGNAL(timeout()), this, SLOT(onAnimationTimer()));
        }
        mAnimationTimer->start();
    }
    else
    {
        if (mAnimationTimer)
        {
            mAnimationTimer->stop();

            disconnect(mAnimationTimer, SIGNAL(timeout()), this, SLOT(onAnimationTimer()));

            delete mAnimationTimer;
            mAnimationTimer = NULL;
        }
    }
}
}
}
