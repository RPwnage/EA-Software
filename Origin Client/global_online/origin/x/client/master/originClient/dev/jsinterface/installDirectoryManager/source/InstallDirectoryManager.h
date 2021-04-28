#ifndef _INSTALLDIRECTORMANAGER_H
#define _INSTALLDIRECTORMANAGER_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QJsonObject>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
class ORIGIN_PLUGIN_API InstallDirectoryManager : public QObject
{
    Q_OBJECT

public:
    InstallDirectoryManager(QObject* parent = 0);
    ~InstallDirectoryManager();

    /// \brief Opens a OS file dialog. Allows the user to change their game download directory.
    Q_INVOKABLE void chooseDownloadInPlaceLocation(QWidget *parent = NULL);

    /// \brief Opens a OS file dialog. Allows the user to change their installer cache directory.
    Q_INVOKABLE void chooseInstallerCacheLocation();

    /// \brief Resets the users installer cache directory.
    Q_INVOKABLE void resetInstallerCacheLocation();

    /// \brief Opens installer cache directory.
    Q_INVOKABLE void browseInstallerCache();

    /// \brief Deletes all installers in the installer cache directory.
    Q_INVOKABLE void deleteInstallers();

    QString downloadInPlaceLocation() const {return mDownloadInPlaceLocation;}

public slots:
    /// \brief Resets the users game download directory.
    Q_INVOKABLE void resetDownloadInPlaceLocation();

    /// \brief TBD
    void resetDiPSignal();

    /// \brief TBD
    void dipChangeFinished();
    
    /// \brief TBD
    void settingsFailed(const QString& errorMessage);
    
    /// \brief Displays a pop-up error message for an invalid directory choice
    void showSettingsErrorMessage(int msg);

    /// \brief Displays a pop-up warning message when we automatically adjusted the users program files location
    void showProgramFilesAdjustedWarningMsg(QString prevLocation, QString newLocation);

    /// \brief Handle non-modal response to the dialog prompt for deleting installers
    void onDeleteInstallersDone(QJsonObject);

    /// \brief Handle non-modal response to the dialog prompt for clearing cache
    void onRemoveCacheDone(QJsonObject);

    /// \brief Handle non-modal response to the dialog prompt for clearing cache
    void onRemoveOldCacheDone(QJsonObject);

    /// \brief Handle non-modal response to the dialog prompt for clearing cache
    void onReallyRemoveOldCacheDone(QJsonObject obj);

private:
    enum FolderToCheck
    {
        checkVsCache,
        checkVsDiP
    };

    void stopWaitingLongOperation();
    void init();
    static QString fixFolderPath(const QString& folder);
    bool removeDir(const QString &dirName);
    void deletedInstallersRefreshed();
    void setDefaultCacheFolder(const QString& folder);
    void setDefaultCacheFolder(const QString& folder, bool removeOldCache, bool isDefaultCache);
    void setDefaultDIPInstallFolder(const QString& folder);
    bool cacheWarning(bool isDefaultFolder);
    void resetCacheSignal(const QString &);
    void cacheChangeFinished();
    void dipWarning();
    bool validateAndSetDip();
    bool createFolderElevated(const QString& folder);
    bool validateCacheFolder();
    bool validateDIPFolder();
    bool isFoldersDifferent(const QString& dir, FolderToCheck foldertocheck);
    bool isFoldersDifferent(const QString& dir1, const QString& dir2);

    QString mDownloadInPlaceLocation;
    QString mCacheDirectoryLocation;
    bool mDipChanged;
    bool mCacheChanged;
    bool mWaitingLongOperation;
    bool mIsDefaultCache;
};
}
}
}
#endif