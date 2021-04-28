/////////////////////////////////////////////////////////////////////////////
// GuardFile.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef GUARDFILE_H
#define GUARDFILE_H

#include <QFile>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        namespace Backup
        {
            class ORIGIN_PLUGIN_API GuardFile
            {
            public:
                /// CTORS/DTOR
                GuardFile(const QString& GuardFileNamePath, const QString& GuardFileName = QString("/.guardfile"));
                ~GuardFile();

                /// \brief returns true if the file exists
                /// \param path to the file
                /// \param file name
                static bool exists(const QString& GuardFileNamePath, const QString& GuardFileName = QString("/.guardfile"));

                /// \brief creates/opens guard file
                bool open();

            private:
                /// \brief closes guard file
                void close();

                QString mGuardFileNamePath;
                QFile* mGuardFile;

                GuardFile(const GuardFile&);
                GuardFile& operator=(const GuardFile&);
            };

            ORIGIN_PLUGIN_API bool setFileAttributes(QString guardFileNamePath, bool isCreating = true);
        }
    }
}

#endif