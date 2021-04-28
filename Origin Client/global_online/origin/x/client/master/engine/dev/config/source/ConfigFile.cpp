/////////////////////////////////////////////////////////////////////////////
// ConfigFile.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/config/ConfigFile.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"
#include "services/Settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "engine/login/LoginController.h"

#include <QList>
#include <QPair>

namespace Origin
{
namespace Engine
{
namespace Config
{

// Static member definitions
ConfigFile ConfigFile::sCurrentConfigFile;
ConfigFile ConfigFile::sModifiedConfigFile;

ConfigFile::ConfigFile(QObject *parent)
: QObject(parent)
{
}

ConfigFile::~ConfigFile()
{
    mOverrides.clear();
}

void ConfigFile::reloadCurrentConfigFile()
{
    sCurrentConfigFile.loadConfigFile(Services::PlatformService::eacoreIniFilename());
}

void ConfigFile::applyModifiedConfigFile()
{
    sCurrentConfigFile = sModifiedConfigFile;
}

QString ConfigFile::getOverride(const QString& sectionName, const QString& settingName) const
{
    OverrideMap::const_iterator configSection;
    QString retval;
    
    configSection = mOverrides.find(sectionName);
    if(configSection != mOverrides.end())
    {
        const SectionMap& section = configSection->second;
        SectionMap::const_iterator entry = section.find(settingName);
        if(entry != section.end())
        {
            retval = entry->second;
        }
    }

    return retval;
}

void ConfigFile::updateOverride(const QString& sectionName, const QString& settingName, const QString& value)
{
    if(mOverrides.find(sectionName) == mOverrides.end())
    {
        mOverrides.insert(sectionName);
    }

    SectionMap& section = mOverrides[sectionName];
    section.insert(settingName);
    section[settingName] = value;
}

void ConfigFile::updateOrDefaultOverride(const QString& sectionName, const QString& settingName, const QString& value)
{
    if(value.isEmpty())
    {
        removeOverride(sectionName, settingName);
    }
    else
    {
        updateOverride(sectionName, settingName, value);
    }
}

void ConfigFile::removeOverride(const QString& sectionName, const QString& settingName)
{
    if(mOverrides.find(sectionName) != mOverrides.end())
    {
        SectionMap& section = mOverrides[sectionName];
        if(section.find(settingName) != section.end())
        {
            // Insert a null string to signal that this needs to be removed from the EACore.ini
            section[settingName] = QString();
        }
    }
}

void ConfigFile::removeAllOverridesForId(const QString& id)
{
    // We have to search every section for matching overrides.
    for(OverrideMap::const_iterator override_iter = mOverrides.begin(); override_iter != mOverrides.end(); ++override_iter)
    {
        const SectionMap& section = override_iter->second;
        for(SectionMap::const_iterator section_iter = section.begin(); section_iter != section.end(); ++section_iter)
        {
            // Is this override for the specific content in question?
            QString settingId = getIdFromSetting(section_iter->first);
            if(!settingId.isEmpty() && id.compare(settingId, Qt::CaseInsensitive) == 0)
            {
                // Don't use the ID-specific version of removeOverride since section_iter->first will look something like OverrideVersion::DR:139544900.
                removeOverride(override_iter->first, section_iter->first);
            }
        }
    }
}

QStringList ConfigFile::getOverriddenIds()
{
    QStringList retval;
             
    // We have to search every section for matching overrides.
    for(OverrideMap::const_iterator override_iter = mOverrides.begin(); override_iter != mOverrides.end(); ++override_iter)
    {
        const SectionMap& section = override_iter->second;
        for(SectionMap::const_iterator section_iter = section.begin(); section_iter != section.end(); ++section_iter)
        {
            // Do we have a valid override for this content?
            if(!section_iter->second.isEmpty())
            {
                // Is this override for a specific piece of content?
                QString settingId = getIdFromSetting(section_iter->first);
                if(!settingId.isEmpty() && !retval.contains(settingId, Qt::CaseInsensitive))
                {
                    retval.push_back(settingId);
                }
            }
        }
    }

    return retval;
}

bool ConfigFile::loadConfigFile(const QString& fileName)
{
    bool retval = false;

    if (QFile::exists(fileName))
    {
        mLoadedFile.clear();
        mOverrides.clear();
        mLoadedFileName = fileName;

        QFile file(fileName);
        file.open(QIODevice::ReadOnly);

        QTextStream stream(&file);
        while (!stream.atEnd())
        {
            mLoadedFile.append(stream.readLine());
        }
        file.close();

        retval = true;
    }

    if (retval)
    {
        readOverridesFromLoadedFile();
    }

    return retval;
}

bool ConfigFile::saveConfigFile(const QString& fileName)
{
    bool retval = false;

    applyOverridesToLoadedFile();

    // If the user had no EACore.ini and will still not have one, we can skip the following logic.
    if(!mLoadedFile.isEmpty())
    {
        // Silently back up one generation of the file we're saving over, but no more than that.
        QString backupPath = Services::PlatformService::commonAppDataPath() + QFileInfo(fileName).fileName() + ".bak";
        if (QFile::exists(fileName))
        {
            if (QFile::exists(backupPath))
            {
                QFile::remove(backupPath);
            }

            QFile::copy(fileName, backupPath);
        }

        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

        // Just dump out the file line by line; all updating happens to the in-memory copy.
        QTextStream stream(&file);
        for(QStringList::const_iterator it = mLoadedFile.constBegin(); it != mLoadedFile.end(); ++it)
        {
            stream << *it << endl;
        }

        file.close();

        retval = true;

        // If there was no EACore.ini file at client startup, but one is created by ODT, then the OverridesEnabled setting needs to be updated
        if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool() == false)
        {
            if (QFile(Services::PlatformService::eacoreIniFilename()).exists())
            {
                Services::writeSetting(Services::SETTING_OverridesEnabled, true);
            }
        }
    }

