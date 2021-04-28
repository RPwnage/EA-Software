#!/usr/bin/python3

import SetupAndHelpers
import QueryPrometheus

######################################################################################################################################################################
######################################################################################################################################################################
######################################### CONFIG AND HELPERS #########################################################################################################
######################################################################################################################################################################
######################################################################################################################################################################
PROMETHEUS_TIME=0
PROMETHEUS_VALUE=1

###### THESE VALUES SHOULD BE MODIFIED HERE OR IN INITIALCONFIG ONLY !!! ######
gBlazeServiceName = "gsqa-franken-pc" 
gRedirector = "" #"winter15.gosredirector.sdev.ea.com"
gEnv = "test"
gDefaultDatasource = "gameplay-services-gs-qos-metrics" #"gameplay-services-internal-ccsdirtycast"

gOTPInstancesOnDefaultDataSource=list()
gGamesAvailableOnDataSource=0

# From testing it seems that for different data sources, different times are needed
gDefaultMetricsUpdateDelay=300
###### THESE VALUES SHOULD BE MODIFIED HERE OR IN INITIALCONFIG ONLY !!! ######

def initialConfig():
    global gBlazeServiceName, gRedirector, gEnv, gDefaultDatasource, gOTPInstancesOnDefaultDataSource, gGamesAvailableOnDataSource
    
    QueryPrometheus.getVaultCertificate()
    
    setConfigToDefault()

    ### Set default datasource to use in this test group. This can be specified as an additional parameters to most functions if the default is not OK
    QueryPrometheus.setDefaultDataSource(gDefaultDatasource)
    SetupAndHelpers.logLine("INFO", "DEFAULT DATA SOURCE SET TO " + gDefaultDatasource)

    ### Get the OTP servers connected to our blaze server
    OTPServers = QueryPrometheus.getSeries( 'voipmetrics_server_cores{mode=~"gameserver", product=~"' + gBlazeServiceName + '"}' ) 

    if OTPServers == "err" or OTPServers == "" or type(OTPServers) is not list or len(OTPServers) < 1:
        SetupAndHelpers.logLine("ERR", "Could not find any OTP instances in namespace " + gDefaultDatasource)

    for otpServer in OTPServers:
        gOTPInstancesOnDefaultDataSource.append(otpServer["deployinfo"])

    SetupAndHelpers.logLine("INFO", "Found " + str(len(gOTPInstancesOnDefaultDataSource)) + " valid OTP instances on default datasource " + gDefaultDatasource + " connected to " + gBlazeServiceName)

    if len(gOTPInstancesOnDefaultDataSource) == 0:
        return

    ### Get the available games to test on
    available_games = QueryPrometheus.querySimpleSingleValue('sum(voipmetrics_games_available{' + getMetricParameters() + '})')

    if available_games == "err" or available_games == "" or "NaN" in str(available_games):
        SetupAndHelpers.logLine("ERR", "Could not get the games available on " + gDefaultDatasource + " connected to " + gBlazeServiceName)

    gGamesAvailableOnDataSource = QueryPrometheus.getFirstValue(available_games)

    if gGamesAvailableOnDataSource == 0:
        SetupAndHelpers.logLine("ERR", "No games available on " + gDefaultDatasource + " connected to " + gBlazeServiceName + ", closing")

    SetupAndHelpers.logLine("INFO", "Found " + str(gGamesAvailableOnDataSource) + " available games on datasource " + gDefaultDatasource + ", connected to " + gBlazeServiceName)
    

def setConfigToDefault():
    global gBlazeServiceName, gRedirector, gEnv, gDefaultDatasource, gOTPInstancesOnDefaultDataSource, gGamesAvailableOnDataSource
    #### Config one setup #### 
    # Change those configs here:
    SetupAndHelpers.changeConfigParamater("USE_STRESS_LOGIN=1")
    SetupAndHelpers.changeConfigParamater("SERVICE=" + gBlazeServiceName)
    SetupAndHelpers.changeConfigParamater("RDIRNAME=" + gRedirector)
    SetupAndHelpers.changeConfigParamater("ENVIRONMENT=" + gEnv)

    # Change this configs in tests below
    SetupAndHelpers.changeConfigParamater("GAME_MODE=1")
    SetupAndHelpers.changeConfigParamater("DEBUG_LEVEL=3")
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=240")
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=30")
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=30")

