///////////////////////////////////////////////////////////////////////////////
// InstallDirectoryManager.cpp
// 
// Copyright (c) 2011-2012, Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "InstallDirectoryManager.h"
#include "services/escalation/IEscalationClient.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"
#include "services/debug/DebugService.h"
#include "engine/igo/IGOController.h"

#include <EAIO/EAFileUtil.h>
#include <EAStdC/EAString.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QApplication>

#include "services/qt/QtUtil.h"
#include "services/log/LogService.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "services/settings/SettingsManager.h"
#include "flows/ClientFlow.h"
#include "EbisuSystemTray.h"
#include "DialogController.h"

#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"

using namespace Origin::Engine::Content;
using namespace Origin::Services;

namespace Origin
{
namespace Client
{
namespace JsInterface
{
InstallDirectoryManager::InstallDirectoryManager(QObject* parent)
: QObject(parent)
, mDipChanged(false)
, mCacheChanged(false)
, mWaitingLongOperation(false)
, mIsDefaultCache(true)
{
    mDownloadInPlaceLocation = fixFolderPath(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionService::currentSession()));
    mCacheDirectoryLocation = fixFolderPath(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR, Origin::Services::Session::SessionService::currentSession()));
    init();
}

InstallDirectoryManager::~InstallDirectoryManager()
{

}


void InstallDirectoryManager::stopWaitingLongOperation()
{
    mWaitingLongOperation = false;
}


void InstallDirectoryManager::init()
{
    ORIGIN_VERIFY_CONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingError(QString const&)), this, SLOT(settingsFailed(const QString&)));

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController()->contentFolders(), SIGNAL(folderValidationError(int)), this, SLOT(showSettingsErrorMessage(int)) );

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController(), SIGNAL(folderError(int)), this, SLOT(showSettingsErrorMessage(int)) );

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController()->contentFolders(), SIGNAL(programFilesAdjustedFrom64bitTo32bitFolder(QString , QString)), this, SLOT(showProgramFilesAdjustedWarningMsg(QString, QString)) );
}


void InstallDirectoryManager::browseInstallerCache()
{
	ORIGIN_LOG_ACTION << "User browsing installer cache";

	QStringList args; 
	args << mCacheDirectoryLocation;
    
#if defined(WIN32)
	QProcess::startDetached("explorer.exe",args);
#elif defined(__APPLE__)
    QProcess::startDetached("open", args);
#endif
    
}

void InstallDirectoryManager::settingsFailed(const QString& errorMessage)
{
	if (errorMessage == "CACHE_DIR_INVALID")
	{
        setDefaultCacheFolder(SettingsManager::downloadCacheDefault());
		showSettingsErrorMessage(FolderErrors::CACHE_INVALID);
	}
	else if (errorMessage == "DIP_DIR_INVALID")
	{
        setDefaultDIPInstallFolder(SettingsManager::downloadInPlaceDefault());
		showSettingsErrorMessage(FolderErrors::DIP_INVALID);
	}
}

// Standardize display of cache location to contain a trailing '\' as it does show at init.
// After changing the location within settings, the folder path appeared without the trailing '\'
// Then, after reloading the cache path, it appeared with a trailing "\cache\", which, apparently, we
// do not need to publicize. From EbisuCacheManager.cpp line 202
// ...
// We used to add the cache tag to the UI layer but it does not need to know that there is an extra folder so that the user can have it deleted without 
// losing any other data. Need to remove it here before we pass this dir back to the UI
// 
//...hence this function.
//
QString InstallDirectoryManager::fixFolderPath(const QString& folder)
{
	QString localFolder = folder;

	if ( localFolder.endsWith(QDir::separator()+QString("cache")+QDir::separator(), Qt::CaseInsensitive) ) // already having our "\cache" folder appended
		localFolder.chop(QString(QDir::separator() + QString("cache")).size());	// remove it

	if ( localFolder.endsWith(QDir::separator()+QString("cache"), Qt::CaseInsensitive) ) // already having our "\cache" folder appended
		localFolder.chop(QString(QString("cache")).size());	// remove it

	if ( !localFolder.endsWith(QDir::separator()) )
		localFolder += QDir::separator();

	return localFolder;
}


