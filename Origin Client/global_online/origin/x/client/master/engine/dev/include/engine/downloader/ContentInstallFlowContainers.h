#ifndef CONTENTINSTALLFLOWCONTAINERS_H
#define CONTENTINSTALLFLOWCONTAINERS_H

#include <QStringList>
#include <QVariant>

#include "services/plugin/PluginAPI.h"
#include "ContentInstallFlowState.h"
#include "services/publishing/CatalogDefinition.h"

namespace Origin
{
namespace Downloader
{
	/// \brief Container for an End User License Agreement.
    struct Eula
    {

	    /// \brief A summary of the purpose of this eula.
        QString Description;

	    /// \brief For the content in which this eula pertains, the name used for install purposes.
        QString InstallName;

	    /// \brief The name of the file containing the text of the license agreement.
        QString FileName;

        /// \brief Are the EULA file types Rtf? We can't tell from the FileName because it gets converted to a temp file.
        bool IsRtfFile;

	    /// \brief Does this eula pertain to software required by the content that is not owned by the content's owners?
        bool IsThirdParty;

	    /// \brief Is it required to accept this eula in order to use the content?
        bool IsOptional;

	    /// \brief Has this eula already been accepted?
        bool accepted;

        /// \brief The signature of the EULA (used for determining if it has been seen before)
        quint32 signature;

	    /// \brief The warning to display if this eula is not accepted.
        QString CancelWarning;

	    /// \brief Supplementary information to the eula description.
        QString ToolTip;

        /// \brief Original EULA filename (from package)
        QString originalFilename;

	    /// \brief Constructor.
        ///
        /// \param description A summary of the purpose of this eula.
        /// \param fileName The name of the file containing the text of the license agreement.
        /// \param isRtfFile Should we use our RTF viewing dialog or the the HTML dialog?
        /// \param isThirdParty Does this eula pertain to software required by the content that is not owned by the content's owners?
        /// \param isOptional Is it required to accept this eula in order to use the content?
        Eula(const QString& description, const QString& fileName, bool isRtfFile, bool isThirdParty, bool isOptional = false) : Description(description), FileName(fileName), IsRtfFile(isRtfFile), IsThirdParty(isThirdParty), IsOptional(isOptional), accepted(false), signature(0)
        {
            Description.replace("&&", "&");
        }

	    /// \brief Constructor.
        ///
        /// \param description A summary of the purpose of this eula.
        /// \param installName For the content in which this eula pertains, the name used for install purposes.
        /// \param fileName The name of the file containing the text of the license agreement.
        /// \param isRtfFile Should we use our RTF viewing dialog or the the HTML dialog?
        /// \param isThirdParty Does this eula pertain to software required by the content that is not owned by the content's owners?
        /// \param isOptional Is it required to accept this eula in order to use the content?
        /// \param cancelMessage The warning to display if this eula is not accepted.
        /// \param toolTip Supplementary information to the eula description.
        Eula(const QString& description, const QString& installName, const QString& fileName, bool isRtfFile, bool isThirdParty, bool isOptional, QString& cancelMessage, QString& toolTip) : Description(description), InstallName(installName), FileName(fileName), IsRtfFile(isRtfFile), IsThirdParty(isThirdParty), IsOptional(isOptional), accepted(false), signature(0), CancelWarning(cancelMessage), ToolTip(toolTip) {}
    };
    
	/// \brief Container for all End User License Agreements associated with a single content item.
    struct EulaList
    {
	    /// \brief Should the client use a dialog which consolidates all eulas into a single window?
        bool isConsolidate;

	    /// \brief The id of the product to which the eulas are associated.
        QString gameProductId;

	    /// \brief The title under which the content was installed.
        QString gameTitle;

	    /// \brief Value of "1" indicates that the list of eulas is for a content update.
        QString isAutoPatch;

	    /// \brief The list of eula objects.
        QList<Eula> listOfEulas;
    };
    