### DEFAULT VALUES
def getMetricParameters(mode="gameserver", env=gEnv, site=".*", host= ".*", product=gBlazeServiceName, deployinfo=".*", port = ".*"):
    ret = 'mode=~"' + mode + '",env=~"' + env + '",site=~"' + site + '",host=~"' + host +'",'
    ret = ret + 'product=~"' + product + '",deployinfo=~"' + deployinfo + '",port=~"' + port + '"'
    return ret

def getGameServerMetricsParameters(env=gEnv, product=".*", deployinfo=".*"):
    ret = 'env=~"' + env + '",product=~"' + product + '",deployinfo=~"' + deployinfo + '"'
    return ret


### Keywords:
# empty string - No data is expected, empty response
# any - skip validation
# data - Data is expected, any data, flat 0 is considered OK
# change - We expect a change in data returned, any change, flat data is not considered ok
###
##
### _g stands for gauge and _c for counter
##
### expected_changes represent a positive or negative number with the expected change of that value - quality check of the metric. This should be passed to CompareChnages function
### expected_range represent a range in which the metric should be - less quality check. This should be passed to CheckREsultInRange function
### After this function is called and queries and expected_range are retrieved, they can be adjusted on the test needs

def getClientsAndGamesQueries():
    queries = dict()
    expected_change = dict()

    queries["clients_active_g"] = 'sum(voipmetrics_clients_active{' + getMetricParameters() + '})'
    queries["clients_added_c"] = 'sum(voipmetrics_clients_added_count{' + getMetricParameters() + '})'
    queries["clients_talking_c"] = 'sum(voipmetrics_clients_talking{' + getMetricParameters() + '})'

    queries["games_available_g"] = 'sum(voipmetrics_games_available{' + getMetricParameters() + '})'
    queries["games_active_g"] = 'sum(voipmetrics_games_active{' + getMetricParameters() + '})'
    queries["games_offline_g"] = 'sum(voipmetrics_games_offline{' + getMetricParameters() + '})'
    queries["games_disconnected_c"] = 'sum(voipmetrics_games_disconnected_count{' + getMetricParameters() + '})'
    queries["games_stuck_g"] = 'sum(voipmetrics_games_stuck{' + getMetricParameters() + '})'
    queries["games_created_c"] = 'voipmetrics_games_created_count{' + getMetricParameters() + '}'

    queries["gamestats_games_ended_pregame_c"] = 'sum(voipmetrics_gamestats_games_ended_pregame_count{' + getMetricParameters() + '})'
    queries["gamestats_games_ended_ingame_c"] = 'sum(voipmetrics_gamestats_games_ended_ingame_count{' + getMetricParameters() + '})'
    queries["gamestats_games_ended_postgame_c"] = 'sum(voipmetrics_gamestats_games_ended_postgame_count{' + getMetricParameters() + '})'
    queries["gamestats_games_ended_migrating_c"] = 'sum(voipmetrics_gamestats_games_ended_migrating_count{' + getMetricParameters() + '})'
    queries["gamestats_games_ended_other_c"] = 'sum(voipmetrics_gamestats_games_ended_other_count{' + getMetricParameters() + '})'

    queries["gamestats_clients_removed_pregame_c"] = 'sum(voipmetrics_gamestats_clients_removed_pregame_count{' + getMetricParameters() + '})'
    queries["gamestats_clients_removed_postgame_c"] = 'sum(voipmetrics_gamestats_clients_removed_postgame_count{' + getMetricParameters() + '})'
    queries["gamestats_clients_removed_ingame_c"] = 'sum(voipmetrics_gamestats_clients_removed_ingame_count{' + getMetricParameters() + '})'
    queries["gamestats_clients_removed_migrating_c"] = 'sum(voipmetrics_gamestats_clients_removed_migrating_count{' + getMetricParameters() + '})'
    queries["gamestats_clients_removed_other_c"] = 'sum(voipmetrics_gamestats_clients_removed_other_count{' + getMetricParameters() + '})'

    queries["voipmetrics_rlmt_count_c"] = "voipmetrics_rlmt_count_count{" + getMetricParameters() + "}"
    queries["voipmetrics_gamestats_clients_removereason_PLAYER_LEFT_c"] = "sum(voipmetrics_gamestats_clients_removereason_PLAYER_LEFT_count{" + getMetricParameters() + "})"

    for key in queries.keys():
        expected_change[key] = 0

    # Do not validate in any way
    expected_change["voipmetrics_gamestats_clients_removereason_PLAYER_LEFT_c"] = 'any'
    # This means we do expect data, any data, this is a very loose validation
    expected_change["voipmetrics_rlmt_count_c"] = 'data'

    return queries, expected_change

