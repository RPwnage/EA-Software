#!/usr/bin/python3

import http.client, urllib.parse
import json, datetime, ssl
import time, math, threading
import requests
import os

import SetupAndHelpers

######################################################################################################################
######################################################################################################################
################################# !!! DO NOT MODIFY THESE VALUES DURING EXECUTION !!! ################################
########################################### PROMETHEUS ##############################################################
PROMETHEUS_ADDRESS = "gs-shared-1m.metrics.darkside.ea.com"
PROMETHEUS_CERT_PATH = SetupAndHelpers.gTestArtifactsFolder + '/PrometheusCerts/gs-shared.cert.pem'
PROMETHEUS_KEY_PATH = SetupAndHelpers.gTestArtifactsFolder + '/PrometheusCerts/gs-shared.key.pem'
# The index of time and value in response json
PROMETHEUS_TIME=0
PROMETHEUS_VALUE=1

# URL's based on datasource
PROMETHEUS_URL = dict()
# Cloud data sources
PROMETHEUS_URL["gameplay-services-internal-ccsdirtycast"] = "/gs-dirtycast-metrics/gameplay-services-internal-ccsdirtycast/api/prom/api/v1/"
PROMETHEUS_URL["gameplay-services-internal-dirtysock-dev"] = "/gs-dirtycast-metrics/gameplay-services-internal-dirtysock-dev/api/prom/api/v1/"
# Bare metal data sources
PROMETHEUS_URL["gameplay-services-gs-qos-metrics"] = "/gs-qos-metrics/gameplay-services-production-qos-rules/api/prom/api/v1/"
# Vault keys
TENANT_NAMESPACE="eadp-gs-arson-dev"
VAULT_ADDR="https://test.ess.ea.com/"
API_VER="v1/"
VAULT_URL_LOGIN_PATH = "auth/approle/login"
VAULT_URL_SECRET_PATH = "secrets/kv/data/arson-blaze"

### Set this value in initial_config ONLY !!! ###
gPrometheusDefaultDataSource=''

HTTPSession = requests.Session()
#######################################################################################################################
####################################### HELPER FUNCTIONS ##############################################################
#######################################################################################################################
def getUnixTimeStamp():
    return (int)(datetime.datetime.now().timestamp())

def getSSLContext():
    sslContext = ssl.create_default_context()
    sslContext.load_cert_chain(PROMETHEUS_CERT_PATH, PROMETHEUS_KEY_PATH)

    return sslContext
    
def getVaultCertificate():
    
    if not os.path.exists("SecretId.cfg"):
        SetupAndHelpers.logLine("ERR", "Error: Cannot find SecretId.cfg. Please add the file into MetricsTester folder. The file should contains: {\"role_id\":\"{arson_role_id}\", \"secret_id\":\"{arson_secret_id}\"")
    else:
        text_file = open("SecretId.cfg", "r")
        secretCfg = json.load(text_file)
      
        ROLE_ID = secretCfg["role_id"]
        SECRET_ID = secretCfg["secret_id"]

        try:
            loginUrl = VAULT_ADDR + API_VER + VAULT_URL_LOGIN_PATH 
            loginBody = {'role_id' : ROLE_ID, 'secret_id' : SECRET_ID}
            loginHeaders = {'X-Vault-Namespace': TENANT_NAMESPACE}
            loginResponse = requests.post(loginUrl, data=json.dumps(loginBody), headers=loginHeaders)
            loginResponseJson  = json.loads(loginResponse.text)
            clientToken = loginResponseJson ["auth"]["client_token"]
            
            contentUrl = VAULT_ADDR + API_VER + VAULT_URL_SECRET_PATH
            contentHeaders = {'X-Vault-Namespace': TENANT_NAMESPACE, 'X-Vault-Token': clientToken}
            contentResponse = requests.get(contentUrl, headers=contentHeaders)
            contentResponseJson = json.loads(contentResponse.text)
            prometheus_gs_shared_cert = contentResponseJson["data"]["data"]["prometheus_gs_shared_cert"]
            prometheus_gs_shared_key = contentResponseJson["data"]["data"]["prometheus_gs_shared_key"]
            try:
                os.makedirs(SetupAndHelpers.gTestArtifactsFolder + "/PrometheusCerts")
            except FileExistsError:
                pass
            text_file = open(PROMETHEUS_CERT_PATH, "w")
            text_file.write(prometheus_gs_shared_cert)
            text_file.close()

            text_file = open(PROMETHEUS_KEY_PATH, "w")
            text_file.write(prometheus_gs_shared_key)
            text_file.close()

        except Exception as err:
            SetupAndHelpers.logLine("ERR", "Error " + str(err))

