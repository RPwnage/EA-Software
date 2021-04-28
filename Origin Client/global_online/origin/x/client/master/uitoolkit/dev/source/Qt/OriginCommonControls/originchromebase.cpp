#include "originchromebase.h"
#include "ui_originchromeBase.h"

namespace Origin {
    namespace UIToolkit {
OriginChromeBase::OriginChromeBase(QWidget* parent) 
: QDialog(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint)
, mContent(NULL)
, uiBase(new Ui::OriginChromeBase)
, mBanner(NULL)
, mMsgBanner(NULL)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    setOriginElementName("OriginChromeBase");
    m_children.append(this);
}


OriginChromeBase::~OriginChromeBase() 
{
    if(mBanner)
    {
        mBanner->deleteLater();
        mBanner = NULL;
    }

    if(mMsgBanner)
    {
        mMsgBanner->deleteLater();
        mMsgBanner = NULL;
    }

	// Don't trust anyone to clean up their own memory.
	removeContent();
	this->removeElementFromChildList(this);

    delete uiBase;
    uiBase = NULL;
}


void OriginChromeBase::setContent(QWidget* content)
{
	if(content)
	{
		removeContent();
		mContent = content;
		uiBase->contentLayout->addWidget(mContent, 1);
        adjustSize();
	}
}


void OriginChromeBase::removeContent()
{
	if(mContent)
	{
		uiBase->contentLayout->removeWidget(mContent);
        mContent->deleteLater();
        mContent = NULL;
	}
}


QWidget* OriginChromeBase::takeContent()
{
    QWidget* content = mContent;
    if(mContent)
    {
        mContent->setParent(NULL);
        uiBase->contentLayout->removeWidget(mContent);
        mContent = NULL;
    }
    return content;
}


QWidget* OriginChromeBase::titleBar() const
{
    return uiBase->titlebar;
}

QFrame* OriginChromeBase::innerFrame() const
{
    return uiBase->innerFrame;
}

bool OriginChromeBase::event(QEvent* event)
{
    OriginUIBase::event(event);
	return QDialog::event(event);
}


void OriginChromeBase::paintEvent(QPaintEvent* event)
{
	OriginUIBase::paintEvent(event);
	QDialog::paintEvent(event);
}


void OriginChromeBase::setContentMargin(const int& marginSize)
{
    uiBase->contentLayout->setContentsMargins(marginSize, -1, marginSize, -1);
}


void OriginChromeBase::ensureCreatedBanner()
{
    if(mBanner == NULL)
    {
        mBanner = new OriginBanner();
        uiBase->innerVLayout->insertWidget(1, mBanner);
        mBanner->setVisible(false);
    }
}


bool OriginChromeBase::bannerVisible() const
{
    if(mBanner)
        return mBanner->isVisible();
    return false;
}


void OriginChromeBase::setBannerText(const QString& text)
{
    ensureCreatedBanner();
    mBanner->setText(text);
}


const QString OriginChromeBase::bannerText() const
{
    if(mBanner)
        return mBanner->text();
    return "";
}


void OriginChromeBase::setBannerIconType(const OriginBanner::IconType& iconType)
{
    ensureCreatedBanner();
    mBanner->setIconType(iconType);
}


const OriginBanner::IconType OriginChromeBase::bannerIconType() const
{
    if(mBanner)
        return mBanner->getIconType();
    return OriginBanner::IconUninitialized;
}


bool OriginChromeBase::msgBannerVisible() const
{
    if(mMsgBanner)
        return mMsgBanner->isVisible();
    return false;
}


void OriginChromeBase::setMsgBannerText(const QString& text)
{
    ensureCreatedMsgBanner();
    if(mMsgBanner)
        mMsgBanner->setText(text);
}


const QString OriginChromeBase::msgBannerText(void) const
{
    if(mMsgBanner)
        return mMsgBanner->text();
    return "";
}


void OriginChromeBase::setMsgBannerIconType(const OriginBanner::IconType& iconType)
{
    ensureCreatedMsgBanner();
    if(mMsgBanner)
        mMsgBanner->setIconType(iconType);
}


const OriginBanner::IconType OriginChromeBase::msgBannerIconType() const
{
    if(mMsgBanner)
        return mMsgBanner->getIconType();
    return OriginBanner::IconUninitialized;
}
}
}
