#include "engine/cloudsaves/PathSubstituter.h"
#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"

#include "SubstitutionPaths.h"

namespace
{
    QMap<QString, QString> StaticSubstitutionPaths;
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    PathSubstituter::PathSubstituter(Content::EntitlementRef entitlement)
    {
        if (StaticSubstitutionPaths.isEmpty())
        {
            // Defined in our platform's SubstitutionPaths.h
            StaticSubstitutionPaths = queryOSSubstitutionPaths();
        }

        // Start with the static OS paths
        mSubstitutionPaths = StaticSubstitutionPaths;

        // Find the canonical path to our installer dir
        QString canonicalPath = QDir(QDir::fromNativeSeparators(entitlement->localContent()->dipInstallPath())).canonicalPath();

        // We can be empty if we can't find our install dir - this will break our
        // substituion code as .startWith("") always returns true
        if (!canonicalPath.isEmpty())
        {
            mSubstitutionPaths["%InstallDir%"] = canonicalPath;
        }
    }

    /**
     * Expands the prefix substitution variables in a path
     * 
     * Returns a null QString if no valid prefix was found
     */
    QString PathSubstituter::untemplatizePath(const QString &pathTemplate) const
    {
        for(QMap<QString, QString>::ConstIterator it = mSubstitutionPaths.constBegin();
            it != mSubstitutionPaths.constEnd();
            it++)
        {
            QString variableName = it.key();
            QString variablePath = it.value();

            if (pathTemplate.startsWith(variableName, Qt::CaseInsensitive))
            {
                // Replace and return
                return QString(pathTemplate).replace(0, variableName.length(), variablePath);
            }
        }
            
        // Not a template!
        return QString();
    }

    QString PathSubstituter::templatizePath(const QString &localPath, QString *matchedPrefix) const
    {
        Qt::CaseSensitivity sensitivity;

#ifdef ORIGIN_PC
        sensitivity = Qt::CaseInsensitive;
#else
        sensitivity = Qt::CaseSensitive;
#endif

        for(QMap<QString, QString>::ConstIterator it = mSubstitutionPaths.constBegin();
            it != mSubstitutionPaths.constEnd();
            it++)
        {
            QString variableName = it.key();
            QString variablePath = it.value();

            if (localPath.startsWith(variablePath, sensitivity))
            {
                if (matchedPrefix)
                {
                    *matchedPrefix = it.value();
                }

                // Replace and return
                return QString(localPath).replace(0, variablePath.length(), variableName);
            }
        }

        return QString();
    }
        
    const QMap<QString, QString> PathSubstituter::substitutionPaths() const
    {
        return mSubstitutionPaths;
    }
}
}
}
