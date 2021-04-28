#!/usr/bin/python3

import os, sys
import time, inspect, datetime
import string, fileinput, signal
import random, glob, atexit
from threading import current_thread

############################################
############ GLOBAL DEFINES ################
############################################

#########################################################################
########## !!! DO NOT MODIFY THESE VALUES DURING EXECUTION !!! ##########

############### MAIN SETTINGS ###############
gBuild = 'final'
gNumDigits = 5
gCase = list()
gTestGroup = ''
gOriginalConfigFile = 'stress.cfg'
gStressClientPath = '../'
gPrintOnly = False
gLogFlag = 1
gLogPath = "test_results.log"

gStressClientsLogsFolder = "logs_stressclients"
gTestArtifactsFolder = "TestArtifacts"
gTestOutputFolder = gTestArtifactsFolder + "/TestOutput"
gTestInputFolder = gTestArtifactsFolder + "/TestInput"

gPrintTraceLog = False
gDelayBetweenTests = 180

############### FUNCTIONAL SETTINGS ###############
# gModifiedConfigFile1 is used by default by all functions
gModifiedConfigFile1 = gTestOutputFolder + '/stress_tmp.cfg'
# for modifying or starting clients with this function it needs to be passed on function call
gModifiedConfigFile2 = gTestOutputFolder + '/stress_tmp2.cfg'

############### GLOBAL VARIABLES ###############
# At anytime of execution this should be properly ordered
gStartedClients = []
# Hardware safe limit
gAbsoluteMaxClients = 1200

########## !!! DO NOT MODIFY THESE VALUES DURING EXECUTION !!! ##########
#########################################################################

############################################
########### ARGUMENTS VALIDATION ###########
############################################
def parseInputArguments():
    global gBuild, gCase, gStressClientPath, gTestGroup, gPrintOnly
    
    if len(sys.argv) < 2 or sys.argv[1] == '--help' or sys.argv[1] == '-h':
        printUsage()
        sys.exit()

    for option in sys.argv[1:]:
        if option.find('--case=') != -1 or option.find('-c=') != -1:
            if option.split("=")[1].find(',') != -1:
                gCase = option.split("=")[1].split(',')

            elif option.split('=')[1].find('-') != -1 and option.split('=')[1].split('-')[0] != '' and option.split('=')[1].split('-')[1] != '':
                for i in range( int(option.split('=')[1].split('-')[0]) , int(option.split('=')[1].split('-')[1]) + 1):
                    gCase.append(i)
            else:
                gCase.append(option.split("=")[1])

        elif option.find('--build=') != -1 or option.find('-b=') != -1:
            gBuild = str(option.split("=")[1])
        elif option.find('--path=') != -1 or option.find('-p=') != -1:
            gStressClientPath = str(option.split("=")[1])
        elif option.find('--testgroup=') != -1 or option.find('-t=') != -1:
            gTestGroup = str(option.split("=")[1])
        elif option.find('--log=') != -1 or option.find('-l=') != -1:
            gLogFlag = int(option.split("=")[1])
        elif option.find('--print') != -1:
            gPrintOnly = True
            return
        else:
            printUsage()
            sys.exit()

    if gTestGroup == '':
        logLine("ERR", "Test group must be specified")
        printUsage()
        sys.exit()

    if int(gCase[0]) < 0:
        printUsage()
        sys.exit()

    if gBuild != "debug" and gBuild != "final":
        logLine("ERR", "Invalid build argument \"" + gBuild + "\'")
        printUsage()
        sys.exit()

    logLine("INFO", "Started with build \"" + gBuild + "\" at path " + gStressClientPath)


def graceful_shutdown():
    logLine("INFO", "Shutting down...")
    removeClients(len(gStartedClients))
    removeCertFile()
    sys.exit()

def interruptHandler(signum, frame):
    # Make sure all clients are closed
    logLine("ERR", "GOT CTRL+C, shutting down")
    # This will trigger the exitHandler
    sys.exit()

def exitHandler():
    # Wait for all clients to be able to generate the PID file
    time.sleep(1)
    # Make sure all clients are closed
    graceful_shutdown()
    
def printUsage():
    print('Print available tests:')
    print('        --print             prints available tests')
    print('Usage for running tests:')
    print('        --testgroup=  or -t=<test group>')
    print('        --case=       or -c=<test number> --- Supported syntax: integer: "1", list of integeres "1,2,3", range of integeres "1-3"')
    print('        --build=      or -b=<final|debug for the stress clients>')
    print('        --path=       or -p=<path to stress client>')
    print('        --log=        or -l=<0 or 1 for enable/disable>')

