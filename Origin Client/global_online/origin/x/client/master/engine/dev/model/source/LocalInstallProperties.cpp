// Download In Place Manifest information class

#include "LocalInstallProperties.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include <QXmlErrorHandler>
#include "TelemetryAPIDLL.h"

class QXmlInputSourceNBSPEater : public QXmlInputSource
{
public:
    QXmlInputSourceNBSPEater(QIODevice* dev) : QXmlInputSource(dev) {}

    QChar next()
    {
        QChar c = QXmlInputSource::next();
        if (c == 0xA0)      // if it's a nbsp
            return 0x20;    // return a regular space
        return c;
    }
};


namespace Origin
{
    namespace Downloader
    {

        void QXmlErrorHandlerOrigin::setLoggingEnabled(bool enabled)
        {
            mEnabled = enabled;
        }

        void QXmlErrorHandlerOrigin::setPath(QString path)
        {
            mPath = path;
        }

        QString	QXmlErrorHandlerOrigin::errorString() const 
        { 
            return ""; 
        }

        bool QXmlErrorHandlerOrigin::error( const QXmlParseException & exception )
        { 
            if (!mEnabled)
                return true;

            QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseError", sError.toUtf8().data());

            ORIGIN_LOG_ERROR << "Manifest Parse Error: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 
            return true; 
        }

        bool QXmlErrorHandlerOrigin::fatalError( const QXmlParseException & exception ) 
        { 
            if (!mEnabled)
                return true;

            QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseFatalError", sError.toUtf8().data());

            ORIGIN_LOG_ERROR << "Manifest Parse FatalError: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 
            return false; 
        }

        bool QXmlErrorHandlerOrigin::warning( const QXmlParseException & exception ) 
        { 
            if (!mEnabled)
                return true;

            QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseWarning", sError.toUtf8().data());

            ORIGIN_LOG_WARNING << "Manifest Parse Warning: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 

            return true; 
        }



        LocalInstallProperties::LocalInstallProperties()
        {
            Clear();
        }

        bool LocalInstallProperties::Parse(QFile &manifestFile, bool isDLC)
        {  
            QMutexLocker locker(&mMutex);

            Clear();

            mIsDLC = isDLC;

            mErrorHandler.setPath(manifestFile.fileName());
            mErrorHandler.setLoggingEnabled(true);

            // Create a reader and point it at ourselves
            QXmlSimpleReader xmlReader;
            xmlReader.setContentHandler(this);
            xmlReader.setErrorHandler(&mErrorHandler);

            // Read the XML in
            QXmlInputSourceNBSPEater fileSource(&manifestFile);
            bool result = xmlReader.parse(fileSource);

            // If we were DiP4, extract the BuildMetaData block so it can be parsed separately
            if (mbParsingDip4Plus)
            {
                manifestFile.reset();
                QByteArray xmlData = manifestFile.readAll();
                QString sXmlData(xmlData);
                
                // This is the main work of extracting the DiP4-specific data, we don't want to duplicate this code
                result = mSoftwareBuildMetadata.ParseSoftwareBuildMetadataBlock(xmlData, manifestFile.fileName());
            }

            // We either reached the end of the file or finished early
            if ((mbFinishedEarly || result) && mbSawGameElement)
            {
                return true;
            }

            ORIGIN_ASSERT(false);
            
            Clear();
            return false;
        }

        bool LocalInstallProperties::Load(const QString& sFileName, bool isDLC)
        {
            ORIGIN_LOG_EVENT << "Loading LocalInstallProperties: " << sFileName;
            QFile manifestFile(sFileName);

	        if (manifestFile.open(QIODevice::ReadOnly))
	        {
		        Parse(manifestFile, isDLC);
		        return true;
	        }

	        ORIGIN_LOG_ERROR << "LocalInstallProperties::fromFile failed to load: " << sFileName;
	        Clear();

            return false;
        }

        void LocalInstallProperties::Clear()
        {
            mIsDLC = false;

            // Clear the metadata block
            mSoftwareBuildMetadata.Clear();

	        mManifestVersion	= VersionInfo(0, 0, 0, 0);
	        
            mLaunchers.clear();

            // Parser state
            mbFinishedEarly = false;
            mbSawGameElement = false;
            mbParsingDip4Plus = false;
            mLauncherBeingParsed.Clear();
            msLauncherLanguageBeingParsed = "";
            mNodePath.clear();

            msAvailableLocales.clear();
        }

        void LocalInstallProperties::SetFeatureFlagDefaultsBasedOnManifestVersion()
        {
            // Forward this call to the 'authoritative' source for these defaults
            mSoftwareBuildMetadata.SetFeatureFlagDefaultsBasedOnManifestVersion(mManifestVersion, mIsDLC);
        }

