///////////////////////////////////////////////////////////////////////////////
/// \file       achievementreleaseinfo.h
/// This file contains the declaration of an AchievementReleaseInfo. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2014
// \copyright  Copyright (c) 2014 Electronic Arts. All rights reserved.

#ifndef __SDK_ACHIEVEMENT_RELEASE_INFO_H__
#define __SDK_ACHIEVEMENT_RELEASE_INFO_H__

#include <QObject>

namespace Origin
{
	namespace Engine
	{
		namespace Achievements
		{
			class AchievementSetReleaseInfo
			{
			public:
				AchievementSetReleaseInfo(const QString &achievementSetId, const QString &masterTitleId, const QString &displayName, const QString &platform) : 
					mAchievementSetId(achievementSetId),
					mMasterTitleId(masterTitleId),
					mDisplayName(displayName),
					mPlatform(platform)
				{

				}

				const QString &achievementSetId() const { return mAchievementSetId; }
				const QString &masterTitleId() const { return mMasterTitleId; }
				const QString &displayName() const { return mDisplayName; }
				const QString &platform() const { return mPlatform; }

			private:
				QString mAchievementSetId;
				QString mMasterTitleId;
				QString mDisplayName;
				QString mPlatform;
			};

			typedef QList<AchievementSetReleaseInfo> AchievementSetReleaseInfoList;
		}
	}
}

#endif //__SDK_ACHIEVEMENT_RELEASE_INFO_H__