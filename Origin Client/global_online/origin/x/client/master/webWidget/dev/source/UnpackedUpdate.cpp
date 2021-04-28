#include "UnpackedUpdate.h"

namespace WebWidget
{
    UnpackedUpdate UnpackedUpdate::unpackFromCompressedPackage(const UpdateIdentifier &identifier, const QString &rootPath, const QString &packagePath)
    {
        // Call up to UnpackedArchive and combine the result with the passed identifier
        UnpackedArchive package(UnpackedArchive::unpackFromCompressedPackage(rootPath, packagePath));

        if (package.isNull())
        {
            return UnpackedUpdate();
        }

        return UnpackedUpdate(package, identifier);
    }
        
    UnpackedUpdate::UnpackedUpdate(const UnpackedArchive &package, const UpdateIdentifier &identifier) :
        UnpackedArchive(package.rootDirectory()),
        mIdentifier(identifier)
    {
    }
}