        bool LocalInstallProperties::FlagStringToBool(const QString& sValue)
        {
	        if (sValue == "1")
		        return true;

	        return sValue.compare("true", Qt::CaseInsensitive) == 0;
        }

        bool LocalInstallProperties::endElement(const QString&, const QString& name, const QString& qName)
        {
            QString sParsingPath(currentNodePath());
            //ORIGIN_LOG_DEBUG << "endElement path:" << sParsingPath;

            if (sParsingPath == "game.runtime.launcher" || sParsingPath == "dipmanifest.runtime.launcher")
            {
                mLaunchers.push_back(mLauncherBeingParsed);
                mLauncherBeingParsed.Clear();     
            }
            else if (mbParsingDip4Plus)
            {
                // Handle all the rest of the DiP 4+ parsing here
                if (!endElementDiP4(name, qName))
                    return false;
            }


            QString sPopping = popPath();
            //ORIGIN_LOG_DEBUG << "name: " << name << " popping: " << sPopping;

            ORIGIN_ASSERT(sPopping.compare(name, Qt::CaseInsensitive) == 0);  // Better be popping the element we expect to be
            return true;
        }
            
        bool LocalInstallProperties::startElement(const QString &, const QString &name, const QString &, const QXmlAttributes &attrs)
        {
            //ORIGIN_LOG_DEBUG << "pushing: " << name;
            pushPath(name);
            QString sParsingPath(currentNodePath());
            //ORIGIN_LOG_DEBUG << "startElement path:" << sParsingPath;

            if (sParsingPath == "dipmanifest")
            {
                mbParsingDip4Plus = true;

                // We've seen the game (in this case, DiPManifest) element (this is required)
                mbSawGameElement = true;

                // Grab the manifest version while we're here
                mManifestVersion = VersionInfo(attrs.value("version"));

                // Set our feature flag defaults
                SetFeatureFlagDefaultsBasedOnManifestVersion();
            }
            else if (sParsingPath == "game")
            {
                // Harvest Game & Manifest Versions from game Element
                mSoftwareBuildMetadata.mGameVersion = VersionInfo(attrs.value("gameVersion"));
                mManifestVersion = VersionInfo(attrs.value("manifestVersion"));
                
                // Set our feature flags
                SetFeatureFlagDefaultsBasedOnManifestVersion();	// Defaults based on mManifestVersion

                // We've seen the game element (this is required)
                mbSawGameElement = true;
                
                if (mManifestVersion < VersionInfo(2, 0, 0, 0))
                {
                    // Manifest versions before 2 don't have feature flags
                    mbFinishedEarly = true;
                    mErrorHandler.setLoggingEnabled(false);     // stop logging anything further
                    return false;
                }
            }
            else if (sParsingPath == "game.metadata.featureflags")
            {
                // Definition of attribute keys
                const QString sAutoUpdateEnabledKey("autoUpdateEnabled");
                const QString sUseGameVersionFromManifestEnabledKey("useGameVersionFromManifestEnabled");
                const QString sExecuteElevatedKey("executeElevated");
                const QString sTreatUpdatesAsMandatory("treatUpdatesAsMandatory");

                if (!attrs.value(sAutoUpdateEnabledKey).isEmpty())
                {
                    mSoftwareBuildMetadata.mbAutoUpdateEnabled	= FlagStringToBool(attrs.value(sAutoUpdateEnabledKey));
                }

                if (!attrs.value(sUseGameVersionFromManifestEnabledKey).isEmpty())
                {
                    mSoftwareBuildMetadata.mbUseGameVersionFromManifestEnabled	= FlagStringToBool(attrs.value(sUseGameVersionFromManifestEnabledKey));
                }

                if (!attrs.value(sExecuteElevatedKey).isEmpty())
                {
                    mSoftwareBuildMetadata.mbExecuteGameElevated = FlagStringToBool(attrs.value(sExecuteElevatedKey));
                }

                if (!attrs.value(sTreatUpdatesAsMandatory).isEmpty())
                {
                    mSoftwareBuildMetadata.mbTreatUpdatesAsMandatory = FlagStringToBool(attrs.value(sTreatUpdatesAsMandatory));
                }

                if (mManifestVersion < VersionInfo(2, 2, 0, 0))
                {
                    // Manifest versions before 2.2 don't need to read anything further (runtime and uninstall nodes)
                    mbFinishedEarly = true;
                    mErrorHandler.setLoggingEnabled(false);     // stop logging anything further
                    return false;
                }
            }
            else if (sParsingPath == "game.metadata.localeinfo")
            {
                const QString sLocale("locale");

                if (!attrs.value(sLocale).isEmpty())
                    msAvailableLocales.append(attrs.value(sLocale));
            }
            else if (sParsingPath == "game.runtime.launcher" || sParsingPath == "dipmanifest.runtime.launcher")
            {
                mLauncherBeingParsed.Clear();
            }
            else if (sParsingPath == "game.runtime.launcher.name" || sParsingPath == "dipmanifest.runtime.launcher.name")
            {
                msLauncherLanguageBeingParsed = "en_US";    // default
                if (!attrs.value("locale").isEmpty())
                    msLauncherLanguageBeingParsed = attrs.value("locale");
            }
            else if (mbParsingDip4Plus)
            {
                // Handle all the rest of the DiP 4+ parsing here
                if (!startElementDiP4(name, attrs))
                    return false;
            }

            return true;
        }


