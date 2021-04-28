///////////////////////////////////////////////////////////////////////////////
// ReportErrorView.cpp
//
// Copyright (c) 2011-2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ui_ReportErrorView.h"
#include "ReportErrorView.h"

#include "Services/debug/DebugService.h"

#include <QWidget>
#include <QDesktopServices>
#include <QUrl>

#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "ProgressView.h"
#include "OriginCommonUIUtils.h"
#include "version/version.h"

int const MAX_NOTES_LENGTH = 2000;
int const MAX_ID_LENGTH = 100;
int const MAX_CE_NUMBER_LENGTH = 20;

const QString urlDiagnostic = "http://download.dm.origin.com/oer/%1.html";
const QString urlPrivacy = "http://tos.ea.com/legalapp/WEBPRIVACY/%0/%1/PC/";

ReportErrorView::ReportErrorView( QString const& locale, QString const& originId ) 
	: QWidget(NULL)
    , ui(new Ui::ReportErrorView)
	, m_locale(locale)
    , m_cmdline_originId(originId)
    , m_progressWindow(NULL)
    , mSubmitButton(NULL)
{
	ui->setupUi(this);	 
	ORIGIN_VERIFY_CONNECT( ui->editDescription->getMultiLineWidget(), SIGNAL(textChanged()), this,       SLOT(updateCharacterCount()));
	ORIGIN_VERIFY_CONNECT( ui->cbIssueType, SIGNAL(currentIndexChanged(QString)),  this, SLOT(onCurrentIssueChanged(QString)));
	ORIGIN_VERIFY_CONNECT( ui->cbConfirmSubmit, SIGNAL(stateChanged(int)), this,   SLOT(onConfirmStateChanged(int)));

    ORIGIN_VERIFY_CONNECT( ui->leUsername, SIGNAL(textEdited(const QString&)), this,        SLOT(validateSubmitReport()));  
	ORIGIN_VERIFY_CONNECT( ui->editDescription->getMultiLineWidget(), SIGNAL(textChanged()), this,       SLOT(validateSubmitReport()));
	ORIGIN_VERIFY_CONNECT( ui->cbIssueType, SIGNAL(stateChanged(int)), this,       SLOT(validateSubmitReport()));
	ORIGIN_VERIFY_CONNECT( ui->cbConfirmSubmit, SIGNAL(stateChanged(int)), this,   SLOT(validateSubmitReport()));

    ORIGIN_VERIFY_CONNECT( ui->btnConfirmSubmit, SIGNAL(linkActivated(const QString&)), this, SLOT(onClickPrivacyQuestion(const QString&)));
    ORIGIN_VERIFY_CONNECT( ui->disclaimerLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(onHelpClicked(const QString&)));

	ui->editDescription->installEventFilter(this);

	ui->cbIssueType->setEditable(false);

    ui->leUsername->installEventFilter(this);
    ui->lblCheckbox->installEventFilter(this);

    ui->ceTooltip->setToolTip(tr("diag_tool_form_ce_help"));

    if(!m_cmdline_originId.isEmpty()) {
        ui->leUsername->setText(m_cmdline_originId);
        ui->leUsername->setDisabled(true);
        ui->cbIssueType->setFocus();
    }
    else {
        ui->leUsername->setFocus();
    }

    updateCharacterCount();

}

ReportErrorView::~ReportErrorView()
{
    onHideProgress();
    if (ui)
    {
        delete ui;
        ui = NULL;
    }
}

bool ReportErrorView::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);
    Q_UNUSED(event);

    // Detect if the "I agree..." label is clicked. If so, toggle the checkbox.
    // A separate label is required for wordwrap.
    if(object == ui->lblCheckbox)
    {
        if(event->type() == QEvent::MouseButtonPress)
            ui->cbConfirmSubmit->toggle();
    }

	return false;
}

void ReportErrorView::updateCharacterCount()
{
	// TODO add default text to box when empty, need to handle focus events
	QString note = ui->editDescription->getPlainText();
	int charsLeft = MAX_NOTES_LENGTH - note.size();

	if ( charsLeft >= 0 )
		ui->lblCharCount->setStyleSheet("QLabel { color:#666666; }");
	else
		ui->lblCharCount->setStyleSheet("QLabel { color:#ff0000; }");

	// TODO have two different strings for above and below char count limit
	ui->lblCharCount->setText(QString(tr("diag_tool_form_description_characters_remaining")).arg(charsLeft));
}