def getLatencyQueries():
    queries = dict()
    expected_range = dict()

    queries["voipmetrics_latency_avg_g"] = "voipmetrics_latency_avg{" + getMetricParameters() + "}"
    expected_range["voipmetrics_latency_avg_g"] = "0:100"
    queries["voipmetrics_latency_min_g"] = "voipmetrics_latency_min{" + getMetricParameters() + "}"
    expected_range["voipmetrics_latency_min_g"] = "0:100"
    queries["voipmetrics_latency_max_g"] = "voipmetrics_latency_max{" + getMetricParameters() + "}"
    expected_range["voipmetrics_latency_max_g"] = "0:100"

    queries["voipmetrics_latebin_0_c"] = "sum(voipmetrics_latebin_0_count{" + getMetricParameters() + "})"
    expected_range["voipmetrics_latebin_0_c"] = "data"
    
    ### Validate only one latebin
    queries["voipmetrics_latebin_1_c"] = "sum(voipmetrics_latebin_1_count{" + getMetricParameters() + "})"
    expected_range["voipmetrics_latebin_1_c"] = "any"
    queries["voipmetrics_latebin_2_c"] = "sum(voipmetrics_latebin_2_count{" + getMetricParameters() + "})"
    expected_range["voipmetrics_latebin_2_c"] = "any"
    queries["voipmetrics_latebin_3_c"] = "sum(voipmetrics_latebin_3_count{" + getMetricParameters() + "})"
    expected_range["voipmetrics_latebin_3_c"] = "any"
    queries["voipmetrics_latebin_4_c"] = "sum(voipmetrics_latebin_4_count{" + getMetricParameters() + "})"
    expected_range["voipmetrics_latebin_4_c"] = "any"

    return queries, expected_range


def getPacketsAndBandwithQueries():
    queries = dict()
    expected_change = dict()

    queries["voipmetrics_games_bandwidth_curr_dn_g"] = "voipmetrics_games_bandwidth_curr_dn{" + getMetricParameters() + "}"
    expected_change["voipmetrics_games_bandwidth_curr_dn_g"] = "change"
    queries["voipmetrics_games_bandwidth_curr_up_g"] = "voipmetrics_games_bandwidth_curr_up{" + getMetricParameters() + "}"
    expected_change["voipmetrics_games_bandwidth_curr_up_g"] ="change"
    queries["voipmetrics_games_packets_curr_dn_g"] = "voipmetrics_games_packets_curr_dn{" + getMetricParameters() + "}"
    expected_change["voipmetrics_games_packets_curr_dn_g"] ="change"
    queries["voipmetrics_games_packets_curr_up_g"] = "voipmetrics_games_packets_curr_up{" + getMetricParameters() + "}"
    expected_change["voipmetrics_games_packets_curr_up_g"] = "change"

    return queries, expected_change

def getTunnelPacketsQueries():
    queries = dict()
    expected_change = dict()

    queries["voipmetrics_tunnelinfo_totalrecvsub_c"] = "rate(voipmetrics_tunnelinfo_totalrecvsub_count{" + getMetricParameters() + "}[5m])"
    expected_change["voipmetrics_tunnelinfo_totalrecvsub_c"] = "change"
    queries["voipmetrics_tunnelinfo_totalrecv_c"] = "rate(voipmetrics_tunnelinfo_totalrecv_count{" + getMetricParameters() + "}[5m])"
    expected_change["voipmetrics_tunnelinfo_totalrecv_c"] = "change"
    queries["voipmetrics_tunnelinfo_recvcalls_c"] = "rate(voipmetrics_tunnelinfo_recvcalls_count{" + getMetricParameters() + "}[5m])"
    expected_change["voipmetrics_tunnelinfo_recvcalls_c"] = "change"
    queries["voipmetrics_tunnelinfo_totaldiscard_c"] = "rate(voipmetrics_tunnelinfo_totaldiscard_count{" + getMetricParameters() + "}[5m])"
    expected_change["voipmetrics_tunnelinfo_totaldiscard_c"] = "any"

    return queries, expected_change

