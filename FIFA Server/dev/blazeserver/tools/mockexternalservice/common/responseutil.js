//Author: Sze Ben
//Date: 2016

var log = require("./logutil");
var CONST_DEFAULT_COMMAND_DELAY_MS = 0; //side: NodeJS already incurs some delay on high PSU

// This helper is used to guard against missing expected values from Blaze Server's TDF encoded requests. In Blaze Server code, not explicitly specifying a TDF member can cause the JSON to encode it as empty, rather than omitting the member.
// Note: does not return true if obj is 'null', as null is value Blaze MPSD PUT requests explicity specify in order to delete members.
function isUnspecified(obj) {
    return ((obj === "") || (obj === undefined));
}

function isUnspecifiedNested(obj /*, level1, level2, ... levelN*/) {
    var args = Array.prototype.slice.call(arguments, 1);
    for (var i = 0; i <= args.length; ++i) {
        if (isUnspecified(obj)) {
            return true;
        }
        obj = obj[args[i]];
    }
    return false;
}

function isUnspecifiedOrNull(obj) {
    return ((obj === "") || (obj === undefined) || (obj === null));
}

function isUnspecifiedOrNullNested(obj /*, level1, level2, ... levelN*/) {
    var args = Array.prototype.slice.call(arguments, 1);
    for (var i = 0; i <= args.length; ++i) {
        if (isUnspecifiedOrNull(obj)) {
            return true;
        }
        obj = obj[args[i]];
    }
    return false;
}



function count(obj) {
    return ((isUnspecified(obj) || (obj === null)) ? 0 : Object.keys(obj).length);
}

function isString(obj) {
    return (obj !== undefined) && (obj !== null) && (Object.prototype.toString.call(obj) === "[object String]");
}

function isArray(obj) {
    return (obj !== undefined) && (obj !== null) && (obj.constructor === Array);
}


