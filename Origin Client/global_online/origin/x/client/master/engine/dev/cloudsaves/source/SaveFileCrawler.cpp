#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "ParsedMatchRule.h"

#include "engine/cloudsaves/FilenameBlacklistManager.h"
#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/cloudsaves/PathSubstituter.h"
#include "engine/content/LocalContent.h"
#include "engine/content/CloudContent.h"
#include <QDebug>
#include <QRegExp>

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    namespace
    {
        bool dirContains(const QString &parent, const QString &descendant)
        {
            // Add the "/"s so we don't think /bar/foobar is a child of /bar/foo
            return (descendant + "/").startsWith(parent + "/");
        }

        // Find the minimum set of directories we need to search
        QSet<QString> calculateSearchRoots(const QList<RootedMatchRule> &rules)
        {
            QSet<QString> searchDirs;

            for(QList<RootedMatchRule>::const_iterator it = rules.constBegin();
                it != rules.constEnd();
                it++)
            {
                RootedMatchRule const &rule = *it;

                if (rule.matchAction() != SaveFileCrawler::IncludeFile)
                {
                    // Excludes can't effect search roots
                    continue;
                }

                bool mergedRoot = false;
                for(QSet<QString>::iterator searchDirIt = searchDirs.begin();
                    searchDirIt != searchDirs.end();)
                {
                    // Note that dirContains also returns true in the == case
                    if (dirContains(*searchDirIt, rule.rootDirectory()))
                    {
                        // This root contains us
                        // Nothing to do
                        mergedRoot = true;
                        break;
                    }
                    else if (dirContains(rule.rootDirectory(), *searchDirIt))
                    {
                        // We contain this root; replace the root
                        // Note we can contain multiple existing roots so keep 
                        // going. QSet will automatically get rid of duplicates
                        searchDirIt = searchDirs.erase(searchDirIt);
    
                        // We didn't actually merge yet so don't set mergedRoot
                    }
                    else
                    {
                        searchDirIt++;
                    }
                }
    
                if (!mergedRoot)
                {
                    searchDirs << rule.rootDirectory();
                }
            }

            return searchDirs;
        };
        
        void findRootedFiles(const QString& path, const QList<RootedMatchRule> &rules, QList<QFileInfo>& matchingFiles)
        {
            QDir dir(path);
            // List all directories; i.e. don't apply the filters to directory names.
            // Do not list the special entries "." and "..".
            // List files.
            // List hidden files (on Unix, files starting with a ".").
            dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files | QDir::Hidden);
            QFileInfoList entryInfoList = dir.entryInfoList();
    
            foreach(QFileInfo fileInfo, entryInfoList)
            {
                QString test = fileInfo.absoluteFilePath();
                if(fileInfo.isDir())
                {
                    // search inside each dir recursively
                    findRootedFiles(fileInfo.filePath(), rules, matchingFiles);
                }
                else
                {
                    for(QList<RootedMatchRule>::const_iterator it = rules.constBegin();
                        it != rules.constEnd();
                        it++)
                    {
                        const ParsedMatchRule &rule = *it;
                        if (rule.matches(fileInfo.filePath()))
                        {
                            if (rule.matchAction() == SaveFileCrawler::ExcludeFile)
                            {
                                // We should exclude this
                                break;
                            }
                            else if (rule.matchAction() == SaveFileCrawler::IncludeFile)
                            {
                                // Only include this if it was a wildcard, otherwise we're just checking this
                                // against a directory which will always pass
                                matchingFiles.append(fileInfo);
                                break;
                            }
                        }
                    }
                }
            }
        }

        QList<EligibleFile> applyFilenameRules(const QList<QFileInfo> &matchedFiles, const QList<FilenameMatchRule> filenameRules)
        {
            QList<EligibleFile> eligibleFiles;

            for(QList<QFileInfo>::const_iterator fileIt = matchedFiles.constBegin();
                fileIt != matchedFiles.constEnd();
                fileIt++)
            {
                QString fileName = (*fileIt).fileName();

                bool blacklisted = false;
                bool explicitlyIncluded = false;
                bool explicitlyExcluded = false;

                for(QList<FilenameMatchRule>::const_iterator it = filenameRules.constBegin();
                    it != filenameRules.constEnd();
                    it++)
                {
                    const FilenameMatchRule &rule = *it;

                    if (rule.matches(fileName))
                    {
                        // What should be do?
                        if (rule.matchAction() == SaveFileCrawler::ExcludeFile)
                        {
                            if (rule.source() == ParsedMatchRule::GlobalBlacklistSource)
                            {
                                // This is a globally blacklisted extension but the content config can
                                // explicitly include it back
                                blacklisted = true;
                            }
                            else
                            {
                                // Excluded by content configuration or internal rules; kill it
                                explicitlyExcluded = true;
                                break;
                            }
                        }
                        else
                        {
                            explicitlyIncluded = true;
                        }
                    }
                }
                
                if (explicitlyExcluded)
                {
                    // Content configuration didn't want this
                }
                else if (!blacklisted)
                {
                    // Include as trusted file
                    eligibleFiles << EligibleFile(*fileIt, true);
                }
                else if (explicitlyIncluded)
                {
                    // We were saved by content configuration but we're not trusted
                    eligibleFiles << EligibleFile(*fileIt, false);
                }
            }

            return eligibleFiles;
        }
    }

    QList<EligibleFile> SaveFileCrawler::findEligibleFiles(const Content::EntitlementRef &entitlement, const PathSubstituter& substituter)
    {
        const QList<EligibleFileRules> &rules = entitlement->localContent()->cloudContent()->getCloudSaveFileCriteria();
        QList<RootedMatchRule> rootedRules;
        QList<FilenameMatchRule> filenameRules;

        QSet<QString> searchDirs;

        // Parse our global blacklist
        FilenameBlacklist globalBlacklist = FilenameBlacklistManager::instance()->currentBlacklist();
        for(FilenameBlacklist::const_iterator it = globalBlacklist.constBegin();
            it != globalBlacklist.constEnd();
            it++)
        {
            // All blacklist rules are exclude rules
            filenameRules << FilenameMatchRule(ParsedMatchRule::GlobalBlacklistSource, *it, SaveFileCrawler::ExcludeFile);
        }

        // Ignore our own temp files
        filenameRules << FilenameMatchRule(ParsedMatchRule::InternalSource, "cloudsaves??????.tmp", SaveFileCrawler::ExcludeFile);

        // Ignore the thumbnail database - if there are any images being synced and
        // the user has Explorer open to the save folder this can get locked behind 
        // us and result in sync errors
        filenameRules << FilenameMatchRule(ParsedMatchRule::InternalSource, "Thumbs.db", SaveFileCrawler::ExcludeFile);
        
        // Parse our our raw content rules
        for(QList<EligibleFileRules>::const_iterator it = rules.constBegin();
            it != rules.constEnd();
            it++)
        {
            if ((*it).first.startsWith("*"))
            {
                // This is a filename rule
                filenameRules << FilenameMatchRule(ParsedMatchRule::ContentConfigurationSource, (*it).first, (*it).second);
            }
            else
            {
                QString untemplatized = substituter.untemplatizePath((*it).first);
    
                if (untemplatized.isNull())
                {
                    ORIGIN_ASSERT(0);
                    ORIGIN_LOG_WARNING << "Cloud Saves: Can't untemplatize path " << qPrintable((*it).first);
                    continue;
                }
            
                RootedMatchRule newRule(ParsedMatchRule::ContentConfigurationSource, untemplatized, (*it).second);

                if (!newRule.isValid())
                {
                    ORIGIN_LOG_EVENT << "Cloud Saves: Can't crawl likely missing path " << qPrintable(untemplatized);
                    continue;
                }

                rootedRules << newRule;
            }

        }

        CloudSaves::LocalStateBackup localStateBackup(entitlement, CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
        RootedMatchRule backupRule(ParsedMatchRule::InternalSource, localStateBackup.backupPath(), IncludeFile);

        if(backupRule.isValid()) 
        {
            rootedRules << backupRule;
        }
        else 
        {
            ORIGIN_LOG_EVENT << "Cloud Saves: Can't crawl backup path --  likely missing path " << qPrintable( localStateBackup.backupPath());
        }

        searchDirs = calculateSearchRoots(rootedRules);

        // Find all files using rooted rules
        QList<QFileInfo> matchingFiles;
        for(QSet<QString>::const_iterator it = searchDirs.constBegin();
            it != searchDirs.constEnd();
            it++)
        {
            findRootedFiles(*it, rootedRules, matchingFiles);
        }

        return applyFilenameRules(matchingFiles, filenameRules);
    }
}
}
}
