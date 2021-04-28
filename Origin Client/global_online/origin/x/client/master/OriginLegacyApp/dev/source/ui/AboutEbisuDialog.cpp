///////////////////////////////////////////////////////////////////////////////
// AboutEbisuDialog.cpp
// 
// Copyright (c) 2010-2012, Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QString>
#include <QUrl>

#include "AboutEbisuDialog.h"
#include "version/version.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originspinner.h"
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include "Qt/originpushbutton.h"

#include "OriginWebController.h"

#include "engine/login/LoginController.h"
#include "ui_AboutEbisuDialog.h"

namespace
{
    QString expandedReleaseNotesUrl()
    {
        const QString locale(Origin::Services::readSetting(Origin::Services::SETTING_LOCALE));

        QString version = QCoreApplication::applicationVersion();
        version.remove(version.lastIndexOf("."), version.size());

        return Origin::Services::readSetting(Origin::Services::SETTING_ReleaseNotesURL).toString().arg(version).arg(locale);
    }
}

AboutEbisuDialog::AboutEbisuDialog(QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::AboutEbisuDialog())
    , mDialogChrome(NULL)
    , mBoxChrome(NULL)
    , mWebController(NULL)
    , mWebView(NULL)
{
    ui->setupUi(this);
    retranslate();

    ORIGIN_VERIFY_CONNECT(ui->eulaLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));
    ORIGIN_VERIFY_CONNECT(ui->releaseNotesLabel, SIGNAL(clicked()), this, SLOT(showReleaseNotes()));

    // Disable the Show Release Notes link if we are offline
    Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
    if ( user )
    {
        ui->releaseNotesLabel->setEnabled(Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), ui->releaseNotesLabel, SLOT(setEnabled(bool)));
    }
}


AboutEbisuDialog::~AboutEbisuDialog()
{
    closeReleaseNotes();
}


void AboutEbisuDialog::showReleaseNotes()
{
    if(mDialogChrome == NULL)
    {
        using namespace Origin::UIToolkit;
        mBoxChrome = 
            new OriginMsgBox(
            OriginMsgBox::NoIcon, 
            "", "", new OriginSpinner()
            );

        // Show spinner while page is loading
        dynamic_cast<OriginSpinner*>(mBoxChrome->content())->startSpinner();

        mDialogChrome = 
            new OriginWindow(
            (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close), 
            mBoxChrome, OriginWindow::Scrollable, QDialogButtonBox::Close);

        ORIGIN_VERIFY_CONNECT(mDialogChrome, SIGNAL(rejected()), this, SLOT(closeReleaseNotes()));
        ORIGIN_VERIFY_CONNECT(mDialogChrome->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(closeReleaseNotes()));

        mWebView = new QWebView();
        mWebController = new Origin::Client::OriginWebController(mWebView, Origin::Client::DefaultErrorPage::centerAlignedPage());
        mWebController->loadTrustedUrl(expandedReleaseNotesUrl());

        ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(loadFinished(bool)), this, SLOT(onReleaseNotesLoaded()));

        Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
        if ( user )
        {
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), mWebView, SLOT(reload()));
        }
    }

    mDialogChrome->showWindow();
}


void AboutEbisuDialog::closeReleaseNotes()
{
    if (mWebView)
    {
        // Destroying the web view will also destroy the web view controller.
        mWebView->deleteLater();
        mWebView = NULL;
    }

    if(mDialogChrome)
    {
        mDialogChrome->close();
        mDialogChrome = NULL;
        // Is deleted inside of originwindow
        mBoxChrome = NULL;
    }
}



void AboutEbisuDialog::linkActivated(const QString& link)
{
    Origin::Services::PlatformService::asyncOpenUrl(QUrl(link));
}