void ReportErrorView::onCurrentIssueChanged(QString const& )
{
    validateSubmitReport();
}

void ReportErrorView::onConfirmStateChanged(int )
{
}

bool ReportErrorView::isIdFieldBlank()
{
    return ui->leUsername->text().length() == 0;
}

bool ReportErrorView::isNotesFieldBlank()
{
	return ui->editDescription->getPlainText().length() == 0;
}

bool ReportErrorView::isIssueFieldBlank()
{
	return ui->cbIssueType->currentIndex() == 0;
}

void ReportErrorView::validateSubmitReport()
{
    if(mSubmitButton)
    {
	    mSubmitButton->setEnabled(
		    ui->editDescription->getPlainText().size() <= MAX_NOTES_LENGTH &&
		    !isIssueFieldBlank() && 
		    !isNotesFieldBlank() && 
		    ui->cbConfirmSubmit->isChecked() &&
            !isIdFieldBlank());
    }
}

void ReportErrorView::setProblemAreas(QStringList const& problemList, QList<QPair<QString,int> > categoryOrder)
{
    mProblemList = problemList;
    mCategoryOrder = categoryOrder;
	ui->cbIssueType->clear();
	ui->cbIssueType->addItems(problemList);
}

void ReportErrorView::setCurrentCategory(const int& category)
{
    ui->cbIssueType->setCurrentIndex(category);
}

void ReportErrorView::setDescription(const QString& description)
{
    ui->editDescription->setHtml(description);
}

QString ReportErrorView::getUserId()
{
    return ui->leUsername->text().left(MAX_ID_LENGTH).toHtmlEscaped();
}

QString ReportErrorView::getMessage()
{
	return ui->editDescription->getPlainText().left(MAX_NOTES_LENGTH).toHtmlEscaped();
}

QString ReportErrorView::getProblemArea()
{
	// We send numbers instead of raw string in the payload, due to security risks. Mapping back to string is done in web backend.
    // Iterate over the list of QPairs to find what text was selected and send it's integer value to the server
    if(mProblemList.contains(ui->cbIssueType->currentText()))
    {
        QList<QPair<QString,int> >::iterator i;
        for(i = mCategoryOrder.begin(); i != mCategoryOrder.end(); ++i)
        {
            QPair<QString,int> categoryValue = *i;
            if(categoryValue.first.contains(ui->cbIssueType->currentText()))
                return QString::number(categoryValue.second);
        }
        return QString::number(-1);
    }
    else
        return QString::number(-1);
}

QString ReportErrorView::getCeNumber()
{
    if(ui->leTicketNumber->text().isEmpty())
        return QString("0");
    return ui->leTicketNumber->text().left(MAX_CE_NUMBER_LENGTH).toHtmlEscaped();
}

void ReportErrorView::onSubmitClicked()
{
    // Prepare for UI updates
    DataCollector *dataInstance = DataCollector::instance();
    qRegisterMetaType<DataCollectedType>("DataCollectedType");

    // disable everything. Process events immediately or else the events sit on the queue and don't get processed till a 
    // break in submitReport
    setSending(true);
    QCoreApplication::processEvents();

    QPoint windowPos = mapToGlobal(pos());
    windowPos.setX(windowPos.x() + 10);
    windowPos.setY(windowPos.y() + 100);

    using namespace Origin::UIToolkit;
    ProgressViewWidget* progressView = new ProgressViewWidget();
    ORIGIN_VERIFY_CONNECT( dataInstance, SIGNAL(backendProgress(DataCollectedType, DataCollectedType)), progressView, SLOT(onBackendProgress(DataCollectedType, DataCollectedType)));
    m_progressWindow = new OriginWindow(OriginWindow::Icon, progressView, OriginWindow::MsgBox, QDialogButtonBox::Cancel);
    m_progressWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("diag_tool_progress_diag_title"), tr("diag_tool_progress_diag_desc"));
    if (m_progressWindow->manager())
        m_progressWindow->manager()->setupButtonFocus();
    m_progressWindow->setTitleBarText(tr("diag_tool_form_title"));
    m_progressWindow->setSystemMenuEnabled(false);
    ORIGIN_VERIFY_CONNECT(m_progressWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
        this, SLOT(onProgressWindowCanceled()));
    // Call for UI updates now, because Diagnostic processing occurs on a different thread
    updateProgressWindow(START_OR_END,CLIENT_LOG);
    emit submitReport();
    m_progressWindow->setModal(true);
    m_progressWindow->showWindow(windowPos);

}

