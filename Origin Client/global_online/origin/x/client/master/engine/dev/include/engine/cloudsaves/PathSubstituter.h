#ifndef _CLOUDSAVES_PATHSUBSTITUTER
#define _CLOUDSAVES_PATHSUBSTITUTER

#include <QString>
#include <QMap>
#include <QDir>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API PathSubstituter
    {
    public:
        PathSubstituter(Content::EntitlementRef entitlement);

        /**
         * Expands the prefix substitution variables in a path
         * 
         * Returns a null QString if no valid prefix was found
         */
        QString untemplatizePath(const QString &pathTemplate) const;
        QString templatizePath(const QString &localPath, QString *matchedPrefix = NULL) const;

        const QMap<QString, QString> substitutionPaths() const;

    protected:
        QMap<QString, QString> mSubstitutionPaths;
    };
}
}
}

#endif
