///////////////////////////////////////////////////////////////////////////////
// ConfigFile.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <QNetworkRequest>
#include <QNetworkReply>
#include "services/settings/Variant.h"
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"
#include "EASTL/map.h"

namespace Origin
{
namespace Engine
{
namespace Config
{
struct insensitive
{
    bool operator() (const QString& lhs, const QString& rhs) const 
    {
        return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
    }
};

typedef QSet< QPair<QString, QString> > ChangedSettingsSet;

/// \brief In-memory object representing the user's EACore.ini config file.
/// Can be used to create write/replace EACore.ini files on the user's hard drive.
class ORIGIN_PLUGIN_API ConfigFile : public QObject
{
	Q_OBJECT

public:

    /// \brief Destructor
    ~ConfigFile();

    /// \return Reference to the configuration file currently in use.
    static const Engine::Config::ConfigFile& currentConfigFile() { return sCurrentConfigFile; }

    /// \return Reference to the configuration file containing any pending modifications.
    static Engine::Config::ConfigFile& modifiedConfigFile() { return sModifiedConfigFile; }

    /// \brief Reloads the current config file from the EACore.ini file on disk.
    /// \sa sCurrentConfigFile
    static void reloadCurrentConfigFile();

    /// \brief Sets the modified config file as the current.
    static void applyModifiedConfigFile();
    
    /// \Detects any changes between two config files.
    /// \return The set of changed settings
    static ChangedSettingsSet detectOverrideChanges();

    /// \brief Get the current value of the given setting as loaded from the config file.
    /// \param sectionName Which part of the config file the setting is in
    /// \param settingName The name of the individual setting in that section
    /// \return The current overridden value of the setting.
    QString getOverride(const QString& sectionName, const QString& settingName) const;

    /// \brief Get the current value of the given setting for the given content as loaded from the config file.
    /// \param sectionName Which part of the config file the setting is in
    /// \param settingName The name of the individual setting in that section
    /// \param id Which item's override we want (content ID or offer ID)
    /// \return The current overridden value of the setting.
    inline QString getOverride(const QString& sectionName, const QString& settingName, const QString& id) const { return getOverride(sectionName, settingName + "::" + id); }

    /// \brief Update the setting's value, or remove it from the list if what is passed in is a default value (eg. 0, empty string).
    /// \param sectionName Which part of the config file has the setting
    /// \param settingName Full name of the individual setting
    /// \param value What the setting should be changed to
    void updateOrDefaultOverride(const QString& sectionName, const QString& settingName, const QString& value);

    /// \brief Update the setting's value, or remove it from the list if what is passed in is a default value (eg. 0, empty string).
    /// \param sectionName Which part of the config file has the setting
    /// \param settingName Full name of the individual setting
    /// \param id Which piece of content we're updating this override for
    /// \param value What the setting should be changed to
    inline void updateOrDefaultOverride(const QString& sectionName, const QString& settingName, const QString& id, const QString& value) { updateOrDefaultOverride(sectionName, settingName + "::" + id, value); }

    /// \brief Remove a specific override from the list, reverting it to the default value for the corresponding setting.
    /// \param sectionName Which part of the config file has the setting
    /// \param settingName Full name of the individual setting
    /// \return True if the setting was removed, False if not.
    void removeOverride(const QString& sectionName, const QString& settingName);

    /// \brief Remove a specific override from the list, reverting it to the default value for the corresponding setting.
    /// \param sectionName Which part of the config file has the setting
    /// \param settingName Full name of the individual setting
    /// \param id Which piece of content we're un-overriding for
    /// \return True if the setting was removed, False if not.
    inline void removeOverride(const QString& sectionName, const QString& settingName, const QString& id) { return removeOverride(sectionName, settingName + "::" + id); }

    /// \brief Search all the overrides for ones which belong to the given item (offer ID/content ID) and remove them.
    /// \param id ID for which we want to revert everything.
    void removeAllOverridesForId(const QString& id);

    /// \brief Returns a list of all unique IDs which have overrides.
    /// \return A list of all IDs which have at least one override set.
    QStringList getOverriddenIds();

    /// \brief Loads the configuration file (EACore.ini currently) from the given file path.
    /// \param fileName The file path to the configuration file.
    /// \return True if the configuration file was loaded and parsed successfully.
    bool loadConfigFile(const QString& fileName);
            
    /// \brief Saves the configuration file (EACore.ini currently) to the given file path.
    /// \param fileName The file path to save the configuration file to.
    /// \return True if the configuration file was saved successfully.
    bool saveConfigFile(const QString& fileName);
    
    /// \brief Overridden assignment (copy) operator.
    /// \param rhs The ConfigFile object to copy from.
    /// \return The ConfigFile object that was copied to (this).
    ConfigFile& operator=(const ConfigFile& rhs);

private:

    /// \brief Private constructor so we can control how many ConfigFile objects exist.
    /// \param parent The parent.
    ConfigFile(QObject* parent = NULL);

    typedef eastl::map<QString, QString, insensitive> SectionMap;
    typedef eastl::map<QString, SectionMap, insensitive> OverrideMap;

    QString getIdFromSetting(const QString& settingName);
    QString stripCommentsFromLine(const QString& line);
    QString getSectionFromLine(const QString& line);
    
    void updateOverride(const QString& sectionName, const QString& settingName, const QString& value);
    inline void updateOverride(const QString& sectionName, const QString& settingName, const QString& id, const QString& value) { updateOverride(sectionName, settingName + "::" + id, value); }

    bool parseOverrideFromLine(const QString& line, QString& settingName, QString& value);
    void updateOverrideInLine(const QString& sectionName, const QString& line, QString& settingName, QString& value, QString& newLine);

    bool sectionContainsValidOverrides(const QString& sectionName);
    
    void readOverridesFromLoadedFile();
    void applyOverridesToLoadedFile();

    /// \brief The config file that Origin is currently using.  Note that this reflects the current state in memory, not the file on disk. 
    static Engine::Config::ConfigFile sCurrentConfigFile;

    /// \brief The config file containing the user's modifications.  When the user clicks "Apply Changes", this data will be written to disk.
    static Engine::Config::ConfigFile sModifiedConfigFile; 
    
    QString mLoadedFileName;
    QStringList mLoadedFile;

    OverrideMap mOverrides;
};

} // namespace Config
} // namespace Engine
} // namespace Origin

#endif
