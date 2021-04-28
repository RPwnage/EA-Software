#include "services/settings/SettingsManager.h"

//  ContentFolders.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef CONTENTFOLDERSCONTROLLER_H
#define CONTENTFOLDERSCONTROLLER_H

#include <QtCore>
#include <QTimer>

/// \brief The root namespace for the %Origin client application.
///
/// \dot
/// 
/// digraph Origin_Client {
/// graph [rankdir="LR"];
/// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
/// 
/// { rank = same; "node1"; "Top Level\nNamespaces"; }
/// { rank = same; "node2"; "node3"; "node10"; "node11"; "node12"; "node20"; "node22"; "node23"; "node25"; "node26"; "node27"; "node28"; "node29"; "node33"; "2nd Level\nNamespaces"; }
/// { rank = same; "node4"; "node5"; "node9"; "node13"; "node14"; "node15"; "node16"; "node17"; "node18"; "node19"; "node21"; "node24"; "node30"; "node31"; "node32"; "3rd Level\nNamespaces"; }
/// { rank = same; "node6"; "node7"; "node8"; "4th Level\nNamespaces"; }
/// 
/// "Top Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "2nd Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "3rd Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "4th Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "node1" [label="Origin" URL="\ref Origin"]
/// "node2" [label="Chat" URL="\ref Origin::Chat"]
/// "node3" [label="Client" URL="\ref Origin::Client"]
/// "node4" [label="CommandLine" URL="\ref Origin::Client::CommandLine"]
/// "node5" [label="JsInterface" URL="\ref Origin::Client::JsInterface"]
/// "node6" [label="ConversationEventDict" URL="\ref Origin::Client::JsInterface::ConversationEventDict"]
/// "node7" [label="ConversionHelpers" URL="\ref Origin::Client::JsInterface::ConversionHelpers"]
/// "node8" [label="JavaScript Bridge" fillcolor="goldenrod1" URL="./bridgedocgen/index.xhtml"]
/// "node9" [label="WebDispatcherRequestBuilder" URL="\ref Origin::Client::WebDispatcherRequestBuilder"]
/// "node10" [label="ContentUtils" URL="\ref Origin::ContentUtils"]
/// "node11" [label="Downloader\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Downloader"]
/// "node12" [label="Engine" URL="\ref Origin::Engine"]
/// "node13" [label="Achievements" URL="\ref Origin::Engine::Achievements"]
/// "node14" [label="CloudSaves\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Engine::CloudSaves"]
/// "node15" [label="Config" URL="\ref Origin::Engine::Config"]
/// "node16" [label="Content\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Engine::Content"]
/// "node17" [label="Debug" URL="\ref Origin::Engine::Debug"]
/// "node18" [label="MultiplayerInvite" URL="\ref Origin::Engine::MultiplayerInvite"]
/// "node19" [label="Social" URL="\ref Origin::Engine::Social"]
/// "node20" [label="Escalation" URL="\ref Origin::Escalation"]
/// "node21" [label="inner" URL="\ref Origin::Escalation::inner"]
/// "node22" [label="Platform" URL="\ref Origin::Platform"]
/// "node23" [label="SDK" URL="\ref Origin::SDK"]
/// "node24" [label="Lsx" URL="\ref Origin::SDK::Lsx"]
/// "node25" [label="Services\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Services"]
/// "node26" [label="StringUtils" URL="\ref Origin::StringUtils"]
/// "node27" [label="UIToolkit" URL="\ref Origin::UIToolkit"]
/// "node28" [label="URI" URL="\ref Origin::URI"]
/// "node29" [label="Util" URL="\ref Origin::Util"]
/// "node30" [label="EntitlementXmlUtil" URL="\ref Origin::Util::EntitlementXmlUtil"]
/// "node31" [label="NetUtil" URL="\ref Origin::Util::NetUtil"]
/// "node32" [label="XmlUtil" URL="\ref Origin::Util::XmlUtil"]
/// "node33" [label="Utilities" URL="\ref Origin::Utilities"]
/// "Top Level\nNamespaces" -> "2nd Level\nNamespaces" -> "3rd Level\nNamespaces" -> "4th Level\nNamespaces";
/// edge [color=black];
/// "node1" -> "node2";
/// "node1" -> "node3" -> "node4";
/// "node3" -> "node5" -> "node6";
/// "node5" -> "node7";
/// "node5" -> "node8";
/// "node3" -> "node9";
/// "node1" -> "node10";
/// "node1" -> "node11";
/// "node1" -> "node12" -> "node13";
/// "node12" -> "node14";
/// "node12" -> "node15";
/// "node12" -> "node16";
/// "node12" -> "node17";
/// "node12" -> "node18";
/// "node12" -> "node19";
/// "node1" -> "node20" -> "node21";
/// "node1" -> "node22";
/// "node1" -> "node23" -> "node24";
/// "node1" -> "node25";
/// "node1" -> "node26";
/// "node1" -> "node27";
/// "node1" -> "node28";
/// "node1" -> "node29" -> "node30";
/// "node29" -> "node31";
/// "node29" -> "node32";
/// "node1" -> "node33";
/// 
/// }
///
/// \enddot
///
namespace Origin
{
    /// \brief The user-based logic of the %Origin client application.
    namespace Engine
	{
        /// \brief Encompases everything to do with content configuration and state.
        ///
        /// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Content+Module+-+TB">Content Module</a>
        /// 
        /// A %Content namespaces diagram is shown below:
        /// \dot
        /// digraph g {
        /// graph [rankdir="LR"];
        /// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
        /// edge [];
        ///
        /// "node00" -> "node0" -> "node1" -> "node2";
        /// "node1" -> "node3";
        /// "node1" -> "node4";
        /// "node4" [label="ProductArt" URL="\ref ProductArt"]
        /// "node3" [label="FolderErrors" URL="\ref FolderErrors"]
        /// "node2" [label="Detail" URL="\ref Detail"]
        /// "node1" [label="Content" URL="\ref Content"]
        /// "node0" [label="Engine" URL="\ref Engine"]
        /// "node00" [label="Origin" URL="\ref Origin"]
        /// }
        /// \enddot
        namespace Content
        {
            /// \brief TBD.
            namespace FolderErrors{
                /// \brief Folder errors error messages.
                enum ErrorMessages{
                    ERROR_NONE,                 ///< No error.
	                CACHE_INVALID,              ///< A prohibited folder.
	                FOLDERS_HAVE_SAME_NAME,     ///< Cache and Dip folders are the same.
	                CACHE_TOO_LONG,             ///< Folder name is too long.
	                DIP_INVALID,                ///< A prohibited folder.
	                DIP_TOO_LONG,               ///< Folder name is too long.
                    CHAR_INVALID,               ///< Folder path contains an invalid character - either non-ASCII or a symbol.
                };
            }
            /// \brief Validates the folder paths used in the downloadcache and DiP install locations.
		    class ContentFolders : public QObject
		    {
                Q_OBJECT

