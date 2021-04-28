/*************************************************************************************************/
/*!
*     \File monitor.cpp
*
*     \Description
*         Monitor application for restarting Blaze automatically
*
*     \Notes
*         None
*
*     \Copyright
*         (c) Electronic Arts. All Rights Reserved.
*
*     \Version
*         08/15/2008 (mickeyw) First version
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/controller/processcontroller.h"
#include "framework/util/shared/blazestring.h"
#include "EASTL/list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <time.h>

namespace monitor
{
    /*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define MAX_PID_SIZE 10
#define MAX_COMMAND_SIZE 8192
#define MAX_OPTIND 15
#define INVALID -99
#define WAIT_TIME 60

    typedef eastl::list<long> CrashTimeList;

    static char* sPidFile;
    static char* sMonitorExternal;
    static char* sMonitorException;
    static uint32_t uAllowedCrashCount;
    static uint32_t uAllowedCrashTime;
    static uint32_t uRestartDelay;
    static char* sLogFile;

    static pid_t sChildPid;
    static CrashTimeList crashList;
    static uint32_t uTotalCrashes;
    static char sExternalCommand[MAX_COMMAND_SIZE];
    static bool bRestartRequired;

    /*** Public Methods ******************************************************************************/

    // Print usage
    static void printUsage()
    {
        printf("monitor [-p PID file] [-e monitor-external]"
            " [-c allowed crash count] [-t allowed crash time]"
            " [-r delay restart in seconds] [-l log file name]\n");
        printf("    -p  sets PID file name\n");
        printf("    -e  sets monitor-external location\n");
        printf("    -m  sets monitor-exception location\n");
        printf("    -c  sets allowed number of server restarts\n");
        printf("    -t  sets allowed restart time interval\n");
        printf("    -r  sets the number of seconds to wait before restart\n");
        printf("    -l  sets log file name\n");
    }

    // Get options from command line
    static bool getOptions(int c, char** v)
    {
        int opt = 0;

        while ((opt = getopt(c, v, "p:e:m:c:t:r:l:")) != -1)
        {
            if (optind > MAX_OPTIND)
            {
                break;
            }

            switch (opt)
            {
            case 'p':
                sPidFile = optarg;
                break;
            case 'e':
                sMonitorExternal = optarg;
                break;
            case 'm':
                sMonitorException = optarg;
                break;
            case 'c':
                uAllowedCrashCount = atoi(optarg);
                break;
            case 't':
                uAllowedCrashTime = atoi(optarg);
                break;
            case 'r':
                uRestartDelay = atoi(optarg);
                break;
            case 'l':
                sLogFile = optarg;
                break;
            case '?':
                printUsage();
                return false;
            }
        }

        if (sPidFile == nullptr || sMonitorExternal == nullptr
            || uAllowedCrashCount == 0 || uAllowedCrashTime == 0
            || sLogFile == nullptr)
        {
            printUsage();
            return false;
        }

        return true;
    }

    // Remove PID file
    static void removePidFile()
    {
        remove(sPidFile);
    }

    // Create PID file
    static void createPidFile()
    {
        FILE* pFile = fopen(sPidFile, "w");

        if (pFile != nullptr)
        {
            char sPid[MAX_PID_SIZE];
            blaze_snzprintf(sPid, MAX_PID_SIZE, "%i", getpid());
            fputs(sPid, pFile);
        }
        else
        {
            perror("Error creating Monitor PID file.");
            removePidFile();
            exit(0);
        }

        fclose(pFile);
    }

    // Check to see if PID file exists
    static bool pidFileExists()
    {
        if (FILE* pFile = fopen(sPidFile, "r"))
        {
            fclose(pFile);
            return true;
        }

        return false;
    }

    // Signal handler
    static void signalHandler(int sig)
    {
        kill(sChildPid, sig);
        removePidFile();
    }

    // Signal handler for defunct / zombie child process
    static void defunctHandler(int sig)
    {
        while (waitpid(-1, nullptr, WNOHANG) > 0);
    }

    static void userSignalHandler(int sig)
    {
        if (sig == SIGUSR2)
        {
            printf("Monitor received SIGUSR2 from child - monitor no longer needs to restart the endpoint");
            bRestartRequired = false;
        }
        else
        {
            printf("Monitor received SIGUSR1 from child.");
            std::string threadCommand;

            threadCommand = sMonitorException;
            threadCommand = threadCommand + " " + sPidFile;

            system(threadCommand.c_str());
        }
    }

    // Create a text file which will be used to send notification email
    static bool createEmailMessage()
    {
        std::ofstream outFile("./textfile.txt");
        if (outFile.is_open())
        {
            outFile << "Blaze server cannot be started." << std::endl << std::endl;
            outFile.close();
            system("echo \"Server:\" >> ./textfile.txt");
            system("hostname >> ./textfile.txt");
            system("echo >> ./textfile.txt");
            system("echo \"Time:\" >> ./textfile.txt");
            system("date >> ./textfile.txt");
            system("echo >> ./textfile.txt");
            system("echo \"Cause:\" >> ./textfile.txt");
            system("echo \"Could not start the server because of problems with the monitor-external script. Please check that the monitor-external script exists and is able to run (try checking python path settings), and restart the server.\" >> ./textfile.txt");
            return true;
        }
        else
        {
            return false;
        }
    }

    // Delete the text file storing the email message, once the email is sent
    static void removeEmailMessage()
    {
        remove("./textfile.txt");
    }

    // Reads in both MAIL_RECIPIENTS and MAIL_SENDER from the monitor-external.cfg file, and sends an email message containing the message stored in the text file
    static void  sendEmail()
    {
        bool foundEmail = false;
        std::string line, email, sender;
        std::size_t pos;
        std::ifstream myFile("../bin/monitor-external.cfg");
        if (myFile.is_open())
        {
            while (!myFile.eof())
            {
                getline(myFile, line);
                if (!foundEmail)
                {
                    pos = line.find("MAIL_RECIPIENTS");
                    if (pos != std::string::npos)
                    {
                        foundEmail = true;
                        email = line.substr(18, 100);
                    }
                }
                else if (foundEmail)
                {
                    pos = line.find("MAIL_SENDER");
                    if (pos != std::string::npos)
                    {
                        sender = line.substr(14, 100);
                    }
                }
            }
            blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "mailx -s \"[Blaze Server][Monitor] Blaze server not started\" %s -- -r \"%s\" < ./textfile.txt", email.c_str(), sender.c_str());
            system(sExternalCommand);
            myFile.close();
        }
        else
        {
            return;
        }
    }

    // Main
    int monitor_main(int argc, char** argv)
    {
        // Check for valid arguments
        if (argc == 1)
        {
            printUsage();
            exit(0);
        }

        // Exit if there are missing arguments
        if (getOptions(argc, argv) == false)
        {
            exit(0);
        }

        // Exit if monitor-external script doesn't exist
        if (FILE* pFile = fopen(sMonitorExternal, "r"))
        {
            fclose(pFile);
            // If there is an error starting up monitor-external script
            if (system(sMonitorExternal) != 0)
            {
                // Only send one email notification, for the master
                if (blaze_strcmp(sPidFile, "../bin/monitor_master.pid") == 0)
                {
                    printf("Error: Unable to start monitor-external file.\n");
                    if (createEmailMessage())
                    {
                        sendEmail();
                        removeEmailMessage();
                    }
                }
                exit(0);
            }
        }
        // Send email notification and exit if monitor-external doesn't exist
        else
        {
            printf("Error: monitor-external file does not exist.\n");
            // Only send one email notification, for the master
            if (blaze_strcmp(sPidFile, "../bin/monitor_master.pid") == 0)
            {
                if (createEmailMessage())
                {
                    sendEmail();
                    removeEmailMessage();
                }
            }
            exit(0);
        }

        int iStatus = 0;

        signal(SIGHUP, signalHandler);
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGCHLD, defunctHandler);
        signal(SIGUSR1, userSignalHandler);
        signal(SIGUSR2, userSignalHandler);

        createPidFile();

        uTotalCrashes = 0;
        bRestartRequired = true;

        char sPid[MAX_PID_SIZE];
        blaze_snzprintf(sPid, MAX_PID_SIZE, "%i", getpid());

        char sMonitorArg[14];
        blaze_snzprintf(sMonitorArg, 14, "--monitor-pid");

        char* sBlazeArgs[argc - MAX_OPTIND + 3];
        int j = 0;
        for (int i = MAX_OPTIND; i < argc; ++i)
        {
            sBlazeArgs[j] = argv[i];
            printf("Blaze arg: %s\n", sBlazeArgs[j]);
            ++j;
        }
        sBlazeArgs[j] = &sMonitorArg[0];
        printf("Blaze arg: %s\n", sBlazeArgs[j]);
        ++j;
        sBlazeArgs[j] = &sPid[0];
        printf("Blaze arg: %s\n", sBlazeArgs[j]);
        ++j;
        sBlazeArgs[j] = nullptr;


        while (1)
        {
            // Update Nagios status file
            blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s status -s=OK", sMonitorExternal);

            if (fork() == 0)
            {
                sleep(3);
                system(sExternalCommand);
                return 0;
            }

            // Fork and wait on the executable being monitored
            if ((sChildPid = fork()) == 0)
            {
                int retVal = execvp(argv[MAX_OPTIND], sBlazeArgs);
                if (retVal == -1)
                {
                    printf("Monitor faild to exec command %s err:%d\n", argv[MAX_OPTIND], errno);
                }
                return 0;
            }

            //blocks until the child (the endpoint) terminates
            int iChildPid = waitpid(sChildPid, &iStatus, 0);

            // Don't restart if PID file is missing
            if (pidFileExists() == false)
            {
                break;
            }

            int iExitCode = INVALID;
            int iSignal = INVALID;

            if (WIFEXITED(iStatus))
            {
                iExitCode = WEXITSTATUS(iStatus);
            }
            if (WIFSIGNALED(iStatus))
            {
                iSignal = WTERMSIG(iStatus);
            }

            //if the child exited normally, we can bail and proceed to monitor shutdown
            if (iExitCode == Blaze::ProcessController::EXIT_NORMAL)
            {
                break;
            }

            // Add latest restart time to list
            crashList.push_back(time(nullptr));

            // Find out the total number of crashes in the list
            // Remove entries that are older than (current time - allowed crash time)
            uint32_t uCount = 0;
            CrashTimeList::iterator itr = crashList.begin();
            while (itr != crashList.end())
            {
                if (*itr < (time(nullptr) - (uAllowedCrashTime * 60)))
                {
                    itr = crashList.erase(itr);
                    continue;
                }
                else
                {
                    uTotalCrashes = crashList.size() - uCount;
                    break;
                }

                uCount++;
                itr++;
            }

            if (WCOREDUMP(iStatus))
            {
                // If crashed while shutting down (draining or not), send out "shutdownCrash" notification
                // Else send out "overcrashes" notification and stop monitor if total crashes == allowed crashes
                // Else if total crashes < allowed crashes, send out "restart" notification            
                // Else exit monitor if total crashes > allowed crashes, send out "notrestarted" notification
                if (!bRestartRequired)
                {
                    blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=shutdownCrash -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                        sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                }
                else if (uTotalCrashes == uAllowedCrashCount)
                {
                    blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=overcrashes -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                        sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                }
                else if (uTotalCrashes < uAllowedCrashCount)
                {
                    blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=restart -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal, sPidFile,
                        iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                }
                else
                {
                    blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=notrestarted -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                        sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                    bRestartRequired = false;
                }
            }
            else
            {
                // server reset
                // Send out "overrestarts" notification and stop monitor if total restarts >= allowed restarts
                // Else restart and send notification related to exit code
                if (uTotalCrashes > uAllowedCrashCount)
                {
                    blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=overrestart -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                        sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                    bRestartRequired = false;
                }
                else
                {
                    if (iExitCode == Blaze::ProcessController::EXIT_FATAL)
                    {
                        blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=fatal -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                            sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                        bRestartRequired = false;
                    }
                    else if (iExitCode == Blaze::ProcessController::EXIT_RESTART)
                    {
                        blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=killed -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                            sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                    }
                    else if (iExitCode == Blaze::ProcessController::EXIT_FAIL)
                    {
                        blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=failed -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                            sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                        bRestartRequired = false;
                    }
                    else if (iExitCode == Blaze::ProcessController::EXIT_MASTER_TERM)
                    {
                        blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s mail -t=masterterm -p=%s -i=%i -exit=%i -sig=%i -cw=%i -ct=%i", sMonitorExternal,
                            sPidFile, iChildPid, iExitCode, iSignal, uAllowedCrashTime, uTotalCrashes);
                    }
                }
            }

            // Mail out notification
            if (fork() == 0)
            {
                system(sExternalCommand);
                return 0;
            }

            // Rotate died log
            blaze_snzprintf(sExternalCommand, MAX_COMMAND_SIZE, "%s rotatelog -l=%s", sMonitorExternal, sLogFile);
            if (fork() == 0)
            {
                system(sExternalCommand);
                return 0;
            }

            // Break and exit if does not need to restart (ie. if total crashes > allowed crashes, or process initiated not-to-restart),
            if (!bRestartRequired)
            {
                break;
            }

            unsigned int timeLeft = uRestartDelay;
            do
            {
                timeLeft = sleep(timeLeft);
            } while (timeLeft > 0);
        }

        removePidFile();

        return 0;
    }
}