##########################################################################################################################################
########################################## Prerequisites for running the script ##########################################################
##########################################################################################################################################

### Python 3 install instructions for CentOS 7:

sudo yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
sudo yum update
sudo yum install -y python36 python36-libs python36-devel python36-pip python36-packages

If the script returns an error like:
"<name> module not found" i
Install it using: "sudo pip install <name>" or "sudo yum install python36-<name>"

After retrieving the scripts from Perforce, fixing the newlines for Unix might be needed, call:
dos2unix *.py in Metrics tester folder

### Before running the tests a properly configured blazeserver, DirtyCast instance and metrics aggregator needs to be setup by the user.

In each TestGroup in initialConfig function the user needs to set:
ServiceName - blaze server service name for the stress clients
Env - environment of the blazeserver
Redirector 
Default Namespace - the datasource where the DC instance pushed the metrics

### OTPTests.py helper functions and Case1 may be used as an example ### 
    ## The functionality is almost self-explanatory, python being fairly easy to understand ##

### Case0 is used for prototyping when writing tests ###

##########################################################################################################################################
########################################## Prometheus docs ###############################################################################
##########################################################################################################################################

Prometheus HTTP API: https://prometheus.io/docs/prometheus/latest/querying/api/

### Querying a single value (using "query?" in URL) will return a vector with following structure:
    {
       "status" : "success",
       "data" : {
          "resultType" : "vector",
          "result" : [
             {
                "metric" : {
                   "__name__" : "up",
                   "job" : "prometheus",
                   "instance" : "localhost:9090"
                },
                "value": [ 1435781451.781, "1" ]
             },
             {
                "metric" : {
                   "__name__" : "up",
                   "job" : "node",
                   "instance" : "localhost:9100"
                },
                "value" : [ 1435781451.781, "0" ]
             }
          ]
       }
    }

# The QuerySimpleSingleValue and QueryListSingleValue functions will query using "query?" and will return only the "data" block from the response above.

### Querying a range of values, expecting a time series (using "query_range?" in URL) will return a matrix with the following structure:
    {
       "status" : "success",
       "data" : {
          "resultType" : "matrix",
          "result" : [
             {
                "metric" : {
                   "__name__" : "up",
                   "job" : "prometheus",
                   "instance" : "localhost:9090"
                },
                "values" : [
                   [ 1435781430.781, "1" ],
                   [ 1435781445.781, "1" ],
                   [ 1435781460.781, "1" ]
                ]
             },
             {
                "metric" : {
                   "__name__" : "up",
                   "job" : "node",
                   "instance" : "localhost:9091"
                },
                "values" : [
                   [ 1435781430.781, "0" ],
                   [ 1435781445.781, "0" ],
                   [ 1435781460.781, "1" ]
                ]
             }
          ]
       }
    }

# The QuerySimple and QueryList functions will query using "query_range?" and will return only the "data" block from the response above.

##########################################################################################################################################
########################################## Script help ###################################################################################
##########################################################################################################################################

### Calling ./TestRunner will print the help:
        Print available tests:
                --print             prints available tests
        Usage for running tests:
                --testgroup=  or -t=<test group>
                --case=       or -c=<test number> --- Supported syntax: integer: "1", list of integeres "1,2,3", range of integeres "1-3"
                --build=      or -b=<final|debug for the stress clients>
                --path=       or -p=<path to stress client>
                --log=        or -l=<0 or 1 for enable/disable> -> NOT IMPLEMENTED

### Example usage:

Print all tests:
./TestRunner --print

Run Case1 from OTPTests:
./TestRunner -t=OTPTests -c=1

Run Case 0,1 and 2 from OTPTests:
./TestRunner -t=OTPTests -c=0,1,2
./TestRunner -t=OTPTests -c=0-2

##########################################################################################################################################
########################################## Script files description ######################################################################
##########################################################################################################################################
### TestRunner 
    -> The runner of the tests, this is the starting module
    -> This module should be stable

### SetupAndHelpers 
    -> Helper functions like logging, sleeping, starting stress clients, stopping stress clients, interrupt handlers etc.
    -> This module should be stable

### QueryPrometheus
    -> Global defines and methods for querying prometheus, comparing values etc.
    -> This module has some INSTABLE functions marked as such and should not be used so far

### QueryGraphite
    -> Global defines and methods for querying graphite, comparing values etc.
    -> OUTDATED, not functional, needs update ?? 

### OTPTests
    -> OTPTests suite
    -> Pretty much stable
    -> In development

### CCAutoscaleTests
    -> CCAutoscaleTests
    -> Do not feature any automated validation, outdated, need update
    -> In development

### MetricsCountTests
    -> Only one test
    -> Stable

### TestArtifacts folder
    -> stress clients configuration generated at runtime are saved here
    -> Prometheus certs are stored here
    -> Any useful output tests have will output here

##########################################################################################################################################
########################################## Adding test groups and tests ##################################################################
##########################################################################################################################################

### Adding new test group:
## Create file <testgroupName>Tests.py
## Following the example from the existing ones, import "SetupAndHelpers" and "QueryPrometheus", create the "initialConfig" function and start writing tests.
## The initialConfig function will be called once at the start of the testgroup
## Tests need to be named Case0, Case1, Case2 etc and have a description at the start (check existing tests).
## All TestGroups should end their file names in "Tests" so that the program can identify them.

