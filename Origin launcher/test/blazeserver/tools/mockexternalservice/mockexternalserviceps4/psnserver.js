//Author: Sze Ben
//Date: 2016

// Mock PS4 fakes use of Sony's orig PS4 APIs/services.

var http = require("http");
var url = require("url");
var baseurlResponses = require("./baseurlresponses");
var sessioninvitationResponses = require("./sessioninvitationresponses");
var util = require("../common/responseutil");
var log = require("../common/logutil");
var simOutage = require("../common/outageutil");

var mPort = 8081;//default
var mConfigReader = null;

// number of ops before the next simulated base url expiry. 0 disables
var mExpireBaseUrlOpCountThreshold = 0;
var mExpireBaseUrlOpCounter = 0;
var mExpireBaseUrlLastUser = "";//used to ensure don't fail on user's refresh
// number of ops before the next simulated auth token expiry. 0 disables
var mExpireTokenOpCountThreshold = 0;
var mExpireTokenOpCounter = 0;
var mExpireTokenLastUser = "";//used to ensure don't fail retries
// number of ops before the next simulated rate limit error. 0 disables
var mRateLimitUpdateOpCountThreshold = 0;
var mRateLimitUpdateOpCounter = 0;

// process cmd line at startup.
process.argv.forEach(function (val, index, array) {
    
    var argvPort = '-port=';
    var argvConfig = '-config=';
    var argvLogDir = '-logdir=';

    // parse args
    if (index !== 0 && index !== 1) {
        var arg = val.toLowerCase();
        if (arg.substr(0, argvPort.length) === argvPort) {
            if (!Number.isNaN(Number(arg.substr(argvPort.length)))) {
                mPort = Number(arg.substr(argvPort.length));
            }
            else {
                console.error("Invalid port specified '" + val + "', using default port " + mPort);
            }
        }
        else if (arg.substr(0, argvLogDir.length) === argvLogDir) {
            log.setLogFileDir(arg.substr(argvLogDir.length));
        }
        else if (arg.substr(0, argvConfig.length) === argvConfig) {
            var configFile = arg.substr(argvConfig.length);
            mConfigReader = require('readline').createInterface({
                input: require('fs').createReadStream(configFile)
            });
        }
        else {
            console.error("Invalid/unhandled cmd line param: " + val);
        }
    }
});

// process config at startup. Note for performance, console/loghttp/logsessions logging by default are disabled
mConfigReader.on('line', function (line) {
    
    var argComment = '//';
    var argRepI = 'reportinterval=';
    var argFile = 'logfile=';
    var argCons = 'logconsole';
    var argHttpI= 'loghttp=info';
    var argHttpT = 'loghttp=trace';
    var argOutage = 'simoutage=';

    var argLogS = 'logsessions';
    var argBUrlExpireI = 'baseurlexpireinterval=';
    var argAuthExpireI = 'authexpireinterval=';
    var updtRateLimitI = 'updateratelimitinterval=';
    var arg = line.trim().toLowerCase();
    // skip comments
    if (arg.substr(0, argComment.length) === argComment) {
        return;
    }

    // parse args
    if (arg.substr(0, argRepI.length) === argRepI) {
        if (!Number.isNaN(Number(arg.substr(argRepI.length)))) {
            sessioninvitationResponses.setMaxOpCountBeforeReport(Number(arg.substr(argRepI.length)));
        }
        else {
            console.error("Invalid reportinterval '" + val + "', using default interval");
        }
    }
    // simulate PSN Base URL expiry
    else if (arg.substr(0, argBUrlExpireI.length) === argBUrlExpireI) {
        if (!Number.isNaN(Number(arg.substr(argBUrlExpireI.length)))) {
            mExpireBaseUrlOpCountThreshold = (Number(arg.substr(argBUrlExpireI.length)));
        }
        else {
            console.error("Invalid baseurlexpireinterval '" + val + "', using default interval");
        }
    }
    // simulate PSN auth token expiry
    else if (arg.substr(0, argAuthExpireI.length) === argAuthExpireI) {
        if (!Number.isNaN(Number(arg.substr(argAuthExpireI.length)))) {
            mExpireTokenOpCountThreshold = (Number(arg.substr(argAuthExpireI.length)));
        }
        else {
            console.error("Invalid authexpireinterval '" + val + "', using default interval");
        }
    }
    // simulate PSN session update rate limit
    else if (arg.substr(0, updtRateLimitI.length) === updtRateLimitI) {
        if (!Number.isNaN(Number(arg.substr(updtRateLimitI.length)))) {
            mRateLimitUpdateOpCountThreshold = (Number(arg.substr(updtRateLimitI.length)));
        }
        else {
            console.error("Invalid updateratelimitinterval '" + val + "', using default interval");
        }
    }
    else if (arg.substr(0, argFile.length) === argFile) {
        log.setLogFileNamePrefix(arg.substr(argFile.length));
    }
    else if (arg.substr(0, argHttpI.length) === argHttpI) {
        log.setDoLogHttpInfo(true);
    }
    else if (arg.substr(0, argHttpT.length) === argHttpT) {
        log.setDoLogHttpTrace(true);
    }
    else if (arg.substr(0, argCons.length) === argCons) {
        log.setDoLogConsole(true);
    }
    else if (arg.substr(0, argLogS.length) === argLogS) {
        sessioninvitationResponses.setDoLogSessions(true);
    }
    else if (arg.substr(0, argOutage.length) === argOutage) {
        var outageRate = parseInt(arg.substr(argOutage.length), 10);
        if (isNaN(outageRate)) {
            console.error("Invalid/unhandled config param/line: " + line);
        }
        else {
            simOutage.setSimOutageRate(outageRate);
        }
    }
    else {
        console.error("Invalid/unhandled config param/line: " + line);
    }
});


