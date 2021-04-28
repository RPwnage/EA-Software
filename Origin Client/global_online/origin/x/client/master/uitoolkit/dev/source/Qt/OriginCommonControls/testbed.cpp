#include "testbed.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "originbanner.h"
#include "originwebview.h"
#include <QtWebKitWidgets/QWebView>


const QString txtScroll = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi. Morbi rhoncus lacinia libero non venenatis. Mauris nec urna vitae nisl aliquet porttitor. Maecenas eget lorem sapien.";

TestBedDialog::TestBedDialog(QWidget* w): QWidget(w)
, mWindow(NULL)
{
    ui.setupUi(this);
    connect(ui.originCloseButton, SIGNAL(clicked(bool)), this, SIGNAL(closeDialog()));
    setupValidationContainers();
    ui.originPushButton_2->setDefault(true);
    connect(ui.originSingleLineEdit_search, SIGNAL(accessoryClicked()), this, SLOT(searchBoxClicked()));
    ui.originTransferBar->getProgressBar()->setValue(75);
}

void TestBedDialog::setupValidationContainers()
{

    connect(ui.rb1, SIGNAL(clicked(bool)), this, SLOT(normalClicked()));
    connect(ui.rb2, SIGNAL(clicked(bool)), this, SLOT(validClicked()));    
    connect(ui.rb3, SIGNAL(clicked(bool)), this, SLOT(validatingClicked()));    
    connect(ui.rb4, SIGNAL(clicked(bool)), this, SLOT(invalidClicked()));    
    ui.radioButtonValidation->setContainerType(Origin::UIToolkit::OriginValidationContainer::ContainerTypeRadioButtonGroup);
    ui.EditValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Normal);
    ui.radioButtonValidation->adjustSize();
    ui.EditValidation->adjustSize();


    // Windows
    connect(ui.btnBannerError, SIGNAL(clicked()), this, SLOT(onShowHideErrorBanner()));
    connect(ui.btnBannerMsg, SIGNAL(clicked()), this, SLOT(onShowHideMsgBanner())); 
    connect(ui.lblWindowMsgBox, SIGNAL(clicked()), this, SLOT(windowMsgBoxClicked()));  
    connect(ui.lblWindowScroll, SIGNAL(clicked()), this, SLOT(windowScrollClicked()));  
    connect(ui.lblWindowScrollMsgBox, SIGNAL(clicked()), this, SLOT(windowScrollMsgBoxClicked()));  
    connect(ui.lblWindowWebView, SIGNAL(clicked()), this, SLOT(windowWebViewClicked()));  
}

void TestBedDialog::normalClicked()
{
    ui.radioButtonValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Normal);
    ui.EditValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Normal);
    ui.EditValidationEdit->setText("This is normal");
    ui.EditValidationEdit->setValidationStatus(Origin::UIToolkit::OriginSingleLineEdit::Normal);
}
void TestBedDialog::validClicked()
{
    ui.radioButtonValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Valid);
    ui.EditValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Valid);
    ui.EditValidationEdit->setText("This is valid");
    ui.EditValidationEdit->setValidationStatus(Origin::UIToolkit::OriginSingleLineEdit::Valid);
}

void TestBedDialog::validatingClicked()
{
    ui.radioButtonValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Validating);
    ui.EditValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Validating);
    ui.EditValidationEdit->setText("This is validating");
    ui.EditValidationEdit->setValidationStatus(Origin::UIToolkit::OriginSingleLineEdit::Validating);
}

void TestBedDialog::invalidClicked()
{
    ui.radioButtonValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);
    ui.EditValidation->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);
    ui.EditValidationEdit->setText("This is invalid");
    ui.EditValidationEdit->setValidationStatus(Origin::UIToolkit::OriginSingleLineEdit::Invalid);
}

void TestBedDialog::windowMsgBoxClicked()
{
    using namespace Origin::UIToolkit;
    OriginMsgBox::IconType icon = OriginMsgBox::NoIcon;
    if(ui.rbNoIcon->isChecked())
        icon = OriginMsgBox::NoIcon;
    else if(ui.rbSuccess->isChecked())
        icon = OriginMsgBox::Success;
    else if(ui.rbError->isChecked())
        icon = OriginMsgBox::Error;
    else if(ui.rbInfo->isChecked())
        icon = OriginMsgBox::Info;
    else if(ui.rbNotice->isChecked())
        icon = OriginMsgBox::Notice;

    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, 
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Close);
    mWindow->setTitleBarText("OriginWindow - MsgBox");
    mWindow->msgBox()->setup(icon, "Message Box Title", 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vitae nibh mi. Fusce facilisis lorem diam, et gravida mi.",
        new OriginPushButton());

    OriginLabel* cornerContent = new OriginLabel();
    cornerContent->setLabelType(OriginLabel::DialogSmall);
    cornerContent->setText("XBox Live(R) Gamertag: Silkychicken\nPlaystation(R) Network Online ID: None");
    mWindow->setCornerContent(cornerContent);

    mWindow->setButtonText(QDialogButtonBox::Ok, "Submit");
    mWindow->setButtonText(QDialogButtonBox::Close, "Cancel");

    connect(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    connect(mWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), mWindow, SLOT(close()));
    connect(mWindow->button(QDialogButtonBox::Close), SIGNAL(clicked()), mWindow, SLOT(close()));
    mWindow->show();
}