bool InstallDirectoryManager::removeDir(const QString &dirName)
{
	bool result = true;

    // if the current cache/dip directory is a sub-folder in the old cache/dip directory don't delete it. Fix for https://developer.origin.com/support/browse/EBIBUGS-23401
    QDir cachedir(mCacheDirectoryLocation);
    QDir dipdir(mDownloadInPlaceLocation);
    if ((dirName == cachedir.absolutePath()) || (dirName == dipdir.absolutePath()))
        return true;    // return true even though we are skipping the removal of this directory so that the other directories in the parent can still get removed

	QDir dir(dirName);

	if (dir.exists(dirName)) 
	{
		Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
			if (info.isDir()) {
				result = removeDir(info.absoluteFilePath());
			}
			else {
				result = QFile::remove(info.absoluteFilePath());
			}

			if (!result) {
				return result;
			}
		}
		result = dir.rmdir(dirName);
	}

	return result;
}

void InstallDirectoryManager::deleteInstallers()
{
	ORIGIN_LOG_ACTION << "User deleting installers";

	//if there are no entitlements just popup the message box, otherwise the completion of the delete installer process relies on a change signal
	//which doesn't happen if there is no entitlements DT#14972
	if( ContentController::currentUserContentController()->entitlements().size()== 0 )
    {
        DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_delete_installers_header"), tr("ebisu_client_delete_installers_text"), tr("ebisu_client_close")));
    }
	else
	{
        DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_delete_installers_dialog_caps"), tr("ebisu_client_delete_installers_dialog_text"), tr("ebisu_client_yes"), tr("ebisu_client_no"), "no", QJsonObject(), this, "onDeleteInstallersDone"));
	}
}

void InstallDirectoryManager::onDeleteInstallersDone(QJsonObject obj)
{
    if (obj["accepted"].toBool())
    {
        mWaitingLongOperation = true;
#if !defined(ORIGIN_MAC)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        ContentController::currentUserContentController()->deleteAllInstallerCacheFiles();
        deletedInstallersRefreshed();
    }
}

void InstallDirectoryManager::deletedInstallersRefreshed()
{
#ifdef TODO
    handle deletion of cache folder
#endif
	/*
		Undoes the last setOverrideCursor().
		Never use setOverrideCursor(QCursor(Qt::ArrowCursor)) here.
	*/
	QApplication::restoreOverrideCursor();
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_delete_installers_header"), tr("ebisu_client_delete_installers_text"), tr("ebisu_client_close")));
	stopWaitingLongOperation();
}


void InstallDirectoryManager::resetInstallerCacheLocation()
{
	if(ContentController::currentUserContentController()->isNonDipFlowActive() && ContentController::currentUserContentController()->contentFolders()->cacheFolderValid())
    {
        DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_settings_uppercase"), tr("ebisu_client_settings_folder_change_error"), tr("ebisu_client_close")));
    }
	else
	{
        // We run this validation just to make sure nothing is crazy on the user's system, and also to create the default folder if it doesn't exist.
        if( !ContentController::currentUserContentController()->contentFolders()->validateCacheFolder( SettingsManager::downloadCacheDefault() ) )
        {
		    return;
        }
		cacheWarning(true);
        setDefaultCacheFolder(SettingsManager::downloadCacheDefault());
	}
}

void InstallDirectoryManager::setDefaultCacheFolder(const QString& folder)
{
	setDefaultCacheFolder(folder, ContentController::currentUserContentController()->contentFolders()->removeOldCache(), true);
}	


