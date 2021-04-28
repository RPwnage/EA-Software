#include "engine/downloader/ContentInstallFlowContainers.h"

namespace Origin
{
namespace Downloader
{
		InstallProgressState::InstallProgressState()
		{
			mBytesDownloaded = 0;
			mTotalBytes = 0;
			mBytesPerSecond = 0;
			mEstimatedSecondsRemaining = 0;
            mProgress = 0.0f;
            mDynamicDownloadRequiredPortionSize = 0;
		}

        InstallProgressState::ProgressState InstallProgressState::progressState(const ContentInstallFlowState& flowState) const
        {
            switch(flowState)
            {
            case ContentInstallFlowState::kPendingDiscChange:
            case ContentInstallFlowState::kPaused:
            case ContentInstallFlowState::kPausing:
            case ContentInstallFlowState::kEnqueued:
            case ContentInstallFlowState::kReadyToStart:
                return kProgressState_Paused;
                break;
            case ContentInstallFlowState::kResuming:
            case ContentInstallFlowState::kPreTransfer:
            case ContentInstallFlowState::kPendingInstallInfo:
            case ContentInstallFlowState::kInvalid:
            case ContentInstallFlowState::kPendingEulaLangSelection:
            case ContentInstallFlowState::kPendingEula:
            case ContentInstallFlowState::kCanceling:
                return kProgressState_Indeterminate;
                break;
            case ContentInstallFlowState::kInitializing:
            case ContentInstallFlowState::kPreInstall:
            case ContentInstallFlowState::kInstalling:
            case ContentInstallFlowState::kPostInstall:
            case ContentInstallFlowState::kPostTransfer:
                // Sometimes an install doesn't have progress. If that's the case, show
                // an indeterminate bar.
                if(mProgress == 0.0f || mProgress == 1.0f)
                {
                    return kProgressState_Indeterminate;
                }
                break;
            default:
                break;
            }
            return kProgressState_Active;
        }

        QString InstallProgressState::progressStateCode(const ContentInstallFlowState& flowState) const
        {
            switch(progressState(flowState))
            {
            case kProgressState_Active:
                return "State-Active";
                break;
            case kProgressState_Indeterminate:
                return "State-Indeterminate";
                break;
            case kProgressState_Paused:
                return "State-Paused";
                break;
            default:
                break;
            }
            return "";
        }

        LastActionInfo::LastActionInfo()
        {
            state = kOperation_None;
            totalBytes = 0;
            flowState = ContentInstallFlowState::kInvalid;
            isDynamicDownload = false;
        }

		InstallArgumentRequest::InstallArgumentRequest()
		{
			contentDisplayName = "";
            productId = "";
			installedSize = 0;
            downloadSize = 0;
			isLocalSource = false;
            isITO = false;
			showShortCut = false;
			nextDiscNum = 0;
			totalDiscNum = 0;
            isPreload = false;
            isPdlc = false;
            isUpdate = false;
            useDefaultInstallOptions = false;
		}

		InstallLanguageRequest::InstallLanguageRequest()
		{
            productId = "";
            showPreviousInstallLanguageWarning = false;
		}

		EulaStateRequest::EulaStateRequest()
		{
			isLocalSource = false;
            isUpdate = false;
            isPreload = false;
            productId = "";
		}

		InstallArgumentResponse::InstallArgumentResponse()
		{
			optionalComponentsToInstall = 0;
			installDesktopShortCut = false;
			installStartMenuShortCut = false;
		}

		EulaLanguageResponse::EulaLanguageResponse()
		{
			selectedLanguage = "";
		}

		EulaStateResponse::EulaStateResponse()
		{
			acceptedBits = 0;
		}

        CancelDialogRequest::CancelDialogRequest()
        {
            contentDisplayName = "";
            productId = "";
            isIGO = false;
            state = kOperation_None;
        }

} // namespace Downloader
} // namespace Origin