def sendHttpRequest(address, url, params):
    body = 'err'
    try:
        #conn = http.client.HTTPSConnection(address, context=getSSLContext())
        #conn.request("GET", url + str(params))
        #resp = conn.getresponse()  
        #body = resp.read()

        # Using same global session using requests API is lot faster than creating a new one every time
        resp = HTTPSession.get("https://" + address + url + str(params), cert=(PROMETHEUS_CERT_PATH, PROMETHEUS_KEY_PATH))
        body = resp.text

        SetupAndHelpers.logLine("TRACE", url + str(params) )

    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Error " + str(err) + " ; full url: " + address + url + " ; params: " + str(params))

    return body

#######################################################################################################################
######################################## GENERIC FUNCTIONS ############################################################
#######################################################################################################################
# STABLE
def setDefaultDataSource(datasource):
    global gPrometheusDefaultDataSource
    gPrometheusDefaultDataSource = datasource

# STABLE
def getSeries(match_query, datasource='', startTime=getUnixTimeStamp() - 1800, stopTime=getUnixTimeStamp()):
    body = ''
    params = urllib.parse.urlencode({
    "match[]" : str(match_query) ,
    "start" : str(startTime) ,
    "end" : str(stopTime) })

    if datasource == '':
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[gPrometheusDefaultDataSource] + 'series?', params) 
    else:
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[datasource] + 'series?', params) 

    try:
        body_json = json.loads(body)
    except:
        SetupAndHelpers.logLine("ERR", "Error loading JSON: \n " + str(body) )
        return "err"

    SetupAndHelpers.logLine("TRACE", "Got " + str(body_json))

    if body_json["status"] == "success" and len(body_json["data"]) > 0:
        return body_json["data"]
    elif body_json["status"] != "success":
        SetupAndHelpers.logLine("ERR", "Prometheus query failed with err: " + str(body_json["status"]) + " , query: \n" + match_query + "\nbody parsed: \n" + str(body_json))
        return "err"
    elif len(body_json["data"]) < 1:
        SetupAndHelpers.logLine("WARN", "Prometheus query " + str(match_query) + " did not return any values")
        return ''
    else:
        SetupAndHelpers.logLine("ERR", "Unkown error on Prometheus query.")

# STABLE
def getAllMetricNamesFromDataSource(datasource='', pattern="voipmetrics_"):
    ret_metrics=list()

    if datasource == '':
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[gPrometheusDefaultDataSource] + 'label/__name__/values', '')
    else:
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[datasource] + 'label/__name__/values', '')

    try:
        body_json = json.loads(body)
    except:
        SetupAndHelpers.logLine("ERR", "Error loading JSON: \n " + str(body) )
        return "err"

    for metric in body_json["data"]:
        if pattern in metric:
            ret_metrics.append(str(metric))

    return ret_metrics

# STABLE
def querySimpleSingleValue(query, timestamp=getUnixTimeStamp(), step=60, datasource=''):
    return querySimple(query, timestamp, timestamp, step, datasource, query_type="query?")