var CONST_CONTENTTYPE = "content-type";
var CONST_CONTENTTYPE_JSON = "application/json";
var CONST_CONTENTLENGTH = "Content-Length:";

//Handle the requests
function onRequest(request, response) {
    
    var isJson = (!util.isUnspecified(request.headers) && request.headers.hasOwnProperty(CONST_CONTENTTYPE) &&
        request.headers[CONST_CONTENTTYPE].substr(0, CONST_CONTENTTYPE_JSON.length) === CONST_CONTENTTYPE_JSON);

    var requestData = [];

    request.on('data', function (data) {
        try {
            requestData.push(data);
        } catch (err) {
            log.ERR("[PSN]onRequest: exception caught: " + err.message + ", stack: " + err.stack);
            return "";
        }
    });
    request.on('end', function (data) {
        try {
            if (isJson) {
                //parse JSON body, before handling request
                const requestBodyJSON = JSON.parse(Buffer.concat(requestData).toString());
                handleOnRequest(request, response, requestBodyJSON, "");
            }
            else {
                const requestBodyBuf = Buffer.concat(requestData);
                handleOnRequest(request, response, "", requestBodyBuf);
            }
        } catch (err) {
            log.ERR("[PSN]onRequest: exception caught: " + err.message + ", stack: " + err.stack);
            return "";
        }
    });
}

