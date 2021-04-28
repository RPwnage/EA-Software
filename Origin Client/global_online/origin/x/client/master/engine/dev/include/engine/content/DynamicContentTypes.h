///////////////////////////////////////////////////////////////////////////////
// DynamicContentTypes.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef DYNAMICCONTENTTYPES_H
#define DYNAMICCONTENTTYPES_H

#include <QString>
#include <QVector>

namespace Origin
{
namespace Engine
{
namespace Content
{
    enum DynamicDownloadChunkStateT
    {
        kDynamicDownloadChunkState_Unknown,
        kDynamicDownloadChunkState_Paused,
        kDynamicDownloadChunkState_Queued,
        kDynamicDownloadChunkState_Downloading,
        kDynamicDownloadChunkState_Installing,
        kDynamicDownloadChunkState_Installed,
        kDynamicDownloadChunkState_Busy,
        kDynamicDownloadChunkState_Error
    };

    QString DynamicDownloadChunkStateToString(DynamicDownloadChunkStateT state);

    DynamicDownloadChunkStateT DynamicDownloadChunkStateFromString(const QString& sState);

    enum DynamicDownloadChunkRequirementT
    {
        kDynamicDownloadChunkRequirementUnknown = 0,
        kDynamicDownloadChunkRequirementLowPriority,
        kDynamicDownloadChunkRequirementRecommended,
        kDynamicDownloadChunkRequirementRequired,   
        kDynamicDownloadChunkRequirementDemandOnly,
        kDynamicDownloadChunkRequirementDynamic
    };

    QString DynamicDownloadChunkRequirementToString(DynamicDownloadChunkRequirementT requirement);

    DynamicDownloadChunkRequirementT DynamicDownloadChunkRequirementFromString(const QString& sRequirement);

    QVector<DynamicDownloadChunkRequirementT> DynamicDownloadChunkRequirementDefaultOrder();

}//Content
}//Engine
}//Origin
#endif