def getCPUQueries():
    queries = dict()
    expected_range = dict()

    queries["voipmetrics_systemload_1_g"] = "avg(voipmetrics_systemload_1{" + getMetricParameters() + "}) * 0.01"
    expected_range["voipmetrics_systemload_1_g"] = "0:0.5"
    queries["voipmetrics_systemload_5_g"] = "avg(voipmetrics_systemload_5{" + getMetricParameters() + "}) * 0.01"
    expected_range["voipmetrics_systemload_5_g"] = "0:0.5"
    queries["voipmetrics_systemload_15_g"] = "avg(voipmetrics_systemload_15{" + getMetricParameters() + "}) * 0.01"
    expected_range["voipmetrics_systemload_15_g"] = "0:0.5"
    queries["voipmetrics_server_cpu_pct_g"] = "avg(voipmetrics_server_cpu_pct{" + getMetricParameters() + "}) * 0.01"
    expected_range["voipmetrics_server_cpu_pct_g"] = "0:20"

    return queries, expected_range

def getIppsOppsQueries():
    queries = dict()
    expected_range = dict()

    queries["gameserver_odmpps_min_g"] = "gameserver_odmpps_min{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_odmpps_min_g"] = "0:35"
    queries["gameserver_odmpps_max_g"] = "gameserver_odmpps_max{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_odmpps_max_g"] = "0:35"
    queries["gameserver_odmpps_median_g"] = "gameserver_odmpps_median{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_odmpps_median_g"] = "0:35"
    queries["gameserver_odmpps_mean_g"] = "gameserver_odmpps_mean{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_odmpps_mean_g"] = "0:35"
    queries["gameserver_odmpps_upper_95_g"] = "gameserver_odmpps_upper_95{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_odmpps_upper_95_g"] = "0:35"
    

    queries["gameserver_idpps_min_g"] = "gameserver_idpps_min{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_idpps_min_g"] = "0:35"
    queries["gameserver_idpps_max_g"] = "gameserver_idpps_max{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_idpps_max_g"] = "0:35"
    queries["gameserver_idpps_median_g"] = "gameserver_idpps_median{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_idpps_median_g"] = "0:35"
    queries["gameserver_idpps_mean_g"] = "gameserver_idpps_mean{" + getGameServerMetricsParameters() + "}"
    expected_range["gameserver_idpps_mean_g"] = "0:35"
    #queries["gameserver_idpps_upper_95_g"] = "gameserver_idpps_upper_95{" + getGameServerMetricsParameters() + "}" 
    #expected_range["gameserver_idpps_upper_95_g"] = "0:35"

    return queries, expected_range

def getSlowQueries():
    queries = dict()

    # this queries return at 5 minutes, less dynamic
    queries["clients_maximum_g"] = 'voipmetrics_clients_maximum{' + getMetricParameters() + '}'
    
    return queries

######################################################################################################################################################################
######################################################################################################################################################################
################################################################### TESTS ############################################################################################
######################################################################################################################################################################
######################################################################################################################################################################
### Tests format Case<number>
### Default mode is execute, in print mode it just prints the description
### The test description log and print mode is mandatory in every test

### Metrics need about 1 to 3 minutes to get updates, so pretty big wait times are needed
def Case0(mode="execute"):
    SetupAndHelpers.logLine("DESC", "PROTOTYPE", False if "print" in mode else True)
    if mode == "print":
        return  
    
    hour = 3600

    clientsQueries, clientsResults = getClientsAndGamesQueries()
    clientsResults = QueryPrometheus.queryList(clientsQueries, QueryPrometheus.getUnixTimeStamp() - (3600), QueryPrometheus.getUnixTimeStamp())
    QueryPrometheus.printDictionaryNicely(clientsResults)

    return
    
    IppsQueries, IppsExpectedRange = getIppsOppsQueries()
    IppsResults = QueryPrometheus.queryList(IppsQueries, QueryPrometheus.getUnixTimeStamp() - (3*hour), QueryPrometheus.getUnixTimeStamp(), datasource="gameplay-services-gs-qos-metrics")
    QueryPrometheus.printDictionaryNicely( QueryPrometheus.timeSeriesToDict(IppsResults) )


