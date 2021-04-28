#!/usr/bin/python3

import os, sys
import time, inspect, datetime
import string, fileinput, signal
import random, glob, importlib
from inspect import getmembers, isfunction
import traceback

import SetupAndHelpers

####################################################################################################################################
#################################################### MAIN ##########################################################################
####################################################################################################################################
def printTestsDescription():
    files = glob.glob('*Tests.py')

    for file in files:
        print("#################################### " + str(file[:-3]) + " ####################################")

        testModule = importlib.import_module(file[:-3])
        functions_list = [o for o in getmembers(testModule) if isfunction(o[1])]

        for function in functions_list:
            if "Case" in str(function[0]):
                method_call = getattr(testModule, str(function[0]))
                method_call("print")

def testGroupSetup():

    ################################ LOAD TEST MODULE  #####################################################
    try:
        testModule = importlib.import_module(SetupAndHelpers.gTestGroup)
    except ModuleNotFoundError as err:
        SetupAndHelpers.logLine("ERR", "Test group not found: " + SetupAndHelpers.gTestGroup + ", err " + str(err))
        sys.exit()
    except SyntaxError as err:
        SetupAndHelpers.logLine("ERR", "Test group has a syntax error: " + SetupAndHelpers.gTestGroup + ", err " + str(err) )
        sys.exit()
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Invalid test group: " + SetupAndHelpers.gTestGroup + ", err " + str(err))
        sys.exit()

    ################################ CALL INITIAL CONFIG FOR THE MODULE  #####################################
    try:
        initialConfigAttr = getattr(testModule, "initialConfig")
    except AttributeError as err:
        SetupAndHelpers.logLine("ERR", "initialConfig function could not be found for test group " + str(SetupAndHelpers.gTestGroup) + ", err " + str (err))
        sys.exit()
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "initialConfig function could not be starte for test group " + str(SetupAndHelpers.gTestGroup) + ", err " + str(err))
        sys.exit()

    try:
        ret = initialConfigAttr()
        if ret == -1:
            SetupAndHelpers.logLine("ERR", "Initial config for testgroup " + str(SetupAndHelpers.gTestGroup) + " failed! ")
            return -1
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Error while calling initial config for  " + str(SetupAndHelpers.gTestGroup) + " err: " + str(err) )
        SetupAndHelpers.logLine("ERR", "Full traceback: \n" + str(traceback.format_exc()))
        return -1

    return testModule

def executeTest(testModule, case):
    ################################ CALL THE TEST  ############################################################    
    try:
        testCallMethod = getattr(testModule, "Case" + str(case))
    except AttributeError as err:
        SetupAndHelpers.logLine("ERR", "Case" + str(case) +" could not be found, err " + str(err))
        sys.exit()
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Case" + str(case) +" could not be started, err " + str(err))
        sys.exit()

    try:
        ret = testCallMethod()
        if ret == -1:
            SetupAndHelpers.logLine("FAIL", "Case" + str(case) + " failed! ")
        else:
            SetupAndHelpers.logLine("DESC", "Case" + str(case) + " passed! ")
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Error while running case " + str(case) + " err: " + str(err) )
        SetupAndHelpers.logLine("ERR", "Full traceback: \n" + str(traceback.format_exc()))

def mainFunction():

    SetupAndHelpers.parseInputArguments()

    if SetupAndHelpers.gPrintOnly:
        printTestsDescription()
        return

    SetupAndHelpers.applicationWideSetup()
    
    testModule = testGroupSetup()

    if testModule == -1:
        return

    for case in SetupAndHelpers.gCase:
        executeTest(testModule, int(case))

        if SetupAndHelpers.gCase.index(case) < len(SetupAndHelpers.gCase) - 1:
            SetupAndHelpers.logLine("INFO", "Sleeping " + str(SetupAndHelpers.gDelayBetweenTests) + " between tests ")
            SetupAndHelpers.sleepLog(SetupAndHelpers.gDelayBetweenTests, log=False)



## Start the program
mainFunction()
