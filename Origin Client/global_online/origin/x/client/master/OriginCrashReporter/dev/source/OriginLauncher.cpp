// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "OriginLauncher.h"

#ifdef ORIGIN_MAC

#include <sys/resource.h>
#include <errno.h>
#include <unistd.h>

#include <QFileInfo>

#endif

#ifdef ORIGIN_PC

#include <windows.h>
#include <EAStdC/EAProcess.h>


void ExecNoWait(wchar_t* executablePath, wchar_t* args, wchar_t* workingDir)
{
    wchar_t commandLine[MAX_PATH*5];
    STARTUPINFOW startupInfo;
    memset(&startupInfo,0,sizeof(startupInfo));
    PROCESS_INFORMATION procInfo;
    memset(&procInfo,0,sizeof(procInfo));
    
    int size = swprintf_s(commandLine, L"%s %s", executablePath, args ? args : L"");
    if( size > 0 )
    {
        startupInfo.cb = sizeof(startupInfo);
        
        BOOL result = ::CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, workingDir, &startupInfo, &procInfo);
        if( result == FALSE )
        {
            if (procInfo.hProcess)
            {
                ::CloseHandle(procInfo.hProcess);
            }
            
            if (procInfo.hThread)
            {
                ::CloseHandle(procInfo.hThread);
            }
        }
    }
}

#endif


OriginLauncher::OriginLauncher( QString originPath, QObject *parent )
    : QObject(parent)
    , mOriginPath( originPath )
{
}


void OriginLauncher::launchOrigin()
{
    
#ifdef ORIGIN_MAC
    
    // We need to point to the bootstrap, not the current exe
    QFileInfo file(mOriginPath);
    QString bootstrap = file.absolutePath().append("/Origin");
    
    QByteArray asciiPath = bootstrap.toLocal8Bit();
    char* originExecutable = asciiPath.data();

    //
    // To properly execute Origin as an app, we'll execute
    // "/usr/bin/open -a <path-to-origin> --args <origin-args>
    //
    // This allows the system to prevent multiple instances of Origin from running.
    //
    
    printf("Launching: %s\n", originExecutable);
    
    // Create a new process.
    int childPid = fork();
    
    // If we are the new child process
    if (! childPid)
    {
		char executable[] = "/usr/bin/open";
        // close all file descriptors beyond stdin, stdout, and stderrr.
        struct rlimit lim;
        if (getrlimit( RLIMIT_NOFILE, &lim ) != 0)
            lim.rlim_cur = 4096;

        for ( rlim_t i = 3; i != lim.rlim_cur; ++i ) close( i );
        
        // Execute the crash helper.
        char parms[] = "-Origin_Restart";
        char* argv[] = { executable, (char*)"-a" , originExecutable, (char*)"--args", parms, ( char* ) 0 };
        execv(executable, argv);
        printf(" Error %d\n", errno);
        perror("OriginLauncher");
    }
    
#endif
    
#ifdef ORIGIN_PC
    
    wchar_t processPath[MAX_PATH];
    wchar_t processDir[MAX_PATH];
    wchar_t arguments[MAX_PATH];
    mOriginPath.toWCharArray( processPath );
    processPath[ mOriginPath.length() ] = (wchar_t) 0;
    EA::StdC::GetCurrentProcessDirectory(processDir);
    wcscpy(arguments, L"-Origin_Restart");
    ExecNoWait(processPath, arguments, processDir);
    
#endif

}