void AboutEbisuDialog::retranslate()
{
    ui->retranslateUi(this);
    if(mDialogChrome)
    {
        mDialogChrome->setButtonText(QDialogButtonBox::Close, tr("ebisu_client_close"));
    }

    if(mWebController)
    {
        mWebController->loadTrustedUrl(expandedReleaseNotesUrl());
    }

    // Make a nice version string
    QString version = QString(EBISU_VERSION_STR).replace(",", ".");

    // EBISU_CHANGELIST_STR is a string environment var that is defined in the build system
    // for display beside the build version. This is the only place it is used.
    version.append(" - " EBISU_CHANGELIST_STR);

    // the label always contains the necessary %1 elements after the retranslate call above
    ui->versionLabel->setText(ui->versionLabel->text().arg(version));

    ui->copyrightLabel->setText(tr("ebisu_client_copyright").arg(QString::number(QDate().currentDate().year())));

    // Replace the %1 placeholder with our locale
    QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);
    // Need to modify the locale string for the ToS and Privacy Policy URL. They expect US/EN, our locales are en_us. Need to split around _
    QStringList localeString = locale.split("_");

    // Check the location of the EULA we accepted in the boot flow if there is one
	int eulaVersion = Origin::Services::readSetting(Origin::Services::SETTING_ACCEPTEDEULAVERSION);
    QString eulaUrl = Origin::Services::readSetting(Origin::Services::SETTING_EulaURL).toString().arg(locale).arg(eulaVersion);
    QString eulaSetting = Origin::Services::readSetting(Origin::Services::SETTING_ACCEPTEDEULALOCATION);
    if (eulaSetting.compare("") != 0)
    {
        // We had a setting there, use it instead
        eulaUrl = eulaSetting;
    }

    QMap<QString, QString> legalLocaleMap;
    legalLocaleMap["zh_CN"] = "sc";
    legalLocaleMap["zh_TW"] = "tc";
    legalLocaleMap["pt_BR"] = "br";
    legalLocaleMap["es_MX"] = "mx";
    const QString localeArg = legalLocaleMap.contains(locale) ? legalLocaleMap.value(locale) : localeString[0];
    QString privUrl = Origin::Services::readSetting(Origin::Services::SETTING_PrivacyPolicyURL).arg(localeArg);
    QString tosUrl =  Origin::Services::readSetting(Origin::Services::SETTING_ToSURL).arg(localeArg);

    // the label always contains the necessary %1 elements after the retranslate call above
    QString eulaLabel = ui->eulaLabel->text();
    eulaLabel = eulaLabel.arg(eulaUrl, privUrl, tosUrl);
    ui->eulaLabel->setText(eulaLabel);

    QString gplURL = Origin::Services::readSetting(Origin::Services::SETTING_GPLURL);
    QString gnuURL = Origin::Services::readSetting(Origin::Services::SETTING_GNUURL);
    if(localeString[0] == "ja"
        || localeString[0] == "de"
        || localeString[0] == "fr")
        gnuURL = gnuURL.arg(localeString[0].toLower());
    else
        gnuURL = gnuURL.arg("en");

    const QString license = tr("ebisu_client_qt_license").arg(tr("application_name")).arg(gnuURL).arg(gplURL);
    ui->lblQtLicense->setText(license);
    ui->lblArchiveLicenseTitle->setText(tr("ebisu_client_libarchive_title"));
    ui->lblArchiveLicense->setText(tr("ebisu_client_libarchive_license"));
    ui->lblOpenSSLLicenseTitle->setText(tr("ebisu_client_openssl_title"));
    ui->lblOpenSSLLicense->setText(tr("ebisu_client_openssl_license")); 

}

void AboutEbisuDialog::changeEvent( QEvent* event )
{
    if (event) 
    {
        switch (event->type()) 
        {
        case QEvent::LanguageChange:
            retranslate();
            break;
            
        default:
            break;
        }
    }
    QWidget::changeEvent(event);
}


void AboutEbisuDialog::onReleaseNotesLoaded()
{
    if(mBoxChrome && mWebController)
    {
        Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
        if ( user )
            mBoxChrome->setTitle(Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()) ? tr("ebisu_client_release_notes_uppercase") : "");

        // We are done loading, stop and hide the spinner
        using namespace Origin::UIToolkit;
        OriginSpinner* contentSpinner = dynamic_cast<OriginSpinner*>(mBoxChrome->content());
        if(contentSpinner)
        {
            contentSpinner->stopSpinner();
            contentSpinner->hide();
        }
        mBoxChrome->setText(mWebController->page()->mainFrame()->toHtml());
    }
}
