#include "services/publishing/SoftwareBuildMetadata.h"

#include <QStack>
#include <QBuffer>
#include "TelemetryAPIDLL.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

    /////////////////////////////////////////////
    // Internal use only declarations
    //

    class XmlInputSourceNBSPEater : public QXmlInputSource
    {
    public:
        XmlInputSourceNBSPEater(QIODevice* dev) : QXmlInputSource(dev) {}

        QChar next();
    };

    /// \brief utility class for reporting on parsing errors
    class SoftwareBuildMetadataParserErrorHandler : public QXmlErrorHandler
    {
    public:
        void setPath(QString name);
        void setLoggingEnabled(bool enabled);

        virtual QString	errorString() const;
        virtual bool error( const QXmlParseException & exception );
        virtual bool fatalError( const QXmlParseException & exception );
        virtual bool warning( const QXmlParseException & exception );

    private:
        bool    mEnabled;
        QString mPath;
    };

    class SoftwareBuildMetadataParser : private QXmlDefaultHandler
    {
        friend class SoftwareBuildMetadata;
        friend class XmlSimpleReader;
    public:
        SoftwareBuildMetadataParser(SoftwareBuildMetadata& SoftwareBuildMetadata) : mSoftwareBuildMetadata(SoftwareBuildMetadata), mFoundMetadataBlock(false), mFinishedEarly(false) {}

    private:
        /// \brief Checks an XML element for certain nodes that contain necessary information.
        /// \param name Name of the XML element.
        /// \param attrs The attributes of the given element.
        /// \return True if the attributes were NOT processed.
        bool startElement(const QString &, const QString & name, const QString &, const QXmlAttributes & attrs);
        bool endElement(const QString&, const QString& name, const QString& qName);
        bool characters(const QString& chars);

        /// \brief Helper functions for keeping track of where in the XML document we're parsing
        /// Note that the path will be all lowercase
        void pushPath(QString sNodeName) { mNodePath.push(sNodeName.toLower()); }
        QString popPath() { return mNodePath.pop(); }
        QString currentNodePath();

    private:
        SoftwareBuildMetadata& mSoftwareBuildMetadata;

        bool mFoundMetadataBlock;
        bool mFinishedEarly;

        /// \brief Stack to keep track of what level in the manifest we're currently parsing
        /// For example.
        /// <a>
        ///    <b>
        ///        <c>hello</c>
        ///    </b>
        /// <a>
        /// When parsing the c element the path will be a.b.c
        QStack<QString> mNodePath;


        /// \brief For reporting errors
        SoftwareBuildMetadataParserErrorHandler  mErrorHandler;
    };

    /////////////////////////////////////////////
    // Internal use only definitions
    //

    bool FlagStringToBool(const QString& sValue)
    {
	    if (sValue == "1")
		    return true;

	    return sValue.compare("true", Qt::CaseInsensitive) == 0;
    }

    QChar XmlInputSourceNBSPEater::next()
    {
        QChar c = QXmlInputSource::next();
        if (c == 0xA0)      // if it's a nbsp
            return 0x20;    // return a regular space
        return c;
    }

    void SoftwareBuildMetadataParserErrorHandler::setLoggingEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    void SoftwareBuildMetadataParserErrorHandler::setPath(QString path)
    {
        mPath = path;
    }

    QString	SoftwareBuildMetadataParserErrorHandler::errorString() const 
    { 
        return ""; 
    }

    bool SoftwareBuildMetadataParserErrorHandler::error( const QXmlParseException & exception )
    { 
        if (!mEnabled)
            return true;

        QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseError", sError.toUtf8().data());

        ORIGIN_LOG_ERROR << "SoftwareBuildMetadata Parse Error: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 
        return true; 
    }

    bool SoftwareBuildMetadataParserErrorHandler::fatalError( const QXmlParseException & exception ) 
    { 
        if (!mEnabled)
            return true;

        QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseFatalError", sError.toUtf8().data());

        ORIGIN_LOG_ERROR << "SoftwareBuildMetadata Parse FatalError: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 
        return false; 
    }

    bool SoftwareBuildMetadataParserErrorHandler::warning( const QXmlParseException & exception ) 
    { 
        if (!mEnabled)
            return true;

        QString sError = QString("line:%1 col:%2 error:%3").arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message());
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("ManifestParseWarning", sError.toUtf8().data());

        ORIGIN_LOG_WARNING << "SoftwareBuildMetadata Parse Warning: (file: " << mPath << " line:" << exception.lineNumber() << " column:" << exception.columnNumber() << ") error: " << exception.message(); 

        return true; 
    }

    bool SoftwareBuildMetadataParser::startElement(const QString &, const QString &name, const QString &, const QXmlAttributes &attrs)
    {
        //ORIGIN_LOG_DEBUG << "pushing: " << name;
        pushPath(name);
        QString sParsingPath(currentNodePath());
        //ORIGIN_LOG_DEBUG << "startElement path:" << sParsingPath;

        // Case is smashed in the parsing path
        if (sParsingPath.contains("buildmetadata"))
        {
            // Mark that we did find at least a valid SoftwareBuildMetadata block
            mFoundMetadataBlock = true;

            // Trim the parsing path down to ignore the stuff we don't care about
            sParsingPath = sParsingPath.mid(sParsingPath.indexOf("buildmetadata", 0, Qt::CaseInsensitive));
        }
        
        if (sParsingPath == "buildmetadata.gameversion")
        {
            mSoftwareBuildMetadata.mGameVersion = VersionInfo(attrs.value("version"));
        }
        else if (sParsingPath == "buildmetadata.featureflags")
        {
            for (int idxFlagAttr = 0; idxFlagAttr < attrs.count(); ++idxFlagAttr)
            {
                QString sAttributeName = attrs.qName(idxFlagAttr);
                QString sAttributeValue = attrs.value(idxFlagAttr);

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
                const QString sDynamicContentSupportEnabled("dynamicContentSupportEnabled");
                const QString sLanguageChangeSupportEnabled("languageChangeSupportEnabled");
                const QString sDLCRequiresParentUpToDate("dlcRequiresParentUpToDate");
                const QString sAllowMultipleInstances("allowMultipleInstances");

                if (sAttributeName.compare(sAutoUpdateEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbAutoUpdateEnabled	= FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sIgnoresEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbIgnoresEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sDeletesEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbDeletesEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sConsolidatedEULAsEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbConsolidatedEULAsEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sShortcutsEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbShortcutsEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sLanguageExcludesEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbLanguageExcludesEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sUseGameVersionFromManifestEnabledKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbUseGameVersionFromManifestEnabled	= FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sForceTouchupInstallerAfterUpdateKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbForceTouchupInstallerAfterUpdate = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sExecuteGameElevatedKey, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbExecuteGameElevated = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sTreatUpdatesAsMandatory, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbTreatUpdatesAsMandatory = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sAllowMultipleInstances, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbAllowMultipleInstances = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sEnableDifferentialUpdate, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbEnableDifferentialUpdate = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sDynamicContentSupportEnabled, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbDynamicContentSupportEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sLanguageChangeSupportEnabled, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbLanguageChangeSupportEnabled = FlagStringToBool(sAttributeValue);

                else if (sAttributeName.compare(sDLCRequiresParentUpToDate, Qt::CaseInsensitive) == 0)
                    mSoftwareBuildMetadata.mbDLCRequiresParentUpToDate = FlagStringToBool(sAttributeValue);
            }
        }
        else if (sParsingPath == "buildmetadata.requirements")
        {
            mSoftwareBuildMetadata.mMinimumRequiredOS = VersionInfo(attrs.value("osMinVersion"));
            mSoftwareBuildMetadata.mRequiredClientVersion = VersionInfo(attrs.value("clientVersion"));
            mSoftwareBuildMetadata.mbGameRequires64BitOS = FlagStringToBool(attrs.value("osReqs64Bit"));
        }
        else if (sParsingPath == "buildmetadata.minimumrequiredversionforonlineuse")
        {
            mSoftwareBuildMetadata.mMinimumRequiredVersionForOnlineUse = VersionInfo(attrs.value("version"));
        }
        else if (sParsingPath == "buildmetadata.autorepairrange")
        {
            mSoftwareBuildMetadata.mAutoRepairVersionMin = VersionInfo(attrs.value("min"));
            mSoftwareBuildMetadata.mAutoRepairVersionMax = VersionInfo(attrs.value("max"));
        }

        return true;
    }

    bool SoftwareBuildMetadataParser::endElement(const QString&, const QString& name, const QString& qName)
    {
        QString sParsingPath(currentNodePath());
        //ORIGIN_LOG_DEBUG << "endElement path:" << sParsingPath;

        bool ret = true;

        // Case is smashed in the parsing path

        // We will abort processing early if we're processing more than just a metadata block, because we aren't interested in the rest
        if (sParsingPath.endsWith("buildmetadata") && !sParsingPath.startsWith("buildmetadata"))
        {
            mFinishedEarly = true;

            mErrorHandler.setLoggingEnabled(false);     // stop logging anything further

            // Stop parsing, we have what we need
            ret = false;
        }

        QString sPopping = popPath();
        //ORIGIN_LOG_DEBUG << "name: " << name << " popping: " << sPopping;

        ORIGIN_ASSERT(sPopping.compare(name, Qt::CaseInsensitive) == 0);  // Better be popping the element we expect to be
        return ret;
    }

    bool SoftwareBuildMetadataParser::characters(const QString& chars)
    {
        QString sParsingPath(currentNodePath());
        //ORIGIN_LOG_DEBUG << "characters path:" << sParsingPath;

        Q_UNUSED(sParsingPath);

        return true;
    }

    QString SoftwareBuildMetadataParser::currentNodePath()
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

    /////////////////////////////////////////////
    // Software Build Metadata definitions
    //

    SoftwareBuildMetadata::SoftwareBuildMetadata()
    {
        Clear();
    }

    SoftwareBuildMetadata::SoftwareBuildMetadata(const SoftwareBuildMetadata& other)
    {
        *this = other;
    }

    SoftwareBuildMetadata& SoftwareBuildMetadata::operator=(const SoftwareBuildMetadata& other)
    {
        mGameVersion						= other.mGameVersion;
	    mMinimumRequiredOS					= other.mMinimumRequiredOS;
        mRequiredClientVersion              = other.mRequiredClientVersion;
	    mbAutoUpdateEnabled					= other.mbAutoUpdateEnabled;	
	    mbIgnoresEnabled					= other.mbIgnoresEnabled;
	    mbDeletesEnabled					= other.mbDeletesEnabled;
	    mbConsolidatedEULAsEnabled			= other.mbConsolidatedEULAsEnabled;
	    mbShortcutsEnabled					= other.mbShortcutsEnabled;
	    mbLanguageExcludesEnabled			= other.mbLanguageExcludesEnabled;
        mbLanguageIncludesEnabled           = other.mbLanguageIncludesEnabled;
	    mbUseGameVersionFromManifestEnabled = other.mbUseGameVersionFromManifestEnabled;
        mbForceTouchupInstallerAfterUpdate  = other.mbForceTouchupInstallerAfterUpdate;
        mbExecuteGameElevated               = other.mbExecuteGameElevated;
        mbTreatUpdatesAsMandatory           = other.mbTreatUpdatesAsMandatory;
        mbAllowMultipleInstances            = other.mbAllowMultipleInstances;
        mbEnableDifferentialUpdate          = other.mbEnableDifferentialUpdate;
        mbDynamicContentSupportEnabled      = other.mbDynamicContentSupportEnabled;
        mbGameRequires64BitOS               = other.mbGameRequires64BitOS;
        mbLanguageChangeSupportEnabled      = other.mbLanguageChangeSupportEnabled;
        mAutoRepairVersionMin               = other.mAutoRepairVersionMin;
        mAutoRepairVersionMax               = other.mAutoRepairVersionMax;
        mMinimumRequiredVersionForOnlineUse = other.mMinimumRequiredVersionForOnlineUse;
        mbDLCRequiresParentUpToDate         = other.mbDLCRequiresParentUpToDate;

        return *this;
    }

    void SoftwareBuildMetadata::Clear()
    {
        mGameVersion		= VersionInfo( 0, 0, 0, 0 );
	    mMinimumRequiredOS	= VersionInfo( 0, 0, 0, 0 );
        mRequiredClientVersion = VersionInfo( 0, 0, 0, 0 );

	    mbAutoUpdateEnabled = false;
	    mbIgnoresEnabled = false;
	    mbDeletesEnabled = false;
	    mbConsolidatedEULAsEnabled = false;
	    mbShortcutsEnabled = false;
	    mbLanguageExcludesEnabled = false;
        mbLanguageIncludesEnabled = false;
	    mbUseGameVersionFromManifestEnabled = false;
        mbForceTouchupInstallerAfterUpdate = false;
        mbExecuteGameElevated = false;
        mbTreatUpdatesAsMandatory = false;
        mbAllowMultipleInstances = false;
        mbEnableDifferentialUpdate = false;
        mbGameRequires64BitOS = false;
        mbLanguageChangeSupportEnabled = false;
        mbDynamicContentSupportEnabled = false;
        mbDLCRequiresParentUpToDate = false;

        mAutoRepairVersionMin = VersionInfo( 0, 0, 0, 0 );
        mAutoRepairVersionMax = VersionInfo( 0, 0, 0, 0 );
        mMinimumRequiredVersionForOnlineUse = VersionInfo( 0, 0, 0, 0 );
    }

    bool SoftwareBuildMetadata::ParseSoftwareBuildMetadataBlock(const QString& xmlData, const QString& dataSource)
    {
        SoftwareBuildMetadataParser parser(*this);

        parser.mErrorHandler.setPath(dataSource);
        parser.mErrorHandler.setLoggingEnabled(true);

        // Create a reader and point it at ourselves
        QXmlSimpleReader xmlReader;
        xmlReader.setContentHandler(&parser);
        xmlReader.setErrorHandler(&parser.mErrorHandler);

        // Read the XML in
        QByteArray rawXml = xmlData.toLocal8Bit();
        QBuffer inputDevice(&rawXml);
        XmlInputSourceNBSPEater xmlSource(&inputDevice);
        bool result = xmlReader.parse(xmlSource);

        // If the parsing failed and we didn't stop it early
        if (!result && !parser.mFinishedEarly)
            return false;

        if (parser.mFoundMetadataBlock)
            return true;
            
        return false;
    }

    void SoftwareBuildMetadata::SetFeatureFlagDefaultsBasedOnManifestVersion(VersionInfo manifestVersion, bool isDLC)
    {
	    mbAutoUpdateEnabled = true;
	    mbIgnoresEnabled = true;
	    mbConsolidatedEULAsEnabled = true;
	    mbShortcutsEnabled = true;
	    mbLanguageExcludesEnabled = true;
        mbLanguageIncludesEnabled = false;
	    mbDeletesEnabled = true;
	    mbUseGameVersionFromManifestEnabled = true;
        mbForceTouchupInstallerAfterUpdate = false;     // always off by default
        mbExecuteGameElevated = false;
        mbTreatUpdatesAsMandatory = false;
        mbAllowMultipleInstances = false;
        mbEnableDifferentialUpdate = true;
        mbDynamicContentSupportEnabled = false; // always off by default
        mbLanguageChangeSupportEnabled = false; // always off by default
        mbDLCRequiresParentUpToDate = false; // always off by default

	    if (manifestVersion < VersionInfo( 1, 1, 0, 0 ))
	    {
		    mbLanguageExcludesEnabled = false;
	    }

	    if (manifestVersion < VersionInfo( 2, 0, 0, 0 ))
	    {
		    mbAutoUpdateEnabled = false;
		    mbIgnoresEnabled = false;
		    mbDeletesEnabled = false;
		    mbShortcutsEnabled = false;
	    }

	    if (manifestVersion < VersionInfo( 2, 1, 0, 0))
	    {
		    mbConsolidatedEULAsEnabled = false;
	    }

	    if (manifestVersion < VersionInfo( 2, 2, 0, 0) || isDLC)
	    {
		    mbUseGameVersionFromManifestEnabled = false;
	    }
            
        if (manifestVersion < VersionInfo( 3, 0, 0, 0))
        {
            mbEnableDifferentialUpdate = false;
        }

        if (manifestVersion >= VersionInfo( 4, 0, 0, 0))
        {
            mbLanguageExcludesEnabled = false;
            mbLanguageIncludesEnabled = true;
        }
    }

} //namespace Publishing

}//namespace Services

}//namespace Origin