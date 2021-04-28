//Author: Sze Ben
//Date: 2020

var http = require("http");
var url = require("url");
var sessionmanagerResponses = require("./sessionmanagerresponses");
var matchesResponses = require("./matchesresponses");

var util = require("../common/responseutil");
var log = require("../common/logutil");
var simOutage = require("../common/outageutil");

var mPort = 8090;//default
var mConfigReader = null;

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
    var argHttpI = 'loghttp=info';
    var argHttpT = 'loghttp=trace';
    var argLogS = 'logsessions';
    var argAuthExpireI = 'authexpireinterval=';
    var updtRateLimitI = 'updateratelimitinterval=';
    var argHttp409matchms = 'http409matchms=';
    var argActJson = 'activityidsjson=';//e.g. { "BlazeSampleActivity": { "category": "competitive", "isteamActivity":"false"}, "BlazeSampleTeamActivityWithScore": { "category": "competitive", "isteamActivity":"true", "scorename": "Score"} }
    var arg = line.trim().toLowerCase();
    // skip comments
    if (arg.substr(0, argComment.length) === argComment) {
        return;
    }

    // parse args
    if (arg.substr(0, argRepI.length) === argRepI) {
        if (!Number.isNaN(Number(arg.substr(argRepI.length)))) {
            sessionmanagerResponses.setMaxOpCountBeforeReport(Number(arg.substr(argRepI.length)));
            matchesResponses.setMaxOpCountBeforeReport(Number(arg.substr(argRepI.length)));
        }
        else {
            console.error("Invalid reportinterval '" + val + "', using default interval");
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
    // simulate PSN Match RPC http 409 when 2 conseq calls are too close together (< the min ms)
    else if (arg.substr(0, argHttp409matchms.length) === argHttp409matchms) {
        if (!Number.isNaN(Number(arg.substr(argHttp409matchms.length)))) {
            matchesResponses.setHttp409matchms(Number(arg.substr(argHttp409matchms.length)));
        }
        else {
            console.error("Invalid http409matchms '" + val + "', disabling");
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
        sessionmanagerResponses.setDoLogSessions(true);
        matchesResponses.setDoLogPSN(true);
    }
    else if (arg.substr(0, argActJson.length) === argActJson) {
        var activityIdsjson = arg.substr(argActJson.length);
        if (!matchesResponses.configureActivities(activityIdsjson)) {
            console.error("Failed configuring activityes, config param/line: " + line);
        }
    }
    else {
        console.error("Invalid/unhandled config param/line: " + line);
    }
});


var CONST_CONTENTTYPE = "content-type";
var CONST_CONTENTTYPE_JSON = "application/json";
var CONST_CONTENTLENGTH = "Content-Length:";
const HTTP401_ERR_RSP_SESSMGR = { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2232972, "message": "Mock PSN: Fake Http 401: Access token is expired." } };
const HTTP401_ERR_RSP_MATCHES = { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269836, "message": "Mock PSN: Fake Http 401: Access token is expired." } };

