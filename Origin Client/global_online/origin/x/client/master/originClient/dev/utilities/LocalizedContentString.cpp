///////////////////////////////////////////////////////////////////////////////
// LocalizedContentString.h
//
// portions taken from 8.6/CoreLanguage.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QtGlobal>

#include "LocalizedContentString.h"
#include "LocalizedDateFormatter.h"

namespace Origin
{

    namespace Client
    {

        LocalizedContentString::LocalizedContentString(const QLocale &locale, QObject* parent)
        : QObject(parent)
        , mLocale(locale)
        {
            mOperationStrMap["UPDATE-NOUN"] = tr("ebisu_client_update");
            mOperationStrMap["UPDATE-READYTOSTART"] = tr("ebisu_client_update_now");
            mOperationStrMap["UPDATE-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["UPDATE-INITIALIZING"] = tr("ebisu_client_verifying_game_files");
            mOperationStrMap["UPDATE-RESUME"] = tr("ebisu_client_game_update_resume_update");
            mOperationStrMap["UPDATE-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["UPDATE-PAUSE"] = tr("ebisu_client_game_update_pause_update");
            mOperationStrMap["UPDATE-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["UPDATE-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["UPDATE-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["UPDATE-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["UPDATE-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["UPDATE-ENQUEUED"] = tr("ebisu_client_update_in_queue");
            mOperationStrMap["UPDATE-RUNNING"] = tr("ebisu_client_updating");
            mOperationStrMap["UPDATE-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["CLOUD-NOUN"] = tr("ebisu_client_cloud_sync");
            mOperationStrMap["CLOUD-READYTOSTART"] = tr("ebisu_client_sync_now");
            mOperationStrMap["CLOUD-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["CLOUD-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["CLOUD-RESUME"] = tr("ebisu_client_resume_sync");
            mOperationStrMap["CLOUD-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["CLOUD-PAUSE"] = tr("ebisu_client_pause_sync");
            mOperationStrMap["CLOUD-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["CLOUD-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["CLOUD-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["CLOUD-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["CLOUD-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["CLOUD-ENQUEUED"] = tr("ebisu_client_sync_in_queue");
            mOperationStrMap["CLOUD-RUNNING"] = tr("ebisu_client_cloud_game_card_status");
            mOperationStrMap["CLOUD-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["DOWNLOAD-NOUN"] = tr("ebisu_client_download");
            mOperationStrMap["DOWNLOAD-READYTOSTART"] = tr("ebisu_client_download_now");
            mOperationStrMap["DOWNLOAD-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["DOWNLOAD-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["DOWNLOAD-RESUME"] = tr("ebisu_client_resume_download");
            mOperationStrMap["DOWNLOAD-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["DOWNLOAD-PAUSE"] = tr("ebisu_client_pause_download");
            mOperationStrMap["DOWNLOAD-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["DOWNLOAD-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["DOWNLOAD-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["DOWNLOAD-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["DOWNLOAD-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["DOWNLOAD-ENQUEUED"] = tr("ebisu_client_download_in_queue");
            mOperationStrMap["DOWNLOAD-RUNNING"] = tr("ebisu_client_downloading");
            mOperationStrMap["DOWNLOAD-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["PRELOAD-NOUN"] = tr("ebisu_client_preload_noun");
            mOperationStrMap["PRELOAD-READYTOSTART"] = tr("ebisu_client_preload_now");
            mOperationStrMap["PRELOAD-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["PRELOAD-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["PRELOAD-RESUME"] = tr("ebisu_client_resume_preload");
            mOperationStrMap["PRELOAD-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["PRELOAD-PAUSE"] = tr("ebisu_client_pause_preload");
            mOperationStrMap["PRELOAD-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["PRELOAD-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["PRELOAD-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["PRELOAD-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["PRELOAD-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["PRELOAD-ENQUEUED"] = tr("ebisu_client_preload_in_queue");
            mOperationStrMap["PRELOAD-RUNNING"] = tr("ebisu_client_preloading");
            mOperationStrMap["PRELOAD-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["INSTALL-NOUN"] = tr("ebisu_client_install");
            mOperationStrMap["INSTALL-READYTOSTART"] = tr("ebisu_client_install_now");
            mOperationStrMap["INSTALL-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["INSTALL-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["INSTALL-RESUME"] = tr("ebisu_client_resume_install");
            mOperationStrMap["INSTALL-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["INSTALL-PAUSE"] = tr("ebisu_client_pause_install");
            mOperationStrMap["INSTALL-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["INSTALL-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["INSTALL-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["INSTALL-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["INSTALL-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["INSTALL-ENQUEUED"] = tr("ebisu_client_install_in_queue");
            mOperationStrMap["INSTALL-RUNNING"] = tr("ebisu_client_installing");
            mOperationStrMap["INSTALL-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["ITO-NOUN"] = tr("ebisu_client_install");
            mOperationStrMap["ITO-READYTOSTART"] = tr("ebisu_client_install_now");
            mOperationStrMap["ITO-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["ITO-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["ITO-RESUME"] = tr("ebisu_client_resume_install");
            mOperationStrMap["ITO-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["ITO-PAUSE"] = tr("ebisu_client_pause_install");
            mOperationStrMap["ITO-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["ITO-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["ITO-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["ITO-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["ITO-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["ITO-ENQUEUED"] = tr("ebisu_client_install_in_queue");
            mOperationStrMap["ITO-RUNNING"] = tr("ebisu_client_copying");
            mOperationStrMap["ITO-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["REPAIR-NOUN"] = tr("ebisu_client_repair");
            mOperationStrMap["REPAIR-READYTOSTART"] = tr("ebisu_client_repair_now");
            mOperationStrMap["REPAIR-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["REPAIR-INITIALIZING"] = tr("ebisu_client_verifying_game_files");
            mOperationStrMap["REPAIR-RESUME"] = tr("ebisu_client_resume_repair");
            mOperationStrMap["REPAIR-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["REPAIR-PAUSE"] = tr("ebisu_client_pause_repair");
            mOperationStrMap["REPAIR-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["REPAIR-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["REPAIR-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["REPAIR-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["REPAIR-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["REPAIR-ENQUEUED"] = tr("ebisu_client_repair_in_queue");
            mOperationStrMap["REPAIR-RUNNING"] = tr("ebisu_client_repairing");
            mOperationStrMap["REPAIR-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");

            mOperationStrMap["UNPACK-NOUN"] = tr("ebisu_client_unpack");
            mOperationStrMap["UNPACK-READYTOSTART"] = tr("ebisu_client_unpack_now");
            mOperationStrMap["UNPACK-PREPARING"] = tr("ebisu_client_preparing");
            mOperationStrMap["UNPACK-INITIALIZING"] = tr("ebisu_client_preparing");
            mOperationStrMap["UNPACK-RESUME"] = tr("ebisu_client_resume_unpack");
            mOperationStrMap["UNPACK-RESUMING"] = tr("ebisu_client_resuming");
            mOperationStrMap["UNPACK-PAUSE"] = tr("ebisu_client_pause_unpack");
            mOperationStrMap["UNPACK-PAUSING"] = tr("ebisu_client_pausing");
            mOperationStrMap["UNPACK-PAUSED"] = tr("ebisu_client_download_paused");
            mOperationStrMap["UNPACK-CANCELLING"] = tr("ebisu_client_canceling");
            mOperationStrMap["UNPACK-FINALIZING"] = tr("ebisu_client_finalizing_download");
            mOperationStrMap["UNPACK-INSTALLING"] = tr("ebisu_client_installing");
            mOperationStrMap["UNPACK-ENQUEUED"] = tr("ebisu_client_unpack_in_queue");
            mOperationStrMap["UNPACK-RUNNING"] = tr("ebisu_client_unpacking");
            mOperationStrMap["UNPACK-COMPLETED"] = tr("ebisu_client_achievements_overview_complete");
        }

        /* ===========================================================================================
        Purpose: Formats a byte value for localized display as: 

        <Bytes in Scaled Units><Unit Label>. 

        For example, 12121 bytes would be formatted to:

        12.1KiB

        Parameters:	nBytes - Bytes to be formatted.

        Returns: o A formatted string containing scaled bytes and a unit label - Success
        o An empty string.											   - Failure

        Notes: o The largest supported unit is always chosen.
        o A decimal point is only shown if a corresponding decimal value exists. 
        o The maximum precision is 1 decimal place.
        o The units supported by this function are:

        KiB - kibibyte ( 1024^1 )
        MiB - mebibyte ( 1024^2 )
        GiB - gibibyte ( 1024^3 )
        =========================================================================================== */
        QString LocalizedContentString::makeByteSizeString( qint64 nBytes, bool bShowGB )
        {
	        const int	 NMAX_DECIMAL_PRECISION = 2;	// Can be increased if needed.

	        // Units
#if defined(ORIGIN_MAC)
	        const qint64 NUNIT_KILOBYTE = 1000;
	        const qint64 NUNIT_MEGABYTE = 1000 * 1000;
	        const qint64 NUNIT_GIGABYTE = 1000 * 1000 * 1000;
#else
            // These are actually "kibibytes", "mebibytes", and "gibibytes" but we're going
            // to call them kilo/mega/giga for the sake of readability.
            const qint64 NUNIT_KILOBYTE = 1024;
            const qint64 NUNIT_MEGABYTE = 1024 * 1024;
            const qint64 NUNIT_GIGABYTE = 1024 * 1024 * 1024;
#endif

	        // Formatting
	        double  nValue;				    // Decimal-converted byte value.
	        qint64 nUnit;				    // Largest unit of the byte value.
	        qint64 nTempBytes    = nBytes; // Modifiable byte value copy.
	        int	    nDecimalCount = 0;	    // Number of decimals places resulting from nBytes / nUnit.
	        QString sUnitLabel;			    // ex. KiB, MiB, GiB.
	        QString sBytes;					// String version of nBytes.
	        QString sResult;			    // Formatted string containing the scaled bytes and unit label.


	        // ------------------------------------------------------------------------------------
	        // Determine the largest supported unit that the byte count fits into and generate its
	        // corresponding label.
	        //

            if	( bShowGB && nBytes >= NUNIT_GIGABYTE )
            { 
                nUnit = NUNIT_GIGABYTE;
                sUnitLabel = QObject::tr("ebisu_client_gigabyte").trimmed();
            }
	        else if (nBytes >= NUNIT_MEGABYTE )
            { 
                nUnit = NUNIT_MEGABYTE;
                sUnitLabel = QObject::tr( "ebisu_client_megabyte").trimmed();
            }
	        else	/* kibibyte */
            { 
                nUnit = NUNIT_KILOBYTE;
                sUnitLabel = QObject::tr( "ebisu_client_kilobyte").trimmed();
            }


	        // ----------------------------------------------------------------------------------------
	        // Count the number of decimal places resulting from the byte count divided by the largest 
	        // unit it fits into. Stop counting once the maximum decimal precision we care about has
	        // been reached.
	        //

	        while ( ( nTempBytes % nUnit ) && ( nDecimalCount < NMAX_DECIMAL_PRECISION ) ) 
	        { 
		        nTempBytes *= 10; 
		        nDecimalCount++; 
	        } 


	        // --------------------------------
	        // Construct the formatted string.
	        //
	        nValue = ( double ) nBytes / nUnit;							// Calculate the number of bytes in scaled units.
            sBytes = mLocale.toString (nValue, 'f', nDecimalCount);

            //append the unit label
	        sResult = QString("<b>%1</b> %2").arg(sBytes).arg(sUnitLabel); 

	        return sResult;
        }


        QString LocalizedContentString::makeProgressDisplayString (qint64 nBytesDownloaded, qint64 totalBytes)
        {
            QString sProgressDisplay = QString("%1 %2 %3").
                    arg(makeByteSizeString( nBytesDownloaded )).
                    arg(QObject::tr( "cc_bytes_of_bytes")).
                    arg(makeByteSizeString( totalBytes ));

            return sProgressDisplay;
        }

        QString LocalizedContentString::makeProgressRateDisplayString (qint64 bps)
        {
		    QString sProgressRateDisplay = QString("%1/%2").
                arg(makeByteSizeString( bps )).
                arg(QObject::tr ("seconds"));
            return sProgressRateDisplay;
        }

        QString LocalizedContentString::makeTimeRemainingString (qint64 elapsedSeconds)
        {
            QString sTimeRemainingDisplay = QString( QObject::tr( "cc_caption_time_remaining")).
                arg(LocalizedDateFormatter().formatInterval(elapsedSeconds, LocalizedDateFormatter::Seconds, LocalizedDateFormatter::Hours));
            return sTimeRemainingDisplay;
        }

        QString LocalizedContentString::flowStateCode(const Downloader::ContentInstallFlowState::value& flowState)
        {
            QString phaseStr;
            switch(flowState)
            {
            case Downloader::ContentInstallFlowState::kReadyToStart:
                phaseStr = "READYTOSTART";
                break;
            case Downloader::ContentInstallFlowState::kPendingDiscChange:
            case Downloader::ContentInstallFlowState::kPaused:
                phaseStr = "PAUSED";
                break;
            case Downloader::ContentInstallFlowState::kPausing:
                phaseStr = "PAUSING";
                break;
            case Downloader::ContentInstallFlowState::kInitializing:
                phaseStr = "INITIALIZING";
                break;
            case Downloader::ContentInstallFlowState::kEnqueued:
                phaseStr = "ENQUEUED";
                break;
            case Downloader::ContentInstallFlowState::kPreTransfer:
            case Downloader::ContentInstallFlowState::kPendingInstallInfo:
            case Downloader::ContentInstallFlowState::kPendingEulaLangSelection:
            case Downloader::ContentInstallFlowState::kPendingEula:
                phaseStr = "PREPARING";
                break;
            case Downloader::ContentInstallFlowState::kResuming:
                phaseStr = "RESUMING";
                break;
            case Downloader::ContentInstallFlowState::kPostTransfer:
                phaseStr = "FINALIZING";
                break;
            case Downloader::ContentInstallFlowState::kCanceling:
                phaseStr = "CANCELLING";
                break;
            case Downloader::ContentInstallFlowState::kPreInstall:
            case Downloader::ContentInstallFlowState::kInstalling:
            case Downloader::ContentInstallFlowState::kPostInstall:
                phaseStr = "INSTALLING";
                break;
            case Downloader::ContentInstallFlowState::kReadyToInstall:
                phaseStr = "READYTOINSTALL";
                break;
            case Downloader::ContentInstallFlowState::kCompleted:
                phaseStr = "COMPLETED";
                break;
            default:
                phaseStr = "RUNNING";
                break;
            }
            return phaseStr;
        }

        QString LocalizedContentString::operationTypeCode(const Downloader::ContentOperationType& operationType)
        {
            QString typeStr;
            switch(operationType)
            {
            case Origin::Downloader::kOperation_Download:
                typeStr = "DOWNLOAD";
                break;
            case Downloader::kOperation_Repair:
                typeStr = "REPAIR";
                break;
            case Downloader::kOperation_Update:
                typeStr = "UPDATE";
                break;
            case Downloader::kOperation_Install:
                typeStr = "INSTALL";
                break;
            case Downloader::kOperation_ITO:
                typeStr = "ITO";
                break;
            case Downloader::kOperation_Unpack:
                typeStr = "UNPACK";
                break;
            case Downloader::kOperation_Preload:
                typeStr = "PRELOAD";
                break;
            default:
            case Downloader::kOperation_None:
                typeStr = "NONE";
                break;
            }
            return typeStr;
        }

        QString LocalizedContentString::operationPhaseDiplay(const Downloader::ContentOperationType& operationType, const Downloader::ContentInstallFlowState::value& flowState)
        {
            return operationPhaseDiplay(operationTypeCode(operationType), flowStateCode(flowState));
        }

        QString LocalizedContentString::operationPhaseDiplay(const QString& operationTypeCode, const QString& flowStateCode)
        {
            return mOperationStrMap.value(operationTypeCode+"-"+flowStateCode);
        }

        QString LocalizedContentString::operationDisplay(const Downloader::ContentOperationType& operationType)
        {
            return mOperationStrMap.value(operationTypeCode(operationType)+"-NOUN");
        }

        QString LocalizedContentString::formatBytes(const qulonglong& bytes)
        {
#if defined(ORIGIN_MAC)
            const qulonglong KB = 1000;
            const qulonglong MB = 1000000;
            const qulonglong GB = 1000000000;
            const qulonglong TB = 1000000000000;
#else
            const qulonglong KB = 1024;
            const qulonglong MB = 1048576;
            const qulonglong GB = 1073741824;
            const qulonglong TB = 1099511627776;
#endif
            const int precision = 2;
            QString formattedString = "";

            if (bytes < KB)
            {
                formattedString = tr("ebisu_client_byte_notation").arg(QString::number(bytes));
            }
            else if (bytes < MB)
            {
                formattedString = tr("ebisu_client_kbyte_notation").arg(QLocale().toString((float)bytes / KB, 'f', precision));
            }
            else if (bytes < GB)
            {
                formattedString = tr("ebisu_client_mbyte_notation").arg(QLocale().toString((float)bytes / MB, 'f', precision));
            }
            else if (bytes < TB)
            {
                formattedString = tr("ebisu_client_gbyte_notation").arg(QLocale().toString((float)bytes / GB, 'f', precision));
            }
            else
            {
                formattedString = tr("ebisu_client_tbyte_notation").arg(QLocale().toString((float)bytes / TB, 'f', precision));
            }
            return formattedString;
        }
    } //namespace Client
} //namespace Origin
