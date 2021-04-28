#!/usr/bin/python3

import http.client, urllib.parse
import json, datetime, ssl
import time, math, threading
from collections import namedtuple

import SetupAndHelpers

######################################## OBSOLETE ##########################################
############################################################################################

######################################################################################################################
######################################################################################################################
################################# !!! DO NOT MODIFY THESE VALUES DURING EXECUTION !!! ################################
########################################### GRAPHITE #################################################################
GRAPHITE_ADDRESS = "internal.graphite.renderer.gos.gameservices.ea.com:80"
GRAPHITE_URL = "/render/?"
# The index of time and value in response json
GRAPHITE_TIME=1
GRAPHITE_VALUE=0

graphiteQueryVariablesPostHosted = "qa-dev.ccs.*.ccs-qa-dev-ccs*.requests.v2.postHosted_connections.eadpcloud-global-qa.aws-use1.*" 
graphiteQueryVariablesTotal = "qa-dev.ccs.*.ccs-qa-dev-ccs*.requests.v2.total_connections.eadpcloud-global-qa.aws-use1.*"
    
queries = dict()
queries["SuccessfulHostedConnectionRequests"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted + ".1-1.*.CREATED.*.count)), 'SuccessfulHostedConnectionRequests')"
queries["TotalSuccessfulConnections"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesTotal + ".*.SUCCESS.count)), 'TotalSuccessfulConnections')"
queries["FailedHostedConnectionRequests"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted + ".1-1.*.*.count)), 'FailedHostedConnectionRequests')"
queries["SubsequentSuccessfulHostedConnectionRequests"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted + ".*.*.CREATED.NOTAPPLICABLE.count)), 'SubsequentSuccessfulHostedConnectionRequests')"
queries["FailedHostedConnecitonRequestsError_DC_BAD_REQUEST"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted + ".1-1.*.DC_BAD_REQUEST.count)), 'FailedHostedConnecitonRequestsError_DC_BAD_REQUEST')"
queries["FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted + ".1-1.*.DC_INSTANCE_DOWN.count)), 'FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN')"
queries["FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted +".1-1.*.CCS_NO_RESOURCE_AVAILABLE.count)), 'FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE')"
queries["FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR"] = "alias(sumSeries(keepLastValue(" + graphiteQueryVariablesPostHosted +".1-1.*.DC_INSTANCE_ERROR.count)), 'FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR')"


#######################################################################################################################
####################################### HELPER FUNCTIONS ##############################################################
#######################################################################################################################
def getUnixTimeStamp():
    return (int)(datetime.datetime.now().timestamp())

def rawDataToNiceDictionary(data_source, raw_data):
    nice_dict = []

    if data_source == "GRAPHITE":
        for i in range(0, len(raw_data)):
            nice_dict.append({"time": raw_data[i][GRAPHITE_TIME], "value": raw_data[i][GRAPHITE_VALUE]})
    elif data_source == "PROMETHEUS":
        for i in range(0, len(raw_data)):
            nice_dict.append({"time": raw_data[i][PROMETHEUS_TIME], "value": raw_data[i][PROMETHEUS_VALUE]})        

    return nice_dict

def sendHttpRequest(address, url, params):
    conn = http.client.HTTPConnection(address)
    conn.request("POST", url + str(params))

    resp = conn.getresponse()   
    body = resp.read()

    return body

#######################################################################################################################
######################################## GENERIC FUNCTIONS ############################################################
#######################################################################################################################

def queryGraphite(query, startTime, stopTime, query_param = "", maxDataPoints=960):
    params = urllib.parse.urlencode({
    "target" : query,
    "format" : "json" ,
    "from" : str(startTime),
    "until" : str(stopTime),        
    "maxDataPoints" : 960 })

    body = sendHttpRequest(GRAPHITE_ADDRESS, GRAPHITE_URL, params)

    try:
        body_json = json.loads(body)
    except:
        return "Error loading JSON!\n " + str(body)

    return body_json


def queryBulkGraphite(startTime, stopTime, query_param = "", maxDataPoints=960):

    params = urllib.parse.urlencode([
    ("target[]" , queries["TotalSuccessfulConnections"]),
    ("target[]" , queries["SuccessfulHostedConnectionRequests"]),
    ("target[]" , queries["FailedHostedConnectionRequests"]),
    ("target[]" , queries["SubsequentSuccessfulHostedConnectionRequests"]),
    ("target[]" , queries["FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE"]),
    ("format" , "json" ),
    ("from" , str(startTime)),
    ("until" , str(stopTime)),      
    ("maxDataPoints" , 960 )])

    body_req1 = sendHttpRequest(GRAPHITE_ADDRESS, GRAPHITE_URL, params)

    params = urllib.parse.urlencode([
    ("target[]" , queries["FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN"]),
    ("target[]" , queries["FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR"]),
    ("target[]" , queries["FailedHostedConnecitonRequestsError_DC_BAD_REQUEST"]),
    ("format" , "json" ),
    ("from" , str(startTime)),
    ("until" , str(stopTime)),      
    ("maxDataPoints" , 960 )])

    body_req2 = sendHttpRequest(GRAPHITE_ADDRESS, GRAPHITE_URL, params)

    try:
        body_json1 = json.loads(body_req1)
        body_json2 = json.loads(body_req2)
    except:
        return "Error loading JSON!\n " + str(body_req1) + str(body_req2)

    d = dict()
    for i in range(0, len(body_json1)):
        if "datapoints" in str(body_json1[i]):
            d[body_json1[i]["target"]] = body_json1[i]["datapoints"]
        else:
            d[body_json1[i]["target"]] = "No Data"

    for i in range(0, len(body_json2)):
        if "datapoints" in str(body_json2[i]):
            d[body_json2[i]["target"]] = body_json2[i]["datapoints"]
        else:
            d[body_json2[i]["target"]] = "No Data"

    return d

def calculateDeltaGraphite(data1, data2):
    if (len(data1) != len(data2)):
        return "Error"

    delta = dict()
    delta["TotalSuccessfulConnections"] = data1["TotalSuccessfulConnections"][0][GRAPHITE_VALUE] - data2["TotalSuccessfulConnections"][0][GRAPHITE_VALUE]
    delta["SuccessfulHostedConnectionRequests"] = data1["SuccessfulHostedConnectionRequests"][0][GRAPHITE_VALUE] - data2["SuccessfulHostedConnectionRequests"][0][GRAPHITE_VALUE]
    delta["FailedHostedConnectionRequests"] = data1["FailedHostedConnectionRequests"][0][GRAPHITE_VALUE] - data2["FailedHostedConnectionRequests"][0][GRAPHITE_VALUE]
    delta["SubsequentSuccessfulHostedConnectionRequests"] = data1["SubsequentSuccessfulHostedConnectionRequests"][0][GRAPHITE_VALUE] - data2["SubsequentSuccessfulHostedConnectionRequests"][0][GRAPHITE_VALUE]
    delta["FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE"] = data1["FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE"][0][GRAPHITE_VALUE] - data2["FailedHostedConnecitonRequestsError_CCS_NO_RESOURCE_AVAILABLE"][0][GRAPHITE_VALUE]
    delta["FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN"] = data1["FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN"][0][GRAPHITE_VALUE] - data2["FailedHostedConnecitonRequestsError_DC_INSTANCE_DOWN"][0][GRAPHITE_VALUE]
    delta["FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR"] = data1["FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR"][0][GRAPHITE_VALUE] - data2["FailedHostedConnecitonRequestsError_DC_INSTANCE_ERROR"][0][GRAPHITE_VALUE]
    delta["FailedHostedConnecitonRequestsError_DC_BAD_REQUEST"] = data1["FailedHostedConnecitonRequestsError_DC_BAD_REQUEST"][0][GRAPHITE_VALUE] - data2["FailedHostedConnecitonRequestsError_DC_BAD_REQUEST"][0][GRAPHITE_VALUE]

    return delta