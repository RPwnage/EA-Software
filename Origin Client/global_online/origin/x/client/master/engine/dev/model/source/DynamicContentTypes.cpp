#include "engine/content/DynamicContentTypes.h"

namespace Origin
{
namespace Engine
{
namespace Content
{
    QString DynamicDownloadChunkStateToString(DynamicDownloadChunkStateT state)
    {
        switch (state)
        {
            case kDynamicDownloadChunkState_Paused:
                return "Paused";
            case kDynamicDownloadChunkState_Queued:
                return "Queued";
            case kDynamicDownloadChunkState_Downloading:
                return "Downloading";
            case kDynamicDownloadChunkState_Installing:
                return "Installing";
            case kDynamicDownloadChunkState_Installed:
                return "Installed";
            case kDynamicDownloadChunkState_Busy:
                return "Busy";
            case kDynamicDownloadChunkState_Error:
                return "Error";
            case kDynamicDownloadChunkState_Unknown:
            default:
                return "Unknown";
        }
    }

    DynamicDownloadChunkStateT DynamicDownloadChunkStateFromString(const QString& sState)
    {
        if (sState.compare("Paused") == 0)
            return kDynamicDownloadChunkState_Paused;
        if (sState.compare("Queued") == 0)
            return kDynamicDownloadChunkState_Queued;
        if (sState.compare("Downloading") == 0)
            return kDynamicDownloadChunkState_Downloading;
        if (sState.compare("Installing") == 0)
            return kDynamicDownloadChunkState_Installing;
        if (sState.compare("Installed") == 0)
            return kDynamicDownloadChunkState_Installed;
        if (sState.compare("Busy") == 0)
            return kDynamicDownloadChunkState_Busy;
        if (sState.compare("Error") == 0)
            return kDynamicDownloadChunkState_Error;
        return kDynamicDownloadChunkState_Unknown;
    }

    QString DynamicDownloadChunkRequirementToString(DynamicDownloadChunkRequirementT requirement)
    {
        switch (requirement)
        {
            case kDynamicDownloadChunkRequirementLowPriority:
                return "low";
            case kDynamicDownloadChunkRequirementRecommended:
                return "recommended";
            case kDynamicDownloadChunkRequirementRequired: 
                return "required";
            case kDynamicDownloadChunkRequirementDemandOnly:
                return "demand_only";
            case kDynamicDownloadChunkRequirementDynamic:
                return "dynamic";
            case kDynamicDownloadChunkRequirementUnknown:
            default:
                return "Unknown";
        }
    }

    DynamicDownloadChunkRequirementT DynamicDownloadChunkRequirementFromString(const QString& sRequirement)
    {
        if (sRequirement.isEmpty() || sRequirement.compare("low", Qt::CaseInsensitive) == 0)
        {
            return kDynamicDownloadChunkRequirementLowPriority;
        }
        else if (sRequirement.compare("required", Qt::CaseInsensitive) == 0)
        {
            return kDynamicDownloadChunkRequirementRequired;
        }
        else if (sRequirement.compare("recommended", Qt::CaseInsensitive) == 0)
        {
            return kDynamicDownloadChunkRequirementRecommended;
        }
        else if (sRequirement.compare("demand_only", Qt::CaseInsensitive) == 0)
        {
            return kDynamicDownloadChunkRequirementDemandOnly;
        }
        // We do not allow 'dynamic' chunks specified in the DiP manifest

        return kDynamicDownloadChunkRequirementUnknown;
    }

    QVector<DynamicDownloadChunkRequirementT> DynamicDownloadChunkRequirementDefaultOrder()
    {
        QVector<DynamicDownloadChunkRequirementT> requirementPriority;

        requirementPriority.push_back(kDynamicDownloadChunkRequirementRequired);
        requirementPriority.push_back(kDynamicDownloadChunkRequirementRecommended);
        requirementPriority.push_back(kDynamicDownloadChunkRequirementLowPriority);
        requirementPriority.push_back(kDynamicDownloadChunkRequirementDemandOnly);
        // Once supported, demand-only chunks will not be downloaded by default, and so in the future they won't have a default priority
        // Dynamic chunks can only be created on demand, they don't have a default priority (they go first when they are requested)

        return requirementPriority;
    }
}//Content
}//Engine
}//Origin