def Case1(mode="execute"):
    SetupAndHelpers.logLine("DESC", "Sanity check of all metrics defined.", False if "print" in mode else True)
    if mode == "print":
        return

    if (gGamesAvailableOnDataSource < 1):
        SetupAndHelpers.logLine("FAIL", "This test needs games on default data source")
        return -1

    ### Stress clients configuration
    setConfigToDefault()
    gamePlayers = 6
    startedPlayers = gamePlayers * gGamesAvailableOnDataSource
    gameLength = 600
    return_error = 0

    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=" + str(gameLength))
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=" + str(gamePlayers))
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=" + str(gamePlayers))

    ### Get clients and games queries and adapt the expected changes
    clientsQueries, clientsExpectedChanges = getClientsAndGamesQueries()    
    clientsExpectedChanges["clients_active_g"] = startedPlayers
    clientsExpectedChanges["games_available_g"] = - int( (startedPlayers/gamePlayers) )
    clientsExpectedChanges["games_active_g"] = int( (startedPlayers/gamePlayers) )

    ### Query clients and games queries before the game
    before_game = QueryPrometheus.queryListSingleValue(clientsQueries)

    ### Save the start time of the test
    testStartTime = QueryPrometheus.getUnixTimeStamp()

    ### Start clients checking there are no errors
    if ( SetupAndHelpers.addClients(startedPlayers, checkForErrors=True) == -1 ):
        return -1

    ### Allow time for the metrics to reach prometheus
    SetupAndHelpers.sleepLog(30)
    ### Print the game state for stress client 1 (maybe there is something wrong with the matchmaking)
    gameState = SetupAndHelpers.getLastGameState(1)
    SetupAndHelpers.logLine("INFO", "Stress client 1 reached state " + str(gameState))
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay - 30)

    ### Query again the clients and games metrics for during the game
    during_game = QueryPrometheus.queryListSingleValue(clientsQueries)
    ### Compare these values with the ones received before the game expecting the changes set
    err = QueryPrometheus.compareChanges(before_game, during_game, clientsExpectedChanges)

    ### Stop the clients
    SetupAndHelpers.removeClients(startedPlayers)
    ### Allow time for the metrics to update
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    # Zero all expectedChanges for clients and games
    clientsQueries, clientsExpectedChanges = getClientsAndGamesQueries()
    clientsExpectedChanges["clients_added_c"] = startedPlayers
    clientsExpectedChanges["clients_active_g"] = - startedPlayers
    clientsExpectedChanges["games_available_g"] =  int( (startedPlayers/gamePlayers) )
    clientsExpectedChanges["games_active_g"] =  - int( (startedPlayers/gamePlayers) )
    clientsExpectedChanges["games_created_c"] = int( (startedPlayers/gamePlayers) )
    clientsExpectedChanges["gamestats_games_ended_ingame_c"] = int( (startedPlayers/gamePlayers) )
    clientsExpectedChanges["gamestats_clients_removed_ingame_c"] = startedPlayers

    ## This should be changed when fixed ## Do not validate
    clientsExpectedChanges["voipmetrics_gamestats_clients_removereason_PLAYER_LEFT_c"] = "any"

    ### Get the metrics after the game
    after_game = QueryPrometheus.queryListSingleValue(clientsQueries)
    ### Compare these values with the ones during the game expecting the newly set expected changes
    err = QueryPrometheus.compareChanges(during_game, after_game, clientsExpectedChanges)

    ### Note the test sstop time
    testStopTime = QueryPrometheus.getUnixTimeStamp()

    ### Query the latency queries from the test start time to test stop time
    latencyQueries, latencyExpectedRanges = getLatencyQueries()
    latencyResults = QueryPrometheus.queryList(latencyQueries, testStartTime, testStopTime)
    ### Check that those queries are in the expected ranges
    err = QueryPrometheus.checkResultIsInRange(latencyResults, latencyExpectedRanges)

    ### Query the packets queries from test start time to test stop time
    packetsQueries, packetsExpectedRanges = getPacketsAndBandwithQueries()
    packetsResults = QueryPrometheus.queryList(packetsQueries, testStartTime, testStopTime)
    ### Calculate some metric stats
    packetsStats = QueryPrometheus.calculateResultsStats(packetsResults)
    err = 0
    # Check those metrics do change
    for item in packetsStats.keys():
        # this means we DO expect a change
        if packetsExpectedRanges[item] == "change":
            if packetsStats[item]["max_value"] == 0 or ( packetsStats[item]["total_fall"] == 0 and packetsStats[item]["total_rise"] == 0 ):
                SetupAndHelpers.logLine("FAIL", "Found no change in value for " + str(item))
                err = -1

    ### Tunnel queries
    tunnelQueries, tunnelExpectedRanges = getTunnelPacketsQueries()
    tunnelResults = QueryPrometheus.queryList(tunnelQueries, testStartTime, testStopTime)
    tunnelStats = QueryPrometheus.calculateResultsStats(tunnelResults)
    err = 0
    # Check those item do change
    for item in tunnelStats.keys():
        # this means we DO expect a change
        if tunnelExpectedRanges[item] == "change":
            if tunnelStats[item]["max_value"] == 0 or ( tunnelStats[item]["total_fall"] == 0 and tunnelStats[item]["total_rise"] == 0 ) :
                SetupAndHelpers.logLine("FAIL", "Found no change in value for " + str(item))
                err = -1

    ### CPU queries
    CPUqueries, CPUExpectedRanges = getCPUQueries()
    CPUResults = QueryPrometheus.queryList(CPUqueries, testStartTime, testStopTime)
    err6 = QueryPrometheus.checkResultIsInRange(CPUResults, CPUExpectedRanges)

    ### Ipps/Ompps Queries ###
    IppsQueries, IppsExpectedRange = getIppsOppsQueries()
    IppsResults = QueryPrometheus.queryList(IppsQueries, testStartTime, testStopTime)
    # Check the response falls in the expected range retrieved from getIppsOppsQueries()
    err = QueryPrometheus.checkResultIsInRange(IppsResults, IppsExpectedRange)

    # Check that those metrics DO CHANGE and are not flat 0
    IppsStats = QueryPrometheus.calculateResultsStats(IppsResults)
    for stat in IppsStats.keys():
        if IppsStats[stat]["max_value"] == 0 or ( IppsStats[stat]["total_rise"] == 0 and IppsStats[stat]["total_fall"] == 0):
            SetupAndHelpers.logLine("FAIL", "Found no change in value for " + str(stat))
            err = -1

    # Just return -1, triggering the test to fail, all the errors that lead to this are logged inside the functions from QueryPrometheus
    if err == -1:
        return -1