void InstallDirectoryManager::setDefaultCacheFolder(const QString& folder, bool removeOldCache, bool isDefaultCache)
{
	ORIGIN_LOG_EVENT << "inside setDefaultCacheFolder.";
	mIsDefaultCache = isDefaultCache;

	if (!ContentController::currentUserContentController()->isNonDipFlowActive() || !ContentController::currentUserContentController()->contentFolders()->cacheFolderValid())
	{
        bool oldCacheLocationValid = ContentController::currentUserContentController()->contentFolders()->cacheFolderValid();
        QString oldCacheLocation = ContentController::currentUserContentController()->contentFolders()->installerCacheLocation();

		mCacheDirectoryLocation = folder;
		//ui->cacheDirectoryLocation->cursorBackward(false, ui->cacheDirectoryLocation->maxLength() );
		ContentController::currentUserContentController()->contentFolders()->setRemoveOldCache(removeOldCache);
        ContentController::currentUserContentController()->stopAllNonDipFlows();
		ContentController::currentUserContentController()->contentFolders()->setInstallerCacheLocation( folder, isDefaultCache );

        if(removeOldCache && oldCacheLocationValid && !oldCacheLocation.isEmpty())
        {
            ORIGIN_LOG_EVENT << "Removing old cache location " << oldCacheLocation;
            removeDir(oldCacheLocation);
        }
	}
}

void InstallDirectoryManager::resetDownloadInPlaceLocation()
{
	ORIGIN_LOG_ACTION << "User resetting DiP location.";

	if(!ContentController::currentUserContentController()->canChangeDipInstallLocation() && ContentController::currentUserContentController()->contentFolders()->downloadInPlaceFolderValid())
    {
        DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_settings_uppercase"), tr("ebisu_client_settings_folder_change_error"), tr("ebisu_client_close")));
    }
	else
	{
		//Display the warning message
		dipWarning();
		//Just tell middleware to update it's DIP folder to the default and we will update once we get a SettingsChanged message back
        setDefaultDIPInstallFolder(SettingsManager::downloadInPlaceDefault());
	}
}

void InstallDirectoryManager::setDefaultDIPInstallFolder(const QString& folder)
{
	if (ContentController::currentUserContentController()->canChangeDipInstallLocation() || !ContentController::currentUserContentController()->contentFolders()->downloadInPlaceFolderValid())
	{
		if ( folder [ folder.size() -1 ] != QDir::separator() )
		{
			QString tempFolder = folder;
			tempFolder += QDir::separator();
			mDownloadInPlaceLocation = tempFolder;
		}
		else
            mDownloadInPlaceLocation = folder;

		//ui->downloadInPlaceLocation->cursorBackward(false, ui->downloadInPlaceLocation->maxLength() );
        // We don't need to stop the flows anymore
		//ContentController::currentUserContentController()->stopAllDipFlowsWithNonChangeableFolders();
		ContentController::currentUserContentController()->contentFolders()->setDownloadInPlaceLocation(mDownloadInPlaceLocation);
        //emit (downloadInPlaceFolderChanged (mDownloadInPlaceLocation));
	}
}