// handle request after parsing body
// \param[in] requestBodyJSON If not empty string, the request's body parsed into JSON
// \param[in] requestBodyBuf If not empty, the non-JSON request's raw body
function handleOnRequest(request, response, requestBodyJSON, requestBodyBuf) {

    try {
        log.logReq(request, requestBodyJSON, requestBodyBuf);
    
        var parsedUrl = url.parse(request.url, true);
        var pathname = parsedUrl.pathname;
        var method = request.method;
        var responseBody = "";
        var responseCode = 400;
    
        var ma = pathname.split('/');
    
        // all valid urls have length >= 2 (Blaze's base-url request is "asm/v1/apps/me/baseUrls/sessionInvitation" or "../sdk:sessionInvitation", and all others start with "v1/")
        if ((ma === null) || (ma.length < 2)) {
            log.logRsp(request, 400, responseBody);
            util.writeResponse(response, 400, responseBody);
            return;
        }

        // For compatibility, handle both versions of Sony's session/invitation APIs:
        // Old version's format: https: ../asm/v1/'module'/'itemId'/'branch'..
        // New version's format: https: ../asm/v1/npServiceLabels/n/'module'/'itemId'/'branch'..
        var isServiceLabelsApi = (ma[2] === "npServiceLabels");
        module = (isServiceLabelsApi ? ma[4] : ma[2]);
        branch = (isServiceLabelsApi ? (ma.length >= 7 ? ma[6] : "") : (ma.length >= 5 ? ma[4] : ""));
        var itemId = (isServiceLabelsApi ? (ma.length >= 6 ? ma[5] : "") : (ma.length >= 4 ? itemId : ""));
        
        // handle a get Base URL call as needed. Note, get Base URL calls do not themselves simulate Base URL expiry
        if ((method === "GET") && ((pathname === "/asm/v1/apps/me/baseUrls/sdk:sessionInvitation") || (pathname === "/asm/v1/apps/me/baseUrls/sessionInvitation"))) {
            responseBody = JSON.stringify(baseurlResponses.get_baseurl_sessioninvitation(request.headers, response));
            responseCode = response.statusCode;
        }
        // simulate expiring Base URL, for any non-get Base URL calls, as needed
        else if ((mExpireBaseUrlOpCountThreshold !== 0) && (++mExpireBaseUrlOpCounter >= mExpireBaseUrlOpCountThreshold) &&
            (util.validateAuthorization(request.headers, null) && mExpireBaseUrlLastUser !== request.headers.authorization)) {
            log.LOG("[PSN]Fake expire BaseUrl,user " + request.headers.authorization);
            responseCode = 404;
            mExpireBaseUrlOpCounter = 0;

            // Blaze will retry user's call with a refreshed Base URL, which should succeed. Ensure don't fake another expiry on his retry (in case our expire threshold is configured low)
            mExpireBaseUrlLastUser = request.headers.authorization;
        }
        else if ((mExpireTokenOpCountThreshold !== 0) && (++mExpireTokenOpCounter >= mExpireTokenOpCountThreshold) &&
            (util.validateAuthorization(request.headers, null) && mExpireTokenLastUser !== request.headers.authorization)) {
            log.LOG("[PSN]Fake expire auth token,user " + request.headers.authorization);
            responseCode = 401;
            mExpireTokenOpCounter = 0;
            mExpireTokenLastUser = request.headers.authorization;
        }
        else {

            switch (method) {
                case "GET":
                    if (module === "users" && branch === "sessions") {
                        //GET /v1/users/{onlineId}/sessions, or, GET /v1/users/{accountId}/sessions
                        responseBody = JSON.stringify(sessioninvitationResponses.get_users_sessions_activity(itemId, response));
                        responseCode = response.statusCode;
                    }
                    else if (module === "sessions") {
                        
                        if (simOutage.shouldSimOutage(request.headers)) {
                            responseBody = "";
                            responseCode = simOutage.outageErrCode();
                        }
                        else if (branch === "") {
                            //GET /v1/sessions/{sessionId}?fields=@default,members,sessionLockFlag&npLanguage={language}
                            responseBody = JSON.stringify(sessioninvitationResponses.get_session((isServiceLabelsApi ? request.headers["x-session-ids"] : itemId), response, isServiceLabelsApi));
                            responseCode = response.statusCode;
                        }
                        else if (branch === "sessionData") {
                            //GET /v1/sessions/{sessionId}/sessionData
                            responseBody = sessioninvitationResponses.get_sessiondata(itemId, response);
                            responseCode = response.statusCode;
                        }
                        else if (branch === "changeableSessionData") {
                            //GET /v1/sessions/{sessionId}/changeableSessionData
                            responseBody = sessioninvitationResponses.get_changeablesessiondata(itemId, response);
                            responseCode = response.statusCode;
                        }
                    }
                    break;
                case "POST":
                    if (module === "sessions") {

                        if (simOutage.shouldSimOutage(request.headers)) {
                            responseBody = "";
                            responseCode = simOutage.outageErrCode();
                        }
                        else {
                            if (branch === "") {
                                //POST /v1/sessions
                                responseBody = JSON.stringify(sessioninvitationResponses.post_session(request.headers, requestBodyBuf, response));
                                responseCode = response.statusCode;
                            }
                            else if (branch === "members") {
                                //POST /v1/sessions/{sessionId}/members?npTitleId={npTitleId}
                                responseBody = sessioninvitationResponses.post_session_member(itemId, request.headers, response);
                                responseCode = response.statusCode;
                            }
                        }
                    }
                    break;
                case "PUT":
                    if (module === "sessions") {
                        
                        if (simOutage.shouldSimOutage(request.headers)) {
                            responseBody = "";
                            responseCode = simOutage.outageErrCode();
                        }
                        // check if we should fake rate limiting for update call
                        else if ((mRateLimitUpdateOpCountThreshold !== 0) && (++mRateLimitUpdateOpCounter >= mRateLimitUpdateOpCountThreshold) &&
                             (sessioninvitationResponses.getGameMemberCount(itemId) > 2)) {
                            // Side: to exercise Blaze's re-choose caller mechanism, we'll just fake rate limits when game member count > 2
                            log.LOG("[PSN]Fake update rate limit hit,NPS(" + itemId + "),user " + ((util.isUnspecifiedNested(request.headers, 'authorization')) ? "N/A" : request.headers.authorization));
                            responseCode = 429;//PSNSESSIONINVITATION_TOO_MANY_REQUESTS
                            mRateLimitUpdateOpCounter = 0;
                        }
                        // note rate limited
                        else if (branch === "") {
                            //PUT /v1/sessions/{sessionId}
                            responseBody = sessioninvitationResponses.put_session(itemId, request.headers, requestBodyJSON, response);
                            responseCode = response.statusCode;
                        }
                        else if (branch === "sessionImage") {
                            //PUT /v1/sessions/{sessionId}/sessionImage
                            responseBody = sessioninvitationResponses.put_sessionimage(itemId, request.headers, requestBodyBuf, response);
                            responseCode = response.statusCode;
                        }
                        else if (branch === "changeableSessionData") {
                            //PUT /v1/sessions/{sessionId}/changeableSessionData
                            responseBody = sessioninvitationResponses.put_changeablesessiondata(itemId, request.headers, requestBodyBuf, response);
                            responseCode = response.statusCode;
                        }
                    }
                    break;
                case "DELETE":
                    if (module === "sessions" && branch === "members" && ma.length >= 6) {
                        //DELETE /v1/sessions/{sessionId}/members/{onlineId}, or, DELETE /v1/sessions/{sessionId}/members/{accountId}
                        responseBody = sessioninvitationResponses.delete_session(itemId, request.headers, response);
                        responseCode = response.statusCode;
                    }
                    break;
                default:
                    responseCode = 400;
            }//switch
        }
        log.logRsp(request, responseCode, responseBody);

        util.writeResponse(response, responseCode, responseBody);
    } catch (err) {
        log.ERR("[PSN]handleOnRequest: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


//create server and listen on the port
log.initLogFileDir();
console.log("Mock PSN service listening on port " + mPort + "..");
server = http.createServer(onRequest).listen(mPort);