void TestBedDialog::windowScrollClicked()
{
    using namespace Origin::UIToolkit;
    OriginLabel* content = new OriginLabel();
    content->setText(txtScroll);
    content->setWordWrap(true);
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        content, OriginWindow::Scrollable);
    mWindow->setTitleBarText("OriginWindow - Scrollable");
    connect(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    mWindow->show();
}

void TestBedDialog::windowScrollMsgBoxClicked()
{
    using namespace Origin::UIToolkit;
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, 
        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Close);
    mWindow->setTitleBarText("OriginWindow - ScrollableMsgBox");
    if(mWindow->scrollableMsgBox())
    {
        mWindow->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, "Message Box Title", txtScroll);
        mWindow->scrollableMsgBox()->setContent(new OriginPushButton());
    }

    OriginLabel* cornerContent = new OriginLabel();
    cornerContent->setLabelType(OriginLabel::DialogSmall);
    cornerContent->setText("XBox Live(R) Gamertag: Silkychicken\nPlaystation(R) Network Online ID: None");
    mWindow->setCornerContent(cornerContent);

    mWindow->setButtonText(QDialogButtonBox::Ok, "Submit");
    mWindow->setButtonText(QDialogButtonBox::Close, "Cancel");

    connect(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    connect(mWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), mWindow, SLOT(close()));
    connect(mWindow->button(QDialogButtonBox::Close), SIGNAL(clicked()), mWindow, SLOT(close()));

    mWindow->show();
}


void TestBedDialog::windowWebViewClicked()
{
    using namespace Origin::UIToolkit;
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, 
        NULL, OriginWindow::WebContent);
    mWindow->setTitleBarText("OriginWindow - WebContent + Server Widget Test Harness");
    mWindow->webview()->setHasSpinner(true);
    connect(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    connect(mWindow->webview()->webview(), SIGNAL(rejected()), mWindow, SLOT(close()));


    mWindow->webview()->webview()->load(QUrl("http://10.14.33.115:8080/demo.html"));
    mWindow->show();
}


void TestBedDialog::searchBoxClicked()
{
    using namespace Origin::UIToolkit;
    OriginLabel* label = new OriginLabel();
    label->setText("You clicked on search!");
    label->setAlignment(Qt::AlignCenter);

    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, 
        label, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Close);
    mWindow->setTitleBarText("Search Dialog");
    mWindow->setButtonText(QDialogButtonBox::Close, "Close");
    connect(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    connect(mWindow->button(QDialogButtonBox::Close), SIGNAL(clicked()), mWindow, SLOT(close()));

    mWindow->show();

}

void TestBedDialog::onShowHideErrorBanner()
{
    if(mWindow)
    {
        using namespace Origin::UIToolkit;
        OriginBanner::IconType icon = OriginBanner::Error;
        if(ui.rbBannerError->isChecked())
            icon = OriginBanner::Error;
        else if(ui.rbBannerSuccess->isChecked())
            icon = OriginBanner::Success;
        else if(ui.rbBannerInfo->isChecked())
            icon = OriginBanner::Info;
        else if(ui.rbBannerNotice->isChecked())
            icon = OriginBanner::Notice;
			
        mWindow->setBannerIconType(icon);
        mWindow->setBannerText("You've successfully activated the following product(s) for the account: jhitz@ea.com");
        mWindow->setBannerVisible(!mWindow->bannerVisible());
    }
}

void TestBedDialog::onShowHideMsgBanner()
{
    if(mWindow)
    {
        using namespace Origin::UIToolkit;
        OriginBanner::IconType bannerIcon = OriginBanner::Success;
        if(ui.rbBannerSuccess->isChecked())
            bannerIcon = OriginBanner::Success;
        else if(ui.rbBannerError->isChecked())
            bannerIcon = OriginBanner::Error;
        else if(ui.rbBannerInfo->isChecked())
            bannerIcon = OriginBanner::Info;
        else if(ui.rbBannerNotice->isChecked())
            bannerIcon = OriginBanner::Notice;

        mWindow->setMsgBannerIconType(bannerIcon);
        mWindow->setMsgBannerText("Password successfully changed");
        mWindow->setMsgBannerVisible(!mWindow->msgBannerVisible());
    }
}
