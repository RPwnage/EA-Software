#ifndef ZIPINFO_H
#define ZIPINFO_H

#include <QString>

class PackageFile;

namespace ZipInfo
{
    PackageFile* processZipFile(QString filename);
}

#endif