def Case2(mode="execute"):
    SetupAndHelpers.logLine("DESC", "Create games to fullfil all the OTP intances found with 4 players each, stop clients pre-game, check games and client metrics update as expected", False if "print" in mode else True)
    if mode == "print":
        return

    if (gGamesAvailableOnDataSource < 1):
        SetupAndHelpers.logLine("FAIL", "This test needs games on default data source")
        return -1

    setConfigToDefault()

    minPlayers = 4
    maxPlayers = 6
    startedPlayers = minPlayers * gGamesAvailableOnDataSource
    gameLength = 360

    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=" + str(gameLength))
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=" + str(minPlayers))
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=" + str(maxPlayers))

    queries, expected_changes= getClientsAndGamesQueries()  

    before_game = QueryPrometheus.queryListSingleValue(queries)
    ### Start clients
    if ( SetupAndHelpers.addClients(startedPlayers, checkForErrors=True) == -1 ):
        return -1

    SetupAndHelpers.sleepLog(15)

    during_game = QueryPrometheus.queryListSingleValue(queries)
    err1 = QueryPrometheus.compareChanges(before_game, during_game, expected_changes)

    expected_changes["games_created_c"] = int( (startedPlayers/minPlayers) )
    expected_changes["clients_added_c"] = startedPlayers
    expected_changes["gamestats_games_ended_pregame_c"] = int( (startedPlayers/minPlayers) )
    expected_changes["gamestats_clients_removed_pregame_c"] = startedPlayers
    ### Stop the clients
    SetupAndHelpers.removeClients(startedPlayers)
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    after_game = QueryPrometheus.queryListSingleValue(queries)
    err2 = QueryPrometheus.compareChanges(during_game, after_game, expected_changes)

    if err1 == -1 or err2 == -1:
        return -1

