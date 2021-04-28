#include "originwindowtemplatescrollable.h"
#include "ui_originwindowcontent.h"
#include <QScrollArea>
#include <QScrollBar>

namespace Origin {
namespace UIToolkit {

OriginWindowTemplateScrollable::OriginWindowTemplateScrollable(QWidget* parent)
: OriginWindowTemplateBase(parent)
, mScrollArea(NULL)
, uiWindowContent(NULL)
{
    uiWindowContent = new Ui::OriginWindowContent();
    uiWindowContent->setupUi(this);

    mScrollArea = new QScrollArea();
    mScrollArea->setObjectName("scrollArea");
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    uiWindowContent->layout->insertWidget(0, mScrollArea);

    connect(mScrollArea->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(adjustScrollAreaSize(int,int)));
}


OriginWindowTemplateScrollable::~OriginWindowTemplateScrollable()
{
    if(mScrollArea)
    {
        mScrollArea->deleteLater();
        mScrollArea = NULL;
    }

    if(uiWindowContent)
    {
        delete uiWindowContent;
        uiWindowContent = NULL;
    }
}


void OriginWindowTemplateScrollable::adjustScrollAreaSize(int min, int max)
{
    const int sizePlusMax = mScrollArea->height() + max;
    if(max >= 0 && mScrollArea->size().height() <= 400)
    {
        int adjustToSize = sizePlusMax;
        if(sizePlusMax >= 400)
            adjustToSize = 400;
        else if(sizePlusMax <= 200)
            adjustToSize = 200;

        mScrollArea->setMinimumHeight(adjustToSize);
        mScrollArea->widget()->adjustSize();
        adjustSize();
    }
}


void OriginWindowTemplateScrollable::setContent(QWidget* content)
{
	QWidget* contentWidget = mScrollArea->takeWidget();
	if(contentWidget) contentWidget->deleteLater();

	content->setAttribute(Qt::WA_NoSystemBackground, true);
	mScrollArea->setWidget(content);
	content->setAutoFillBackground(false);
}


QWidget* OriginWindowTemplateScrollable::getContent() const
{
    if(mScrollArea && mScrollArea->widget())
        return mScrollArea->widget();
    return NULL;
}


void OriginWindowTemplateScrollable::setCornerContent(QWidget* content)
{
    if(content && uiWindowContent)
    {
        delete uiWindowContent->cornerContent;
        uiWindowContent->cornerContent = content;
        uiWindowContent->exitLayout->insertWidget(0, uiWindowContent->cornerContent);
        uiWindowContent->cornerContent->show();
        adjustSize();
    }
}


QWidget* OriginWindowTemplateScrollable::getCornerContent() const
{
    return uiWindowContent->cornerContent;
}


void OriginWindowTemplateScrollable::addButtons(QDialogButtonBox::StandardButtons buttons)
{
    if(uiWindowContent && uiWindowContent->buttonBox)
    {
        OriginButtonBox* bb = uiWindowContent->buttonBox;
        if (buttons & QDialogButtonBox::Retry) bb->addButton(QDialogButtonBox::Retry);
        if (buttons & QDialogButtonBox::Close) bb->addButton(QDialogButtonBox::Close);
        if (buttons & QDialogButtonBox::Cancel) bb->addButton(QDialogButtonBox::Cancel);
        if (buttons & QDialogButtonBox::Ok) bb->addButton(QDialogButtonBox::Ok);
        if (buttons & QDialogButtonBox::Yes) bb->addButton(QDialogButtonBox::Yes);
        if (buttons & QDialogButtonBox::No) bb->addButton(QDialogButtonBox::No);

        if(buttons == QDialogButtonBox::NoButton)
        {
            bb->hide();
            uiWindowContent->cornerContent->hide();
            uiWindowContent->exitInfo->setProperty("style", QVariant::Invalid);
            uiWindowContent->exitInfo->setStyle(QApplication::style());
            // Change the exitInfo style to hasButton if a button is added outside of here.
            connect(bb, SIGNAL(buttonAdded()), this, SLOT(onButtonAdded()));
        }
        else
        {
            uiWindowContent->exitInfo->setProperty("style","hasButtons");
            uiWindowContent->exitInfo->setStyle(QApplication::style());
        }

        connect(bb, SIGNAL(clicked(QAbstractButton*)), bb, SLOT(buttonBoxClicked(QAbstractButton*)));  
    }
}


OriginButtonBox* OriginWindowTemplateScrollable::getButtonBox() const
{
    if(uiWindowContent && uiWindowContent->buttonBox)
        return uiWindowContent->buttonBox;
    return NULL;
}


void OriginWindowTemplateScrollable::onButtonAdded()
{
    uiWindowContent->exitInfo->setProperty("style","hasButtons");
    uiWindowContent->exitInfo->setStyle(QApplication::style());
}

}
}