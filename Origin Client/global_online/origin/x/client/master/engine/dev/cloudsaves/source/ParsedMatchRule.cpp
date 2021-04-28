#include "ParsedMatchRule.h"

namespace
{
    // This converts a wildcard to a regexp working on paths
    // QRegExp's QRegEXp::Wildcard format doesn't consider / special so "foo/*/bar" would match "foo/one/two/bar" 
    QString convertPathWildcardToRegexp(const QString &wildcard)
    {
        // Escape the wildcard so it become an escaped regexp
        QString regexp = QRegExp::escape(wildcard);

        // Replace the escaped glob characters with regexp special characters
        regexp = regexp.replace("\\*", "[^/]*");
        regexp = regexp.replace("\\?", "[^/]");

        return regexp;
    }
}


namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    RootedMatchRule::RootedMatchRule(RuleSource source, const QString &untemplatized, SaveFileCrawler::MatchAction action) :
        ParsedMatchRule(source, action)
    {
        QStringList pathParts = untemplatized.split('/');

        QStringList fixedPathParts;
        QStringList variablePathParts;

        // Find the fixed part of our path
        // Keep moving path elements to m_rootDirectory until we hit a wildcard and then bail
        // The last element is the filename pattern so ignore it when calculating the root directory
        while(pathParts.length() > 1)
        {
            const QString &firstPart = pathParts.first();

            if (firstPart.contains('?') || firstPart.contains('*'))
            {
                // We have a wildcard!
                break;
            }

            fixedPathParts << pathParts.takeFirst();
        }

        // Anything left is part of the variable path
        variablePathParts = pathParts;
        
        // Canonicalize the root directory
        m_rootDirectory = QFileInfo(fixedPathParts.join("/")).canonicalFilePath();

        // Build our find our canonicalized directory wildcard
        const QString canonicalWildcard(m_rootDirectory + "/" + variablePathParts.join("/"));

        // Convert it to a regexp
        QString canonicalRegexp(convertPathWildcardToRegexp(canonicalWildcard));
        
        // Add an implicit .* at the end of the path there's no terminal filename pattern
        // This means we search the directory and all of its subdirectories
        if (variablePathParts.last().isEmpty())
        {
            canonicalRegexp += ".*";
        }

        m_matchPattern = QRegExp(canonicalRegexp, Qt::CaseInsensitive);
    }


    FilenameMatchRule::FilenameMatchRule(RuleSource source, const QString &wildcard, SaveFileCrawler::MatchAction action) :
        ParsedMatchRule(source, action)
    {
        m_matchPattern = QRegExp(wildcard, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
}
}
}