	/// \brief Container for a list of locales in which a particular content item can be installed.
    struct EulaLangList
    {
	    /// \brief The id of the product.
        QString gameProductId;

	    /// \brief List of locale strings available for install. String format: [ISO language code]_[ISO country code], ex. en_US
        QList<QString> listOfEulaLang;
    };
    
	/// \brief Container for data pertaining to operation progress.
	struct InstallProgressState
	{
        enum ProgressState
        {
            kProgressState_Paused = 0,
            kProgressState_Indeterminate,
            kProgressState_Active
        };

	    /// \brief Number of bytes currently processed.
		qint64 mBytesDownloaded;

	    /// \brief Total number of bytes to process.
		qint64 mTotalBytes;

	    /// \brief Byte processing rate.
		qint64 mBytesPerSecond;

	    /// \brief Time to completion.
		qint64 mEstimatedSecondsRemaining;

        /// \brief The operation progress between 0 and 1.
        float mProgress;

        /// \brief The required portion size for dynamic downloads.
        qint64 mDynamicDownloadRequiredPortionSize;

        /// \brief The display progress state of the operation
        ProgressState progressState(const ContentInstallFlowState& flowState) const;

        /// \brief The display progress state of the operation
        QString progressStateCode(const ContentInstallFlowState& flowState) const;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API InstallProgressState();
	};
    
	/// \brief Contains information for the display of the download information and insert disc dialogs.
	struct InstallArgumentRequest
	{

	    /// \brief The name under which the content will be installed.
		QString contentDisplayName;

	    /// \brief Unique product id for the content. Useful for hash map lookups.
        QString productId;

	    /// \brief The size of the content in bytes after it's been downloaded and decompressed.
		qint64 installedSize;

	    /// \brief The size of the content in bytes that need to be downloaded (compressed).
		qint64 downloadSize;


	    /// \brief Will the content be from local media.
		bool isLocalSource;

        /// \brief Is this an ITO flow.
        bool isITO;

	    /// \brief Can a desk top short cut to the content be created?
		bool showShortCut;

	    /// \brief Is the content Download In Place?
		bool isDip;

	    /// \brief Is this content in a preload status? (Can be installed but not played yet)
        bool isPreload;

        /// \brief Is this content pdlc
        bool isPdlc;

        /// \brief Is this content an update
        bool isUpdate;

        /// \brief Use the default install options (install location, create desktop/start menu shortcuts) and bypass the dialog (for Free To Play Optimizations)
        bool useDefaultInstallOptions;

	    /// \brief List of End User License Agreements which the user is not required to accept.
		EulaList optionalEulas;

	    /// \brief For ITO, the next disc number required to continue the install.
		int nextDiscNum;

	    /// \brief For ITO, the total number of discs required for install.
		int totalDiscNum;

	    /// \brief For ITO, indicates when the user inserts the wrong disc after a disc change prompt.
        int wrongDiscNum;

	    /// \brief The absolute path of the root directory where the content will be installed.
        QString installPath;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API InstallArgumentRequest();
	};
    
	/// \brief Contains information for the display of the language selection dialog.
	struct InstallLanguageRequest
	{
	    /// \brief The product id for the content in which the listed languages are available.
        QString productId;

        /// \brief The display name for the content in which the listed languages are available.
        QString displayName;

	    /// \brief List of locale strings in which the content can be installed. Format [ISO language code]_[ISO country code], ex: en_US
		QStringList availableLanguages;

        /// \brief The set containing locale strings that require additional downloads
        QList<QString> additionalDownloadRequiredLanguages;

        /// \brief The flag that indicates whether the UI should show a warning about selecting the previously installed language
        bool showPreviousInstallLanguageWarning;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API InstallLanguageRequest();
	};
    
	/// \brief Contains information for the display of a content's End User License Agreements.
	struct EulaStateRequest
	{

	    /// \brief  Is this content in a preload status? (Can be installed but not played yet)?
        bool isPreload;

