/////////////////////////////////////////////////////////////////////////////
// GuardFile.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#if defined(ORIGIN_PC)
#define NOMINMAX
#include <windows.h>
#endif

#include <QDir>
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/Settings/SettingsManager.h"
#include "engine/backup/GuardFile.h"

namespace Origin
{
    namespace Engine
    {
        namespace Backup
        {
            namespace
            {
                void createPath(const QString& myLocalPath)
                {
                    QDir qdir(myLocalPath);
                    ORIGIN_LOG_EVENT << "[information] Testing for existence of path: " << myLocalPath;
                    if (!qdir.exists())
                    {
                        ORIGIN_LOG_EVENT << "[warning] Path: [" << myLocalPath << "] doesn't exist.";
                        if(qdir.mkpath(myLocalPath))
                        {
                            ORIGIN_LOG_EVENT << "[warning] Path: [" << myLocalPath << "] created.";
                        }
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "[information] Path: [" << myLocalPath << "] already exists.";
                    }
                }
                QString cleanPath(const QString& myPath, const QString& myFile)
                {
                    QString myLocalPath(QDir::toNativeSeparators(myPath));
                    QString myLocalFile(QDir::toNativeSeparators(myFile));
                    if (myLocalPath.endsWith(QDir::separator()) && myLocalFile.startsWith(QDir::separator()))
                    {
                        myLocalPath.chop(1);
                    }
                    if (!myLocalPath.endsWith(QDir::separator()) && !myLocalFile.startsWith(QDir::separator()))
                    {
                        myLocalPath += QString(QDir::separator());
                    }
                    createPath(myLocalPath);
                    return myLocalPath + myLocalFile;
                }
            }
            GuardFile::GuardFile(const QString& GuardFileNamePath, const QString& GuardFileName)
                : mGuardFileNamePath(GuardFileNamePath)
                , mGuardFile(NULL)
            {
                mGuardFileNamePath = cleanPath(GuardFileNamePath, GuardFileName);
                open();
            }

            bool GuardFile::exists(const QString& GuardFileNamePath, const QString& GuardFileName)
            {
                QString guardFileNamePath = cleanPath(GuardFileNamePath, GuardFileName);
                ORIGIN_LOG_EVENT << " [information] Testing for file existence: " << guardFileNamePath;
                return QFile::exists(guardFileNamePath);
            }

            GuardFile::~GuardFile()
            {
                close();
            }

            bool GuardFile::open()
            {
                mGuardFile = new QFile(mGuardFileNamePath);
                ORIGIN_ASSERT(mGuardFile);
                bool isOk = mGuardFile->open(QIODevice::ReadWrite);
                ORIGIN_LOG_EVENT_IF(!isOk) << " [warning] Could not open/create guard file: " << mGuardFileNamePath;

                isOk = setFileAttributes(mGuardFileNamePath);
                ORIGIN_LOG_EVENT_IF(!isOk) << " [warning] Could not set file attributes for guard file " << mGuardFileNamePath;

                if (isOk)
                {
                    ORIGIN_LOG_EVENT << "[-----] Created guard file: " << mGuardFileNamePath;
                } 
                else
                {
                    ORIGIN_LOG_EVENT << "[error] Could not create guard file: " << mGuardFileNamePath;
                }
                return isOk;
            }

            void GuardFile::close()
            {
                if (NULL!=mGuardFile)
                {
                    mGuardFile->close();
                    bool isOk = setFileAttributes(mGuardFileNamePath, false);
                    ORIGIN_LOG_EVENT_IF(!isOk) << " [warning] Could not set file attributes for guard file " << mGuardFileNamePath;
                    isOk = mGuardFile->remove();
                    ORIGIN_LOG_EVENT_IF(!isOk) << " [warning] Could not remove guard file " << mGuardFileNamePath;
                    mGuardFile->deleteLater();
                    mGuardFile =  NULL;
                    ORIGIN_LOG_EVENT << "[-----] Deleted guard file: " << mGuardFileNamePath;
                    mGuardFileNamePath.clear();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Helpers
            bool setFileAttributes(QString guardFileNamePath, bool isCreating)
            {
                bool isSetAttribs;
                guardFileNamePath = QDir::toNativeSeparators(guardFileNamePath);
#if defined(ORIGIN_PC)
                DWORD attribs = isCreating? (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_TEMPORARY) : (FILE_ATTRIBUTE_NORMAL);
                std::wstring guardFileNamePathW = guardFileNamePath.toStdWString();
                isSetAttribs = SetFileAttributes(guardFileNamePathW.c_str(),attribs);
                ORIGIN_ASSERT(isSetAttribs);
#elif defined(ORIGIN_MAC)
                isSetAttribs =  true;
#else
#error setFileAttributes: Must specialize for other platform.
#endif
                return isSetAttribs;
            }
        }
    }
}
