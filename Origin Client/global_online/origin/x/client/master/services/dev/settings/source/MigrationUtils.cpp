//
//  MigrationUtils.cpp
//
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.
//

#include "MigrationUtils.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/log/LogService.h"

#include <QSettings>

#ifdef _WIN32
#include "Shlobj.h" // for SHGetSpecialFolderPath
#include <EAIO/EAIniFile.h>
#include "wchar.h"
#endif

namespace Origin
{
    namespace Services
    {
		static const QString EAD_REG_ACCESS_STAGING_KEY  = ("SOFTWARE\\Origin Games");	//actual key will have contentID appended

		typedef struct
		{
			QString mLocaleKey;
			QString mLocale;
			QString mVersionKey;
			QString mVersion;
			QString mFolderKey;
			QString mFolder;
		} VisitSectionData;

		bool VisitSectionEntries(const wchar_t* key, const wchar_t* value, void* pContext)
		{
			VisitSectionData* data = static_cast<VisitSectionData*>(pContext);

			QString keyStr = QString::fromWCharArray(key);
			
			if(keyStr.contains(data->mLocaleKey))
			{
				data->mLocale = QString::fromWCharArray(value);
			}
			else if(keyStr.contains(data->mVersionKey))
			{
				data->mVersion = QString::fromWCharArray(value);
			}
			else if(keyStr.contains(data->mFolderKey))
			{
				data->mFolder = QString::fromWCharArray(value);
			}

			return (data->mLocale.isEmpty() || data->mVersion.isEmpty() || data->mFolder.isEmpty());
		}

		bool MigrationUtils::getLegacyOriginIniContentData(const QString& contentId, QString& installedLocale, QString& installedVersion, QString& installedFolder)
		{
// No migration possible here on any other platform
#ifdef _WIN32
			// get local application data directory
			TCHAR sBaseDirBuffer[MAX_PATH+1]; 
			SHGetSpecialFolderPath(NULL, sBaseDirBuffer, CSIDL_COMMON_APPDATA, false);
			QString sBaseDir = QString::fromWCharArray(sBaseDirBuffer);

			// Check ending slashes for OS compatibility ?
			if (!sBaseDir.endsWith("\\"))
				sBaseDir.append("\\");

			QString sLegacyIniFilename = QString("%1Electronic Arts\\EA Core\\prefs\\ORIGIN.ini").arg(sBaseDir);
			EA::IO::IniFile iniFile(sLegacyIniFilename.toStdWString().c_str());
			
			if(iniFile.SectionExists(L"EACore"))
			{
				VisitSectionData visitData;
				visitData.mLocaleKey = QString(":;%1_locale").arg(contentId);
				visitData.mFolderKey = QString(":;%1_install_location").arg(contentId);
				visitData.mVersionKey = QString(":;%1_lastcheckedversion").arg(contentId);

				iniFile.EnumEntries(L"EACore", VisitSectionEntries, &visitData);

				if(visitData.mLocale.isEmpty())
				{
					readInstalledLocaleFromRegistry(contentId, visitData.mLocale);
				}

				if(!(visitData.mLocale.isEmpty() && visitData.mVersion.isEmpty() && visitData.mFolder.isEmpty()))
				{
					installedLocale = visitData.mLocale;
					installedFolder = visitData.mFolder;
					installedVersion = visitData.mVersion;
					return true;
				}
			}
#endif
			return false;
		}

		// This function matches how 8.x gets locale if it's not present in CorePreferences
		void MigrationUtils::readInstalledLocaleFromRegistry(const QString& contentId, QString& localeOut)
		{
			// attempt 2: read it from the actual registry entry
			QString regKey ("HKEY_LOCAL_MACHINE\\" + EAD_REG_ACCESS_STAGING_KEY);
			regKey.append("\\");
			regKey.append(contentId);

            // Simply declaring QSettings causes the 'regKey' to be created.
            // Since we don't want to create a registry entry if it doesn't already exist,
            // check for existence first.
            if( PlatformService::OSPathExists(regKey) )
            {
			    QSettings settings(regKey, QSettings::NativeFormat);
			    localeOut = settings.value("Locale").toString();
            }
            else
            {
                localeOut.clear();
            }

			if (localeOut.isEmpty())
			{
				ORIGIN_LOG_WARNING << "Couldn't find associated locale for the content item: " << contentId << " ... Possibly due to Origin being uninstalled/reinstalled mid-download...getting general";
				
				localeOut = Origin::Services::readSetting(SETTING_LOCALE).toString();

				// If we couldn't find an associated locale for this content item, perhaps it got removed from Origin.ini and we need to get the generic one
				if (localeOut.isEmpty())
				{
					// attempt 3: default to en_US
					ORIGIN_LOG_WARNING << "GetAssociatedLocale couldn't get a general locale!!!... defaulting to en_US";
					localeOut = "en_US";
				}
			}
		}
	}
}