//  MigrationUtils.h
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.

#ifndef _MIGRATION_UTILS_H
#define _MIGRATION_UTILS_H

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
		class ORIGIN_PLUGIN_API MigrationUtils
		{
		public:
			static bool getLegacyOriginIniContentData(const QString& contentId, QString& installedLocale, QString& installedVersion, QString& installedFolder);

		private:
			static void readInstalledLocaleFromRegistry(const QString& contentId, QString& localeOut);
		};
	}
}

#endif