void InstallDirectoryManager::chooseInstallerCacheLocation()
{
	ORIGIN_LOG_ACTION << "User choosing installer cache location.";

	// Test whether the change is allowed either because the folder is not being used or the current folder is invalid
	bool isChangeAllowed = !ContentController::currentUserContentController()->isNonDipFlowActive() || !ContentController::currentUserContentController()->contentFolders()->cacheFolderValid();

	if (!isChangeAllowed)
	{
        DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_settings_uppercase"), tr("ebisu_client_settings_folder_change_error"), tr("ebisu_client_close")));
		ORIGIN_LOG_WARNING << "Change of installer cache location not allowed.";

		return;
	}

	QString installerCacheLocation = ContentController::currentUserContentController()->contentFolders()->installerCacheLocation();

	if (installerCacheLocation.isEmpty() || installerCacheLocation.isNull())
	{
		ORIGIN_LOG_WARNING << "Current installer cache location is empty or invalid.";
		return;
	}

	ORIGIN_LOG_EVENT << "Current installer cache location: " << installerCacheLocation;
	// Add a parent here so the client stays in front when the QFileDialog is closed
	QString dir = QFileDialog::getExistingDirectory(EbisuSystemTray::instance()->primaryWindow(), tr("ebisu_client_open_directory"),
		installerCacheLocation, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
		return;

	dir = QDir::toNativeSeparators(dir);

	if(!isFoldersDifferent(mCacheDirectoryLocation, dir))	// was there actually a change?
		return;

    // The all-folders validator called when the dialog is closed will emit
    // a signal and cause an error to be shown, but the single folder validator
    // is silent so we have to show the invalid folder error dialog ourself.
	if( !ContentController::currentUserContentController()->contentFolders()->validateCacheFolder( dir ) ) // directory invalid?
    {
		return;
    }

    if (!isFoldersDifferent(dir, checkVsDiP)) // Are DiP and Cache the same folders
		return;

	mCacheDirectoryLocation = dir;

    using namespace Origin::UIToolkit;
    QString formatStr("\"%1\"  ");
    formatStr = formatStr.arg(ContentController::currentUserContentController()->contentFolders()->installerCacheLocation());

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("",
        tr("ebisu_client_settings_uppercase"), tr("ebisu_client_delete_cache_directory").arg("\n" + formatStr),
        tr("ebisu_client_yes"), tr("ebisu_client_no"), "yes", QJsonObject(), this, "onRemoveCacheDone"));
}

void InstallDirectoryManager::onRemoveCacheDone(QJsonObject obj)
{
    bool isRemoveOldCache = obj["accepted"].toBool();
    
    bool isDefaultCache = isFoldersDifferent(mCacheDirectoryLocation, SettingsManager::downloadCacheDefault()) == false;

    setDefaultCacheFolder(mCacheDirectoryLocation, isRemoveOldCache, isDefaultCache);
}

void InstallDirectoryManager::chooseDownloadInPlaceLocation( QWidget *parent )
{
    // Make sure that the value is up to date.
    mDownloadInPlaceLocation = fixFolderPath(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionService::currentSession()));
    if (ContentController::currentUserContentController()->canChangeDipInstallLocation() || !ContentController::currentUserContentController()->contentFolders()->downloadInPlaceFolderValid())
    {
        QString dir;
		// Add a parent here so the client stays in front when the QFileDialog is closed
        if(parent == NULL)
        {
            parent = EbisuSystemTray::instance()->primaryWindow();
        }

        dir.clear();
        dir = QFileDialog::getExistingDirectory(parent, tr("ebisu_client_open_directory"),
                                                mDownloadInPlaceLocation,
                                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (!dir.isEmpty())
        {
            dir = QDir::toNativeSeparators(dir);

            if(isFoldersDifferent(mDownloadInPlaceLocation, dir)	// was there actually a change?
#ifdef ORIGIN_PC
                || dir.contains(";")	// or do we have a forbidden character
                                        // in our existing folder?
#endif
            )
            {
                if( !ContentController::currentUserContentController()->contentFolders()->validateDIPFolder( dir ) ) // directory invalid?
				{
                    return;
				}

                if (!isFoldersDifferent(dir, checkVsCache)) // Are DiP and Cache the same folders
                    return;
                
                mDownloadInPlaceLocation = dir;
                validateAndSetDip();
            }			
        }
    }
}