    return retval;
}

ConfigFile& ConfigFile::operator=(const ConfigFile& rhs)
{
    mLoadedFileName = rhs.mLoadedFileName;
    mLoadedFile = rhs.mLoadedFile;
    mOverrides = rhs.mOverrides;
    return (*this);
}

void ConfigFile::readOverridesFromLoadedFile()
{
    QString currSection, currLine;

    for(QStringList::const_iterator it = mLoadedFile.constBegin(); it != mLoadedFile.constEnd(); ++it)
    {
        currLine = stripCommentsFromLine(*it);
        
        QString sectionName = getSectionFromLine(currLine);
        if(!sectionName.isEmpty())
        {
            currSection = sectionName;
            if(mOverrides.find(currSection) == mOverrides.end())
            {
                mOverrides.insert(currSection);
            }
            continue;
        }

        if (!currSection.isEmpty() && !currLine.isEmpty())
        {
            QString settingName, value;
            if (parseOverrideFromLine(currLine, settingName, value))
            {
                SectionMap& section = mOverrides[currSection];
                section.insert(settingName);
                section[settingName] = value;
            }
        }
    }
}

QString ConfigFile::getIdFromSetting(const QString& settingName)
{
    QString retval;

    int id_begin = settingName.indexOf("::");
    if (id_begin >= 0)
    {
        // Skip the colons themselves.
        retval = settingName.mid(id_begin + 2);
        retval = retval.trimmed();
    }

    return retval;
}

QString ConfigFile::stripCommentsFromLine(const QString& line)
{
    QString retval = line;

    int comment_begin = line.indexOf(';');
    if (comment_begin >= 0)
    {
        // Strip any part of the line that is a comment including the comment marker itself.
        retval = retval.left(comment_begin);
        retval = retval.trimmed();
    }

    return retval;
}

QString ConfigFile::getSectionFromLine(const QString& line)
{
    QString retval;

    // Does this line start with a section marker?
    if (!line.isEmpty() && line[0] == '[')
    {
        // The section header must be well-formed, with an open bracket, at least one character of name and a close bracket. 
        int endPos = line.indexOf(']');
        if (endPos >= 2)
        {
            retval = line.left(endPos + 1);
        }
    }

    return retval;
}

bool ConfigFile::parseOverrideFromLine(const QString& line, QString& settingName, QString& value)
{
    bool retval = false;

    settingName = "";
    value = "";
    QString strippedLine = stripCommentsFromLine(line);
    
    int equalPos = strippedLine.indexOf('=');
    if (equalPos > 0)
    {
        settingName = strippedLine.left(equalPos).trimmed();
        value = strippedLine.mid(equalPos + 1).trimmed();
    }

    // We'll only have non-empty strings for both sides if there is an equals 
    // with at least one non-space character to the left and to the right of it.
    if (!settingName.isEmpty() && !value.isEmpty())
    {
        retval = true;
    }

    return retval;
}

void ConfigFile::applyOverridesToLoadedFile()
{
    QString currSection, currLine, sectionName, settingName, value;
    int lineEnding;

    // Create a local copy of our overrides map so we can remove from it as we go.
    OverrideMap overrides = mOverrides;
    
    // Keep a map of line endings for existing sections.  
    // This will allow us to insert new  overrides into existing sections later.
    eastl::map<QString, int, insensitive> sectionEnds;
    QStack<QString> sectionOrder;

    // First, update any existing overrides with new values (or remove them if necessary).
    // If any are updated/removed, remove them from our local map so we know we've already updated/removed them.
    for (int count = 0; count < mLoadedFile.count(); ++count)
    {
        currLine = mLoadedFile[count];

        sectionName = getSectionFromLine(currLine);
        if (!sectionName.isEmpty())
        {
            currSection = sectionName;

            // Update our section endings map.
            if(sectionEnds.find(currSection) == sectionEnds.end())
            {
                sectionEnds.insert(currSection);
                sectionOrder.push(currSection);
            }
            sectionEnds[currSection] = count + 1;
            continue;
        }

        if (overrides.find(currSection) != overrides.end() && !currLine.isEmpty())
        {
            QString newLine = currLine;
            updateOverrideInLine(currSection, currLine, settingName, value, newLine);

            // This line had an override, but it was set to an invalid value, we should remove it.
            if (newLine.isEmpty())
            {
                mLoadedFile.removeAt(count);
                --count;
            }
            else
            {
                mLoadedFile[count] = newLine;
            }

            // We've now processed this override so remove it from our local map.
            SectionMap& section = overrides[currSection];
            section.erase(settingName);

            // If there are no more overrides in the current section, remove it from the local map.
            if(section.empty())
            {
                overrides.erase(currSection);
            }

            // Update our section endings map.
            sectionEnds[currSection] = count + 1;
        }
    }
    
    // Next, add any new overrides to existing sections and remove from our local map as they are updated.
    // Note that we are traversing in reverse so any additions do not skew our original sectionEndings.
    while (!sectionOrder.isEmpty())
    {
        currSection = sectionOrder.pop();
        lineEnding = sectionEnds[currSection];
        
        SectionMap& section = overrides[currSection];
        for(eastl::map<QString, QString>::const_iterator it = section.begin(); it != section.end(); ++it)
        {
            settingName = it->first;
            value = it->second;

            if(!settingName.isEmpty() && !value.isEmpty())
            {
                mLoadedFile.insert(lineEnding, settingName + "=" + value);
            }
        }

        overrides.erase(currSection);
    }

    // Next, add any new sections to the end of the file.
    for(OverrideMap::const_iterator override_iter = overrides.begin(); override_iter != overrides.end(); ++override_iter)
    {
        currSection = override_iter->first;

        // Only add if the given section has at least one valid override.
        if(sectionContainsValidOverrides(currSection))
        {
            mLoadedFile.append("");
            mLoadedFile.append(currSection);
        
            const SectionMap& section = override_iter->second;
            for(SectionMap::const_iterator section_iter = section.begin(); section_iter != section.end(); ++section_iter)
            {
                settingName = section_iter->first;
                value = section_iter->second;

                if(!value.isEmpty())
                {
                    mLoadedFile.append(settingName + "=" + value);
                }
            }
        }
    }
}

bool ConfigFile::sectionContainsValidOverrides(const QString& sectionName)
{
    SectionMap& section = mOverrides[sectionName];
    for(SectionMap::const_iterator it = section.begin(); it != section.end(); ++it)
    {
        if(it->second != QString())
        {
            return true;
        }
    }

    return false;
}

void ConfigFile::updateOverrideInLine(const QString& sectionName, const QString& line, QString& settingName, QString& value, QString& newLine)
{
    QString strippedLine = stripCommentsFromLine(line);
    QString oldValue;
    
    settingName = "";
    value = "";

    // If we extract a valid key and value, we need to insert those into the original line. 
    // That way we can preserve comments and such.
    if (parseOverrideFromLine(strippedLine, settingName, oldValue))
    {
        // At least one of these will be empty if the override was not found in the map.
        if(mOverrides.find(sectionName) != mOverrides.end())
        {
            SectionMap& section = mOverrides[sectionName];
            if(section.find(settingName) != section.end())
            {
                value = section[settingName];

                // If we have a valid override for this, change what's in the file to match what we have. 
                // If there is an invalid stored value, that means the override has been reset and we should remove it.
                if (!value.isEmpty())
                {
                    // Update existing line, swapping old for new.
                    newLine = settingName + "=" + value;

                    int comment_begin = line.indexOf(';');
                    if (comment_begin >= 0)
                    {
                        newLine += " " + line.mid(comment_begin);
                    }
                }
                else
                {
                    newLine = "";
                }
            }
        }
    }
}

ChangedSettingsSet ConfigFile::detectOverrideChanges()
{
    const OverrideMap& overrides1 = sCurrentConfigFile.mOverrides;
    const OverrideMap& overrides2 = sModifiedConfigFile.mOverrides;

    // Value 1 = section name, Value 2 = override name
    ChangedSettingsSet modifiedOverrides;
    
    // Populate our modifiedOverrides map with all overrides contained in either map.
    // We will be removing any unmodified overrides later.
    for(OverrideMap::const_iterator override_iter = overrides1.begin(); override_iter != overrides1.end(); ++override_iter)
    {
        const SectionMap& section = override_iter->second;
        for(SectionMap::const_iterator section_iter = section.begin(); section_iter != section.end(); ++section_iter)
        {
            modifiedOverrides.insert(QPair<QString, QString>(override_iter->first, section_iter->first));
        }
    }
    
    for(OverrideMap::const_iterator override_iter = overrides2.begin(); override_iter != overrides2.end(); ++override_iter)
    {
        const SectionMap& section = override_iter->second;
        for(SectionMap::const_iterator section_iter = section.begin(); section_iter != section.end(); ++section_iter)
        {
            modifiedOverrides.insert(QPair<QString, QString>(override_iter->first, section_iter->first));
        }
    }

    // Remove any overrides that are identical between maps.
    ChangedSettingsSet::iterator it = modifiedOverrides.begin();
    while(it != modifiedOverrides.end())
    {
        const QString& sectionName = it->first;
        const QString& settingName = it->second;
        
        OverrideMap::const_iterator override_iter1 = overrides1.find(sectionName);
        OverrideMap::const_iterator override_iter2 = overrides2.find(sectionName);

        // Section must exist in both maps.
        if(override_iter1 != overrides1.end() && override_iter2 != overrides2.end())
        {
            const SectionMap& section1 = override_iter1->second;
            const SectionMap& section2 = override_iter2->second;

            SectionMap::const_iterator section_iter1 = section1.find(settingName);
            SectionMap::const_iterator section_iter2 = section2.find(settingName);

            // Override must exist in both maps.
            if(section_iter1 != section1.end() && section_iter2 != section2.end())
            {
                const QString& value1 = section_iter1->second;
                const QString& value2 = section_iter2->second;

                // Values must be the same in both maps.
                if(value1.compare(value2, Qt::CaseInsensitive) == 0)
                {
                    it = modifiedOverrides.erase(it);
                    continue;
                }
            }
        }

        ++it;
    }

    return modifiedOverrides;
}

} // Config
} // Engine
} // Origin