//Handle the requests
function onRequest(request, response) {

    var isJson = (!util.isUnspecified(request.headers) && request.headers.hasOwnProperty(CONST_CONTENTTYPE) &&
        request.headers[CONST_CONTENTTYPE].toLowerCase().substr(0, CONST_CONTENTTYPE_JSON.length) === CONST_CONTENTTYPE_JSON);

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

        var ma = pathname.split(/\/|\?/);

        // Guard vs empty reqs. Min poss len is 2 for 'v1/matches'
        if ((ma === null) || (ma.length < 2)) {
            log.logRsp(request, 400, responseBody);
            util.writeResponse(response, 400, responseBody);
            return;
        }

        // 2 Sony APIs URL formats:
        //  Non-service-labels:   https: {baseurl}/v1/{module}/{itemId}/{branch}/{postBranchItemId1}/{postBranchItemId2} ..
        //  Service-labels:       https: {baseurl}/v1/npServiceLabels/{#}/{module}/{itemId}/{branch}/{postBranchItemId1}/{postBranchItemId2} ..
        var isServiceLabelsUrl = ((ma.length >= 3) && (ma[2] === "npServiceLabels"));
        module = (isServiceLabelsUrl ? (ma.length >= 5 ? ma[4] : "") : (ma.length >= 3 ? ma[2] : ""));
        branch = (isServiceLabelsUrl ? (ma.length >= 7 ? ma[6] : "") : (ma.length >= 5 ? ma[4] : ""));
        var itemId = (isServiceLabelsUrl ? (ma.length >= 6 ? ma[5] : "") : (ma.length >= 4 ? ma[3] : ""));
        var postBranchItemId1 = (isServiceLabelsUrl ? (ma.length >= 8 ? ma[7] : "") : (ma.length >= 6 ? ma[5] : ""));
        var postBranchItemId2 = (isServiceLabelsUrl ? (ma.length >= 9 ? ma[8] : "") : (ma.length >= 7 ? ma[6] : ""));

        if ((mExpireTokenOpCountThreshold !== 0) && (++mExpireTokenOpCounter >= mExpireTokenOpCountThreshold) &&
            (util.validateAuthorization(request.headers, null) && mExpireTokenLastUser !== request.headers.authorization)) {
            log.LOG("[PSN]Fake expire auth token,user " + request.headers.authorization);
            responseBody = ((module.toLowerCase() === "matches") ? HTTP401_ERR_RSP_MATCHES : HTTP401_ERR_RSP_SESSMGR);
            responseCode = 401;
            mExpireTokenOpCounter = 0;
            mExpireTokenLastUser = request.headers.authorization;
        }
        else {

            switch (method) {
                case "GET": {
                    switch (module.toLowerCase()) {
                        case "matches": {
                            //GET /v1/matches/{matchId}?npServiceLabel={#}       (GEN5_TODO: confirm w/sony npServiceLabel for Matches is a query param not in URL? Validate service label from blaze non empty as needed)
                            var npServiceLabel = (ma.length >= 5 ? ma[4] : "");
                            responseBody = JSON.stringify(matchesResponses.get_match(response, itemId, npServiceLabel));//itemId = {matchId}
                            responseCode = response.statusCode;
                            break;
                        }
                        case "playersessions": {
                            if (branch === "") {
                                //GET /v1/playerSessions?fields=...
                                responseBody = JSON.stringify(sessionmanagerResponses.get_playersession(response, request.headers, true));//tail arg is notFoundIsStatusOk
                                responseCode = response.statusCode;
                            }
                            else {
                                log.WARN("[PSN]unhandled GET " + module+ ":" + branch);
                                responseCode = 400;
                            }
                            break;
                        }
                        case "users": {
                            if (branch.toLowerCase() === "playersessions") {//if time for consistency reformat code here to switch branch, instead of if-elses
                                //GET /v1/users/{accountId}/playerSessions?memberFilter=player,spectator&platformFilter=[PS4|PS5]
                                responseBody = JSON.stringify(sessionmanagerResponses.get_users_playersessions(response, itemId, postBranchItemId1));//itemId = {accountId}, postBranchItemId1 = memberFilter=player,spectator&platformFilter=[PS4|PS5]
                                responseCode = response.statusCode;
                            }
                            else if (branch.toLowerCase() === "playersessionsinvitations") {
                                //GET /v1/users/{accountId}/playerSessionsInvitations
                                log.WARN("[PSN]NOT_IMPL: GET " + module + " branch:" + branch);
                                responseCode = 501;
                            }
                            else {
                                log.WARN("[PSN]unhandled GET " + module + " branch:" + branch);
                                responseCode = 400;
                            }
                            break;
                        }
                        default: {
                            log.WARN("[PSN]unhandled module for GET:" + module);
                            responseCode = 400;
                        }
                    }
                    break;
                }
                case "POST": {
                    switch (module.toLowerCase()) {
                        case "matches": {
                            if (simOutage.shouldSimOutage(request.headers)) {
                                responseBody = "";
                                responseCode = simOutage.outageErrCode();
                            }
                            else {
                                switch (branch) {
                                    case "": {
                                        //POST /v1/matches  (create match)
                                        responseBody = JSON.stringify(matchesResponses.post_match(response, request.headers, requestBodyJSON));
                                        responseCode = response.statusCode;
                                        break;
                                    }
                                    case "results": {
                                        //POST /v1/matches/{matchId}/results
                                        responseBody = JSON.stringify(matchesResponses.post_match_results(response, request.headers, requestBodyJSON, itemId));//itemId = {matchId}
                                        responseCode = response.statusCode;
                                        break;
                                    }
                                    case "players": {
                                        if (postBranchItemId2 === "add") {
                                            //POST /v1/matches/{matchId}/players/actions/add
                                            responseBody = JSON.stringify(matchesResponses.post_join_match(response, request.headers, requestBodyJSON, itemId));//itemId = {matchId}
                                            responseCode = response.statusCode;
                                        }
                                        else if (postBranchItemId2 === "remove") {
                                            //POST /v1/matches/{matchId}/players/actions/remove
                                            responseBody = JSON.stringify(matchesResponses.post_leave_match(response, request.headers, requestBodyJSON, itemId));//itemId = {matchId}
                                            responseCode = response.statusCode;
                                        }
                                        else {
                                            log.WARN("[PSN]unhandled/missing POST:" + module + "../" + branch + "/" + postBranchItemId1 + "/{item}:" + postBranchItemId2);
                                            responseCode = 400;
                                        }
                                        break;
                                    }
                                    default: {
                                        log.WARN("[PSN]unhandled POST:" + module + " branch:" + branch);
                                        responseCode = 400;
                                    }
                                }//branch
                            }
                            break;
                        }
                        case "playersessions": {
                            switch (branch.toLowerCase()) {
                                case "": {
                                    //POST /v1/playerSessions   (create playerSession)
                                    responseBody = JSON.stringify(sessionmanagerResponses.post_playersession(response, request.headers, requestBodyJSON));
                                    responseCode = response.statusCode;
                                    break;
                                }
                                case "joinablespecifiedusers": {
                                    //POST /v1/playerSessions/{sessionId}/joinableSpecifiedUsers
                                    responseBody = JSON.stringify(sessionmanagerResponses.post_playersession_joinableusers(response, request.headers, requestBodyJSON, itemId));
                                    responseCode = response.statusCode;
                                    break;
                                }
                                case "member": {
                                    switch (postBranchItemId1) {
                                        case "players": {
                                            //POST /v1/playerSessions/{sessionId}/member/players
                                            responseBody = JSON.stringify(sessionmanagerResponses.post_playersession_player(response, request.headers, requestBodyJSON, itemId));
                                            responseCode = response.statusCode;
                                            break;
                                        }
                                        case "spectators": {
                                            //POST /v1/playerSessions/{sessionId}/member/spectators
                                            responseBody = JSON.stringify(sessionmanagerResponses.post_playersession_spectator(response, request.headers, requestBodyJSON, itemId));
                                            responseCode = response.statusCode;
                                            break;
                                        }
                                        default: {
                                            log.WARN("[PSN]unhandled POST " + module + "," + branch + " postBranchItemId1:" + postBranchItemId1);
                                            responseCode = 400;
                                        }
                                    }
                                    break;
                                }
                                case "sessionmessage": {
                                    //POST /v1/playerSessions/{sessionId}/sessionMessage
                                    responseCode = 501; //NOT_IMPL
                                    break;
                                }
                                case "invitations": {
                                    //POST /v1/playerSessions/{sessionId}/invitations
                                    responseCode = 501; //NOT_IMPL
                                    break;
                                }
                                default: {
                                    log.WARN("[PSN]unhandled POST " + module + ":" + branch);
                                    responseCode = 400;
                                }
                            }//switch branch
                            break;
                        }
                        default: {
                            log.WARN("[PSN]unhandled module for POST:" + module);
                            responseCode = 400;
                        }
                    }
                    break;
                }
                case "PATCH": {
                    switch (module.toLowerCase()) {
                        case "matches": {
                            // PATCH /v1/matches/{matchId}
                            responseBody = JSON.stringify(matchesResponses.patch_match(response, request.headers, requestBodyJSON, itemId));//itemId = {matchId}
                            responseCode = response.statusCode;
                            break;
                        }
                        case "playersessions": {
                            //future update: add rate limiting abil:
                            //// check if we should fake rate limiting for update call
                            //if ((mRateLimitUpdateOpCountThreshold !== 0) && (++mRateLimitUpdateOpCounter >= mRateLimitUpdateOpCountThreshold) &&
                            //    (sessionmanagerResponses.getPlayerSessPlayerCount(itemId) > 2)) {
                            //    // Side: to exercise Blaze's re-choose caller mechanism, we'll just fake rate limits when game member count > 2
                            //    log.LOG("[PSN]Fake update rate limit hit,Sess(" + itemId + "),user " + ((util.isUnspecifiedNested(request.headers, 'authorization')) ? "N/A" : request.headers.authorization));
                            //    responseCode = 429;//PSNSESSIONMANAGER_TOO_MANY_REQUESTS
                            //    mRateLimitUpdateOpCounter = 0;
                            //}
                            //// note rate limited
                            if (branch === "") {
                                //PATCH /v1/playersessions/{sessionId}
                                responseBody = sessionmanagerResponses.patch_session(response, request.headers, requestBodyJSON, itemId);//itemId = {sessionId}
                                responseCode = response.statusCode;
                            }
                            else if (branch === "members") {
                                //PATCH /v1/playerSessions/{sessionId}/members/{accountId}
                                responseBody = sessionmanagerResponses.patch_session_member_properties(response, request.headers, requestBodyJSON, itemId, postBranchItemId1);//itemId = {sessionId}, postBranchItemId = {accountId}
                                responseCode = response.statusCode;
                            }
                            break;
                        }
                        default: {
                            log.WARN("[PSN]unhandled module for PATCH:" + module);
                            responseCode = 400;
                        }
                    }
                    break;
                }
                case "PUT": {
                    switch (module.toLowerCase()) {
                        case "matches": {

                            switch (branch) {
                                case "status": {
                                    //PUT /v1/matches/{matchId}/status
                                    responseBody = JSON.stringify(matchesResponses.put_match_status(response, request.headers, requestBodyJSON, itemId));//itemId = {matchId}
                                    responseCode = response.statusCode;
                                    break;
                                }
                                default: {
                                    log.WARN("[PSN]unhandled PUT" + module + ":" + branch);
                                    responseCode = 400;
                                }
                            }
                            break;
                        }
                        case "playersessions": {
                            //future update: add rate limiting abil:
                            //// check if we should fake rate limiting for update call
                            //if ((mRateLimitUpdateOpCountThreshold !== 0) && (++mRateLimitUpdateOpCounter >= mRateLimitUpdateOpCountThreshold) &&
                            //    (sessionmanagerResponses.getGameMemberCount(itemId) > 2)) {
                            //    // Side: to exercise Blaze's re-choose caller mechanism, we'll just fake rate limits when game member count > 2
                            //    log.LOG("[PSN]Fake update rate limit hit,Sess(" + itemId + "),user " + ((util.isUnspecifiedNested(request.headers, 'authorization')) ? "N/A" : request.headers.authorization));
                            //    responseCode = 429;//PSNSESSIONMANAGER_TOO_MANY_REQUESTS
                            //    mRateLimitUpdateOpCounter = 0;
                            //}
                            //// note rate limited
                            //else
                            switch (branch) {
                                case "leader": {
                                    ///PUT /v1/playerSessions/{sessionId}/leader
                                    responseBody = sessionmanagerResponses.put_session_leader(response, request.headers, requestBodyJSON, itemId);//itemId = {sessionId}
                                    responseCode = response.statusCode;
                                    break;
                                }
                                default: {
                                    log.WARN("[PSN]unhandled PUT" + module + ":" + branch);
                                    responseCode = 400;
                                }
                            }
                            break;
                        }
                        default: {
                            log.WARN("[PSN]unhandled module for PUT:" + module);
                            responseCode = 400;
                        }
                    }
                    break;
                }
                case "DELETE": {
                    switch (module.toLowerCase()) {
                        //(There is no DELETE for matches currently)

                        case "playersessions": {
                            if (branch === "members") {
                                //DELETE /v1/playerSessions/{sessionId}/members/{accountId}
                                responseBody = sessionmanagerResponses.delete_playersession_member(response, request.headers, itemId, postBranchItemId1);//itemId = {sessionId}, postBranchItemId1={accountId}
                                responseCode = response.statusCode;
                            }
                            else if (branch.toLowerCase() === "joinablespecifiedusers") {
                                //DELETE /v1/playerSessions/{sessionId}/joinableSpecifiedUsers
                                responseBody = sessionmanagerResponses.delete_playersession_joinableusers(response, request.headers, itemId);//itemId = {sessionId}
                                responseCode = response.statusCode;
                            }
                            break;
                        }
                        default: {
                            log.WARN("[PSN]unhandled module for DELETE:" + module);
                            responseCode = 400;
                        }
                    }
                    break;
                }
                default: {
                    responseCode = 400;
                }
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



