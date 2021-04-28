#ifndef _CLOUDSAVES_PARSEDMATCHRULE_H
#define _CLOUDSAVES_PARSEDMATCHRULE_H

#include <QString>
#include <QRegExp>
#include <QStringList>

#include "engine/cloudsaves/SaveFileCrawler.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    ///
    /// \brief Base class for file match rules that have been parsed
    ///
    /// This class and its subclasses are internal to SaveFileCrawler
    ///
    class ParsedMatchRule
    {
    public:
        /// \brief Indicates where a given rule originated from
        enum RuleSource
        {
            /// \brief Originated from the SRM-managed global filename blacklist
            GlobalBlacklistSource,
            /// \brief Originated from content-specific configuration
            ContentConfigurationSource,
            /// \brief Originated from internal rules. This is used to ignore our temporary files
            InternalSource
        };

        virtual ~ParsedMatchRule(){}

        /// \brief Returns the action to be taken if this rule matches
        SaveFileCrawler::MatchAction matchAction() const
        {
            return m_matchAction;
        }

        /// \brief Checks if the passed path matches this rule
        bool matches(const QString &path) const
        {
            return m_matchPattern.exactMatch(path);
        }

        /// \brief Returns the source of the rule
        RuleSource source() const 
        {
            return m_ruleSource;
        }

        /// \brief Returns if this rule is valid
        virtual bool isValid() const = 0;

    protected:
        ParsedMatchRule(RuleSource ruleSource, SaveFileCrawler::MatchAction matchAction) : 
             m_matchAction(matchAction), m_ruleSource(ruleSource)
        {
        }

        QRegExp m_matchPattern;

    private:
        SaveFileCrawler::MatchAction m_matchAction;
        RuleSource m_ruleSource;
    };

    /// Represents a match rule for an unrooted filename pattern
    class FilenameMatchRule : public ParsedMatchRule
    {
    public:
        ///
        /// \brief Constructs a new filename match rule
        ///
        /// \param  source    Source of the match rule
        /// \param  wildcard  Filename wildcard pattern to match
        /// \param  action    Action to take if the rule is matched
        ///
        FilenameMatchRule(RuleSource source, const QString &wildcard, SaveFileCrawler::MatchAction action);

        virtual ~FilenameMatchRule(){}

        virtual bool isValid() const
        {
            return true;
        }
    };

    /// Represents a match rule rooted to a particular filesystem location
    class RootedMatchRule : public ParsedMatchRule
    {
    public:
        ///
        /// \brief Constructs a new rooted match rule
        ///
        /// \param  source         Source of the match rule
        /// \param  untemplatized  Untemplatized rooted path. Templatized paths can be untemplatized with PathSubstituter
        /// \param  action         Action to take if the rule is matched
        ///
        RootedMatchRule(RuleSource source, const QString &untemplatized, SaveFileCrawler::MatchAction action);

        virtual ~RootedMatchRule(){}
        
        virtual bool isValid() const
        {
            return !m_rootDirectory.isEmpty();
        }

        /// \brief Returns the most specific directory that has to be searched to yield all files matching this rule
        QString rootDirectory() const
        {
            return m_rootDirectory;
        }
        
    private:
        QString m_rootDirectory;
    };
}
}
}

#endif