# STABLE
def querySimple(query, startTime=getUnixTimeStamp(), stopTime=getUnixTimeStamp(), step=60, datasource='', query_type="query_range?"):
    global gPrometheusDefaultDataSource

    params = urllib.parse.urlencode({
        "query" : query,
        "start" : str(startTime) ,
        "end" : str(stopTime),
        "step" : str(step) })

    if datasource == '':
        if gPrometheusDefaultDataSource == '':
            SetupAndHelpers.logLine("ERR", "Invalid datasource")
            return  
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[gPrometheusDefaultDataSource] + query_type, params)
    else:
        body = sendHttpRequest(PROMETHEUS_ADDRESS, PROMETHEUS_URL[datasource] + query_type, params)

    if body == 'err':
        return 'err'

    try:
        body_json = json.loads(body)
    except:
        SetupAndHelpers.logLine("ERR", "Error loading JSON: \n " + str(body) )
        return "err"

    SetupAndHelpers.logLine("TRACE", "Got " + str(body_json))

    if len(body_json["data"]["result"]) > 1:
        SetupAndHelpers.logLine("WARN", "Prometheus query " + str(query) + " did return more than 1 time series. The functions are designed to handle only the first one, adjust the query to return only one!")
        diff_labels=set()
        for i in range(0, len( body_json["data"]["result"] ) ):
            for j in range(0, len( body_json["data"]["result"] ) ):
                diff_labels = diff_labels.union( [k for k in body_json["data"]["result"][i]["metric"] if body_json["data"]["result"][i]["metric"][k] != body_json["data"]["result"][j]["metric"][k]] )  
        SetupAndHelpers.logLine("WARN", "The different dimensions are: " + str(diff_labels))

    if body_json["status"] == "success" and len(body_json["data"]["result"]) > 0:
        return body_json["data"]
    elif body_json["status"] != "success":
        SetupAndHelpers.logLine("ERR", "Prometheus query failed with err: " + str(body_json["status"]) + " , query: \n" + query + "\nbody parsed: \n" + str(body_json))
        return "err"
    elif len(body_json["data"]["result"]) < 1:
        SetupAndHelpers.logLine("TRACE", "Prometheus query " + str(query) + " did not return any values")
        return ''
    else:
        SetupAndHelpers.logLine("ERR", "Unkown error on Prometheus query.")
        return "err"

# STABLE
def queryListSingleValue(queries, timestamp=getUnixTimeStamp(), step=60, datasource=''):
    global gPrometheusDefaultDataSource
    SetupAndHelpers.logLine("INFO", "Querying for " + str(queries.keys()))
    return_dict = dict()

    for key in queries.keys():
        return_dict[key] = querySimpleSingleValue(queries[key], timestamp, step, datasource)

    return return_dict

# STABLE
def queryList(queries, startTime=getUnixTimeStamp(), stopTime=getUnixTimeStamp(), step=60, datasource='', query_type="query_range?"):
    global gPrometheusDefaultDataSource
    SetupAndHelpers.logLine("INFO", "Querying for " + str(queries.keys()))
    return_dict = dict()

    for key in queries.keys():
        return_dict[key] = querySimple(queries[key], startTime, stopTime, step, datasource, query_type)

    return return_dict

# STABLE
def getFirstValue(results):
    # Sometimes, even if you query for only one value the Prometheus will respond with 2 entries, one NaN and one the actual response
    if results == '':
        return ''

    try:
        if results["resultType"] == "matrix":           
            for data in results["result"][0]['values']:
                if "NaN" not in str(data):
                    return int( data[PROMETHEUS_VALUE] )
                else:
                    return 0

        elif results["resultType"] == "vector":
            if "NaN" not in str(results):
                return int ( results["result"][0]["value"][PROMETHEUS_VALUE] )
            else:
                return 0

    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Cought err "+ str(err))
        return "err"

    return "err"

#STABLE
def multipleTimeSeriesToDict(results_dict):
    ret_dict = dict()
    for key in results_dict.keys():
        ret_dict[key] = timeSeriesToDict(results_dict[key])

    return ret_dict

# STABLE 
# Convert a time serie to a dictionary with: key - timestamp
# For cycling use: for key in dictionary.keys() so that you have access to both keys and values
def timeSeriesToDict(results, resultIndex = 0):
    ret_list = dict()
    # Sometimes, even if you query for only one value the Prometheus will respond with 2 entries, one NaN and one the actual response
    if results == '':
        return results

    try:
        if results["resultType"] == "matrix" and len(results["result"]) > 0:            
            for data in results["result"][resultIndex]['values']:
                if "NaN" not in data:
                    ret_list[str(data[PROMETHEUS_TIME])] = (str( data[PROMETHEUS_VALUE] ) )


        elif results["resultType"] == "vector" and len(results["result"][0]) > 0:
            if "NaN" not in str( results["result"][0]["value"] ) :
                ret_list[str(results["result"][0]["value"][PROMETHEUS_TIME])] = (str( results["result"][0]["value"][PROMETHEUS_VALUE] ) )

        return ret_list
    except Exception as err:
        SetupAndHelpers.logLine("ERR", "Cought err "+ str(err))
        return "err"

