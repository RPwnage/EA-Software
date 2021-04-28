#ifndef _ACHIEVEMENTREKEASEINFOPROXY_H
#define _ACHIEVEMENTREKEASEINFOPROXY_H

/**********************************************************************************************************
* This class is part of Origin's JavaScript bindings and is not intended for use from C++
*
* All changes to this class should be reflected in the documentation in jsinterface/doc
*
* See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
* ********************************************************************************************************/

#include <QObject>
#include <QPointer>
#include <engine/achievements/achievementsetreleaseinfo.h>

namespace Origin
{
	namespace Client
	{
		namespace JsInterface
		{
			class AchievementSetReleaseInfoProxy : public QObject
			{
				Q_OBJECT
				friend class AchievementManagerProxy;
			public:
				Q_PROPERTY(QString achievementSetId READ achievementSetId);
				QString achievementSetId() const { return mpReleaseInfo != NULL ? mpReleaseInfo->achievementSetId() : ""; } ///< The achievementSetId associated with this release info.

				Q_PROPERTY(QString masterTitleId READ masterTitleId);
				QString masterTitleId() const { return mpReleaseInfo != NULL ? mpReleaseInfo->masterTitleId() : ""; } ///< The masterTitle of the game associated with this release info.

				Q_PROPERTY(QString displayName READ displayName);
				QString displayName() const { return mpReleaseInfo != NULL ? mpReleaseInfo->displayName() : ""; } ; ///< The game display name associated with this release info.

				Q_PROPERTY(QString platform READ platform);
				QString platform() const { return mpReleaseInfo != NULL ? mpReleaseInfo->platform() : ""; } ; ///< The platform associated with this release info.
			private:
				explicit AchievementSetReleaseInfoProxy(const Origin::Engine::Achievements::AchievementSetReleaseInfo * achievementSetReleaseInfo, QObject *parent) : 
						QObject(parent),
						mpReleaseInfo(achievementSetReleaseInfo)
					{
					}

			private:
				const Origin::Engine::Achievements::AchievementSetReleaseInfo * mpReleaseInfo;
			};
		}
	}
}

#endif //_ACHIEVEMENTREKEASEINFOPROXY_H