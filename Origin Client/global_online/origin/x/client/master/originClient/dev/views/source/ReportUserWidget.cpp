// *********************************************************************
//  ReportUserWidget.cpp
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include "ReportUserWidget.h"
#include "ui_ReportUserWidget.h"
#include "services/debug/DebugService.h"

#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"

ReportUserWidget::ReportUserWidget(
    bool fromSDK, const QString& username,
    QWidget* parent /* = NULL */) 
    : QWidget(parent)
    , ui(new Ui::ReportUserWidget)
{
    ui->setupUi(this);
    // fill the values of the reason
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_choose_one"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_cheating"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_child"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_harassment"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_offensive"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_spam"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_suicide"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_terrorist"));
    ui->reportReasonDropdown->addItem(tr("ebisu_client_report_reason_hack"));
    ui->reportReasonDropdown->setCurrentIndex(fromSDK ? 1 : 0);
    ORIGIN_VERIFY_CONNECT(ui->reportReasonDropdown, SIGNAL(currentIndexChanged(int)),
                            this, SLOT(dropdownChanged(int)));

    ui->reportWhereDropdown->addItem(tr("ebisu_client_report_choose_one"));
    ui->reportWhereDropdown->addItem(tr("ebisu_client_profilewidget_report_content_username"));
    ui->reportWhereDropdown->addItem(tr("ebisu_client_profilewidget_report_content_avatar"));
    ui->reportWhereDropdown->addItem(tr("ebisu_client_profilewidget_report_content_non_origin_game"));
    ui->reportWhereDropdown->addItem(tr("ebisu_client_chat_room_name"));
    ui->reportWhereDropdown->addItem(tr("ebisu_client_in_game_abuse"));
    ui->reportWhereDropdown->setCurrentIndex(fromSDK ? 5 : 0);
    ORIGIN_VERIFY_CONNECT(ui->reportWhereDropdown, SIGNAL(currentIndexChanged(int)),
                        this, SLOT(dropdownChanged(int)));

    ui->reportReasonLabel->setText(tr("ebisu_client_report_friend_body").arg(username));
}

ReportUserWidget::~ReportUserWidget()
{
}

QString ReportUserWidget::reportReason()
{
    QString resultStr;

     int index = ui->reportReasonDropdown->currentIndex();
    // we might want to revisit this but this is what exists now so I am keeping it
    switch (index) {
        case 1:
            resultStr = QString(REPORT_CHEATING_STR);
            break;
        case 2:
            resultStr = QString(REPORT_CHILD_SOLICITATION_STR);
            break;
        case 3:
            resultStr = QString(REPORT_HARASSMENT_STR);
            break;
        case 4: 
            resultStr = QString(REPORT_OFFENSIVE_CONTENT_STR);
            break;
        case 5:
            resultStr = QString(REPORT_SPAM_STR);
            break;
        case 6:
            resultStr = QString(REPORT_SUICIDE_THREAT_STR);
            break;
        case 7:
            resultStr = QString(REPORT_TERRORIST_THREAT_STR);
            break;
        case 8:
            resultStr = QString(REPORT_CLIENT_HACK_STR);
            break;
        default:
            break;
    };

    return resultStr;
}

QString ReportUserWidget::reportWhere() 
{
    QString resultStr;
    int index = ui->reportWhereDropdown->currentIndex();

    switch (index)
    {
        case 1:
           resultStr = QString(REPORT_NAME_STR);
           break;
        case 2:
            resultStr = QString(REPORT_AVATAR_STR);
            break;
        case 3:
            resultStr = QString(REPORT_CUSTOM_GAME_NAME_STR);
            break;
        case 4:
            resultStr = QString(REPORT_CHAT_ROOM_NAME_STR);
            break;
        case 5:
            resultStr = QString(REPORT_IN_GAME_STR);
            break;
        default:
            break;
    };

    return resultStr;
}

QString ReportUserWidget::reportComments()
{
    return ui->reportInfoText->toPlainText();
}

void ReportUserWidget::dropdownChanged(int)
{
    // both dropdowns have to have a valid selection in order to do the submit
    if (ui->reportWhereDropdown->currentIndex() == 0 ||
        ui->reportReasonDropdown->currentIndex() == 0)
        emit reasonAndWhereChosen(false);
    else 
        emit reasonAndWhereChosen(true);
}

void ReportUserWidget::reportSubmitted()
{
    // the report was submitted, so hide the contents
    ui->reportReasonLabel->hide();
    ui->reportReasonDropdown->hide();
    ui->reportWhereDropdown->hide();
    ui->reportWhereLabel->hide();
    ui->reportInfoText->hide();
    ui->reportInfoLabel->hide();
}

ReportUserDialog::ReportUserDialog(QString userName,  quint64 id, const QString& masterTitle)
    : QObject()
    , mId(id)
    , mMasterTitle(QString(masterTitle))
    , mWindow(NULL)
    , mReportUserWidget(NULL)
{
    initWindow(userName);
}

void ReportUserDialog::initWindow(QString userName)
{

    using namespace Origin::UIToolkit;
    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close | OriginWindow::Minimize;
    OriginWindow::WindowContext windowContext = OriginWindow::Normal;
    mReportUserWidget = new ReportUserWidget(!mMasterTitle.isEmpty(), userName);
    mWindow = new OriginWindow(titlebarItems,
        mReportUserWidget, OriginWindow::MsgBox, QDialogButtonBox::Ok |QDialogButtonBox::Cancel, 
        OriginWindow::ChromeStyle_Dialog, windowContext);
    mWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_report_friend_title"),
        tr("ebisu_client_report_confidential"));

    if (mWindow->manager())
        mWindow->manager()->setupButtonFocus();

    mWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_report_submit"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(checkClose(int)));
    ORIGIN_VERIFY_CONNECT(mWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onReportRequested()));
    ORIGIN_VERIFY_CONNECT(mWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), mWindow, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(mReportUserWidget, SIGNAL(reasonAndWhereChosen(bool)), this, SLOT(updateButtons(bool)));
    if (mMasterTitle.isEmpty())
    {
    mWindow->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(destroyed()), this, SIGNAL(reportUserDialogDelete()))
}

void ReportUserDialog::checkClose(int status)
{
    // only close the dialog if it is a rejection
    if (status == QDialog::Rejected) {
        mWindow->close();
    }
}

void ReportUserDialog::updateButtons(bool enableSubmit)
{
    // whether or not submit should be enabled
    mWindow->button(QDialogButtonBox::Ok)->setDisabled(!enableSubmit);
}

void ReportUserDialog::onReportRequested()
{
    QString comments = mReportUserWidget->reportComments().toHtmlEscaped().left(1024);
    emit submitReportUser(mId, mReportUserWidget->reportReason(), mReportUserWidget->reportWhere(), mMasterTitle, comments);
}

void ReportUserDialog::onReportSubmitted()
{
    // say thank you for submitting and only allow closing
    mReportUserWidget->reportSubmitted();
    mWindow->msgBox()->setText(tr("ebisu_client_report_success"));
    mWindow->button(QDialogButtonBox::Ok)->hide();
    mWindow->button(QDialogButtonBox::Cancel)->setText(tr("ebisu_client_close"));
}