	    /// \brief  Is the content being Installed Through Origin from local media?
		bool isLocalSource;

	    /// \brief Is a previously installed content being updated?
        bool isUpdate;

	    /// \brief The product id for the content being installed.
        QString productId;

	    /// \brief A list of End User License Agreement objects for the installing content.
		EulaList eulas;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API EulaStateRequest();
	};
    
	/// \brief Contains user response information from the download information dialog.
	struct InstallArgumentResponse
	{

	    /// \brief Flags indicating which optional components the user chose to install.
		qint32 optionalComponentsToInstall;

	    /// \brief Should a desktop short cut be created during install?
		bool installDesktopShortCut;

	    /// \brief Should a startmenu short cut be created during install?
		bool installStartMenuShortCut;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API InstallArgumentResponse();
	};
    
	/// \brief Contains user's choice for install locale.
	struct EulaLanguageResponse
	{

	    /// \brief The selected locale string. Format [ISO language code]_[ISO country code], ex: en_US
		QString selectedLanguage;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API EulaLanguageResponse();
	};
    
	/// \brief Contains accept/reject reponses the user made to End User License Agreements.
	struct EulaStateResponse
	{

	    /// \brief Indicates which eulas where accepted and which were rejected.
		qint32 acceptedBits;

	    /// \brief Constructor.
		ORIGIN_PLUGIN_API EulaStateResponse();
	};


    /// \brief The install flow action that is currently running
    enum ContentOperationType
    {
        kOperation_None = 0,
        kOperation_Repair,
        kOperation_Update,
        kOperation_Install,
        kOperation_ITO,
        kOperation_Download,
        kOperation_Unpack,
        kOperation_Preload
    };

    struct LastActionInfo
    {
        /// \brief What will the user be canceling?
        ContentOperationType state;
        qint64 totalBytes;
        ContentInstallFlowState flowState;
        bool isDynamicDownload;
        LastActionInfo();
    };

	/// \brief Contains information for the display of "Are you sure you want to cancel" dialog.
    struct CancelDialogRequest
    {
	    /// \brief The name under which the content would have been installed if not canceled.
        QString contentDisplayName;

	    /// \brief  Unique product id for the content. Useful for hash map lookups.
        QString productId;

	    /// \brief Was the install cancellation initiated from the In Game Overlay?
        bool isIGO;

        /// \brief What will the user be canceling?
        ContentOperationType state;

	    /// \brief Constructor.
        ORIGIN_PLUGIN_API CancelDialogRequest();
    };
    
	/// \brief Contains information for the display of the 3PDD install dialog.
    struct ThirdPartyDDRequest
    {

	    /// \brief Will origin monitor execution of the installer for this content?
        bool monitoredInstall;

	    /// \brief Will origin monitor the play status of the content?
        bool monitoredPlay;

	    /// \brief The id of the content.
        QString productId;

	    /// \brief Name of the third party platform which must be launched to complete install.
        Origin::Services::Publishing::PartnerPlatformType partnerPlatform;

	    /// \brief The registry key for the content.
        QString cdkey;

	    /// \brief The name under which the content will be installed.
        QString displayName;
    };
	
} // namespace Downloader
} // namespace Origin

Q_DECLARE_METATYPE(Origin::Downloader::InstallProgressState);
Q_DECLARE_METATYPE(Origin::Downloader::InstallArgumentRequest);
Q_DECLARE_METATYPE(Origin::Downloader::InstallLanguageRequest);
Q_DECLARE_METATYPE(Origin::Downloader::EulaStateRequest);
Q_DECLARE_METATYPE(Origin::Downloader::InstallArgumentResponse);
Q_DECLARE_METATYPE(Origin::Downloader::EulaLanguageResponse);
Q_DECLARE_METATYPE(Origin::Downloader::EulaStateResponse);
Q_DECLARE_METATYPE(Origin::Downloader::CancelDialogRequest);

#endif // CONTENTINSTALLFLOWVARIANTMAPS_H