            public:

                /// \brief The ContentFolders constructor.
                /// \param parent TBD.
                ContentFolders(QObject *parent);

                /// \brief The ContentFolders destructor.
                ~ContentFolders();

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Removes the old cache. </summary>
                /// <returns name="mRemoveOldCache">	Returns true if it succeeds, false if it fails. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool removeOldCache() const { return mRemoveOldCache; }

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Gets the installer cache location. </summary>
                /// <returns name="mInstallerCacheLocation">   TBD.
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                const QString& installerCacheLocation() const { return mInstallerCacheLocation; }

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Gets the download in place location. </summary>
                /// <returns name="mDownloadInPlaceLocation">   TBD.
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                const QString& downloadInPlaceLocation() const { return mDownloadInPlaceLocation; }
                
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Sets a remove old cache. </summary>
                /// <param name="s">	Set to true to remove the old cache. </param>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                void setRemoveOldCache(bool s);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Sets an installer cache location. </summary>
                /// <param name="newLocation">	The cache location. </param>
                /// <param name="isDefaultCache">	Indicates whether this is Origin's default cache location. </param>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                void setInstallerCacheLocation(const QString &newLocation, bool isDefaultCache);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Sets a download in place location. </summary>
                /// <param name="newLocation">	The download in place location. </param>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                void setDownloadInPlaceLocation(const QString &newLocation);