void ReportErrorView::onProgressWindowCanceled()
{
    if (m_progressWindow)
    {
        m_progressWindow->button(QDialogButtonBox::Cancel)->setText(tr("diag_tool_canceling"));
        m_progressWindow->button(QDialogButtonBox::Cancel)->setEnabled(false);
        ProgressViewWidget* pv = dynamic_cast<ProgressViewWidget *>(m_progressWindow->content());
        if (pv) {
            pv->setProgressCancel();
        }
        emit cancelSubmit();
    }
}

void ReportErrorView::setProgressCancelButton(bool status)
{
	if(m_progressWindow)
		m_progressWindow->button(QDialogButtonBox::Cancel)->setEnabled(status);
}

void ReportErrorView::updateProgressWindow(DataCollectedType completedType, DataCollectedType startingType)
{
	if(m_progressWindow)
	{
		ProgressViewWidget* pv = dynamic_cast<ProgressViewWidget *>(m_progressWindow->content());
		if (pv) {
			pv->onBackendProgress(completedType,startingType);
		}
	}
}

void ReportErrorView::setSubmitButton(Origin::UIToolkit::OriginPushButton *submitButton)
{
    mSubmitButton = submitButton;
}

void ReportErrorView::setSending(bool sending)
{
    if (sending) {
		mSubmitButton->setText(tr("diag_tool_form_button_submitting"));
    }
	else {
        mSubmitButton->setText(tr("diag_tool_form_button_submit"));
    }
    ui->lblOriginID->setDisabled(sending);
    // never enabled when there was an origin id passed in
    if (!m_cmdline_originId.isEmpty())
        ui->leUsername->setDisabled(true);
    else 
    	ui->leUsername->setDisabled(sending);
    ui->lblErrorType->setDisabled(sending);
    ui->lblCETicketNumber->setDisabled(sending);
	ui->cbIssueType->setDisabled(sending);
    ui->lblDescription->setDisabled(sending);
	ui->editDescription->setDisabled(sending);
    ui->lblCharCount->setDisabled(sending);
	ui->cbConfirmSubmit->setDisabled(sending);
	ui->btnConfirmSubmit->setDisabled(sending);
    if(mSubmitButton)
    {
	    mSubmitButton->setDisabled(sending);
    }
    ui->disclaimerLabel->setDisabled(sending);
    ui->leTicketNumber->setDisabled(sending);

    if (!sending && m_progressWindow) 
       onHideProgress();
        
}

void ReportErrorView::onClickPrivacyQuestion(const QString& whichPrivacy)
{
    QString url;
    if (whichPrivacy == QString("diagnostic")) {
        url = urlDiagnostic.arg(m_locale);
    }
    else {
        QStringList localeString = m_locale.split("_");
        QMap<QString, QString> legalLocaleMap;
        legalLocaleMap["zh_CN"] = "sc";
        legalLocaleMap["zh_TW"] = "tc";
        legalLocaleMap["pt_BR"] = "br";
        legalLocaleMap["es_MX"] = "mx";
        const QString localeArg = legalLocaleMap.contains(m_locale) ? legalLocaleMap.value(m_locale) : localeString[0];
        url = urlPrivacy.arg(localeString[1]).arg(localeArg);
    }
    QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

void ReportErrorView::onHelpClicked(const QString& url)
{
        QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

void ReportErrorView::onHideProgress()
{
    if (m_progressWindow) {
        m_progressWindow->close();
        m_progressWindow = NULL;
    }
}