        bool LocalInstallProperties::characters(const QString& chars)
        {
            QString sParsingPath(currentNodePath());
            //ORIGIN_LOG_DEBUG << "characters path:" << sParsingPath;

            if (sParsingPath == "game.uninstall.path" || sParsingPath == "dipmanifest.uninstall.path")
            {
                ORIGIN_LOG_DEBUG << "Found uninstall path: " << chars;
                msUninstallPath = chars;
            }
            else if (sParsingPath == "game.runtime.launcher.name" || sParsingPath == "dipmanifest.runtime.launcher.name")
            {
                ORIGIN_LOG_DEBUG << "Found launcher for locale: " << msLauncherLanguageBeingParsed << " name: " << chars;
                mLauncherBeingParsed.mLanguageToCaptionMap[msLauncherLanguageBeingParsed] = chars;
            }
            else if (sParsingPath == "game.runtime.launcher.filepath" || sParsingPath == "dipmanifest.runtime.launcher.filepath")
            {
                ORIGIN_LOG_DEBUG << "Found launcher filepath:" << chars;
                mLauncherBeingParsed.msPath = chars;
            }
            else if (sParsingPath == "game.runtime.launcher.parameters" || sParsingPath == "dipmanifest.runtime.launcher.parameters")
            {
                ORIGIN_LOG_DEBUG << "Found launcher parameters:" << chars;
                mLauncherBeingParsed.msParameters = chars;
            }
            else if (sParsingPath == "game.runtime.launcher.executeelevated" || sParsingPath == "dipmanifest.runtime.launcher.executeelevated")
            {
                ORIGIN_LOG_DEBUG << "Found launcher elevation tag:" << chars;
                mLauncherBeingParsed.mbExecuteElevated = FlagStringToBool(chars);
            }
            else if (sParsingPath == "game.runtime.launcher.requires64bitos" || sParsingPath == "dipmanifest.runtime.launcher.requires64bitos")
            {
                ORIGIN_LOG_DEBUG << "Found launcher requires64BitOS tag:" << chars;
                mLauncherBeingParsed.mbRequires64BitOS = FlagStringToBool(chars);
            }
            else if (sParsingPath == "game.runtime.launcher.trial" || sParsingPath == "dipmanifest.runtime.launcher.trial")
            {
                ORIGIN_LOG_DEBUG << "Found launcher trial tag:" << chars;
                mLauncherBeingParsed.mbIsTimedTrial = FlagStringToBool(chars);
            }
            else if(sParsingPath == "game.contentids.contentid" || sParsingPath == "dipmanifest.contentids.contentid")
            {
                ORIGIN_LOG_DEBUG << "Found contentId:" << chars;
                mContentIds.push_back(chars);
            }
            else if (mbParsingDip4Plus)
            {
                // Handle all the rest of the DiP 4+ parsing here
                if (!charactersDiP4(chars))
                    return false;
            }

            return true;
        }

        bool LocalInstallProperties::startElementDiP4(const QString & name, const QXmlAttributes & attrs)
        {
            QString sParsingPath(currentNodePath());
            
            // Nothing DiP4-specific here yet

            // Most of the real work here is done by SoftwareBuildMetadata's parse method, which we do not want to duplicate here

            return true;
        }

        bool LocalInstallProperties::endElementDiP4(const QString& name, const QString& qName)
        {
            // Nothing DiP4-specific here yet
            return true;
        }

        bool LocalInstallProperties::charactersDiP4(const QString& chars)
        {
            QString sParsingPath(currentNodePath());

            if (sParsingPath == "dipmanifest.installmetadata.locales")
            {
                ORIGIN_LOG_DEBUG << "Found DiP4 locale list: " << chars;

                QStringList locales = chars.split(",", QString::SkipEmptyParts);
                if (locales.size() > 0)
                    msAvailableLocales.append(locales);
            }

            return true;
        }

        QString LocalInstallProperties::currentNodePath()
        {
            QString sPath;
            for (QStack<QString>::iterator it = mNodePath.begin(); it != mNodePath.end(); it++)
            {
                sPath += *it;
                sPath += ".";
            }

            if (sPath.isEmpty())
                return "";

            return sPath.left( sPath.length() - 1 );        // trim off the last .
        }
    }
}