############################################
############ HELPER FUNCTIONS ##############
############################################
### Severities:
## TRACE = debug logging
## DESC = description of tests
## WARN = warnings
## FAIL = test fails 
## ERR = program errors
###
def logLine(severity, line, print_header = True):
    global gPrintTraceLog
    if severity == "TRACE" and gPrintTraceLog == False:
        return
    
    color = ''
    boldText = '\033[01m'
    redColor='\033[31m'
    orangeColor = '\033[33m'
    greenColor = '\033[32m'
    resetText = '\033[0m'

    callerName = str(sys._getframe(1).f_code.co_name)
    callerName = (callerName[:20] + "..():") if len(callerName) > 22 else callerName + "():"

    header = severity + (6 - len(severity)) * ' ' + datetime.datetime.now().strftime('%d-%m-%Y %H:%M:%S') + ' '
    header = header + current_thread().name + (12 - len(current_thread().name)) * ' ' + callerName + (25 - len(callerName) ) * ' '

    if severity == "ERR":
        color = redColor
    elif severity == "FAIL":
        color = orangeColor
    elif severity == "DESC":
        color = greenColor
    elif severity == "WARN":
        color = orangeColor

    if print_header == True:
        print( color + header + resetText +  str(line))
    else:
        print(str(sys._getframe(1).f_code.co_name) + '():' + (12 - len(str(sys._getframe(1).f_code.co_name)) ) * ' ' +  str(line)) 

def sleepLog(seconds, sleepIncrement=60, log=True, logProgress=False):
    if seconds > 5 and log == True:
        logLine("INFO", "Sleeping for " + str(seconds) + " seconds")

    if seconds > sleepIncrement:
        for i in range(0, int(seconds/sleepIncrement) ):
            time.sleep(sleepIncrement)
            if logProgress == True:
                logLine("INFO", "Passed " + str(sleepIncrement)+ " seconds")
        time.sleep(seconds%sleepIncrement)
    else:
        time.sleep(seconds)

def changeConfigParamater(configParameter, configFile = gModifiedConfigFile1):
    if "=" not in configParameter:
        logLine("ERR", "Invalid parameters")
        return

    parameterName=configParameter.split("=")[0]+"="
    for line in fileinput.input(configFile, inplace=1):
        if parameterName in line:
            line = configParameter+"\n"
        sys.stdout.write(line)

def getStringIndex(numericalIndex):
    if numericalIndex >= 99999:
        logLine('ERR', 'Maximum client index ( 99999 ) reached, shutting down.')
        graceful_shutdown()

    stringIndex = str(numericalIndex)
    stringIndex = '0' * (gNumDigits - len(stringIndex)) + stringIndex
    return stringIndex

## Starts Clients starting with "startNumber" index to "stopNumber" index including both of them
## Safe flag is for checking and making sure duplicate Clients are not started - potentialy slow operation, default True
def startClients(startNumber, stopNumber, configFile = gModifiedConfigFile1, checkForErrors=False, safe=True):
    logLine("INFO", "Called with range " + str(startNumber) + ":" + str(stopNumber) + " cfg: " + configFile + ", starting clients...")

    if (startNumber < 1 or stopNumber < 1 or stopNumber < startNumber ):
        logLine('ERR', 'Invalid parameters')
        return

    if (len(gStartedClients) >= gAbsoluteMaxClients):
        logLine('ERR', 'Maximum number of clients (' + str(gMaxClients) + ') reached, shutting down.')
        graceful_shutdown()

    count = 0
    currentWorkingDirectory = os.getcwd()
    for clientIndex in range(startNumber, stopNumber + 1):
        if safe == True and clientIndex in gStartedClients:
            logLine('WARN', 'Trying to start duplicate instance ' + str(clientIndex))
            continue

        stringIndex = getStringIndex(clientIndex)
        sysstr = gStressClientPath + 'gsrvstress' + gBuild[0] + ' ' + currentWorkingDirectory + '/' +  configFile + ' ' + stringIndex + ' &> ' + currentWorkingDirectory + '/' + gStressClientsLogsFolder + '/gsrvstress' + stringIndex + '.log'
        logLine("TRACE", 'Executing: ' + sysstr )
        count = count + 1
        gStartedClients.append(clientIndex)
        os.system('(' + sysstr + ')&')
        time.sleep(0.2)

    logLine("INFO", "Successfully started " + str(count) + " clients out of " + str(stopNumber + 1 - startNumber))
    logLine("INFO", "There are currently " + str(len(gStartedClients)) + " started clients")

    if checkForErrors == True:
        logLine("INFO","Checking stressclient " + str(startNumber) + " log file for errors.")
        sleepLog(5)
        return checkClientForErrors(startNumber)


def stopClients(startNumber, stopNumber):
    logLine("INFO", "Called with range " + str(startNumber) + ":" + str(stopNumber) + ", stopping clients...")

    if (startNumber < 1 or stopNumber < 1 or stopNumber < startNumber ):
        logLine('ERR', 'Invalid parameters')
        return

    count = 0
    for clientIndex in range(startNumber, stopNumber + 1):
        currentFile = 'gsrvstress' + gBuild[0] + str(clientIndex) + '.pid';
        if os.path.exists(currentFile):
            pidFile = open(currentFile, 'r')
            pid = pidFile.readline()
            pidFile.close()
            logLine("TRACE", "Stopping " + currentFile)
            count = count + 1
            os.system('kill -TERM ' + pid)
            try:
                gStartedClients.remove(clientIndex)
            except ValueError:
                logLine("TRACE", "Client " + str(clientIndex) + " was stopped, but not present in global vector")
        else:
            if clientIndex in gStartedClients:
                logLine("TRACE", "Client " + str(clientIndex) + " pid file was not found (client is not running), removing from global vector if present")
                gStartedClients.remove(clientIndex)

    logLine("INFO", "Successfully stopped " + str(count) + " clients out of " + str(stopNumber + 1 - startNumber))
    logLine("INFO", "There are currently " + str(len(gStartedClients)) + " started clients")