				/// \brief validates local cache folders
				bool validateFolder(const QString& folder, bool isCacheFolder = false);


                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Checks if the dip folder is valid. </summary>
                /// <param name="dip">	The dip install location. </param>
                /// <returns>	Returns true if valid, false if not. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool validateDIPFolder(const QString & dip);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Checks if the cache folder is valid. </summary>
                /// <param name="cache">	The cache install location. </param>
                /// <returns>	Returns true if valid, false if not. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool validateCacheFolder(const QString & cache);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Checks if all folders are valid. </summary>
                /// <param name="dipFolder">	The dip install location. </param>
                /// <param name="cacheFolder">	The cache install location. </param>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool validateAllFolders(const QString &dipFolder, const QString &cacheFolder);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Indicates whether the dip location is valid. </summary>
                /// <returns>	Returns true if valid, false if not. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool downloadInPlaceFolderValid() {return mDownloadInPlaceFolderValid;}

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Indicates whether the cache location is valid. </summary>
                /// <returns>	Returns true if valid, false if not. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool cacheFolderValid() {return mIsCacheFolderValid;}
                
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Detects whether symbols are in either the cache path or the dip path. </summary>
                /// <returns>	Returns CACHE_INVALID or DIP_INVALID depending on the invalid path. </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                int validatePaths();
                
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	This allows the QRunnable thread to set this member var appropriately
                /// when the thread begins/ends </summary>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                void setAsyncFolderCheckIsRunning(bool running);
                
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	Initiates the periodic validation of the cahce/dip folders.  This is the first
                /// in a chain of functions that are involved in checking the folders.  This function is called on
                /// a seperate thread by the QRunnable ValidateFoldersAsync so that all of the folder validation
                /// is done off the main thread, in case of file system blocking - for example, if a path is set
                /// for a network drive that has been disconnected, EBIBUGS-18578</summary>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                void periodicFolderValidation();
                
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	On Mac, if path is a mount point, verify that it is valid (still connected and writeable)</summary>
                /// <returns>   True, if PC; true, if not a mount point; true if it is a valid mount point;
                /// false, if it is an invalid mount point </returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool verifyMountPoint(const QString &path);

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                /// <summary>	On PC, the 32bit touchup installers have problems installing to Program Files, so we automically adjust it to the x86 folder all the time</summary>
                /// <returns>   On Mac always false; On PC true, if we adjusted the folder, false if we didn't need to</returns>
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                bool adjustFolderFor32bitOn64BitOS(QString &folderName);
            signals:
                void folderValidationError(int error);
                void validateFailed();
                void contentFoldersValid();
                void programFilesAdjustedFrom64bitTo32bitFolder(QString oldPath, QString newPath);

            private:
                // prevent copies from being made
                ContentFolders(ContentFolders const& from);
                ContentFolders& operator=(ContentFolders const& from);

                bool checkFolders(const QString &, const bool);


                bool verifyWritableCache(const QString &cleanFolder);
                bool isCdRom( QDir &sysCheckFolderPath);
                void getForbiddenDirs(QStringList &forbiddenDirs);
                void getForbiddenSubDirs(QStringList &forbiddenSubDirs);

                QString mInstallerCacheLocation;
                QString mDownloadInPlaceLocation;
	            bool mIsCacheFolderValid;
	            bool mDownloadInPlaceFolderValid;
                bool mRemoveOldCache;
                
                bool mSuppressFolderValidationError;
                bool mAsyncFolderCheckIsRunning;
                
                QTimer mValidateFoldersTimer;
                QMutex mValidateFolderMutex;
                QMutex mThreadRunningMutex;

				bool createFolderElevated(const QString & folder );
                
                // Periodically, we check if the download/installer folders are still valid.
                // It's best to do this on a seperate thread, so that the main thread is never
                // blocked by file system operations; for example, if the download path is set
                // to a network drive that loses connection - EBIBUGS 18578.
                class ValidateFoldersAsync : public QRunnable
                {
                    public:
                        ValidateFoldersAsync(ContentFolders* parent);
                        virtual void run();
                    private:
                        QPointer<ContentFolders> mParent;
                };

            private slots:
                void onSettingChanged(const QString&, const Origin::Services::Variant&);
                void onValidateFoldersTimeout();
		    };
        }
	}
}

#endif // CONTENTFOLDERSCONTROLLER_H
