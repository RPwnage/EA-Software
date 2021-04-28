#ifndef _CLOUDSAVES_SAVEFILECRAWLER_H
#define _CLOUDSAVES_SAVEFILECRAWLER_H

#include <QPair>
#include <QFileInfo>

#include "services/plugin/PluginAPI.h"
#include "engine/content/ContentTypes.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class PathSubstituter;

    class ORIGIN_PLUGIN_API EligibleFile
    {
    public:
        EligibleFile(const QFileInfo &info, bool trusted) :
            mFileInfo(info), mTrusted(trusted)
        {
        }

        QFileInfo info() const { return mFileInfo; }
        bool isTrusted() const { return mTrusted; }

    private:
        QFileInfo mFileInfo;
        bool mTrusted;
    };

    namespace SaveFileCrawler
    {
        enum MatchAction
        {
            IncludeFile,
            ExcludeFile
        };

        typedef QPair<QString, MatchAction> EligibleFileRules;

        ORIGIN_PLUGIN_API QList<EligibleFile> findEligibleFiles(const Content::EntitlementRef &entitlement, const PathSubstituter& substituter);
    };

} 
}
}

#endif