## Adds "number" of clients at the end
def addClients(number, configFile = gModifiedConfigFile1, checkForErrors=False, safe=False):
    if number < 1:
        return

    clientsNumber = STARTED_CLIENTS_LEN()
    if clientsNumber != 0:
        startNumber = gStartedClients[clientsNumber - 1] + 1
        stopNumber = gStartedClients[clientsNumber - 1] + number
    else:
        startNumber = 1
        stopNumber = number

    return startClients(startNumber, stopNumber, configFile, checkForErrors, safe)

## Removes "number" clients from the start
def removeClients(number):
    if number < 1:
        return

    clientsNumber = STARTED_CLIENTS_LEN()
    if clientsNumber == 0 or number < 1:
        return

    if clientsNumber - 1 < number:
        stopNumber = clientsNumber - 1
    else:
        stopNumber = number

    stopClients(gStartedClients[0], gStartedClients[stopNumber])

    
def removeCertFile():
    if os.path.exists(gTestArtifactsFolder + "/PrometheusCerts/gs-shared.cert.pem"):
        os.remove(gTestArtifactsFolder + "/PrometheusCerts/gs-shared.cert.pem")
    if os.path.exists(gTestArtifactsFolder + "/PrometheusCerts/gs-shared.key.pem"):
        os.remove(gTestArtifactsFolder + "/PrometheusCerts/gs-shared.key.pem")

# Check clients for obvious errors
def checkClientForErrors(clientIndex):
    stringIndex = getStringIndex(clientIndex)
    clientLogPath = gStressClientsLogsFolder + "/gsrvstress" + stringIndex + ".log"
    fileSize = int( os.path.getsize(clientLogPath) )
    if not os.path.exists(clientLogPath):
        logLine("ERR", "Can't find log file " + str(clientLogPath))
        return -1

    if fileSize > 100000:
        logLine("ERR", "File too big")
        return -1

    fileContents = open(clientLogPath).read()

    if "ERR_SYSTEM" in fileContents:
        logLine("ERR", 'Found "ERR_SYSTEM" in log file for client index ' + stringIndex + ', check the stress clients and blazeserver are configure properly ')
        return -1
    elif "GAMEMANAGER_ERR_INVALID_SCENARIO_NAME" in fileContents:
        logLine("ERR", 'Found "GAMEMANAGER_ERR_INVALID_SCENARIO_NAME" in log file for client index ' + stringIndex + ', check the blazeserver is configured for dirtycaststress scenario ')
        return -1
    elif "StressLogin Error" in fileContents:
        logLine("ERR", 'Found "StressLogin Error" in log file for client index ' + stringIndex + ', check stress login is enabled both for the blazeserver and stress clients ' )
        return -1
    elif "EA Servers are currently down" in fileContents:
        logLine("ERR", 'Found "EA Servers are currently down" in log file for client index ' + stringIndex + ', check the blazeserver is up and responsive ')
        return -1

    return 0

def getLastGameState(clientIndex):
    stringIndex = getStringIndex(clientIndex)
    clientLogPath = gStressClientsLogsFolder + "/gsrvstress" + stringIndex + ".log"
    if not os.path.exists(clientLogPath):
        logLine("ERR", "Can't find log file " + str(clientLogPath))
        return "err"

    fileSize = int( os.path.getsize(clientLogPath) )

    if fileSize > 100000:
        logLine("ERR", "File too big")
        return -1

    for line in reversed(list(open(clientLogPath))):
        # format is changing state kConnected -> kCreatedGame
        if "changing state" in line:        
            return line.split("-> ")[1]

    return "err"

def STARTED_CLIENTS_LEN():
    global gStartedClients
    return len(gStartedClients)

############################################
#### TESTS INITIAL VALIDATION AND SETUP ####
############################################
def applicationWideSetup():
    if not os.path.exists(gStressClientsLogsFolder):
        os.system('mkdir ' + gStressClientsLogsFolder)
    else:
        os.system('rm -r ' + gStressClientsLogsFolder)
        os.system('mkdir ' + gStressClientsLogsFolder)      

    if not os.path.exists(gTestOutputFolder):
        os.system('mkdir ' + gTestOutputFolder)
    
    #Handler for CTRL+C
    atexit.register(exitHandler)
    signal.signal(signal.SIGINT, interruptHandler)

    #Create copies of config files to be safely modified by the script
    os.system('cp ' + gStressClientPath + gOriginalConfigFile + ' '  + gModifiedConfigFile1)
    os.system('cp ' + gStressClientPath + gOriginalConfigFile + ' '  + gModifiedConfigFile2)

    os.system('chmod 644 ' + gModifiedConfigFile1)
    os.system('chmod 644 ' + gModifiedConfigFile2)