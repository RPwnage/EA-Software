///////////////////////////////////////////////////////////////////////////////
// LocalInstallProperties.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _LOCALINSTALLPROPERTIES_H
#define _LOCALINSTALLPROPERTIES_H

#include <list>
#include <map>
#include "services/common/VersionInfo.h"
#include "services/plugin/PluginAPI.h"
#include "services/publishing/SoftwareBuildMetadata.h"

#include <QString>
#include <QStringList>
#include <QFile>
#include <QStack>
#include <QMutex>
#include <QtXml/QXmlSimpleReader>
#include "DiPManifest.h"

namespace Origin
{
    namespace Downloader
    {
        /// \brief utility class for reporting on parsing errors
        class ORIGIN_PLUGIN_API QXmlErrorHandlerOrigin : public QXmlErrorHandler
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


        /// \brief Local install properties class.  This is actually a strict subset of the DiP manifest format.
        class ORIGIN_PLUGIN_API LocalInstallProperties : private QXmlDefaultHandler
        {
            friend class XmlSimpleReader;
        public:
            /// \brief The LocalInstallProperties constructor.
            LocalInstallProperties();
            
            /// \brief Parses an existing manifest file.
            /// \param manifestFile A QFile object created from an existing manifest file.
            /// \param isDLC A boolean flag indicating whether the content is DLC
            /// \return True if no errors were encountered during parsing.
            bool Parse(QFile & manifestFile, bool isDLC);
            
            /// \brief Loads the manifest file from disk.
            /// \param sFileName The path to the file to load.
            /// \param isDLC A boolean flag indicating whether the content is DLC
            /// \return True if the file was loaded successfully.
            bool Load(const QString& sFileName, bool isDLC);
            
            /// \brief Checks if auto updates are enabled for this content.
            /// \return True if auto updates are enabled for this content.
            bool GetAutoUpdateEnabled() const { return mSoftwareBuildMetadata.mbAutoUpdateEnabled; }
            
            /// \brief Gets the game version (set by game teams).
            /// \return A VersionInfo object representing the game version.
            const VersionInfo& GetGameVersion() const { return mSoftwareBuildMetadata.mGameVersion; }

            /// \brief Checks if the content should use the game version from the manifest.
            /// \return True if the content should use the game version from the manifest.
            bool GetUseGameVersionFromManifestEnabled() const { return mSoftwareBuildMetadata.mbUseGameVersionFromManifestEnabled; }
            
            /// \brief Gets the manifest version (DiP version, i.e. 1.1, 2.0, etc.).
            /// \return A VersionInfo object representing the manifest version.
            const VersionInfo GetManifestVersion() const { return mManifestVersion; }

            /// \brief Checks if the game must be run with elevated priveleges.  (i.e. through the ORiginClientService)
            /// \return True if the game should be run elevated.
            bool GetExecuteElevated() { return mSoftwareBuildMetadata.mbExecuteGameElevated; }

            /// \brief If specified, this path should point to the registry location that contains the uninstall executable/parameters to run.
            const QString& GetUninstallPath() { return msUninstallPath; }

            /// \brief Launchers for this item (if available.)
            const Origin::Engine::Content::tLaunchers& GetLaunchers() { return mLaunchers; }

            /// \brief Checks if the content should not allow launching when the game is out of date.
            bool GetTreatUpdatesAsMandatory() { return mSoftwareBuildMetadata.mbTreatUpdatesAsMandatory; }

            /// \brief Allows us to launch the content again, even if it's already playing
            bool GetAllowMultipleInstances() { return mSoftwareBuildMetadata.mbAllowMultipleInstances; }

            /// \brief List of available locales 
            const QStringList& GetAvailableLocales() { return msAvailableLocales; }

            /// \brief List of contentIds in the manifest
            const QStringList& GetContentIDs() { return mContentIds; }


        private:
            /// \brief Checks an XML element for certain nodes that contain necessary information.  Ignores any elements not named "game" or "featureFlags".
            /// \param name Name of the XML element.
            /// \param attrs The attributes of the given element.
            /// \return True if the attributes were NOT processed.
            bool startElement(const QString &, const QString & name, const QString &, const QXmlAttributes & attrs);
            bool endElement(const QString&, const QString& name, const QString& qName);
            bool characters(const QString& chars);

            bool startElementDiP4(const QString & name, const QXmlAttributes & attrs);
            bool endElementDiP4(const QString& name, const QString& qName);
            bool charactersDiP4(const QString& chars);
            
            /// \brief Clears all information contained in this manifest.
            void Clear();
            
            /// \brief Sets up the default feature flags for DiP Manifest versions <= 2.1.  
            /// 
            /// Feature flag reading was implemented for 2.2 and above.
            void SetFeatureFlagDefaultsBasedOnManifestVersion();
            
            /// \brief Converts a string value to a boolean.
            /// \param sValue A string that represents a boolean value.
            /// \return True if a XML flag string is "1" or "true" as per w3 spec: www.w3.org/TR/xmlschema-2/\#boolean
            bool FlagStringToBool(const QString& sValue);


            /// \brief Helper functions for keeping track of where in the XML document we're parsing
            /// Note that the path will be all lowercase
            void pushPath(QString sNodeName) { mNodePath.push(sNodeName.toLower()); }
            QString popPath() { return mNodePath.pop(); }
            QString currentNodePath();

        private:
            /// \brief Arbitrary string describing the game version.
            //VersionInfo mGameVersion;

            /// \brief True if the content being parsed is DLC
            bool mIsDLC;

            /// \brief DiP manifest version.
            VersionInfo mManifestVersion;

            Origin::Services::Publishing::SoftwareBuildMetadata mSoftwareBuildMetadata;
            
            /// \brief True if game should check for published updates.
            //bool mbAutoUpdateEnabled;
            
            /// \brief True if the content manager should use the gameVersion in the manifest to compare against the version in the entitlement response in checking whether an update has been published.
            //bool mbUseGameVersionFromManifestEnabled;

            /// \brief True if game should be run elevated.
            //bool mbExecuteElevated;

            /// \brief True if a detected update is mandatory. (i.e. don't allow launch before updating.)
            //bool mbTreatUpdatesAsMandatory;

            /// \brief True if the XML parsing finished early, either because the manifest version was less than 2.0, or because we found the "featureFlags" node.
            bool mbFinishedEarly;
            
            /// \brief True if the XML contained a "game" node.
            bool mbSawGameElement;

            /// \brief True if the XML is a DiP 4.0+ schema format
            bool mbParsingDip4Plus;

            /// \brief Uninstall registry key.  Follows same convention as msInstallerPath
            QString msUninstallPath;

            /// \brief List of Launchers
            Origin::Engine::Content::tLaunchers mLaunchers;

            /// \brief Place to keep launcher data while parsing
            Origin::Engine::Content::LauncherDescription mLauncherBeingParsed;       
            QString msLauncherLanguageBeingParsed;

            /// \brief List of all available locales
            QStringList msAvailableLocales;

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
            QXmlErrorHandlerOrigin  mErrorHandler;

            // \brief The contentIds in the manifest.
            QStringList mContentIds;

            /// \brief For thread safety
            QMutex mMutex;
        };

    }
}

#endif