bool InstallDirectoryManager::cacheWarning(bool isDefaultFolder)
{
    QString currentFolder = ContentController::currentUserContentController()->contentFolders()->installerCacheLocation();
    QString newCacheFolder = mCacheDirectoryLocation;

    // Do some reformatting to make sure the path fits on a single line
    QString formatStr("\"%1\"  ");
    formatStr = formatStr.arg(currentFolder);

	if( isFoldersDifferent(currentFolder, newCacheFolder) )
	{	
        if (isDefaultFolder)
        {
            //Just display a message to the user and then return. We've already sent a message to middleware to do the update
            DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("",
                tr("ebisu_client_settings_uppercase"),
                tr("ebisu_client_delete_cache_directory").arg("\n" + formatStr), tr("ebisu_client_yes"), tr("ebisu_client_no"), 
                "no", QJsonObject(), this, "onRemoveOldCacheDone"));

            //else just return true. Middleware is already going to restore the cache to default and tell us when it's done and then we will update
            return true;
        }

		if( !ContentController::currentUserContentController()->contentFolders()->validateCacheFolder( newCacheFolder ) ) // directory invalid?
		{
			setDefaultCacheFolder(currentFolder, false, mIsDefaultCache); // revert folder
			return false;
		}
		else
		{
            // If user presses Yes - REALLY DELETE OLD CACHE CONTENT & APPLY NEW PATH
            DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("",
                tr("ebisu_client_settings_uppercase"),
                tr("ebisu_client_delete_cache_directory").arg("\n" + formatStr), tr("ebisu_client_yes"), tr("ebisu_client_no"),
                "no", QJsonObject(), this, "onReallyRemoveOldCacheDone"));
			
			return true;
		}
	}
	return true;
}	

void InstallDirectoryManager::onRemoveOldCacheDone(QJsonObject obj)
{
    bool removeOldCache = obj["accepted"].toBool();
    if (removeOldCache)	// REALLY DELETE OLD CACHE CONTENT & APPLY NEW PATH
    {
        ContentController::currentUserContentController()->contentFolders()->setRemoveOldCache(true);
    }
}

void InstallDirectoryManager::onReallyRemoveOldCacheDone(QJsonObject obj)
{
    bool removeOldCache = obj["accepted"].toBool();
    setDefaultCacheFolder(mCacheDirectoryLocation, removeOldCache, false);
}

void InstallDirectoryManager::resetDiPSignal()
{
	mDipChanged=true;
}

void InstallDirectoryManager::resetCacheSignal(const QString &)
{
	mCacheChanged=true;
}

void InstallDirectoryManager::dipChangeFinished()
{
	static bool once=false;
	if(once)
		return;

	once=true;
	if(mDipChanged)
	{
		validateAndSetDip();
		mDipChanged=false;
	}
	once=false;
}

void InstallDirectoryManager::cacheChangeFinished()
{
	static bool once=false;
	if(once)
		return;

	once=true;
	if(mCacheChanged)
	{
		cacheWarning(false);
		mCacheChanged=false;
	}
	once=false;
}


void InstallDirectoryManager::dipWarning()
{
	//Just display a message to the user and then return. We've already sent a message to middleware to do the update
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_installation_settings_changed_caps"), tr("ebisu_client_settings_games_remain"), tr("ebisu_client_close")));
}

bool InstallDirectoryManager::validateAndSetDip()
{
  QString currentFolder = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionService::currentSession());
	QString newDIPFolder = mDownloadInPlaceLocation;

	if( currentFolder.compare(newDIPFolder, Qt::CaseInsensitive) !=0	// was there actually a change?
#ifdef ORIGIN_PC
		|| newDIPFolder.contains(";")	// or do we have a forbidden character
										// in our existing folder?
#endif
	)
	{
		if( !ContentController::currentUserContentController()->contentFolders()->validateDIPFolder( newDIPFolder ) ) // directory invalid?
		{
			setDefaultDIPInstallFolder(currentFolder);	// revert folder
			return false;
		}
		else
		{
			setDefaultDIPInstallFolder(newDIPFolder);	// apply new folder
			dipWarning();
			return true;
		}
	}

	// old and new folder are the same, so do not apply it again....
	return true;
}