def printDictionaryNicely(input_dict):
    print ( json.dumps(input_dict, indent = 4) )

# STABLE
# initial, final and expected should be dictionaries with the key the metric name
### Keywords:
# empty string - No data is expected, empty response
# any - skip validation
# data - Data is expected, any data, flat 0 is considered OK
# change - We expect a change in data returned, any change
###
### _g stands for gauge and _c for counter
def compareChanges(initial, final, expected_change, only_gauges=False, only_counters=False, only_list=[]):
    SetupAndHelpers.logLine("INFO", "Comparing changes for " + str(initial.keys()))
    err = 0

    if not len(initial) == len(final) and len(final) == len(expected_change):
        SetupAndHelpers.logLine("ERR", "Input error")
        return -1

    for key in initial.keys():
        if only_gauges == True and "_g" not in key:
            continue
        if only_counters == True and "_c" not in key:
            continue
        if len(only_list) > 0 and key not in only_list:
            continue

        try:
            # This means we don't expect this metric to return anything - empty
            if expected_change[key] == 'any':
                continue
            # This means we don't expect this metric to return anything - empty
            elif expected_change[key] == '' and final[key] == '' and initial[key] == '':
                continue
            # We expect any data, and got some data
            elif expected_change[key] == 'data' and ( final[key] != '' or initial[key] != '' ):
                continue
            # Expecting no change might equal to empty initial and final response - if the aggregator is configured this way
            elif expected_change[key] == 0 and ( initial[key] == '' and final[key] == '' ):
                SetupAndHelpers.logLine("INFO", "Empty response for " + str(key) + " , expected change was: " + str(expected_change[key]))
                continue
            # Got empty response when data was expected
            elif  expected_change[key] != '' and ( initial[key] == '' or final[key] == '' ):
                SetupAndHelpers.logLine("FAIL", "Empty response for " + str(key) + " , expected change was: " + str(expected_change[key]))
                err = -1
                continue
            # Got data when none was expected ????????
            elif expected_change[key] == '' and ( initial[key] != '' or final[key] != '' ):
                SetupAndHelpers.logLine("WARN", "Got data, when none was expected for " + str(key))
                continue

            diff = int( getFirstValue(final[key]) ) - int( getFirstValue(initial[key]) )
            if not diff == int(expected_change[key]):
                SetupAndHelpers.logLine("FAIL", "DID NOT met up the expected change for " + str(key) + " expected: " + str(expected_change[key]) + " vs actual: " + str(diff) )
                err = -1
        except Exception as e:
            SetupAndHelpers.logLine("ERR", "Cought err at key " + str(key) + ", err: " + str(e))
            err = -1

    return err