function writeResponse(response, responseCode, responseBody) {
    try {
        if (isUnspecifiedOrNull(responseBody)) {
            responseBody = "";
        }
        var rspBodyStr = (isString(responseBody) ? responseBody : JSON.stringify(responseBody));
        response.writeHead(responseCode, { "Content-Type": "application/json", "Content-Length": rspBodyStr.length });
        response.write(rspBodyStr);
        response.end();
    } catch (err) {
        log.ERR("[HTTP]writeResponse: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

function setResponseXblHeaders(response, xblLogHeader, retryAfterHeader) {
    try {
        if (xblLogHeader !== "") {
            response.setHeader('X-Xbl-Log', xblLogHeader);
        }
        if (retryAfterHeader !== "") {
            response.setHeader('Retry-After', retryAfterHeader);
        }
    } catch (err) {
        log.ERR("[HTTP]setResponseXblHeaders: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// future enhancement, add simulated rate limiting employing the below:
function setResponsePsnHeaders(response, rateLimitRemaining, rateLimitLimit, rateLimitPeriod) {
    try {
        if (rateLimitRemaining !== "") {
            response.setHeader('X-RateLimit-Remaining', rateLimitRemaining);
        }
        if (rateLimitLimit !== "") {
            response.setHeader('X-RateLimit-Limit', rateLimitLimit);
        }
        if (rateLimitPeriod !== "") {
            response.setHeader('X-RateLimit-TimePeriod', rateLimitPeriod);
        }
    } catch (err) {
        log.ERR("[HTTP]setResponsePsnHeaders: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


// returns whether the request has an authorization header, which can be used to identify the caller
function validateAuthorization(requestHeaders, response) {
    if (isUnspecifiedNested(requestHeaders, 'authorization')) {
        log.ERR("[HTTP]validateAuthorization: can't get user id, missing 'authorization' in req headers");
        if (response !== null)
            response.statusCode = 401;
        return false;
    }
    if (requestHeaders.authorization === "0") {
        log.ERR("[HTTP]validateAuthorization: 'authorization' in req headers was '0' (invalid)");
        if (response !== null)
            response.statusCode = 401;
        return false;
    }
    return true;
}

// returns whether the requested npServiceLabel is valid
function validateReqNpServiceLabel(response, npServiceLabel) {
    try {
        if (isUnspecifiedOrNull(npServiceLabel) || isNaN(npServiceLabel) || npServiceLabel < 0) {
            log.ERR("[PSN]validateReqNpServiceLabel: npServiceLabel(" + (isUnspecifiedOrNull(npServiceLabel) ? "<N/A>" : npServiceLabel) + ") invalid");
            response.statusCode = 400;
            return false;
        }
        return true;
    } catch (err) {
        log.ERR("[PSN]validateReqNpServiceLabel: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \param[in] minLen If specified/non-null, check list is at least this len
function validateReqListLen(response, list, minLen, maxLen, logContext) {
    var logPrefix = isUnspecifiedOrNull(logContext) ? "validateReqListLen" : logContext;
    if (!isArray(list)) {
        if (isUnspecifiedOrNull(minLen) || minLen === 0)
            return true;
        log.ERR("[PSN:" + logPrefix + "] invalid list");
        response.statusCode = 400;
        return false;
    }
    if (!isUnspecifiedOrNull(minLen) && (list.length < minLen)) {
        log.ERR("[PSN:" + logPrefix + "] list's len(" + list.length + ") < min allowed len(" + minLen + ")");
        response.statusCode = 400;
        return false;
    }
    if (!isUnspecifiedOrNull(maxLen) && (list.length > maxLen)) {
        log.ERR("[PSN:" + logPrefix + "] list's len(" + list.length + ") > max allowed len(" + maxLen + ")");
        response.statusCode = 400;
        return false;
    }
    return true;
}

// \param[in] minLen If specified/non-null, check string is at least this len
function validateReqStringLen(response, str, minLen, maxLen, logContext) {//can add regex/alphanumeric validation param options
    var logPrefix = isUnspecifiedOrNull(logContext) ? "validateReqStringLen" : logContext;
    if (!isString(str)) {
        if (isUnspecifiedOrNull(minLen) || minLen === 0)
            return true;
        log.ERR("[PSN:" + logPrefix + "] invalid string");
        response.statusCode = 400;
        return false;
    }
    if (!isUnspecifiedOrNull(minLen) && (str.length < minLen)) {
        log.ERR("[PSN:" + logPrefix + "] str(" + str + ")'s len(" + str.length + ") < min allowed len(" + minLen + ")");
        response.statusCode = 400;
        return false;
    }
    if (!isUnspecifiedOrNull(maxLen) && (str.length > maxLen)) {
        log.ERR("[PSN:" + logPrefix + "] str(" + str + ")'s len(" + str.length + ") > max allowed len(" + maxLen + ")");
        response.statusCode = 400;
        return false;
    }
    return true;
}

// \param[in] strMinLen If specified/non-null, check each string is at least this len
function validateReqListIsOfStrings(response, list, strMinLen, strMaxLen, logContext) {
    var logPrefix = isUnspecifiedOrNull(logContext) ? "validateReqListIsOfStrings" : logContext;
    if (!isArray(list)) {
        log.ERR("[PSN:" + logPrefix + "] " + (isUnspecifiedOrNull(logContext) ? "invalid arg" : logContext) + ": expected type Array check failed");
        response.statusCode = 400;
        return false;
    }
    var logContextInList = logContext + "(listItem)";
    for (var i in list) {
        if (!validateReqStringLen(response, i, (isUnspecifiedOrNull(strMinLen) ? 1 : strMinLen), strMaxLen, logContextInList)) {
            return false;//rps set
        }
    }
    return true;
}



module.exports.CONST_DEFAULT_COMMAND_DELAY_MS = CONST_DEFAULT_COMMAND_DELAY_MS;
module.exports.isUnspecified = isUnspecified;
module.exports.isUnspecifiedNested = isUnspecifiedNested;
module.exports.isUnspecifiedOrNull = isUnspecifiedOrNull;
module.exports.isUnspecifiedOrNullNested = isUnspecifiedOrNullNested;
module.exports.count = count;
module.exports.isString = isString;
module.exports.isArray = isArray;
module.exports.writeResponse = writeResponse;
module.exports.setResponseXblHeaders = setResponseXblHeaders;
module.exports.setResponsePsnHeaders = setResponsePsnHeaders;
module.exports.validateAuthorization = validateAuthorization;
module.exports.validateReqNpServiceLabel = validateReqNpServiceLabel;
module.exports.validateReqStringLen = validateReqStringLen;
module.exports.validateReqListIsOfStrings = validateReqListIsOfStrings;