def Case3(mode="execute"):
    SetupAndHelpers.logLine("DESC", "Create games to fullfil all the OTP intances found with 4 players each, stop clients post-game, check games and client metrics update as expected", False if "print" in mode else True)
    if mode == "print":
        return

    if (gGamesAvailableOnDataSource < 1):
        SetupAndHelpers.logLine("FAIL", "This test needs games on default data source")
        return -1

    setConfigToDefault()

    minPlayers = 6
    maxPlayers = 6
    startedPlayers = minPlayers * gGamesAvailableOnDataSource
    gameLength = 150

    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=" + str(gameLength))
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=" + str(minPlayers))
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=" + str(maxPlayers))
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=240")

    queries, expected_changes = getClientsAndGamesQueries() 
    expected_changes["clients_active_g"] = startedPlayers
    expected_changes["games_active_g"] = gGamesAvailableOnDataSource
    expected_changes["games_available_g"] = - gGamesAvailableOnDataSource

    before_game = QueryPrometheus.queryListSingleValue(queries)
    ### Start clients
    if ( SetupAndHelpers.addClients(startedPlayers, checkForErrors=True) == -1 ):
        return -1
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    during_game = QueryPrometheus.queryListSingleValue(queries)
    err1 = QueryPrometheus.compareChanges(before_game, during_game, expected_changes)

    expected_changes["clients_active_g"] = - startedPlayers
    expected_changes["clients_added_c"] = startedPlayers
    expected_changes["games_active_g"] = - gGamesAvailableOnDataSource
    expected_changes["games_available_g"] = gGamesAvailableOnDataSource
    expected_changes["games_created_c"] = int( (startedPlayers/minPlayers) )
    expected_changes["gamestats_games_ended_postgame_c"] = int( (startedPlayers/minPlayers) )
    expected_changes["gamestats_clients_removed_postgame_c"] = startedPlayers
    ### Stop the clients
    SetupAndHelpers.removeClients(startedPlayers)
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    after_game = QueryPrometheus.queryListSingleValue(queries)
    err2 = QueryPrometheus.compareChanges(during_game, after_game, expected_changes)

    if err1 == -1 or err2 == -1:
        return -1

def Case4(mode="execute"):
    SetupAndHelpers.logLine("DESC", "Ipps/Ompps test. Short game time so in 360 seconds multiple games will start and end", False if "print" in mode else True)
    if mode == "print":
        return

    setConfigToDefault()

    minPlayers = 6
    maxPlayers = 6
    startedPlayers = minPlayers * gGamesAvailableOnDataSource
    gameLength = 200

    err = 0

    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=" + str(gameLength))
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=" + str(minPlayers))
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=" + str(maxPlayers))
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=30")

    testStartTime = QueryPrometheus.getUnixTimeStamp()
    ### Start clients
    if ( SetupAndHelpers.addClients(startedPlayers, checkForErrors=True) == -1 ):
        return -1
    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    SetupAndHelpers.removeClients(startedPlayers)

    SetupAndHelpers.sleepLog(gDefaultMetricsUpdateDelay)

    SetupAndHelpers.addClients(startedPlayers, checkForErrors=True)

    testStopTime = QueryPrometheus.getUnixTimeStamp()

    IppsQueries, IppsExpectedRange = getIppsOppsQueries()
    IppsResults = QueryPrometheus.queryList(IppsQueries, testStartTime, testStopTime)
    err = QueryPrometheus.checkResultIsInRange(IppsResults, IppsExpectedRange)

    IppsStats = QueryPrometheus.calculateResultsStats(IppsResults)

    for stat in IppsStats.keys():
        if IppsStats[stat]["max_value"] == 0 or ( IppsStats[stat]["total_rise"] == 0 and IppsStats[stat]["total_fall"] == 0):
            SetupAndHelpers.logLine("FAIL", "Found no change in value for " + str(stat))
            err = -1

    if err == 0:
        return -1