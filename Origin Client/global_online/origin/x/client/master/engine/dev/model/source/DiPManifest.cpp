// Download In Place Manifest information class

#include "DiPManifest.h"
#include "services/downloader/StringHelpers.h"
#include "services/log/LogService.h"
#include <algorithm>
#include <QtXML/QDomNodeList>
#include <QtXML/QDomDocument>
#include <QDebug>
#include "EAIO/FnMatch.h"
#include <QBuffer>
#include "TelemetryAPIDLL.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Downloader
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Helper to check for a file/folder name in a list of wildcards.
        bool SearchWildcards(const QString& sFileOrFolder, const QStringList::const_iterator& begin, const QStringList::const_iterator& end)
        {
            QString sNameToCheck(sFileOrFolder);
	        sNameToCheck = sNameToCheck.toLower();
	        sNameToCheck.replace("\\", "/");
            
	        for (QStringList::const_iterator it = begin; it != end; it++)
	        {
		        // If the leftmost n characters (folder path length) of sNameToCheck match, then this file or folder is excluded
		        QString sPattern = *it;
                if (sPattern.right(1) == "/")     // this is a folder so add a * for wildcard matching
                    sPattern += "*";
                if (EA::IO::FnMatch(sPattern.toStdWString().c_str(), sFileOrFolder.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname))
                    return true;
	        }
            
	        return false;    
        }
        
        // Helper to extract wildcard from Xml element.
        QString ExtractWildcard(const QDomElement& element)
        {
            QString sValue (element.text());
            if (!sValue.isEmpty ())
            {
                sValue.replace("\\", "/");		// ensure only forward slashes
                sValue = sValue.toLower();
                if (sValue.left(1) == "/")		// Strip any leading slash as ZIP file paths don't start with one
                    sValue = sValue.right( sValue.length()-1 );
                
                //  if (sValue.right(1) == "/")     // this is a folder so add a * for wildcard matching
                //      sValue += "*";
            }
            
            return sValue;
        }
        
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        LocalizedInfo::LocalizedInfo()
        {
        }

        LocalizedInfo::LocalizedInfo(const LocalizedInfo& other)
        {
	        *this = other;
        }

        LocalizedInfo& LocalizedInfo::operator=(const LocalizedInfo& other)
        {
	        msTitle = other.msTitle;

	        mEulas = other.mEulas;

	        //	Exclusions
            mWildcardExcludes = other.mWildcardExcludes;
	        mOptComponents = other.mOptComponents;

            mDownloadRequired = other.mDownloadRequired;

	        return *this;
        }

        ProgressiveInstallChunk::ProgressiveInstallChunk() :
            mChunkID(0),
            mChunkRequirement(Origin::Engine::Content::kDynamicDownloadChunkRequirementLowPriority)
        {

        }

        ProgressiveInstallChunk::ProgressiveInstallChunk(const ProgressiveInstallChunk& other)
        {
            *this = other;
        }

        ProgressiveInstallChunk& ProgressiveInstallChunk::operator=(const ProgressiveInstallChunk& other)
        {
            mChunkID = other.mChunkID;
            mChunkRequirement = other.mChunkRequirement;
            mChunkName = other.mChunkName;
            mFilePatterns = other.mFilePatterns;

            return *this;
        }

        bool ProgressiveInstallChunk::SetChunkID(const QString& id)
        {
            bool parseOk = false;
            int intId = id.toInt(&parseOk);

            if (parseOk)
            {
                mChunkID = intId;
                return true;
            }

            return false;
        }

        bool ProgressiveInstallChunk::SetChunkRequirement(const QString& requirement)
        {
            Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement = Origin::Engine::Content::DynamicDownloadChunkRequirementFromString(requirement);
            if (chunkRequirement == Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown)
            {
                return false;
            }
            
            mChunkRequirement = chunkRequirement;

            return true;
        }

        bool FlagStringToBool(const QString& sValue)
        {
	        if (sValue == "1")
		        return true;

	        return sValue.compare("true", Qt::CaseInsensitive) == 0;
        }

        DiPManifest::DiPManifest() : mIsDLC(false)
        {
        }

        DiPManifest::DiPManifest(const DiPManifest& other)
        {
	        *this = other;
        }

        bool DiPManifest::IsEmpty() const
        {
	        return (mLocalizedInfoMap.empty() && msInstallerPath.isEmpty());
        }

        QStringList DiPManifest::GetSupportedLocales() const
        {
	        QStringList langs;

	        tLocalizedInfoMap::const_iterator it = mLocalizedInfoMap.begin();

	        while ( it != mLocalizedInfoMap.end() )
	        {
		        langs.push_back(it->first);
		        it++;
	        }

	        return langs;
        }

        // Sample
        /*
        <?xml version="1.0" encoding="utf-16" ?> 
        <game gameVersion="1.0.0.0" manifestVersion="1.0">
	        <metadata>
		        <localeInfo locale="en_US">
			        <title>Battlefield: Bad Company� 2</title> 
			        <eula name="EAEULA">/__Installer/en_US/00_en-us_eula.rtf</eula> 
			        <eula name="DirectX">/__Installer/en_US/01_en-us_eula.rtf</eula> 
			        <eula name="VCRedist">/__Installer/en_US/02_eula.1033.txt</eula> 
			        <exclude>ExcludeMe_US.txt</exclude>
		        </localeInfo>
		        <localeInfo locale="en_GB">
			        <title>Battlefield: Bad Company� 2</title> 
			        <eula name="EAEULA">/__Installer/en_GB/00_en-uk_eula.rtf</eula> 
			        <eula name="DirectX">/__Installer/en_GB/01_en-uk_eula.rtf</eula> 
			        <eula name="VCRedist">/__Installer/en_GB/02_eula.1033.txt</eula> 
		        </localeInfo>
	        </metadata>
	        <executable>
		        <filePath>/__Installer/DISK1/EASetup.exe</filePath> 
		        <parameters>-silent "INSTALLLOCATION=""{installLocation}""" -locale {locale}</parameters> 
	        </executable>
	        <installManifest>
		        <filePath>/Support/mnfst.txt</filePath> 
	        </installManifest>
        </game>
        */
        bool DiPManifest::Parse(QFile &manifestFile, bool isDLC)
        {
	        Clear();

            mIsDLC = isDLC;

	        QDomDocument oManifestXML;
	        if (!oManifestXML.setContent(&manifestFile))
	        {
		        ORIGIN_LOG_ERROR << "Error parsing DiP manifest.";
		        return false;
	        }

            // Check for and parse DiP Manifest 4.0 and newer
            QDomElement oDiPManifestHeader = oManifestXML.elementsByTagName(QString("DiPManifest")).item(0).toElement();
            if (!oDiPManifestHeader.isNull() && QDomNode::ElementNode == oDiPManifestHeader.nodeType())
            {
                // Retrieve the manifest version
                mManifestVersion = VersionInfo( oDiPManifestHeader.attribute("version") );
                if (mManifestVersion < VersionInfo(4,0))
                {
                    ORIGIN_LOG_ERROR << "Manifest version doesn't match format";
                    return false;
                }

                if (!ParseManifestV4(oDiPManifestHeader))
                    return false;
            }
            else
            {
                // DiP Manifest 3.1 and previous
	            // overall game information
	            // <game gameVersion="1.0.0.0" manifestVersion="1.0">
	            QDomElement oElement_Game = oManifestXML.elementsByTagName(QString("game")).item(0).toElement();
	            Q_ASSERT(!oElement_Game.isNull() && QDomNode::ElementNode == oElement_Game.nodeType());
	            if (oElement_Game.isNull()) 
	            {
		            ORIGIN_LOG_ERROR << "Missing required game Element in DiP manifest";
		            return false;
	            }

	            if (!ParseManifestV31Earlier(oElement_Game))
                    return false;
            }

	        return !mLocalizedInfoMap.empty();
        }

        bool DiPManifest::ParseManifestV31Earlier(const QDomElement& oElement_Game)
        {
            // Harvest Game & Manifest Versions from game Element
	        mSoftwareBuildMetadata.mGameVersion = VersionInfo( oElement_Game.attribute("gameVersion") );
	        mManifestVersion = VersionInfo( oElement_Game.attribute("manifestVersion") );

	        // Feature flags
	        mSoftwareBuildMetadata.SetFeatureFlagDefaultsBasedOnManifestVersion(mManifestVersion, mIsDLC);	// Defaults based on mManifestVersion

            //
            // content IDs
            if (!(mManifestVersion < VersionInfo("2.0.0.0")))
            {
                if (!ParseManifestContentIds(oElement_Game))
                {
                    ORIGIN_LOG_ERROR << "Unable to parse content IDs";
                    return false;
                }
            }
	        // meta data 
	        QDomNode oElement_MetaData = oElement_Game.toElement().elementsByTagName(QString("metadata")).item(0);
	        Q_ASSERT(!oElement_MetaData.isNull());
	        if (oElement_MetaData.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required metadata Element in DiP manifest";
		        return false;
	        }

            // Process ignores/deletes
            ParseIgnoreDeletes(oElement_MetaData.toElement());

	        // Process metadata child Elements
	        for (QDomElement oMetaDataChildElement = oElement_MetaData.firstChildElement(); !oMetaDataChildElement.isNull(); 
			        oMetaDataChildElement = oMetaDataChildElement.nextSiblingElement())
	        {
		        //
		        // Locale Info
		        if (0 == oMetaDataChildElement.nodeName().compare("localeInfo", Qt::CaseInsensitive))
		        {
                    const QString sDownloadRequiredKey("downloadRequired");

			        QDomElement oLocaleElement = oMetaDataChildElement.toElement();  // Just for clarity

			        LocalizedInfo lInfo;
			        lInfo.msLocale = oLocaleElement.attribute("locale");

                    if(oLocaleElement.hasAttribute(sDownloadRequiredKey))
                    {
                        lInfo.mDownloadRequired = FlagStringToBool(oLocaleElement.attribute(sDownloadRequiredKey));
                    }
                    else
                    {
                        lInfo.mDownloadRequired = false;
                    }

			        // Title
			        QDomNode oElement_LocalizedGameTitle = oLocaleElement.elementsByTagName(QString("title")).item(0);
			        if (!oElement_LocalizedGameTitle.isNull())
				        lInfo.msTitle = oElement_LocalizedGameTitle.toElement().text();

			        // EULA(s)
			        QDomNodeList lstElements_Eula = oLocaleElement.elementsByTagName("eula");
			        for (int iEulaIdx = 0; iEulaIdx < lstElements_Eula.count(); iEulaIdx++)
			        {
				        QDomElement oElement_Eula = lstElements_Eula.item(iEulaIdx).toElement();

				        EulaInfo oEulaInfo;
				        oEulaInfo.mName = oElement_Eula.attribute("name");
				        oEulaInfo.mFileName = oElement_Eula.text();
				        lInfo.mEulas.push_back(oEulaInfo);										
			        }

			        // OPTIONAL Excluded files
			        QDomNodeList lstElements_Exclude = oLocaleElement.toElement().elementsByTagName("exclude");
			        for (int iExcludeIdx = 0; iExcludeIdx < lstElements_Exclude.count(); iExcludeIdx++)
			        {
				        QDomElement oElement_Exclude = lstElements_Exclude.item(iExcludeIdx).toElement();

				        QString sValue = ExtractWildcard(oElement_Exclude);
				        if (!sValue.isEmpty())
						    lInfo.mWildcardExcludes.push_back(sValue);
			        }

			        // MY: Get optional component installs
			        QDomNodeList lstElements_OptComponents = oLocaleElement.toElement().elementsByTagName("optinstall");
			        for (int iOptCompIdx = 0; iOptCompIdx < lstElements_OptComponents.count(); iOptCompIdx++)
			        {
				        QDomElement oElement_OptComponent = lstElements_OptComponents.item(iOptCompIdx).toElement();

				        OptComponentInfo optInfo;

				        optInfo.mFlagName  = oElement_OptComponent.attribute("optFlag");

				        QDomNode oElement_optCompEula = oElement_OptComponent.elementsByTagName(QString("optinstallEula")).item(0);
				        if (!oElement_optCompEula.isNull ())
				        {
					        optInfo.mEulaName  = oElement_optCompEula.toElement().attribute ("name");
					        optInfo.mInstallName  = oElement_optCompEula.toElement().attribute ("installname");
					        //if the install name is not available use the EULA name
					        if(optInfo.mInstallName.isEmpty())
						        optInfo.mInstallName = optInfo.mEulaName;
					        optInfo.mEulaFileName = oElement_optCompEula.toElement().text();
				        }
				        QDomNode oElement_optCompCancel = oElement_OptComponent.elementsByTagName(QString("optCancelWarning")).item(0);
				        if (!oElement_optCompCancel.isNull ())
				        {
					        optInfo.mCancelWarning = oElement_optCompCancel.toElement().text();
				        }
				        else
					        optInfo.mCancelWarning.clear();

				        QDomNode oElement_optCompToolTip = oElement_OptComponent.elementsByTagName(QString("optToolTip")).item(0);
				        if (!oElement_optCompToolTip.isNull ())
				        {
					        optInfo.mToolTip = oElement_optCompToolTip.toElement().text();
				        }
				        else
					        optInfo.mToolTip.clear();


				        lInfo.mOptComponents.push_back(optInfo);
			        }

			        if ( !lInfo.IsEmpty() )
			        {
				        mLocalizedInfoMap[lInfo.msLocale] = lInfo;
			        }
		        }
		        else if (0 == oMetaDataChildElement.nodeName().compare("OS", Qt::CaseInsensitive))
		        {
			        // manifest includes a minimum OS version, record it here
			        mSoftwareBuildMetadata.mMinimumRequiredOS = VersionInfo( oMetaDataChildElement.attribute("minVersion") );
		        }
                else if (0 == oMetaDataChildElement.nodeName().compare("featureFlags", Qt::CaseInsensitive))
                {
                    ParseFeatureFlags(oMetaDataChildElement);
                }
	        }

	        //
	        // Executable information
	        QDomElement oElement_Executable = oElement_Game.elementsByTagName(QString("executable")).item(0).toElement();
	        if (!oElement_Executable.isNull()) 
	        {
                ParseTouchupBlock(oElement_Executable);
	        }
            else
            {
                ORIGIN_LOG_WARNING << "Missing required executable Element in DiP manifest";
            }


            // Uninstall Element?
            QDomElement elementUninstall = oElement_Game.elementsByTagName(QString("uninstall")).item(0).toElement();
            if (!elementUninstall.isNull())
            {
                QDomElement elementUninstallPath = elementUninstall.elementsByTagName(QString("path")).item(0).toElement();
                if (!elementUninstall.isNull())
                    msUninstallPath = elementUninstallPath.text();
            }

            // Runtime
            ParseRuntimeBlock(oElement_Game);

            return true;
        }

        bool DiPManifest::ParseManifestV4(const QDomElement& oDiPManifestHeader)
        {
            QDomElement oElement_SoftwareBuildMetadata = oDiPManifestHeader.elementsByTagName(QString("buildMetaData")).item(0).toElement();
            Q_ASSERT(!oElement_SoftwareBuildMetadata.isNull() && QDomNode::ElementNode == oElement_SoftwareBuildMetadata.nodeType());
            if (oElement_SoftwareBuildMetadata.isNull())
            {
                ORIGIN_LOG_ERROR << "Missing build metadata block";
                return false;
            }

            // Set feature flags defaults
	        mSoftwareBuildMetadata.SetFeatureFlagDefaultsBasedOnManifestVersion(mManifestVersion, mIsDLC);	// Defaults based on mManifestVersion

            // Serialize the build metadata block as text
            QString softwareBuildMetadataText;
            QTextStream softwareBuildMetadataTextStream(&softwareBuildMetadataText);
            oElement_SoftwareBuildMetadata.save(softwareBuildMetadataTextStream, 0);

            if (!mSoftwareBuildMetadata.ParseSoftwareBuildMetadataBlock(softwareBuildMetadataText, mFileName))
            {
                ORIGIN_LOG_ERROR << "Unable to build metadata block";
                return false;
            }

            if (!ParseManifestContentIds(oDiPManifestHeader))
            {
                ORIGIN_LOG_ERROR << "Unable to parse content IDs";
                return false;
            }

            // install metadata 
	        QDomElement oElement_InstallMetaData = oDiPManifestHeader.elementsByTagName(QString("installMetaData")).item(0).toElement();
	        Q_ASSERT(!oElement_InstallMetaData.isNull());
	        if (oElement_InstallMetaData.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required installMetadata Element in DiP manifest";
		        return false;
	        }

            // install metadata / patching 
	        QDomElement oElement_InstallMetaDataPatching = oElement_InstallMetaData.elementsByTagName(QString("patching")).item(0).toElement();
	        Q_ASSERT(!oElement_InstallMetaDataPatching.isNull());
	        if (oElement_InstallMetaDataPatching.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required installMetadata Patching Element in DiP manifest";
		        return false;
	        }

            // Progressive Install Chunks
            QDomElement oElement_InstallMetaDataProgressive = oElement_InstallMetaData.elementsByTagName(QString("progressive")).item(0).toElement();
	        if (!oElement_InstallMetaDataProgressive.isNull()) 
	        {
                for (QDomElement oProgressiveChunkElement = oElement_InstallMetaDataProgressive.firstChildElement(); !oProgressiveChunkElement.isNull(); 
			        oProgressiveChunkElement = oProgressiveChunkElement.nextSiblingElement())
	            {
		            //
		            // Chunks
		            if (0 == oProgressiveChunkElement.nodeName().compare("chunk", Qt::CaseInsensitive))
		            {
                        ProgressiveInstallChunk chunkDefinition;

                        if (chunkDefinition.SetChunkID(oProgressiveChunkElement.attribute("index"))
                            && chunkDefinition.SetChunkRequirement(oProgressiveChunkElement.attribute("required")))
                        {
                            chunkDefinition.SetChunkName(oProgressiveChunkElement.attribute("name"));

                            // Go through all the include elements beneath
                            for (QDomElement oProgressiveChunkIncludeElement = oProgressiveChunkElement.firstChildElement(); !oProgressiveChunkIncludeElement.isNull(); 
			                        oProgressiveChunkIncludeElement = oProgressiveChunkIncludeElement.nextSiblingElement())
	            
                            {
                                // Make sure we're looking at an include node
                                if (0 == oProgressiveChunkIncludeElement.nodeName().compare("include", Qt::CaseInsensitive))
		                        {
                                    QString newPattern = oProgressiveChunkIncludeElement.text();
                                    if (!newPattern.isEmpty())
                                    {
                                        chunkDefinition.AddFilePattern(newPattern);
                                    }
                                }
                            }

                            // Save the chunk definition to the list, assuming it specified at least one pattern
                            if (!chunkDefinition.HasPatterns())
                            {
                                mProgressiveInstallChunks[chunkDefinition.GetChunkID()] = chunkDefinition;
                            }
                        }
                    }
                }
	        }

            // Process ignore/deletes
            if (!ParseIgnoreDeletes(oElement_InstallMetaDataPatching))
            {
                ORIGIN_LOG_ERROR << "Unable to parse ignore/deletes";
                return false;
            }
            
            // Parse runtime block (launchers, etc)
            if (!ParseRuntimeBlock(oDiPManifestHeader))
            {
                ORIGIN_LOG_ERROR << "Unable to parse runtime block";
                return false;
            }

            // install metadata / patching 
	        QDomElement oElement_Touchup = oDiPManifestHeader.elementsByTagName(QString("touchup")).item(0).toElement();
	        Q_ASSERT(!oElement_Touchup.isNull());
	        if (oElement_Touchup.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required touchup Element in DiP manifest";
		        return false;
	        }

            if (!ParseTouchupBlock(oElement_Touchup))
            {
                ORIGIN_LOG_ERROR << "Unable to parse touchup block";
                return false;
            }

            // Uninstall Element?
            QDomElement elementUninstall = oDiPManifestHeader.elementsByTagName(QString("uninstall")).item(0).toElement();
            if (!elementUninstall.isNull())
            {
                QDomElement elementUninstallPath = elementUninstall.elementsByTagName(QString("path")).item(0).toElement();
                if (!elementUninstall.isNull())
                    msUninstallPath = elementUninstallPath.text();
            }

            // locale-specific data
            QDomElement oElement_LocaleFilters = oElement_InstallMetaData.elementsByTagName(QString("localeFilters")).item(0).toElement();
	        Q_ASSERT(!oElement_LocaleFilters.isNull());
	        if (oElement_LocaleFilters.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required localeFilters Element in DiP manifest";
		        return false;
	        }

            // locale-specific game titles
            QDomElement oElement_GameTitles = oDiPManifestHeader.elementsByTagName(QString("gameTitles")).item(0).toElement();
	        Q_ASSERT(!oElement_GameTitles.isNull());
	        if (oElement_GameTitles.isNull()) 
	        {
		        ORIGIN_LOG_ERROR << "Missing required gameTitles Element in DiP manifest";
		        return false;
	        }

            // Process gameTitles child Elements
	        for (QDomElement oGameTitlesChildElement = oElement_GameTitles.firstChildElement(); !oGameTitlesChildElement.isNull(); 
			        oGameTitlesChildElement = oGameTitlesChildElement.nextSiblingElement())
	        {
		        //
		        // Includes
		        if (0 == oGameTitlesChildElement.nodeName().compare("gameTitle", Qt::CaseInsensitive))
		        {
                    // Fetch attribute locale
                    QString sLocale = oGameTitlesChildElement.attribute("locale");

                    // Get map entry
                    LocalizedInfo &lInfo = mLocalizedInfoMap[sLocale];
                    lInfo.msLocale = sLocale;

                    // Get title
				    lInfo.msTitle = oGameTitlesChildElement.text();
                }
            }

            QSet<QString> allLanguagePatterns;

            // Process localeFilters child Elements
	        for (QDomElement oLocaleFilterChildElement = oElement_LocaleFilters.firstChildElement(); !oLocaleFilterChildElement.isNull(); 
			        oLocaleFilterChildElement = oLocaleFilterChildElement.nextSiblingElement())
	        {
		        //
		        // Includes
		        if (0 == oLocaleFilterChildElement.nodeName().compare("includes", Qt::CaseInsensitive))
		        {
                    // Fetch attribute locale
                    QString sLocale = oLocaleFilterChildElement.attribute("locale");
                    bool downloadRequired = false;

                    const QString sDownloadRequiredKey("downloadRequired");
                    if(oLocaleFilterChildElement.hasAttribute(sDownloadRequiredKey))
                    {
                        downloadRequired = FlagStringToBool(oLocaleFilterChildElement.attribute(sDownloadRequiredKey));
                    }
                    else
                    {
                        downloadRequired = false;
                    }

                    // Get map entry
                    LocalizedInfo &lInfo = mLocalizedInfoMap[sLocale];
                    lInfo.msLocale = sLocale;
                    lInfo.mDownloadRequired = downloadRequired;

                    // Process localeFilters/includes child Elements
	                for (QDomElement oLocaleFilterIncludeItem = oLocaleFilterChildElement.firstChildElement(); !oLocaleFilterIncludeItem.isNull(); 
			                oLocaleFilterIncludeItem = oLocaleFilterIncludeItem.nextSiblingElement())
	                {
		                //
		                // Include items
		                if (0 == oLocaleFilterIncludeItem.nodeName().compare("include", Qt::CaseInsensitive))
		                {
                            QString sIncludeValue = ExtractWildcard(oLocaleFilterIncludeItem);
				            if (!sIncludeValue.isEmpty())
                            {
						        lInfo.mWildcardIncludes.push_back(sIncludeValue);
                                if (!allLanguagePatterns.contains(sIncludeValue))
                                    allLanguagePatterns.insert(sIncludeValue);
                            }
                        }
                    }
                }
            }

            // Store it off as a basic set
            mAllLanguageFiles = QStringList::fromSet(allLanguagePatterns);

            // Process eulas child Elements
	        for (QDomElement oEulasElement = oElement_InstallMetaData.firstChildElement(); !oEulasElement.isNull(); 
			        oEulasElement = oEulasElement.nextSiblingElement())
	        {
                //
		        // Includes
		        if (0 == oEulasElement.nodeName().compare("eulas", Qt::CaseInsensitive))
		        {
                    // Fetch attribute locale
                    QString sLocale = oEulasElement.attribute("locale");

                    // Get map entry
                    LocalizedInfo &lInfo = mLocalizedInfoMap[sLocale];
                    lInfo.msLocale = sLocale;

                    // EULA(s)
			        QDomNodeList lstElements_Eula = oEulasElement.elementsByTagName("eula");
			        for (int iEulaIdx = 0; iEulaIdx < lstElements_Eula.count(); iEulaIdx++)
			        {
				        QDomElement oElement_Eula = lstElements_Eula.item(iEulaIdx).toElement();

				        EulaInfo oEulaInfo;
				        oEulaInfo.mName = oElement_Eula.attribute("name");
				        oEulaInfo.mFileName = oElement_Eula.text();

                        // Get installed size
                        bool parseSuccess = false;
                        qint64 installedSize = oElement_Eula.attribute("installedSize").toLongLong(&parseSuccess);
                        if (parseSuccess)
                        {
                            oEulaInfo.mInstalledSize = installedSize;
                        }

				        lInfo.mEulas.push_back(oEulaInfo);										
			        }

                    QDomNodeList lstElements_OptEulas = oEulasElement.elementsByTagName("optInstallEula");
			        for (int iOptEulaIdx = 0; iOptEulaIdx < lstElements_OptEulas.count(); iOptEulaIdx++)
			        {
				        QDomElement oElement_OptEula = lstElements_OptEulas.item(iOptEulaIdx).toElement();

                        OptComponentInfo optInfo;

				        optInfo.mFlagName      = oElement_OptEula.attribute("flag");
					    optInfo.mEulaName      = oElement_OptEula.attribute("name");
					    optInfo.mInstallName   = oElement_OptEula.attribute("installName");
                        optInfo.mCancelWarning = oElement_OptEula.attribute("cancelWarning");
				        optInfo.mToolTip       = oElement_OptEula.attribute("toolTip");

                        // Get installed size
                        bool parseSuccess = false;
                        qint64 installedSize = oElement_OptEula.attribute("installedSize").toLongLong(&parseSuccess);
                        if (parseSuccess)
                        {
                            optInfo.mInstalledSize = installedSize;
                        }

					    //if the install name is not available use the EULA name
					    if(optInfo.mInstallName.isEmpty())
						    optInfo.mInstallName = optInfo.mEulaName;

					    optInfo.mEulaFileName = oElement_OptEula.text();

				        lInfo.mOptComponents.push_back(optInfo);
                    }
                }
            }

            return true;
        }

        bool DiPManifest::ParseFeatureFlags(const QDomElement& oFeatureFlagsElement)
        {
            // Definition of attribute keys
            const QString sAutoUpdateEnabledKey("autoUpdateEnabled");
            const QString sIgnoresEnabledKey("ignoresEnabled");
            const QString sDeletesEnabledKey("deletesEnabled");
            const QString sConsolidatedEULAsEnabledKey("consolidatedEULAsEnabled");
            const QString sShortcutsEnabledKey("shortcutsEnabled");
            const QString sLanguageExcludesEnabledKey("languageExcludesEnabled");
            const QString sUseGameVersionFromManifestEnabledKey("useGameVersionFromManifestEnabled");
            const QString sForceTouchupInstallerAfterUpdateKey("forceTouchupInstallerAfterUpdate");
            const QString sExecuteGameElevatedKey("executeGameElevated");
            const QString sTreatUpdatesAsMandatory("treatUpdatesAsMandatory");
            const QString sEnableDifferentialUpdate("enableDifferentialUpdate");
            const QString sAllowMultipleInstances("allowMultipleInstances");

            if (oFeatureFlagsElement.hasAttribute(sAutoUpdateEnabledKey))
                mSoftwareBuildMetadata.mbAutoUpdateEnabled	= FlagStringToBool(oFeatureFlagsElement.attribute(sAutoUpdateEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sIgnoresEnabledKey))
                mSoftwareBuildMetadata.mbIgnoresEnabled = FlagStringToBool(oFeatureFlagsElement.attribute(sIgnoresEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sDeletesEnabledKey))
                mSoftwareBuildMetadata.mbDeletesEnabled = FlagStringToBool(oFeatureFlagsElement.attribute(sDeletesEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sConsolidatedEULAsEnabledKey))
                mSoftwareBuildMetadata.mbConsolidatedEULAsEnabled = FlagStringToBool(oFeatureFlagsElement.attribute(sConsolidatedEULAsEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sShortcutsEnabledKey))
                mSoftwareBuildMetadata.mbShortcutsEnabled = FlagStringToBool(oFeatureFlagsElement.attribute(sShortcutsEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sLanguageExcludesEnabledKey))
                mSoftwareBuildMetadata.mbLanguageExcludesEnabled = FlagStringToBool(oFeatureFlagsElement.attribute(sLanguageExcludesEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sUseGameVersionFromManifestEnabledKey))
                mSoftwareBuildMetadata.mbUseGameVersionFromManifestEnabled	= FlagStringToBool(oFeatureFlagsElement.attribute(sUseGameVersionFromManifestEnabledKey));

            if (oFeatureFlagsElement.hasAttribute(sForceTouchupInstallerAfterUpdateKey))
                mSoftwareBuildMetadata.mbForceTouchupInstallerAfterUpdate = FlagStringToBool(oFeatureFlagsElement.attribute(sForceTouchupInstallerAfterUpdateKey));

            if (oFeatureFlagsElement.hasAttribute(sExecuteGameElevatedKey))
                mSoftwareBuildMetadata.mbExecuteGameElevated = FlagStringToBool(oFeatureFlagsElement.attribute(sExecuteGameElevatedKey));

            if (oFeatureFlagsElement.hasAttribute(sTreatUpdatesAsMandatory))
                mSoftwareBuildMetadata.mbTreatUpdatesAsMandatory = FlagStringToBool(oFeatureFlagsElement.attribute(sTreatUpdatesAsMandatory));

            if (oFeatureFlagsElement.hasAttribute(sAllowMultipleInstances))
                mSoftwareBuildMetadata.mbAllowMultipleInstances = FlagStringToBool(oFeatureFlagsElement.attribute(sAllowMultipleInstances));

            if (oFeatureFlagsElement.hasAttribute(sEnableDifferentialUpdate))
                mSoftwareBuildMetadata.mbEnableDifferentialUpdate = FlagStringToBool(oFeatureFlagsElement.attribute(sEnableDifferentialUpdate));
        
            return true;
        }

        bool DiPManifest::ParseManifestContentIds(const QDomElement& oRootElement)
        {
            QDomNode oElement_ContentIDs = oRootElement.toElement().elementsByTagName(QString("contentIDs")).item(0);
            if (oElement_ContentIDs.isNull())
            {
		        ORIGIN_LOG_EVENT << "DiP manifest lists no content IDs";
            }
            else
            {
                for (QDomElement oContentIDsChildElement = oElement_ContentIDs.firstChildElement(); !oContentIDsChildElement.isNull();
                    oContentIDsChildElement = oContentIDsChildElement.nextSiblingElement())
                {
                    // Get individual content ID
                    if (0 == oContentIDsChildElement.nodeName().compare("contentID", Qt::CaseInsensitive))
                    {
                        QDomElement oContentIDElement = oContentIDsChildElement.toElement();
                        QString sValue (oContentIDElement.text());
                        if (!sValue.isEmpty())
                        {
                            mContentIDs.push_back(sValue);
                        }
                    }
                }
            }

            return true;
        }

        bool DiPManifest::ParseRuntimeBlock(const QDomElement& oRootElement)
        {
            QDomElement elementRuntime = oRootElement.elementsByTagName(QString("runtime")).item(0).toElement();
            if (!elementRuntime.isNull())
            {
                // Launchers
                QDomNodeList elementLaunchers = elementRuntime.elementsByTagName("launcher");
                for (int i = 0; i < elementLaunchers.count(); i++)
                {
                    Origin::Engine::Content::LauncherDescription launcher;

                    QDomElement elementLauncher = elementLaunchers.item(i).toElement();

                    if (!elementLauncher.isNull())
                    {
                        // Get all of the localized names for this launcher
                        QDomNodeList elementNames = elementLauncher.elementsByTagName("name");
                        for (int nameIndex = 0; nameIndex < elementNames.count(); nameIndex++)
                        {
                            QDomElement elementName = elementLauncher.elementsByTagName("name").item(nameIndex).toElement();
                            if (!elementName.isNull())
                            {
                                QString sLocale = elementName.attribute("locale", "en_US");     // default to en_US
                                launcher.mLanguageToCaptionMap[sLocale] = elementName.text();
                            }
                        }


                        // filePath
                        QDomElement elementFilePath = elementLauncher.elementsByTagName(QString("filePath")).item(0).toElement();
                        if (!elementFilePath.isNull())
                            launcher.msPath = elementFilePath.text();

                        // parameters
                        QDomElement elementParameters = elementLauncher.elementsByTagName(QString("parameters")).item(0).toElement();
                        if (!elementParameters.isNull())
                            launcher.msParameters = elementParameters.text();

                        // executeElevated
                        QDomElement elementExecuteElevated = elementLauncher.elementsByTagName(QString("executeElevated")).item(0).toElement();
                        if (!elementExecuteElevated.isNull())
                            launcher.mbExecuteElevated = FlagStringToBool(elementExecuteElevated.text());

                        // requires64BitOS
                        QDomElement elementRequires64BitOS = elementLauncher.elementsByTagName(QString("requires64BitOS")).item(0).toElement();
                        if (!elementRequires64BitOS.isNull())
                            launcher.mbRequires64BitOS = FlagStringToBool(elementRequires64BitOS.text());

                        // trial
                        QDomElement elementTrial = elementLauncher.elementsByTagName(QString("trial")).item(0).toElement();
                        if (!elementTrial.isNull())
                            launcher.mbIsTimedTrial = FlagStringToBool(elementTrial.text());

                    }

                    Q_ASSERT(launcher.mLanguageToCaptionMap.size() > 0);
                    Q_ASSERT(!launcher.msPath.isNull());

                    mLaunchers.push_back(launcher);
                }
            }

            return true;
        }

        bool DiPManifest::ParseIgnoreDeletes(const QDomElement& oRootElement)
        {
            // Process metadata child Elements
	        for (QDomElement oMetaDataChildElement = oRootElement.firstChildElement(); !oMetaDataChildElement.isNull(); 
			        oMetaDataChildElement = oMetaDataChildElement.nextSiblingElement())
	        {

		        // Get files slated for deletion
		        if (0 == oMetaDataChildElement.nodeName().compare("delete", Qt::CaseInsensitive))
		        {
                    QString sValue = ExtractWildcard(oMetaDataChildElement.toElement());
			        if (!sValue.isEmpty ())
				        mWildcardDeletes.push_back(sValue);
		        }
		        else if (0 == oMetaDataChildElement.nodeName().compare("ignore", Qt::CaseInsensitive))
		        {
			        QString sValue = ExtractWildcard(oMetaDataChildElement.toElement());
			        if (!sValue.isEmpty ())
				        mWildcardIgnores.push_back(sValue);
		        }

                else if (0 == oMetaDataChildElement.nodeName().compare("excludeFromDifferential", Qt::CaseInsensitive)
                      || 0 == oMetaDataChildElement.nodeName().compare("noDiffPatching", Qt::CaseInsensitive) )
		        {
                    QString sValue = ExtractWildcard(oMetaDataChildElement.toElement());
			        if (!sValue.isEmpty ())
				        mWildcardExcludesFromDifferential.push_back(sValue);
		        }
            }

            return true;
        }

        bool DiPManifest::ParseTouchupBlock(const QDomElement& oTouchupElement)
        {
            // Executable File Path Element?
	        QDomElement oElement_ExecutablePath = oTouchupElement.elementsByTagName(QString("filePath")).item(0).toElement();
	        if (!oElement_ExecutablePath.isNull())
		        msInstallerPath =  oElement_ExecutablePath.text();

            // Executable Parameters Element?
	        QDomElement oElement_ExecutableParams = oTouchupElement.elementsByTagName(QString("parameters")).item(0).toElement();
	        if (!oElement_ExecutableParams.isNull())
		        msInstallerParams =  oElement_ExecutableParams.text();


            // Update parameters
            QDomElement oElement_UpdateParams = oTouchupElement.elementsByTagName(QString("updateParameters")).item(0).toElement();
            if (!oElement_UpdateParams.isNull())
                msUpdateParams =  oElement_UpdateParams.text();

            // Repair parameters
            QDomElement oElement_RepairParams = oTouchupElement.elementsByTagName(QString("repairParameters")).item(0).toElement();
            if (!oElement_RepairParams.isNull())
                msRepairParams =  oElement_RepairParams.text();

            return true;
        }

        bool DiPManifest::Load(const QString& sFileName, bool isDLC)
        {
            QFile manifestFile(sFileName);

            mFileName = sFileName;

	        if (manifestFile.open(QIODevice::ReadOnly))
	        {
		        return Parse(manifestFile, isDLC);
	        }

	        ORIGIN_LOG_ERROR << "DiPManifest::fromFile failed to load: " << sFileName;
	        Clear();

	        return false;
        }

        void DiPManifest::Clear()
        {
            mIsDLC = false;
	        mLocalizedInfoMap.clear();
	        msInstallerPath.clear();
	        msInstallerParams.clear();
            msUpdateParams.clear();
            msRepairParams.clear();
            msUninstallPath.clear();
            mLaunchers.clear();
	        mManifestVersion	= VersionInfo( 0, 0, 0, 0 );
            mSoftwareBuildMetadata.Clear();
	        
        }

        DiPManifest& DiPManifest::operator=(const DiPManifest& other)
        {
	        mLocalizedInfoMap.clear();

	        tLocalizedInfoMap::const_iterator it = other.mLocalizedInfoMap.begin();

	        while ( it != other.mLocalizedInfoMap.end() )
	        {
		        mLocalizedInfoMap[it->first] = it->second;
		        it++;
	        }

	        msInstallerPath						= other.msInstallerPath;
	        msInstallerParams					= other.msInstallerParams;
            msUpdateParams                      = other.msUpdateParams;
            msRepairParams                      = other.msRepairParams;
	        mManifestVersion					= other.mManifestVersion;
            mLaunchers                          = other.mLaunchers;

            mSoftwareBuildMetadata               = other.mSoftwareBuildMetadata;

	        mWildcardDeletes					= other.mWildcardDeletes;
	        mWildcardIgnores					= other.mWildcardIgnores;
            mWildcardExcludesFromDifferential   = other.mWildcardExcludesFromDifferential;

            msUninstallPath                     = other.msUninstallPath;


	        return *this;
        }

        const LocalizedInfo& DiPManifest::GetSelectedInfo(const QString& sLocale) const
        {
	        static LocalizedInfo mEmptyInfo;

	        tLocalizedInfoMap::const_iterator it = mLocalizedInfoMap.find(sLocale);

	        return it == mLocalizedInfoMap.end() ? mEmptyInfo : it->second;
        }

        bool DiPManifest::IsLocaleSupported(const QString& lang) const
        {
            //in theory this shouldn't happen but it seems to, so just to be safe...
            if (lang.isEmpty())
                return false;

	        QStringList langs = GetSupportedLocales();

	        QStringList::const_iterator it = langs.constBegin();

	        while ( it != langs.constEnd() )
	        {
		        if ( lang.compare(*it, Qt::CaseInsensitive) == 0 )
			        return true;

		        it++;
	        }

	        return false;
        }

        bool DiPManifest::CheckIfLocaleRequiresAdditionalDownload(const QString& lang) const
        {
            //in theory this shouldn't happen but it seems to, so just to be safe...
            if (lang.isEmpty())
                return false;

            tLocalizedInfoMap::const_iterator it = mLocalizedInfoMap.begin();

	        while ( it != mLocalizedInfoMap.end() )
	        {
                if(lang.compare(it->first, Qt::CaseInsensitive) == 0)
                {
                    return it->second.mDownloadRequired;
                }

		        it++;
	        }

	        return false;
        }
    }
}
