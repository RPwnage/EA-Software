//Author: Sze Ben
//Date: 2016

var fs = require('fs');
var util = require("./responseutil");
var _CONST_MAX_LOGFILE_LINES = 1000000;
var _LOGREQ_PREFIX = "[HTTP]----------------------\n>  Req: ";
var _LOGRSP_PREFIX = "[HTTP]\n<  Rsp: Status ";

var mDoLogConsole = false;
var mDoLogHttpInfo = false;
var mDoLogHttpTrace = false;
// does log file if prefix is non-empty:
var mLogFileNamePrefix = "";
var mLogFileName = "";
var mLogFileLineCount = 0;
var mLogFileDir = "";
var mCurrTimeStamp = 0;
var mCurrTimeStampStr = "";

function setDoLogConsole(val) { mDoLogConsole = val; }
function setDoLogHttpInfo(val) { mDoLogHttpInfo = val; }
function setDoLogHttpTrace(val) { mDoLogHttpTrace = val; }
function setLogFileNamePrefix(val) { mLogFileNamePrefix = val; }
function setLogFileDir(val) { mLogFileDir = val; }

function LOG(msg) { logInternal(msg); }
function ERR(msg) { logInternal("[ERR]" + msg); }
function WARN(msg) { logInternal("[WARN]" + msg); }
function logInternal(msg) {
    try {
        // for eff, cached time stamp and file name
        if ((mCurrTimeStamp === 0) || (mCurrTimeStamp !== Date.now())) {
            mCurrTimeStamp = Date.now();
            mCurrTimeStampStr = new Date().toISOString().replace(/\:/g, "_").replace(/\./g, "_");
        }
        var logStr = mCurrTimeStampStr + msg;

        if (mDoLogConsole === true) {
            console.log(logStr);
        }

        if (mLogFileNamePrefix !== "") {
            if ((mLogFileLineCount === 0) || (mLogFileLineCount >= _CONST_MAX_LOGFILE_LINES)) {
                mLogFileLineCount = 1;
                mLogFileName = ((mLogFileDir !== "" ? (mLogFileDir + "/") : "") + mLogFileNamePrefix + (mCurrTimeStampStr) + ".log");
            }
            fs.appendFile(mLogFileName, "\n" + logStr, function (err) {
                if (err) { return console.log(err); }
            });
            ++mLogFileLineCount;
        }
    } catch (err) {
        // not much can be done if write stream closed etc
    }
}
// due to larger post requests w/PSN session image etc, cap logged bytes
const CONST_MAX_LOGGED_REQ_RSP_BYTES = 4096;

function logReq(request, requestBodyJSON, requestBodyBuf) {
    try {
        if ((mDoLogHttpInfo === false) && (mDoLogHttpTrace === false)) {
            return;
        }
        var logstr = _LOGREQ_PREFIX + request.method + " " + request.url;        
        if (mDoLogHttpTrace === true) {
            if (!util.isUnspecified(request.headers)) {
                for (var k in request.headers) {
                    logstr = logstr + "\n>  " + k + ": " + request.headers[k];
                }
            }
            if (!util.isUnspecified(requestBodyBuf)) {
                logstr = logstr + "\n>  Body: " + requestBodyBuf.slice(0, CONST_MAX_LOGGED_REQ_RSP_BYTES).toString() + " ..";
            }
            else if (!util.isUnspecified(requestBodyJSON)) {
                logstr = logstr + "\n>  Body: " + JSON.stringify(requestBodyJSON);
            }
        }
        else if ((mDoLogHttpInfo === true) &&
            !util.isUnspecifiedNested(request.headers, 'authorization')) {
            logstr = logstr + "\n>  User: " + request.headers["authorization"];
        }
        
        LOG(logstr);
    } catch (err) {
        ERR("[logutil]logReq: exception caught: " + err.message + ", stack: " + err.stack);
    }
}
function logRsp(request, responseCode, responseBody) {
    try {
        if ((mDoLogHttpInfo === false) && (mDoLogHttpTrace === false)) {
            return;
        }
        var logstr = _LOGRSP_PREFIX + responseCode;
        if (mDoLogHttpTrace === true && !util.isUnspecifiedOrNull(responseBody)) {
            var rspBodyStr = (util.isString(responseBody) ? responseBody : JSON.stringify(responseBody));
            logstr = logstr + "\n<  Body: " + rspBodyStr.slice(0, CONST_MAX_LOGGED_REQ_RSP_BYTES).toString() + " ..";
        }
        LOG(logstr);
    } catch (err) {
        ERR("[logutil]logRsp: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


function initLogFileDir() {
    // if no log dir specified, make a timestamped dir
    if (mLogFileDir === "") {
        mLogFileDir = new Date().toISOString().replace(/\:/g, "_").replace(/\./g, "_");
    }
    if (!fs.existsSync(mLogFileDir)) {
        fs.mkdirSync(mLogFileDir);
    }
}


module.exports.LOG = LOG;
module.exports.ERR = ERR;
module.exports.WARN = WARN;
module.exports.setDoLogConsole = setDoLogConsole;
module.exports.setLogFileNamePrefix = setLogFileNamePrefix;
module.exports.setLogFileDir = setLogFileDir;
module.exports.setDoLogHttpInfo = setDoLogHttpInfo;
module.exports.setDoLogHttpTrace = setDoLogHttpTrace;
module.exports.initLogFileDir = initLogFileDir;
module.exports.logReq = logReq;
module.exports.logRsp = logRsp;