# STABLE
# results_dict should be a dictionary
### Keywords:
# empty string - No data is expected, empty response
# any - skip validation
# data - Data is expected, any data, flat 0 is considered OK
# change - We expect a change in data returned, any change
###
### _g stands for gauge and _c for counter
def checkResultIsInRange(results_dict, expected_range, only_gauges=False, only_counters=False, only_list=[]):
    SetupAndHelpers.logLine("INFO", "Checking results for " + str(results_dict.keys()))

    ret_value = 0
    for res_key in results_dict.keys():
        if only_gauges == True and "_g" not in res_key:
            continue
        if only_counters == True and "_c" not in res_key:
            continue
        if len(only_list) > 0 and res_key not in only_list:
            continue

        # This means we don't expect this metric to return anything - empty
        if expected_range[res_key] == 'any':
            continue
        # This means we don't expect this metric to return anything - empty
        elif results_dict[res_key] == '' and expected_range[res_key] == '':
            continue
        # We expect any data, and got some data
        elif results_dict[res_key] != '' and expected_range[res_key] == 'data':
            continue
        # Expecting no change might equal to empty initial and final response - if the aggregator is configured this way
        elif expected_range[res_key] == "0:0" and ( initial[res_key] == '' and final[res_key] == '' ):
            SetupAndHelpers.logLine("INFO", "Empty response for " + str(res_key) + " , expected range was: " + str(expected_range[res_key]))
            continue
        # Got empty response when data was expected
        elif results_dict[res_key] == '' and expected_range[res_key] != '':
            SetupAndHelpers.logLine("FAIL", "Empty result for " + str(res_key) + " , expected range was " + str(expected_range[res_key]))
            ret_value = -1
            continue
        # Got data when none was expected ????????
        elif results_dict[res_key] != '' and expected_range[res_key] == '':
            SetupAndHelpers.logLine("WARN", "Got data when none was expected for " + str(res_key))
            continue

        res_dict = timeSeriesToDict(results_dict[res_key])

        try:
            minExpectedValue = float(expected_range[res_key].split(":")[0])
            maxExpectedValue = float(expected_range[res_key].split(":")[1])

            for key in res_dict.keys():
                if float(res_dict[key]) > maxExpectedValue or float(res_dict[key]) < minExpectedValue:
                    SetupAndHelpers.logLine("FAIL", "Found value " + str(res_dict[key]) + " for " + str(res_key) + " at " + str(key) + " not in range [ " + str(minExpectedValue) + " : " + str(maxExpectedValue) + " ]")
                    ret_value = -1
                    break
        except Exception as err:
            SetupAndHelpers.logLine("ERR", "Cought err " + str(err) + " for " + str(res_key))

    return ret_value

# STABLE
def calculateResultsStats(results_dict):
    SetupAndHelpers.logLine("INFO", "Calculating metrics stats for " + str(results_dict.keys()))

    return_dict = dict()

    for key in results_dict.keys():
        # Metric did not return any values
        if results_dict[key] == '':
            SetupAndHelpers.logLine("WARN", "Empty result for " + str(key))
            continue

        return_dict[key] = dict()
        total_sum = 0

        average = 0
        max_value = 0
        min_value = 0
        total_fall = 0
        total_rise = 0
        fall_count = 0
        rise_count = 0
        avg_fall = 0
        avg_rise = 0

        timeSeriesDict = timeSeriesToDict(results_dict[key])
        last_value = 0

        for key_value in timeSeriesDict.keys():
            if float(timeSeriesDict[key_value]) > float( max_value ):
                max_value = float (timeSeriesDict[key_value])

            if float(timeSeriesDict[key_value]) < float( min_value ):
                min_value = float (timeSeriesDict[key_value])

            total_sum = total_sum + float (timeSeriesDict[key_value])

            if float(timeSeriesDict[key_value]) < float( last_value ):
                total_fall = total_fall + ( float(last_value) - float(timeSeriesDict[key_value]) )
                fall_count = fall_count + 1

            if float(timeSeriesDict[key_value]) > float( last_value ):
                total_rise = total_rise + ( float(timeSeriesDict[key_value]) - float(last_value) )
                rise_count = rise_count +  1

            last_value = float(timeSeriesDict[key_value])

        if len(timeSeriesDict) > 0:
            return_dict[key]["average"] = float( total_sum ) / float ( len(timeSeriesDict) )
        else:
            return_dict[key]["average"] = 0

        return_dict[key]["max_value"] = float(max_value)
        return_dict[key]["min_value"] = float(min_value)
        return_dict[key]["total_fall"] = float (total_fall)
        return_dict[key]["total_rise"] = float(total_rise)
        return_dict[key]["fall_count"] = float(fall_count)
        return_dict[key]["rise_count"] = float(rise_count)

        if float(fall_count) > 0:
            return_dict[key]["avg_fall"] = float(total_fall) / float(fall_count)
        else:
            return_dict[key]["avg_fall"] = 0

        if float(rise_count) > 0:
            return_dict[key]["avg_rise"] = float(total_rise) / float(rise_count)
        else:
            return_dict[key]["avg_rise"] = 0

    return return_dict