bool InstallDirectoryManager::createFolderElevated(const QString& folder)
{
#if defined(ORIGIN_PC)
	QString szSD("D:(A;OICI;GA;;;WD)");    // Discretionary ACL, Allow full control to everyone
#else
    // TODO: implement permissions, if needed, here
    QString szSD;
#endif

    QString escalationReasonStr = "InstallDirectoryManager createFolderElevated (" + folder + ")";
    int escalationError = 0;
    QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
    if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
        return false;

	int result = escalationClient->createDirectory(folder, szSD);

    return Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", result, escalationClient->getSystemError());
}

void InstallDirectoryManager::showSettingsErrorMessage(int msg)
{
    QString messageText;
	switch(msg)
	{
	case FolderErrors::CACHE_INVALID:
		messageText = QString(tr("ebisu_client_settings_invalid_cache_folder"));
		break;
	case FolderErrors::FOLDERS_HAVE_SAME_NAME:
		messageText = QString(tr("ebisu_client_settings_invalid_folder_the_same"));
		break;
	case FolderErrors::CACHE_TOO_LONG:
		messageText =  QString(tr("ebisu_client_settings_invalid_cache_folder_too_long"));
		break;
	case FolderErrors::DIP_INVALID:
		messageText = QString(tr("ebisu_client_settings_invalid_game_folder"));
		break;
	case FolderErrors::DIP_TOO_LONG:
		messageText = QString(tr("ebisu_client_settings_invalid_game_folder_too_long"));
		break;
    case FolderErrors::CHAR_INVALID:
        messageText = QString(tr("ebisu_client_settings_invalid_folder_char"));
        break;

    default:
        break;

	}

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_settings_uppercase"), messageText, tr("ebisu_client_close")));
}

void InstallDirectoryManager::showProgramFilesAdjustedWarningMsg(QString prevLocation, QString newLocation)
{
    QString messageText = QString(tr("ebisu_client_settings_invalid_folder_64to32bitWarn").arg(prevLocation).arg(newLocation));
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_settings_uppercase"), messageText, tr("ebisu_client_close")));
}

bool InstallDirectoryManager::validateCacheFolder()
{
	return ContentController::currentUserContentController()->contentFolders()->validateCacheFolder(mCacheDirectoryLocation);
}

bool InstallDirectoryManager::validateDIPFolder()
{
	return ContentController::currentUserContentController()->contentFolders()->validateDIPFolder(mDownloadInPlaceLocation);
}



bool InstallDirectoryManager::isFoldersDifferent(const QString& dir, FolderToCheck foldertocheck)
{
	ORIGIN_LOG_EVENT << "calling isFoldersDifferent (const QString& dir, FolderToCheck foldertocheck)";
	QString folder;
	switch (foldertocheck)
	{
	case checkVsCache:
		folder = mCacheDirectoryLocation;
		break;
	case checkVsDiP:
		folder = mDownloadInPlaceLocation;
		break;
	}
	if (!isFoldersDifferent(dir, folder))
	{
		showSettingsErrorMessage(FolderErrors::FOLDERS_HAVE_SAME_NAME);
		return false;
	}

	return true;
}

bool InstallDirectoryManager::isFoldersDifferent(const QString& dir1, const QString& dir2)
{
	ORIGIN_LOG_EVENT << "calling isFoldersDifferent (const QString& dir1, const QString& dir2)";
	QString localDir1 = dir1;
	QString localDir2 = dir2;

	// Append trailing slashes.
	if(!localDir1.endsWith(QDir::separator()))
		localDir1.append(QDir::separator());
	if(!localDir2.endsWith(QDir::separator()))
		localDir2.append(QDir::separator());

	// Convert double slashes, etc. to be consistent.
	localDir1 = QDir::toNativeSeparators(localDir1);
	localDir2 = QDir::toNativeSeparators(localDir2);

	return (localDir1.compare(localDir2, Qt::CaseInsensitive) != 0);
}

}
}
}
