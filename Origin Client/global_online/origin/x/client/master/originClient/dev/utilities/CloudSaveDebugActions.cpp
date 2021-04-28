#include <services/debug/DebugService.h>
#include <engine/content/ContentConfiguration.h>
#include <engine/content/LocalContent.h>
#include <engine/content/CloudContent.h>
#include <engine/content/Entitlement.h>
#include <QAction>
#include "originpushbutton.h"

#include "CloudSaveDebugActions.h"
#include "ShowEligibleFilesPopup.h"
#include "engine/cloudsaves/ClearRemoteStateManager.h"
#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/cloudsaves/PathSubstituter.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"

#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"

using namespace Origin::Engine::CloudSaves;

namespace Origin
{
namespace Client
{

CloudSaveDebugActions::CloudSaveDebugActions(Engine::Content::EntitlementRef entitlement, QObject *parent) :
    QObject(parent),
    mEntitlement(entitlement)
{
    QAction *actionQueryUsage = new QAction("Query Cloud Usage", this);
    ORIGIN_VERIFY_CONNECT(actionQueryUsage, SIGNAL(triggered()), this, SLOT( queryUsage()));
    mActions.append(actionQueryUsage);

    QAction *actionClearRemoteArea = new QAction("Clear Remote Cloud Area", this);
    ORIGIN_VERIFY_CONNECT(actionClearRemoteArea, SIGNAL(triggered()), this, SLOT(clearRemoteArea()));
    mActions.append(actionClearRemoteArea);

    QAction *actionShowEligibleSaveFiles = new QAction("Show Eligible Files", this);
    ORIGIN_VERIFY_CONNECT(actionShowEligibleSaveFiles, SIGNAL(triggered()), this, SLOT(showEligibleSaveFiles()));
    mActions.append(actionShowEligibleSaveFiles);
}

QList<QAction*> CloudSaveDebugActions::actions() const
{
    return mActions;
}

void CloudSaveDebugActions::queryUsage()
{
    using namespace UIToolkit;

    OriginWindow* windowChrome = new OriginWindow(
        (OriginWindow::TitlebarItems)(OriginWindow::Close),
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Close);

    const qint64 usage = RemoteUsageMonitor::instance()->usageForEntitlement(mEntitlement);
    QString usageString;

    if (usage == RemoteUsageMonitor::UsageUnknown)
    {
        usageString = "Unknown";
    }
    else
    {
        usageString = QString::number(usage);
    }

    windowChrome->msgBox()->setup(OriginMsgBox::NoIcon, 
                "Usage",
		        QString("Bytes used: ") + usageString);
            
    windowChrome->setDefaultButton(QDialogButtonBox::Close);
    windowChrome->manager()->setupButtonFocus();

    windowChrome->setAttribute(Qt::WA_DeleteOnClose, true);
    ORIGIN_VERIFY_CONNECT(windowChrome, SIGNAL(rejected()), windowChrome, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(windowChrome->button(QDialogButtonBox::Close), SIGNAL(clicked()), windowChrome, SLOT(close()));

    windowChrome->exec();
}

void CloudSaveDebugActions::clearRemoteArea()
{
    using namespace UIToolkit;

    ClearRemoteStateManager::instance()->clearRemoteStateForEntitlement(mEntitlement);

    OriginWindow::alert(OriginMsgBox::NoIcon, "CLOUD SAVE DEBUG", "Remote save area has been cleared.", tr("ebisu_client_notranslate_close"));
}
	
void CloudSaveDebugActions::showEligibleSaveFiles()
{
	ShowEligibleFilesPopup * mEligibleFilesPopup = new ShowEligibleFilesPopup("Eligible Files");
	mEligibleFilesPopup->addFileList(eligibleSaveFiles());
	mEligibleFilesPopup->showDialog();
}

QStringList CloudSaveDebugActions::eligibleSaveFiles() const
{
    // We need these to help crawl save files
	PathSubstituter substituter(mEntitlement);

	const QList<SaveFileCrawler::EligibleFileRules> rules =
        mEntitlement->localContent()->cloudContent()->getCloudSaveFileCriteria();

	QList<EligibleFile> eligibleFiles = SaveFileCrawler::findEligibleFiles(mEntitlement, substituter);
	QStringList paths;

	for(QList<SaveFileCrawler::EligibleFileRules>::const_iterator it = rules.constBegin();
		it != rules.constEnd();
		it++)
	{
		QString templatized = (*it).first;
		QString ruleType = (*it).second == SaveFileCrawler::IncludeFile ? "Include: " : "Exclude: ";
		paths.append(ruleType + templatized);
	}

	paths.append("-----------------------------------------------------------------------");


	for(QList<EligibleFile>::const_iterator it = eligibleFiles.constBegin();
		it != eligibleFiles.constEnd();
		it++) 
	{
		const QFileInfo &info = (*it).info();
		const QString path(info.absoluteFilePath());
		const QString trusted = (*it).isTrusted() ? "Trusted" : "Untrusted";

		paths.append(path + " (" + trusted + ")");
	}

    return paths;
}

}
}
