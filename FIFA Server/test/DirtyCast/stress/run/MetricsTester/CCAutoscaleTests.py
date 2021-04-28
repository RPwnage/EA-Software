#!/usr/bin/python3

import SetupAndHelpers
import QueryPrometheus
import QueryGraphite

###################################################################################
###################### CONFIG AND HELPERS #########################################
###################################################################################
gBlazeServiceName = "gosblaze-dvladu-pc"
gRedirector = "winter15.gosredirector.sdev.ea.com"
gEnv = "dev"

def initialConfig():
    ########################################
    # 1 is DEDICATED SERVER + VOIP_DISABLED
    # 2 is NETWORK_DISABLED + VOIP_PEER_TO_PEER
    # 3 is PEER_TO_PEER_FULL_MESH + VOIP_DISABLED
    # 4 is CLIENT_SERVER_PEER_HOSTED
    ########################################

    #### Config one setup (VOIP) #### 
    # Change those configs here:
    SetupAndHelpers.changeConfigParamater("SERVICE=" + gBlazeServiceName)
    SetupAndHelpers.changeConfigParamater("RDIRNAME=" + gRedirector)
    SetupAndHelpers.changeConfigParamater("ENVIRONMENT=" + gEnv)

    # Change this configs in tests below
    SetupAndHelpers.changeConfigParamater("GAME_MODE=2")
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=150")
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=20")
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=30")
    
    #### Config two setup (GAME) ####
    # Change those configs here:
    SetupAndHelpers.changeConfigParamater("SERVICE=gosblaze-dvladu-pc-1", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("RDIRNAME=winter15.gosredirector.sdev.ea.com", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("ENVIRONMENT=dev", SetupAndHelpers.gModifiedConfigFile2)

    # Change this configs in tests below
    SetupAndHelpers.changeConfigParamater("GAME_MODE=3", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=150", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=4", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=4", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=20", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=30", SetupAndHelpers.gModifiedConfigFile2)


def getMetricParameters(mode="concierge", env=gEnv, site=".*", host= ".*", product=gBlazeServiceName, deployinfo=".*", port = ".*"):
    ret = 'mode=~"' + mode + '",env=~"' + env + '",site=~"' + site + '",host=~"' + host +'",'
    ret = ret + 'product=~"' + product + '",deployinfo=~"' + deployinfo + '",port=~"' + port + '"'
    return ret

###################################################################################
################################### TESTS #########################################
###################################################################################
def Case0(mode="execute"):
    SetupAndHelpers.logLine("INFO", "PROTOTYPE", False if "print" in mode else True)
    if mode == "print":
        return

    SetupAndHelpers.addClients(52)
    SetupAndHelpers.sleepLog(60)
    SetupAndHelpers.removeClients(52)

def Case1(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Small # of clients. No scale-up/scale-down triggered.", False if "print" in mode else True)
    if mode == "print":
        return

    SetupAndHelpers.addClients(52)
    SetupAndHelpers.sleepLog(120)
    SetupAndHelpers.removeClients(52)


def Case2(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Significant # of clients. Slow rising. Scale-up/scale-down triggered", False if "print" in mode else True)
    if mode == "print":
        return
        

    SetupAndHelpers.addClients(52)
    SetupAndHelpers.sleepLog(120)

    for i in range(1,20):
        SetupAndHelpers.addClients(20)
        SetupAndHelpers.sleepLog(25)

    SetupAndHelpers.sleepLog(10 * 60)
    SetupAndHelpers.removeClients(len(SetupAndHelpers.gStartedClients))
    

def Case3(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Big # of clients. Slow rising. Random add/remove clients. Scale-up/scale-down triggered", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.addClients(100)
    SetupAndHelpers.sleepLog(60)

    min_clients = 100
    max_clients = 600

    for i in range(1, 300):
        if ( len(SetupAndHelpers.gStartedClients) < max_clients ):
            SetupAndHelpers.addClients( random.randint(0, round( len(SetupAndHelpers.gStartedClients) / 2 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 20) )

        if (len(SetupAndHelpers.gStartedClients) > min_clients ):
            SetupAndHelpers.removeClients( random.randint( 0 , round( len(SetupAndHelpers.gStartedClients) / 6 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 20) )

        SetupAndHelpers.sleepLog(random.randint(0, 20) )


    SetupAndHelpers.sleepLog(120)
    SetupAndHelpers.removeClients( len(SetupAndHelpers.gStartedClients) )

def Case4(mode="execute"):
    SetupAndHelpers.logLine("INFO","Description: Big # of clients. Fast rising. Scale-up/scale-down triggered ", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=1200")
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=600")

    SetupAndHelpers.addClients(600)
    SetupAndHelpers.sleepLog(10 * 60)
    SetupAndHelpers.removeClients(600)

def Case5(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Test using 2 blaze servers. Small # of clients. Slow rising. No scale-up/scale-down triggered", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.addClients(52, gModifiedConfigFile1)
    SetupAndHelpers.addClients(52, gModifiedConfigFile2)

    SetupAndHelpers.sleepLog(120)
    SetupAndHelpers.removeClients( len(SetupAndHelpers.gStartedClients) )

def Case6(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Subsequent requests. Big # of clients. Fast rising. Scale-up/scale-down triggered", False if "print" in mode else True)
    if mode == "print":
        return
        
def Case7(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Big # of clients. Fast rising. Scale-up/scale-down triggered. Scale-out/scale-in triggered", False if "print" in mode else True)
    SetupAndHelpers.logLine("WARN", "CPU REQUESTS VALUE SHOULD BE SET ACCORDINGLY FOR THIS TEST", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=600")
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=60")

    SetupAndHelpers.addClients(300)
    SetupAndHelpers.sleepLog(500)
    SetupAndHelpers.removeClients(len(SetupAndHelpers.gStartedClients))

def Case8(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description: Big # of clients. Fast rising. Scale-up/scale-down should not trigger", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=5")

    for i in range(1,10):
        SetupAndHelpers.addClients(200)
        SetupAndHelpers.sleepLog(15)
        SetupAndHelpers.removeClients(len(SetupAndHelpers.gStartedClients))
        SetupAndHelpers.sleepLog(30)

def Case9(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description (NOT IN TEST PLAN): Big # of clients. Fast rising. Keep # of players stable with small fluctuations for a long time. Scale-up/scale-down triggered.", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.addClients(52)
    SetupAndHelpers.sleepLog(60)

    target_players = 600

    for i in range(1, 100):
        if ( len(SetupAndHelpers.gStartedClients) <= target_players ):
            SetupAndHelpers.addClients( random.randint(0, round( target_players / 2 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 20) )

        if (len(SetupAndHelpers.gStartedClients) >= target_players ):
            SetupAndHelpers.removeClients( random.randint( 0 , round( target_players / 6 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 20) )

        SetupAndHelpers.sleepLog(random.randint(0, 20) )


    SetupAndHelpers.sleepLog(90)
    SetupAndHelpers.removeClients( len(SetupAndHelpers.gStartedClients) )

def Case10(mode="execute"):
    SetupAndHelpers.logLine("INFO", "Description (NOT IN TEST PLAN): Big # of clients. Slow rising. Random add/remove clients. Scale-up/scale-down triggered", False if "print" in mode else True)
    if mode == "print":
        return
        
    SetupAndHelpers.addClients(100)
    SetupAndHelpers.sleepLog(10)
    SetupAndHelpers.addClients(100)
    SetupAndHelpers.sleepLog(10)
    SetupAndHelpers.addClients(100)
    SetupAndHelpers.sleepLog(10)
    SetupAndHelpers.addClients(100)
    SetupAndHelpers.sleepLog(10)

    min_clients = 200
    max_clients = 900

    for i in range(1, 300):
        if ( len(SetupAndHelpers.gStartedClients) < max_clients ):
            SetupAndHelpers.addClients( random.randint(0, round( len(SetupAndHelpers.gStartedClients) / 2 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 10) )

        if (len(SetupAndHelpers.gStartedClients) > min_clients ):
            removeClients( random.randint( 0 , round( len(SetupAndHelpers.gStartedClients) / 3 ) ) )
            SetupAndHelpers.sleepLog( random.randint(0, 10) )

        SetupAndHelpers.SetupAndHelpers.sleepLog(random.randint(0, 10) )


    SetupAndHelpers.sleepLog(120)
    SetupAndHelpers.removeClients( len(SetupAndHelpers.gStartedClients) )
