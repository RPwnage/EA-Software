#!/usr/bin/python3

import SetupAndHelpers
import QueryPrometheus

###################################################################################
###################### CONFIG AND HELPERS #########################################
###################################################################################
gOTPBlazeServiceName = "gsqa-15.1.x-pc"
gOTPEnv = "test"
gOTPRedirector = ""

gCCBlazeServiceName = "gosblaze-almihalache-pc"
gCCEnv = "dev"
gCCRedirector = "winter15.gosredirector.sdev.ea.com"

def initialConfig():
    
    QueryPrometheus.getVaultCertificate()
    
    #### Config one setup (OTP) #### 
    # Change those configs here:
    SetupAndHelpers.changeConfigParamater("SERVICE=" + gOTPBlazeServiceName)
    SetupAndHelpers.changeConfigParamater("RDIRNAME=" + gOTPRedirector)
    SetupAndHelpers.changeConfigParamater("ENVIRONMENT=" + gOTPEnv)

    # Change this configs in tests below
    SetupAndHelpers.changeConfigParamater("GAME_MODE=1")
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=60")
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=4")
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=10")
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=5")
    

    #### Config two setup (CC) ####
    # Change those configs here:
    SetupAndHelpers.changeConfigParamater("SERVICE=" + gCCBlazeServiceName, SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("RDIRNAME=" + gCCRedirector, SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("ENVIRONMENT=" + gCCEnv , SetupAndHelpers.gModifiedConfigFile2)

    # Change this configs in tests below
    SetupAndHelpers.changeConfigParamater("GAME_MODE=3", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("GAME_LENGTH=60", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("MIN_PLAYERS=4", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("MAX_PLAYERS=4", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("WAIT_FOR_PLAYERS=10", SetupAndHelpers.gModifiedConfigFile2)
    SetupAndHelpers.changeConfigParamater("WAIT_BETWEEN_GAMES=5", SetupAndHelpers.gModifiedConfigFile2)

    return


###################################################################################
################################### TESTS #########################################
###################################################################################
### Tests format Case<number>
### Default mode is execute, in print mode it just prints the description
### The test description log and print mode is mandatory in every test

### Metrics need about 1 to 3 minutes to get updates, so pretty big wait times are needed
def Case0(mode="execute"):
    SetupAndHelpers.logLine("DESC", "PROTOTYPE", False if "print" in mode else True)
    if mode == "print":
        return

def Case1(mode="execute"):
    messageToLog = '(NOT IN TEST PLAN) Test steps: \n'
    messageToLog = messageToLog + "\t\t- start 200 stress clients in OTP mode, and 200 in CC mode (each type with a different configuration file) to generate traffic and trigger metrics\n"
    messageToLog = messageToLog + "\t\t- get all metric names known by Prometheus for each namespace (using /series URL)\n"
    messageToLog = messageToLog + "\t\t- get all DirtyCast instances running in that namespace by type: OTP, CC, QOS\n"
    messageToLog = messageToLog + "\t\t- query every instance for every metric and note metrics that do return data and which don't\n"
    messageToLog = messageToLog + "\t\t- read known metrics for each instance type from TestArtifacts/TestInput folder\n"
    messageToLog = messageToLog + "\t\t- compare known metrics read from file with the metrics returned by Prometheus and output a diff\n"
    messageToLog = messageToLog + "\t\t- save all informations gathered in TestArtifacts/TestOutput\n"
    SetupAndHelpers.logLine("DESC", messageToLog, False if "print" in mode else True)
    if mode == "print":
        return
    SetupAndHelpers.logLine("DESC", "DURING THIS TEST MULTIPLE WARNINGS REGARDING MULTIPLE TIME SERIES RECEIVED WILL BE LOGGED, THE TEST WILL CHECK EACH OF THEM AND LOG AN ERROR IF THERE IS INDEED A PROBLEM")

    err = 0

    expectedMetricsOTP  = open(SetupAndHelpers.gTestInputFolder + "/otp_known_metrics.txt").readlines()
    expectedMetricsOTP = [x.strip() for x in expectedMetricsOTP] 

    expectedMetricsCC  = open(SetupAndHelpers.gTestInputFolder + "/cc_known_metrics.txt").readlines()
    expectedMetricsCC = [x.strip() for x in expectedMetricsCC] 

    expectedMetricsQOS  = open(SetupAndHelpers.gTestInputFolder + "/qos_known_metrics.txt").readlines()
    expectedMetricsQOS = [x.strip() for x in expectedMetricsQOS] 

    # QA Namespace - the clients will connect to DC instances in this namespace
    QueryPrometheus.setDefaultDataSource("gameplay-services-internal-ccsdirtycast")
    # Create traffic in OTP mode
    SetupAndHelpers.addClients(200, checkForErrors=True)
    # Create traffic in CC mode
    SetupAndHelpers.addClients(200, configFile=SetupAndHelpers.gModifiedConfigFile2, checkForErrors=True)
    # Let the clients generate some traffic to trigger as much metrics as possible
    SetupAndHelpers.sleepLog(180)

    supportedDimensions=["env", "host", "port", "prototunnelver", "product", "site", "mode", "deployinfo", "datastreamid", "cluster", "tenantID", "job", "hostname", "namespace", " instance"]
    
    # Query this number of same type server instances, protection to not stress with too many queries
    maxServersToQuery=3
    
    for key in QueryPrometheus.PROMETHEUS_URL.keys():
        file = open(SetupAndHelpers.gTestOutputFolder + "/Metrics__" +  str(key)+".txt", "w")

        OTPServers = QueryPrometheus.getSeries( 'voipmetrics_server_cores{mode=~"gameserver"}' , datasource=str(key) ) 
        CCServers = QueryPrometheus.getSeries( 'voipmetrics_server_cores{mode=~"concierge"}', datasource=str(key)  )
        QOSServers = QueryPrometheus.getSeries('voipmetrics_server_cores{mode=~"qos"}', datasource=str(key)  )
        allMetrics = QueryPrometheus.getAllMetricNamesFromDataSource(datasource=str(key), pattern="voipmetrics_" )
        allMetrics = allMetrics + QueryPrometheus.getAllMetricNamesFromDataSource(datasource=str(key), pattern="gameserver_" )

        OTPCount = 0
        CCCount = 0
        QOSCount = 0

        # Using sets becase they only hold unique values
        OKMetricsOTP = set()
        OKMetricsCC = set()
        OKMetricsQOS = set()
        NotOKMetricsOTP = set()
        NotOKMetricsCC = set()
        NotOKMetricsQOS = set()

        print("### Running on namespace " + str(key) + " ###")
        file.write("### Running on namespace " + str(key) + " :\n\n")

        if OTPServers != "err":
            OTPCount = len(OTPServers)
            print("# OTP Servers: " + str(OTPCount) + " #")
            file.write("# OTP Servers: " + str(OTPCount) + " #\n")
        if CCServers != "err":
            CCCount = len(CCServers)
            print("# CC Servers: " + str(CCCount) + " #")
            file.write("# CC Servers: " + str(CCCount) + " #\n")
        if QOSServers != "err":
            QOSCount = len(QOSServers)
            print("# QOS Servers: " + str(QOSCount) + " #")
            file.write("# QOS Servers: " + str(QOSCount) + " #\n")
        if allMetrics != "err":
            print("## Total metrics on datasource: " + str(len(allMetrics)) + " ##")
            file.write("## Total metrics on datasource (" + str(len(allMetrics)) + "): ##\n\n")
        
        allMetrics.sort()

        for metric in allMetrics:
            file.write(str(metric) + "\n")

            for otpServer in OTPServers: 
                if OTPServers.index(otpServer) == maxServersToQuery:
                    break

                query = ''
                result = ''
                query = str(metric) + '{mode=~"gameserver", deployinfo=~"' + str(otpServer["deployinfo"]) + '"}'
                result = QueryPrometheus.querySimpleSingleValue(query, datasource=str(key))

                unresponsiveMetrics = list()
                if result == 'err':
                    SetupAndHelpers.logLine("ERR", "Can't get value for " + str(query))
                    continue
                elif result == '':
                    continue

                OKMetricsOTP.add(metric)
                if len(result["result"]) > 1:
                    # We got more than 1 time serie, check the different dimensions between them
                    diff_labels=set()
                    for i in range(0, len( result["result"] ) ):
                        for j in range(0, len( result["result"] ) ):
                            diff_labels = diff_labels.union( [k for k in result["result"][i]["metric"] if result["result"][i]["metric"][k] != result["result"][j]["metric"][k]] )
                    for diff_label in diff_labels:
                        # If the different dimension between those  is in the supportedDimenesions there is a problem
                        if diff_label in supportedDimensions:
                            SetupAndHelpers.logLine("FAIL", "Multiple series returned for " + str(otpServer["deployinfo"]) + " " + str(metric)  + " different values for " + str(diff_label))
                            break

            # if metric did not return any data for any of the instances add it to not ok list
            if metric not in OKMetricsOTP:
                NotOKMetricsOTP.add(metric)

            for ccServer in CCServers: 
                if CCServers.index(ccServer) == maxServersToQuery:
                    break

                query = ''
                result = ''
                query = str(metric) + '{mode=~"concierge", deployinfo=~"' + str(ccServer["deployinfo"]) + '"}'
                result = QueryPrometheus.querySimpleSingleValue(query, datasource=str(key))

                if result == 'err':
                    SetupAndHelpers.logLine("ERR", "Can't get value for " + str(query))
                    continue
                elif result == '':
                    continue

                OKMetricsCC.add(metric)
                if len(result["result"]) > 1:
                    diff_labels=set()
                    for i in range(0, len( result["result"] ) ):
                        for j in range(0, len( result["result"] ) ):
                            diff_labels = diff_labels.union( [k for k in result["result"][i]["metric"] if result["result"][i]["metric"][k] != result["result"][j]["metric"][k]] )                   
                    for diff_label in diff_labels:
                        if diff_label in supportedDimensions:
                            SetupAndHelpers.logLine("FAIL", "Multiple series returned for " + str(ccServer["deployinfo"]) + " " + str(metric)  + " different values for " + str(diff_label))
                            break

            # if metric did not return any data for any of the instances add it to not ok list
            if metric not in OKMetricsCC:
                NotOKMetricsCC.add(metric)

            for qosServer in QOSServers: 
                if QOSServers.index(qosServer) == maxServersToQuery:
                    break

                query = ''
                result = ''
                query = str(metric) + '{mode=~"qos", deployinfo=~"' + str(qosServer["deployinfo"]) + '"}'
                result = QueryPrometheus.querySimpleSingleValue(query, datasource=str(key))

                if result == 'err':
                    SetupAndHelpers.logLine("ERR", "Can't get value for " + str(query))
                    continue
                elif result == '':
                    #SetupAndHelpers.logLine("WARN", "No data for " + str(query))
                    continue

                OKMetricsQOS.add(metric)
                if len(result["result"]) > 1:
                    diff_labels=set()
                    for i in range(0, len( result["result"] ) ):
                        for j in range(0, len( result["result"] ) ):
                            diff_labels = diff_labels.union( [k for k in result["result"][i]["metric"] if result["result"][i]["metric"][k] != result["result"][j]["metric"][k]] )
                    for diff_label in diff_labels:
                        if diff_label in supportedDimensions:
                            SetupAndHelpers.logLine("FAIL", "Multiple series returned for " + str(qosServer["deployinfo"]) + " " + str(metric)  + " different values for " + str(diff_label))
                            break

            # if metric did not return any data for any of the instances add it to not ok list
            if metric not in OKMetricsQOS:
                NotOKMetricsQOS.add(metric)


        if OTPCount > 0:    
            file.write("\n\n## Metrics that responded with data in OTP (" + str(len(OKMetricsOTP)) + "): \n")
            OKMetricsOTP = list(OKMetricsOTP)
            OKMetricsOTP.sort()
            for item in OKMetricsOTP:
                file.write(str(item) + "\n")

            file.write("\n\n## Metrics that DID NOT responded with data in OTP (" + str(len(NotOKMetricsOTP)) + "): \n")
            NotOKMetricsOTP = list(NotOKMetricsOTP)
            NotOKMetricsOTP.sort()
            for item in NotOKMetricsOTP:
                file.write(str(item) + "\n")

        if CCCount > 0:
            file.write("\n\n## Metrics that responded with data in CC (" + str(len(OKMetricsCC)) + "): \n")
            OKMetricsCC = list(OKMetricsCC)
            OKMetricsCC.sort()
            for item in OKMetricsCC:
                file.write(str(item) + "\n")

            file.write("\n\n## Metrics that DID NOT responded with data in CC (" + str(len(NotOKMetricsCC)) + "): \n")
            NotOKMetricsCC = list(NotOKMetricsCC)
            NotOKMetricsCC.sort()
            for item in NotOKMetricsCC:
                file.write(str(item) + "\n")

        if QOSCount > 0:
            file.write("\n\n## Metrics that responded with data in QOS (" + str(len(OKMetricsQOS)) + "): \n")
            OKMetricsQOS = list(OKMetricsQOS)
            OKMetricsQOS.sort()
            for item in OKMetricsQOS:
                file.write(str(item) + "\n")

            file.write("\n\n## Metrics that DID NOT responded with data in QOS (" + str(len(NotOKMetricsQOS)) + "): \n")
            NotOKMetricsQOS = list(NotOKMetricsQOS)
            NotOKMetricsQOS.sort()
            for item in NotOKMetricsQOS:
                file.write(str(item) + "\n")

        

        for metric in allMetrics:
            if ( metric not in OKMetricsOTP ) and ( metric not in OKMetricsCC ) and ( metric not in OKMetricsQOS ):
                SetupAndHelpers.logLine("FAIL", "Metric " + str(metric) + " did not responded in neither OTP, CC or QOS mode.")
                err = -1

        if OTPCount > 0:
            file.write("\n\n## Metrics that are in the expected list but not returned by Prometheus for OTP: \n")
            for expectedMetric in expectedMetricsOTP:
                if expectedMetric not in allMetrics:
                    file.write(str(expectedMetric) + " \n")

        if CCCount > 0:
            file.write("\n\n## Metrics that are in the expected list but not returned by Prometheus for CC: \n")
            for expectedMetric in expectedMetricsCC:
                if expectedMetric not in allMetrics:
                    file.write(str(expectedMetric) + " \n")

        if QOSCount > 0:
            file.write("\n\n## Metrics that are in the expected list but not returned by Prometheus for QOS: \n")
            for expectedMetric in expectedMetricsQOS:
                if expectedMetric not in allMetrics:
                    file.write(str(expectedMetric) + " \n")

        file.close()

    SetupAndHelpers.removeClients(400)

    SetupAndHelpers.logLine("DESC", "DURING THIS TEST MULTIPLE WARNINGS REGARDING MULTIPLE TIME SERIES RECEIVED WILL BE LOGGED, THE TEST WILL CHECK EACH OF THEM AND LOG AN ERROR IF THERE IS INDEED A PROBLEM")

    return err