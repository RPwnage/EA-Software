#include "originwindowtemplatemsgbox.h"
#include "ui_originwindowcontent.h"
#include "originmsgbox.h"

namespace Origin {
namespace UIToolkit {
OriginWindowTemplateMsgBox::OriginWindowTemplateMsgBox(QWidget* parent)
: OriginWindowTemplateBase(parent)
, mMsgBox(NULL)
, uiWindowContent(NULL)
{
    uiWindowContent = new Ui::OriginWindowContent();
    uiWindowContent->setupUi(this);

    mMsgBox = new OriginMsgBox();
    uiWindowContent->layout->insertWidget(0, mMsgBox);
}


OriginWindowTemplateMsgBox::~OriginWindowTemplateMsgBox()
{
    if(mMsgBox)
    {
        mMsgBox->deleteLater();
        mMsgBox = NULL;
    }

    if(uiWindowContent)
    {
        delete uiWindowContent;
        uiWindowContent = NULL;
    }
}


void OriginWindowTemplateMsgBox::setContent(QWidget* content)
{
	mMsgBox->setContent(content);
	//if(mMsgBox->height() < 200)
	//	mMsgBox->resize(mMsgBox->width(), 200);
 //   QApplication::processEvents();
 //   adjustSize();
}


QWidget* OriginWindowTemplateMsgBox::getContent() const
{
    if(mMsgBox && mMsgBox->content())
        return mMsgBox->content();
    return NULL;
}


void OriginWindowTemplateMsgBox::setCornerContent(QWidget* content)
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


QWidget* OriginWindowTemplateMsgBox::getCornerContent() const
{
    return uiWindowContent->cornerContent;
}


void OriginWindowTemplateMsgBox::addButtons(QDialogButtonBox::StandardButtons buttons)
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


OriginButtonBox* OriginWindowTemplateMsgBox::getButtonBox() const
{
    if(uiWindowContent && uiWindowContent->buttonBox)
        return uiWindowContent->buttonBox;
    return NULL;
}


void OriginWindowTemplateMsgBox::onButtonAdded()
{
    uiWindowContent->exitInfo->setProperty("style","hasButtons");
    uiWindowContent->exitInfo->setStyle(QApplication::style());
}

}
}