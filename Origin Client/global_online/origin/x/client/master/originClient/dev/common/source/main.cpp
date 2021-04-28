///////////////////////////////////////////////////////////////////////////////
// main.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "OriginApplication.h"
#include "services/debug/DebugService.h"

#include "version/version.h"

#include <string.h>

#include <EAStdC/EAString.h>

#ifdef ORIGIN_MAC
#ifndef DEBUG
#include <unistd.h>
#endif
#endif

extern "C" Q_DECL_EXPORT const char* OriginClientVersion();
extern "C" Q_DECL_EXPORT int OriginApplicationStart(int argc, char** argv);

// TBD: the following is only here to satisfy the legacy OriginClient.lib; eliminate at earliest convenience.
bool g_bStartNotAllowed;
uint64_t g_startupTime = 0;

int main(int argc, char** argv)
{
    //	Set defaults for Qt TTY Output
#ifdef _DEBUG
    bool enableTTYOutput = true;
#else
    bool enableTTYOutput = false;
#endif

    for (int i = 0; i < argc; ++i)
    {
        const char* STARTUPTIME_ARG_STR = "-startupTime:";

        //unfortunately, we have to perpetuate this hack to handle case where 9.0 is already installed, but user launches ITO install with a disc that has 8.4 installer on it.
        //That version of the installer launches Origin with this flag, expecting to write out a registry value, and then exit.
        //if we don't detect this flag and exit, users will end up seeing Origin relaunched after exiting
        if(strcmp(argv[i], "/setAcceptedEULAVersion") == 0)
        {
            // EBIBUGS-18036
            // EBIBUGS-14086
            // Return true to force-quit Origin. This is a result of the 8.5 installer still starting Origin with the 8.4-only command-line arg 
            // "/setAcceptedEULAVersion", then starting Origin again with the original command-line args.
            return 0;
        }
        
        // TTY setting has to be done before exec.
        else if (strcmp(argv[i], "-Origin_EnableTTY") == 0)
        {
            enableTTYOutput = true;
        }

        else if (EA::StdC::Stristr(argv[i], STARTUPTIME_ARG_STR) != NULL)
        {
            //const char* where = argv[i] + EA::StdC::Strlen(STARTUPTIME_ARG_STR);
            const char* where = argv[i] + strlen(STARTUPTIME_ARG_STR);
            g_startupTime = EA::StdC::AtoI64(where);

            // remove argument
            for (int j = i + 1; j < argc; ++j)
                argv[j - 1] = argv[j];
            argv[--argc] = NULL;
            --i;
        }
    }

    Q_INIT_RESOURCE(ebisu);
    
#ifdef ORIGIN_MAC
#ifndef DEBUG
    {
        // Simple check this was started from the bootstrap/prevent "regular" users from directly starting the client
        char* value = getenv("mac-param");
        if (!value)
            return -1;
        
        int fd = atoi(value);
        if (lseek(fd, 0, SEEK_SET) < 0)
            return -1;
        
        time_t currT = time(NULL);
        time_t fileT = 0;
        ssize_t dataSize = read(fd, &fileT, sizeof(fileT));
        close(fd);
        
        if (dataSize != sizeof(fileT) || (currT < fileT || currT > (fileT + 60)))
            return -1;
    }
#endif
    
    if (argc > 0 && argv && argv[0])
    {
        // Define where to find the platform plugin
        const char* macOSPath = strstr(argv[0], "/MacOS/");
        if (macOSPath)
        {
            ptrdiff_t len = macOSPath - argv[0];
        
            const char* pluginDirName = "/plugins/platforms";
            char* pluginPath = new char[len + strlen(pluginDirName) + 1];
            strncpy(pluginPath, argv[0], len);
            pluginPath[len] = '\0';
            strcat(pluginPath, pluginDirName);
            
            qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", QByteArray(pluginPath));
            qputenv("QT_QPA_PLATFORM", QByteArray("cocoa"));
            
            delete[] pluginPath;
        }
    }
#endif
    
    OriginApplication originApplication(argc, argv);
    
    // Set mac library paths in order to load images correctly in the client
#ifdef ORIGIN_MAC
    QDir dir(QApplication::applicationDirPath());
    dir.cd("../plugins");
    QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif
    
    if (!enableTTYOutput)
    {
        Origin::Services::DebugService::disableDebugMessages();
    }

    return originApplication.exec();
}

Q_DECL_EXPORT int OriginApplicationStart(int argc, char** argv)
{
    return main(argc, argv);
}

Q_DECL_EXPORT const char* OriginClientVersion()
{
  return EALS_VERSION_P_DELIMITED;
}