##########################################################################################################################################
########################################## Scripts capabilities ##########################################################################
##########################################################################################################################################

### Those scripts are able to:
- change during runtime the configuration file for stress clients
- manage stress clients (start, stop and a simple error check)
- send http requests
- query Prometheus
- compare metrics

### This is script is NOT ABLE TO:
- start/stop/config blazesever - this needs to be done by the user
- start/stop/config any DC instance - this needs to be done by the user


##########################################################################################################################################
########################################## Function helper ###############################################################################
##########################################################################################################################################

Quick start function descriptions:

From SetupAndHelpers:
    sleepLog(seconds, sleepIncrement=60, log=True, logProgress=False)
        seconds to sleep
        log - true or false to log that the script will sleep, ignored if seconds is under 5 logging will not be done at all
        logProgress - true or false to log progress during sleep ()
        sleepIncrement - the increment used to log progress (by default 60, log progress every 60 seconds)

    changeConfigParamater(configParameter, configFile = gModifiedConfigFile1)
        change config parameter of stress client config file
        configParameter - parameter to change (example: "GAME_LENGTH=360")
        configFile - which config file to change, by default uses gModifiedConfigFile1, if 2 config files are needed gModifiedConfigFile2 is available

    def logLine(severity, line, print_header = True)
        severity
            ### Severities:
            ## TRACE = debug logging
            ## DESC = description of tests
            ## WARN = warnings
            ## FAIL = test fails 
            ## ERR = program errors
            ###
        line - line to log
        print_header - true or false to print header containing: severity, current date, time, thread and caller function

    addClients(number, configFile = gModifiedConfigFile1, checkForErrors=False, safe=False)
        Starts stress clients, add in the name stands for the fact that it adds clients to a queue
        number - number of stress clients to start
        configFile - the config file passed to the stress clients (by default uses gModifiedConfigFile1)
        checkForErrors - true or false to check for errors the first client started (does check for the usual errors stress clients encounter)
        safe - true or false to check for duplicate stress clients index - possible slow

    removeClients(number)
        Stop stress clients, remove in the name stands for the fact that it removes clients from a queue
        number - the number of stress clients to stop

    checkClientForErrors(clientIndex)
        Checks stress client with the clientIndex log file for errors and logs if any found, returns -1 on fail

    getLastGameState(clientIndex)
        Reads the log file of stress client with index clientIndex and gets the current state (kConnecting, kConnected, kInGame etc.)


QueryPrometheus:
    getUnixTimeStamp()
        returns the unix like timestamp
    setDefaultDataSource()
        set defualt data source to use (for example: ""gameplay-services-internal-ccsdirtycast"")
    getSeries(match_query, datasource='', startTime=getUnixTimeStamp() - 1800, stopTime=getUnixTimeStamp())
        Sets a request to the datasource at url /series, this is used to populate the variables in grafana
        match_query - the query
        datasource - '' means default data source
        startTime, stopTime - time frame to query results
    getAllMetricNamesFromDataSource(datasource='',pattern="voipmetrics_")
        Get all metrics available on datasource with the given pattern
    querySimpleSingleValue(query, timestamp=getUnixTimeStamp(), step=60, datasource='')
        Queries the datasoruce for query received returning only the last value
    querySimple(query, startTime=getUnixTimeStamp(), stopTime=getUnixTimeStamp(), step=60, datasource='', query_type="query_range?")
        Queries the datasource for  query received returning a time series between stopTime and startTime with step 
    queryListSingleValue(queries, timestamp=getUnixTimeStamp(), step=60, datasource='')
        Queries the datasource for all the queries received in queries dictionary returning the last value for each of them (returns a dictionary)
    queryList(queries, startTime=getUnixTimeStamp(), stopTime=getUnixTimeStamp(), step=60, datasource='', query_type="query_range?")
        Queries the datasource for all the queries received in queries dictionary returning the a timeseries for each of them
    getFirstValue(results)
        Get first value from a query result, pass the result of any query function to this to get the first value
    multipleTimeSeriesToDict(results_dict)
        Converts a results dictionary (received from QueryList functions) to a dictionary of dictionaries for easy cycling thhrough the results
    timeSeriesToDict(results, resultIndex = 0)
        Converts a result to a dictionary
    printDictionaryNicely(input_dict)
        Prints a dictionary nicely
    compareChanges(initial, final, expected_change, only_gauges=False, only_counters=False, only_list=[])
        Compares the changes between final and initial query results with the expected change
        Initial, final and expected_change are dictionaries with results and expected changes
        An expected change of 2 means expecting an increment from final over initial of 2
        only_gauges and only_counters are used to limit the comparing
        only_list will overrirde the keys to compare to the ones passed
    checkResultIsInRange(results_dict, expected_range, only_gauges=False, only_counters=False, only_list=[])
        Checks the results in results_dict are in the expected range
        The parameters are the same as for CompareChanges
    calculateResultsStats(results_dict)
        Calculates some datafrom the results_dict:
            max_value
            min_value
            total_fall
            total_rise
            fall_count
            rise_count
            avg_fall
            avg_rise
        Returns those in a dictionary.