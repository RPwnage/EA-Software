
var util = require("../common/responseutil");
var log = require("../common/logutil");

// Valid Sony match statuses
var PsnMatchStatus = {
    WAITING: 'WAITING',
    PLAYING: 'PLAYING',
    ONHOLD: 'ONHOLD',
    CANCELLED: 'CANCELLED',
    COMPLETED: 'COMPLETED',
    SCHEDULED: 'SCHEDULED'
};
// Valid Sony match grouping types
var PsnGroupingType = {
    NON_TEAM_MATCH: 'NON_TEAM_MATCH',
    TEAM_MATCH: 'TEAM_MATCH'
};
// Valid Sony match competitionTypes
var PsnCompetitionType = {
    COMPETITIVE: 'COMPETITIVE',
    COOPERATIVE: 'COOPERATIVE'
};
// Valid Sony match resultType
var PsnResultType = {
    RESULT: 'RESULT',
    SCORE: 'SCORE'
};
// Valid Sony match playerType
var PsnPlayerType = {
    PSN_PLAYER: 'PSN_PLAYER',
    NON_PSN_PLAYER: 'NON_PSN_PLAYER',
    NPC: 'NPC'
};

const DEFAULT_MATCH_EXPIRATION_TIME = 3600;
const DEFAULT_ACTIVITY_GROUPING_TYPE = PsnGroupingType.NON_TEAM_MATCH;
const DEFAULT_ACTIVITY_COMPETITION_TYPE = PsnCompetitionType.COMPETITIVE;
const DEFAULT_ACTIVITY_RESULT_TYPE = PsnResultType.RESULT;


// map holding active matches
var mMatchByIdMap = {};

// for next matchId
var mMatchIdCounter = 1;

// map holding configured activityIds (see configureActivities(activityIdsjson))
var mConfiguredActivityByIdMap = {};


// whether to log PSN specific info. for performance by default disabled
var mDoLog = false;

// Metrics
var mGuageMembers = 0;
var mRequestsIgnoredAlreadyJoined = 0;
var mRequestsIgnoredAlreadyLeft = 0;

// number of update ops before the next logged summary report
var mMaxOpCountBeforeNextReport = 1;
var mOpCountBeforeNextReport = 0;
function setMaxOpCountBeforeReport(val) { mMaxOpCountBeforeNextReport = val; }

function makeHttp400ErrRsp(reasonStr) {
    return { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269185, "message": "Bad request ('Mock PSN: " + reasonStr + "')" } };
}
function makeHttp500ErrRsp(reasonStr) {
    return { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2270080, "message": "Internal server error ('Mock PSN: " + reasonStr + "')" } };
}

const __HTTP404_ERR_RSP = { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269197, "message": "Resource not found (Mock PSN Service does not have specified Match resource)" } };

const __HTTP409_ERR_RSP = { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269321, "message": "Mock PSN: Fake Http 409 Conflict" } };
// If not 0, simulate PSN Match https://p.siedev.net/support/issue/8425/_Rare_http_409_calling_Match_Web_API known behavior.  I.e. PSN may return http 409 when 2 conseq calls are too close together (< the min ms):
var mHttp409matchms = 0;
var mHttp409matchLastTime = 0;
var mHttp409matchLastId = "";
function setHttp409matchms(val) { mHttp409matchms = val; }
function shouldSimMatchHttp409(response, matchId) {
    if (mHttp409matchms === 0) {
        return false;//disabled
    }
    var shouldSim = false;
    var currTime = Date.now();
    if (mHttp409matchLastId !== matchId) {
        mHttp409matchLastId = matchId;
    }
    else if ((currTime - mHttp409matchLastTime) < mHttp409matchms) {
        mDoLog && LOG("[Mat]FAKING HTTP 409:  on 2 conseq received reqs vs MatchId(" + mHttp409matchLastId + ") (" + (currTime - mHttp409matchLastTime) + ")ms apart.  Min allowed is (" + mHttp409matchms + ")ms");//http409matchms
        response.statusCode = 409;
        shouldSim = true;
    }
    mHttp409matchLastTime = currTime;
    return shouldSim;
}



function processSummaryReport(guageMembersChange) {
    try {
        // update guageMembersChange
        mGuageMembers += guageMembersChange;
        if (++mOpCountBeforeNextReport >= mMaxOpCountBeforeNextReport) {
            LOG("[Mat] SUMMARY REPORT:\n   Games/members: " + util.count(mMatchByIdMap) + "/" + mGuageMembers +
                "\n   ReqIgnored: AlreadyLeft:" + mRequestsIgnoredAlreadyLeft + ", AlreadyJoined:" + mRequestsIgnoredAlreadyJoined);
            mOpCountBeforeNextReport = 0;
        }
    } catch (err) {
        ERR("[Mat]processSummaryReport: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


function configureActivities(activityIdsjson) {
    try {
        const activityIdsBodyJSON = JSON.parse(activityIdsjson);
        for (var i in activityIdsBodyJSON) {
            //validate activityid data 1st
            if (!util.isString(i) || i.length === 0) {
                ERR("[Mat]configureActivities: invalid key in activityidsjson");
                return false;
            }
            var nextActivityIdData = activityIdsBodyJSON[i];
            if (util.count(nextActivityIdData) === 0) {
                ERR("[Mat]configureActivities: empty data for key("+ i +") in activityidsjson");
                return false;
            }
            for (var j in nextActivityIdData) {
                if (j === "category") {
                    if (nextActivityIdData[j] !== "competitive" && nextActivityIdData[j] !== "progress" && nextActivityIdData[j] !== "openended") {
                        ERR("[Mat]configureActivities: invalid (" + j + ") value(), for activity(" + i + ")");
                        return false;
                    }
                }
                else if (j === "isteam") {
                    if (nextActivityIdData[j] !== "true" && nextActivityIdData[j] !== "false") {
                        ERR("[Mat]configureActivities: invalid (" + j + ") value(), for activity(" + i + ")");
                        return false;
                    }
                }
                else if (j === "scorename") {
                    if (!util.isString(nextActivityIdData[j]) || nextActivityIdData[j].length === 0) {
                        ERR("[Mat]configureActivities: invalid (" + j + ") value(), for activity(" + i + ")");
                        return false;
                    }
                }
                else {
                    ERR("[Mat]configureActivities: invalid member for activity("+ i +")");
                    return false;
                }
            }
            //validated. add it:
            mConfiguredActivityByIdMap[i.toLowerCase()] = nextActivityIdData;
        }
        return true;
    } catch (err) {
        ERR("[Mat]configureActivities: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}
function getConfiguredActivity(activityId) {
    if (!util.isString(activityId) || !mConfiguredActivityByIdMap.hasOwnProperty(activityId.toLowerCase())) {
        mDoLog && WARN("[Mat]getGroupingTypeForConfiguredActivity: no such configured activityId:" + (util.isString(activityId) ? activityId : "<not-string>"));
        return null;
    }
    return mConfiguredActivityByIdMap[activityId.toLowerCase()];
}
function getGroupingTypeForConfiguredActivity(activityId) {
    var configuredActivity = getConfiguredActivity(activityId);
    if (util.isUnspecifiedOrNullNested(configuredActivity, 'isteam')) {
        mDoLog && WARN("[Mat]getGroupingTypeForConfiguredActivity: activityId(" + (util.isString(activityId) ? activityId : "<not-string>") + ") did not have an 'isteam' in psnserverps5 config. Returning default (" + DEFAULT_ACTIVITY_GROUPING_TYPE + ")");
        return DEFAULT_ACTIVITY_GROUPING_TYPE;//"NON_TEAM_MATCH";
    }
    return (((configuredActivity.isteam === true) || (configuredActivity.isteam === "true")) ? PsnGroupingType.TEAM_MATCH.toString() : PsnGroupingType.NON_TEAM_MATCH.toString());
}
function getCompetitionTypeForConfiguredActivity(activityId) {
    var configuredActivity = getConfiguredActivity(activityId);
    if (util.isUnspecifiedOrNullNested(configuredActivity, 'isteam')) {
        mDoLog && WARN("[Mat]getCompetitionTypeForConfiguredActivity: activityId(" + (util.isString(activityId) ? activityId : "<not-string>") + ") did not have an 'isteam' in psnserverps5 config. Returning default (" + DEFAULT_ACTIVITY_COMPETITION_TYPE + ")");
        return DEFAULT_ACTIVITY_COMPETITION_TYPE;//"COMPETITIVE";
    }
    return (configuredActivity.category === "competitive" ? PsnCompetitionType.COMPETITIVE.toString() : PsnCompetitionType.COOPERATIVE.toString());
}
function getResultTypeForConfiguredActivity(activityId) {
    var configuredActivity = getConfiguredActivity(activityId);
    if (util.isUnspecifiedOrNullNested(configuredActivity, 'scorename')) {
        return DEFAULT_ACTIVITY_RESULT_TYPE;//"RESULT";
    }
    return (!util.isUnspecifiedOrNull(configuredActivity.scorename) ? PsnResultType.SCORE.toString() : PsnResultType.RESULT.toString());
}






// \brief  GET {baseUrl}/v1/matches/{matchId}?npServiceLabel={#}
function get_match(response, matchId, npServiceLabel) {
    try {
        if (util.isUnspecified(matchId)) {
            ERR("[Mat]get_match: request missing matchId.");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req missing matchId");
        }
        //npServiceLabel maybe passed in by GET req
        //if (npServiceLabel !== null && !util.validateReqNpServiceLabel(response, npServiceLabel)) {//GEN5_TODO verify whether needed
        //    return "";//rsp set
        //}

        var mps = getMatchInternal(matchId);
        if (!util.isUnspecifiedOrNull(mps)) {
            response.statusCode = 200;
            // mock async updates before returning it, as needed.
            handleAutomaticUpdates(mps);
            return mps;
        }
        // not found
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[Mat]get_match: exception caught: " + err.message + ", stack: " + err.stack);
        return makeHttp500ErrRsp(err.message + ", stack: " + err.stack);
    }
}
// Returns match from internal map, or "" if not found.
function getMatchInternal(matchId) {
    try {
        if (util.isUnspecified(matchId)) {
            return "";
        }
        if (mMatchByIdMap.hasOwnProperty(matchId)) {
            return mMatchByIdMap[matchId];
        }
        return "";
    } catch (err) {
        ERR("[Mat]getMatchInternal: exception caught: " + err.message + ", stack: " + err.stack);
        return makeHttp500ErrRsp(err.message + ", stack: " + err.stack);
    }
}

// \brief  POST {baseUrl}/v1/matches .    Reference Blaze Server psnmatches.rpc's createMatch(). 
function post_match(response, requestHeaders, requestBodyJSON) {
    try {
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]post_match: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[Mat]post_match: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);
        //if (!util.validateReqNpServiceLabel(response, npServiceLabel)) {//GEN5_TODO verify whether needed
        //    return "";//rsp set
        //}
        mDoLog && LOG("[Mat]CREATE MATCH");

        //Poss enhacnement: These checks from docs are aribtrarily ordered currently,  test/confirm order of checks PSN actually does:
        if (!util.isString(requestBodyJSON.activityId)) {
            ERR("[Mat]post_match: failed to get valid activityId in request");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid activityId");
        }
        var expiryTimeIntS = (util.isUnspecified(requestBodyJSON.expirationTime) ? DEFAULT_MATCH_EXPIRATION_TIME : requestBodyJSON.expirationTime);
        if (!validateReqExpiryTime(response, expiryTimeIntS)) {
            return makeHttp400ErrRsp("req invalid expiryTime");//response.statusCode set
        }
        var zoneId = (util.isUnspecified(requestBodyJSON.zoneId) ? "" : requestBodyJSON.zoneId);//poss enhancement: validate len
        if (!util.isString(zoneId)) {
            ERR("[Mat]post_match: invalid zoneId in request");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid zoneId");
        }
        var groupingType = getGroupingTypeForConfiguredActivity(requestBodyJSON.activityId);
        if (groupingType === "") {
            ERR("[Mat]post_match: invalid activityId in request, groupingType not configured w/mock service. Configure using psnserverps5.cfg");
            response.statusCode = 400;//GEN5_TODO: PSN doesn't seem to fail currently, maybe bug on PSN side
            return makeHttp400ErrRsp("Mock PSN psnserverps5.cfg setup issue: groupingType not configured");
        }
        var competitionType = getCompetitionTypeForConfiguredActivity(requestBodyJSON.activityId);
        if (competitionType === "") {
            ERR("[Mat]post_match: invalid activityId in request, competitionType not configured w/mock service. Configure using psnserverps5.cfg");
            response.statusCode = 400;
            return makeHttp400ErrRsp("Mock PSN psnserverps5.cfg setup issue: competitionType not configured");
        }
        var resultType = getResultTypeForConfiguredActivity(requestBodyJSON.activityId);
        if (resultType === "") {
            ERR("[Mat]post_match: invalid activityId in request, resultType not configured w/mock service. Configure using psnserverps5.cfg");
            response.statusCode = 500;
            return makeHttp500ErrRsp("Mock PSN psnserverps5.cfg setup issue: resultType not configured");
        }

        //GEN5_TODO refactor common match validations of reqs into common helper (see above + patch_match, post_join_match, put_status etc)

        // validate req'splayers, teams, before committing anything
        if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster)) {
            if (!validateReqPlayersList(response, requestBodyJSON.inGameRoster.players, requestBodyJSON.inGameRoster.teams, null, false)) {
                ERR("[Mat]post_match: failed validating req players. Not creating match");
                return makeHttp400ErrRsp("req invalid players/state");//response.statusCode set
            }
            var valTeamErr = validateReqMatchTeams(response, requestBodyJSON.inGameRoster.teams, null, groupingType);
            if (valTeamErr !== "") {
                ERR("[Mat]post_match: failed validating inGameRoster teams. Not creating match");
                return makeHttp400ErrRsp(valTeamErr);//response.statusCode set
            }
        }

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        var matchId = mMatchIdCounter++;
        
        // Init new match, fill in defaulted fields based on PSN.
        //GEN5_TODO: confirm whether these timestamps even appear in rsps if not (yet) set. Maybe omittable here?: thot he docs state the getMatchDetail req always has em?
        //GEN5_TODO: audit/confirm all defaults below/above correct
        mps = {
            "matchId": matchId.toString(),
            "activityId": requestBodyJSON.activityId,
            "npServiceLabel": npServiceLabel,
            "zoneId": zoneId,
            "expirationTime": expiryTimeIntS,
            "matchStartTimestamp": "",
            "matchEndTimestamp": "",
            "lastPausedTimestamp": "",
            "status": PsnMatchStatus.WAITING,
            "groupingType": groupingType,
            "competitionType": competitionType,
            "resultType": resultType,
            "inGameRoster": {
                "players": [],
                "teams": []
            },
            "_internal": {//internal data. Not part of real PSN API
                "expiryCountdownTimestamp": -1 //-1 means unset
            }
        };
        // Init match's players, teams (pass false here since technically *optional* at start)
        if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster)) {
            if (!upsertPlayersToMatch(response, mps, requestBodyJSON.inGameRoster.players, false)) {
                ERR("[Mat]post_match: failed adding players");
                return makeHttp400ErrRsp("req invalid players/state");//response.statusCode set
            }
            if (!upsertTeamsToMatch(response, mps, requestBodyJSON.inGameRoster.teams, false)) {
                ERR("[Mat]post_match: failed adding teams");
                return makeHttp400ErrRsp("req invalid teams/state");//response.statusCode set
            }
        }

        // Store to owning map
        mMatchByIdMap[matchId] = mps;

        // this should kick off since we start in WAITING:
        if (mps.status === PsnMatchStatus.WAITING) {
            startExpiryCountdown(mps);
        }
        // we'll do this once up front in case (should be done tho in get_match() also)
        handleAutomaticUpdates(mps);

        // Successful create
        response.statusCode = 201;
        ++mOpCountBeforeNextReport;
        mDoLog && LOG("[Mat]CREATED MATCH " + matchId);
        return { "matchId" : mps.matchId };
    } catch (err) {
        ERR("[Mat]post_match: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// \brief  PATCH {baseUrl}/v1/matches/{matchId} .  update match details 
function patch_match(response, requestHeaders, requestBodyJSON, matchId) {
    try {
        if (shouldSimMatchHttp409(response, matchId)) {
            return __HTTP409_ERR_RSP;//response.statusCode set
        }

        mDoLog && LOG("[Mat]PATCH MATCH " + matchId);

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]patch_match: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[Mat]patch_match: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);

        var mps = get_match(response, matchId, npServiceLabel);
        if (util.isUnspecifiedOrNull(mps)) {
            mDoLog && WARN("[Mat]patch_match: can't update non-existent match");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        // Do this check *after* the get_match() which called handleAutomaticUpdates() above: PSN spec: If match status is already COMPLETED or CANCELLED, update attempts get error:
        if (!util.isUnspecifiedOrNull(mps.status) && ((mps.status === PsnMatchStatus.COMPLETED) || (mps.status === PsnMatchStatus.CANCELLED))) {
            mDoLog && WARN("[Mat]patch_match: can't update match already in status(" + mps.status + ")");
            response.statusCode = 400;
            return makeHttp400ErrRsp("already in status(" + mps.status + ")");
        }
        // validate req's players, teams updates, before committing anything
        if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster)) {

            if (!validateReqPlayersList(response, requestBodyJSON.inGameRoster.players, requestBodyJSON.inGameRoster.teams, null, false)) {
                ERR("[Mat]patch_match: failed validating req players. Not patching match:" + matchId);
                return makeHttp400ErrRsp("req invalid players/state");//response.statusCode set
            }
            var groupingType = getGroupingTypeForConfiguredActivity(mps.activityId);
            var valTeamErr = validateReqMatchTeams(response, requestBodyJSON.inGameRoster.teams, null, groupingType);
            if (valTeamErr !== "") {
                ERR("[Mat]patch_match: failed validating inGameRoster teams. Not patching match:" + matchId);
                return makeHttp400ErrRsp(valTeamErr);//response.statusCode set
            }
        }

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        //could maybe refactor validations here into helper reusable with POST match flow:
        //validate expirationTime
        if (!util.isUnspecifiedOrNull(requestBodyJSON.expirationTime) && (requestBodyJSON.expirationTime !== getExpiryTimeS(mps))) {
            mDoLog && LOG("[Mat].. expirationTime(" + mps.expirationTime + "->" + requestBodyJSON.expirationTime + ")");
            mps.expirationTime = requestBodyJSON.expirationTime;
            startExpiryCountdown(mps);// PSN spec: new countdown will begin
        }
        //zoneId
        if (!util.isUnspecifiedOrNull(requestBodyJSON.zoneId) && (mps.zoneId !== requestBodyJSON.zoneId)) {
            mDoLog && LOG("[Mat].. zoneId(" + mps.zoneId + "->" + requestBodyJSON.zoneId + ")");
            mps.zoneId = requestBodyJSON.zoneId.toString();
        }
        //status
        var statusErrRsp = upsertMatchStatus(response, mps, requestBodyJSON.status);
        if (statusErrRsp !== "") {
            ERR("[Mat]patch_match: " + statusErrRsp);
            return statusErrRsp;//response.statusCode set
        }

        //Commit inGameRoster
        if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster)) {

            //We need to pass isPatchReq true below to update joinFlags by PSN spec:
            //"If a player/team not previously included in the in -game roster is specified with inGameRoster, the player/team will now join the match.Meanwhile, if a player/team already included in the in -game roster is not specified with inGameRoster, the player/team will be removed from the match.
            //However, the removed player/team is not deleted from the in -game roster.The participation status(joinFlag) is merely switched, and the player/team remains in the in -game roster as part of its history."

            //inGameRoster.players
            if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster.players)) {

                // pass true for PATCH req to ensure marking all pre existing in match that aren't in req, as joinFlag=false
                if (!upsertPlayersToMatch(response, mps, requestBodyJSON.inGameRoster.players, true)) {
                    return makeHttp400ErrRsp("req invalid players/state");//response.statusCode set
                }
            }
            //inGameRoster.teams
            if (!util.isUnspecifiedOrNull(requestBodyJSON.inGameRoster.teams)) {

                // pass true for PATCH req to ensure marking all pre existing in match that aren't in req, as joinFlag=false
                if (!upsertTeamsToMatch(response, mps,requestBodyJSON.inGameRoster.teams, true)) {
                    return makeHttp400ErrRsp("req invalid teams/state");//response.statusCode set
                }
            }
            if (!assertMatchHasPlayersList(response, mps)) {//GEN5_TODO: technically might not be needed this check
                return makeHttp400ErrRsp("req invalid state of players list");//response.statusCode set
            }
        }

        ++mOpCountBeforeNextReport;
        response.statusCode = 204;
        mDoLog && LOG("[Mat]PATCHED MATCH " + matchId);
        return "";
    } catch (err) {
        ERR("[Mat]patch_match: Exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// \brief  PUT {baseUrl}/v1/matches/{matchId}/status .  Update match status 
function put_match_status(response, requestHeaders, requestBodyJSON, matchId) {
    try {
        if (shouldSimMatchHttp409(response, matchId)) {
            return __HTTP409_ERR_RSP;//response.statusCode set
        }

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]put_match_status: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[Mat]put_match_status: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("invalid req JSON body");
        }
        if (util.isUnspecifiedOrNull(requestBodyJSON.status)) {
            ERR("[Mat]put_match_status: invalid status in req to update match:" + matchId);
            response.statusCode = 400;
            return makeHttp400ErrRsp("invalid req status");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);

        var mps = get_match(response, matchId, npServiceLabel);
        if (util.isUnspecifiedOrNull(mps)) {
            mDoLog && WARN("[Mat]put_match_status: can't update non-existent match");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        // Do this check *after* the get_match() which called handleAutomaticUpdates() above: PSN spec: If match status is already COMPLETED or CANCELLED, update attempts get error:
        if (!util.isUnspecifiedOrNull(mps.status) && ((mps.status === PsnMatchStatus.COMPLETED) || (mps.status === PsnMatchStatus.CANCELLED))
            && (mps.status !== requestBodyJSON.status)) {
            mDoLog && WARN("[Mat]put_match_status: can't update match already in status(" + mps.status + ")");
            response.statusCode = 400;
            return makeHttp400ErrRsp("already in status(" + mps.status + ")");
        }

        if (mps.status === requestBodyJSON.status) {//GEN5_TODO: dcheck this is actually what PSN does too, it shouldn't say restart expiry timer below or anything?
            response.statusCode = 204;//ok, no change
            return "";
        }
        mDoLog && LOG("[Mat]PUT MATCH STATUS(" + mps.status + "->" + requestBodyJSON.status + ") " + matchId);


        var statusErrRsp = upsertMatchStatus(response, mps, requestBodyJSON.status);
        if (statusErrRsp !== "") {
            ERR("[Mat]put_match_status: " + statusErrRsp);
            return statusErrRsp;//response.statusCode set
        }


        mDoLog && LOG("[Mat]PUTTED MATCH STATUS(" + mps.status + "->" + requestBodyJSON.status + ") " + matchId);
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[Mat]put_match_status: Exception caught: " + err.message + ", stack: " + err.stack);
        return makeHttp500ErrRsp(err.message + ", stack: " + err.stack);
    }
}

function upsertMatchStatus(response, mps, requestStatus) {
    try {
        if (util.isUnspecifiedOrNull(requestStatus)) {
            return "";//req didn't specify, NOOP
        }
        if (util.isUnspecifiedOrNull(mps) || util.isUnspecifiedOrNullNested(response, 'statusCode')) {
            ERR("[Mat]upsertMatchStatus: internal err, match or response arg n/a");
            response.statusCode = 500;
            return makeHttp400ErrRsp("internal err, match or response arg n/a");
        }
        var matchId = mps.matchId;
        if (!validateReqMatchStatus(requestStatus)) {
            ERR("[Mat]upsertMatchStatus: invalid status in req to update match:" + matchId);
            response.statusCode = 400;
            return makeHttp400ErrRsp("invalid req status");
        }

        // PSN spec: If the status changes or expirationTime is updated, the expirationTime timer will be reset, and a new countdown will begin. Update below:
        switch (requestStatus) {
            case PsnMatchStatus.SCHEDULED: {
                //GEN5_TODO check valid transitions/errors returned by PSN, but I believe you can't for instance go to this after PLAYING
                clearExpiryCountdown(mps);
                break;
            }
            case PsnMatchStatus.WAITING: {
                //GEN5_TODO check valid transitions/errors returned by PSN, but I believe you can't for instance go to this after PLAYING
                startExpiryCountdown(mps);
                break;
            }
            case PsnMatchStatus.CANCELLED: {
                clearExpiryCountdown(mps);
                break;
            }
            case PsnMatchStatus.COMPLETED: {
                mps.matchEndTimestamp = Date.now().toString();
                clearExpiryCountdown(mps);
                break;
            }
            case PsnMatchStatus.ONHOLD: {
                mps.lastPausedTimestamp = Date.now().toString();
                startExpiryCountdown(mps);
                break;
            }
            case PsnMatchStatus.PLAYING: {
                // PSN spec: the following checks are performed:
                // At least one player must be in the inGameRoster. 
                // If the groupingType is TEAM_MATCH, at least one team must be in the inGameRoster.
                var pcount = getMatchPlayerCount(mps, true);
                if (pcount <= 0) {
                    ERR("[Mat]put_match_status: can't goto PLAYING: no players in roster");
                    response.statusCode = 400;
                    return makeHttp400ErrRsp("Unexpected state and poss Blaze issue: trying to goto PLAYING but NO players yet in Match?");
                }
                if (!util.isUnspecifiedOrNull(mps.groupingType) && (mps.groupingType === PsnGroupingType.TEAM_MATCH)) {
                    if ((util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams') || (mps.inGameRoster.teams.length === 0))) {
                        ERR("[Mat]put_match_status: can't goto PLAYING: teams empty");
                        response.statusCode = 500;
                        return makeHttp500ErrRsp("Likely title/test setup issue detected: NO Teams in this Teams-Activity Match! Was game properly setup with Teams?");
                    }
                }
                // PSN spec: matchStartTimestamp is recorded the first time the status of a match becomes PLAYING.//GEN5_TODO: docs state 'first' time, does PSN gaurd against that
                mps.matchStartTimestamp = Date.now().toString();
                clearExpiryCountdown(mps);
                break;
            }
            default: {
                ERR("[Mat]put_match_status: unhandled status(" + requestStatus + ")");
                response.statusCode = 400;
                return makeHttp400ErrRsp("unhandled/invalid Status");
            }

        }//switch (requestStatus)

        mps.status = requestStatus;
        mDoLog && LOG("[Mat].. (" + mps.status + "->" + requestStatus + ") " + matchId);
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[Mat]put_match_status: Exception caught: " + err.message + ", stack: " + err.stack);
        return makeHttp500ErrRsp(err.message + ", stack: " + err.stack);
    }
}



// \brief  POST /v1/matches/{matchId}/players/actions/add  . Reference Blaze Server psnmatches.rpc's joinMatch().
function post_join_match(response, requestHeaders, requestBodyJSON, matchId) {
    try {
        if (shouldSimMatchHttp409(response, matchId)) {
            return __HTTP409_ERR_RSP;//response.statusCode set
        }

        mDoLog && LOG("[Mat]JOIN MATCH " + matchId);

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]post_join_match: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[Mat]put_match_status: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON body");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);
      
        var mps = get_match(response, matchId, npServiceLabel);
        if (util.isUnspecifiedOrNull(mps)) {
            mDoLog && WARN("[Mat]post_join_match: can't update non-existent match");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        // Do this check *after* the get_match() which called handleAutomaticUpdates() above: PSN spec: If match status is already COMPLETED or CANCELLED, update attempts get error:
        if (!util.isUnspecifiedOrNull(mps.status) && ((mps.status === PsnMatchStatus.COMPLETED) || (mps.status === PsnMatchStatus.CANCELLED))) {
            mDoLog && WARN("[Mat]post_join_match: can't update match already in status(" + mps.status + ")");
            response.statusCode = 400;
            return makeHttp400ErrRsp("already in status(" + mps.status + ")");
        }
        
        // This join match request has a different format from others, instead of a 'teams' section, it includes a 'teamId' in the players objects. Use teamsListToCheckVsPlayers to validate below.
        var teamsListToCheckVsPlayers = (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams') ? null : mps.inGameRoster.teams);
        // validate req's players list exists, and is valid, before committing anything
        var reqPlayers = requestBodyJSON.players;
        if (!validateReqPlayersList(response, reqPlayers, teamsListToCheckVsPlayers, mps, true)) {
            ERR("[Mat]post_join_match: failed validating req players. Not joining match:"+ matchId);
            return makeHttp400ErrRsp("req invalid players/state");//rsp set
        }
        
        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        // join/update members. this does not update teams yet
        if (!upsertPlayersToMatch(response, mps, reqPlayers, false)) {
            return makeHttp400ErrRsp("req invalid players/state");//rsp set
        }

        // update teams

        // To simplify, build a local req to reuse 'upsertTeamsToMatch' helper below:
        var tempReqTeamsList = [];
        for (var i in reqPlayers) {
            if (util.isUnspecifiedOrNull(reqPlayers[i].teamId))
                continue;
            var tempTeam = findTeamInMatchTeamsList(tempReqTeamsList, reqPlayers[i].teamId);
            if (tempTeam === "") {
                tempTeam = { "teamId": reqPlayers[i].teamId, "members": [] };
                tempReqTeamsList.push(tempTeam);//add team
            }
            tempTeam.members.push({"playerId":reqPlayers[i].playerId});//add team member
        }
       
        if (!upsertTeamsToMatch(response, mps, tempReqTeamsList, false)) {
            return makeHttp400ErrRsp("req invalid teams/state");//rsp set
        }
        
        // Successful
        ++mOpCountBeforeNextReport;
        response.statusCode = 204;
        mDoLog && LOG("[Mat]JOINED MATCH " + matchId);
        return "";
    } catch (err) {
        ERR("[Mat]post_join_match: exception caught: " + err.message + ", stack: " + err.stack);
        return makeHttp500ErrRsp(err.message + ", stack: " + err.stack);
    }
}


// \brief  POST /v1/matches/{matchId}/players/actions/remove .  Reference Blaze Server psnmatches.rpc's leaveMatch().
function post_leave_match(response, requestHeaders, requestBodyJSON, matchId) {
    try {
        if (shouldSimMatchHttp409(response, matchId)) {
            return __HTTP409_ERR_RSP;//response.statusCode set
        }

        mDoLog && LOG("[Mat]LEAVE MATCH " + matchId);

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]post_leave_match: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecifiedOrNullNested(requestBodyJSON, 'players')) {
            ERR("[Mat]post_leave_match: failed to get request's valid JSON body's players list");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid players");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);
        
        var mps = get_match(response, matchId, npServiceLabel);
        if (util.isUnspecifiedOrNull(mps)) {
            mDoLog && WARN("[Mat]post_leave_match: can't update non-existent match");
            response.statusCode = 404;
            ++mRequestsIgnoredAlreadyLeft;
            return __HTTP404_ERR_RSP;
        }
        // Do this check *after* the get_match() which called handleAutomaticUpdates() above: PSN spec: If match status is already COMPLETED or CANCELLED, update attempts get error:
        if (!util.isUnspecifiedOrNull(mps.status) && ((mps.status === PsnMatchStatus.COMPLETED) || (mps.status === PsnMatchStatus.CANCELLED))) {
            mDoLog && WARN("[Mat]post_leave_match: can't update match already in status(" + mps.status + ")");
            response.statusCode = 400;
            return makeHttp400ErrRsp("already in status(" + mps.status + ")");
        }
        if (!assertMatchHasPlayersList(response, mps)) {
            return "";//rsp set
        }

        // remove
        for (var i in requestBodyJSON.players) {

            if (!util.isUnspecifiedOrNull(requestBodyJSON.players[i].playerId)) {

                var existingPlayer = findPlayerInMatch(mps, requestBodyJSON.players[i].playerId, null);
                if (!util.isUnspecifiedOrNull(existingPlayer) && existingPlayer.joinFlag !== false) {
                    existingPlayer.joinFlag = false;
                    processSummaryReport(-1);
                }
                else {
                    ++mRequestsIgnoredAlreadyLeft;
                }
                // mark joinFlag false in any of match's teams it was in
                ensureTeamJoinedFalseForPlayer(mps, requestBodyJSON.players[i].playerId);
            }
        }

        // Successful leave
        ++mOpCountBeforeNextReport;
        response.statusCode = 204;
        mDoLog && LOG("[Mat]LEAVED MATCH " + matchId);
        return "";
    } catch (err) {
        ERR("[Mat]post_leave_match: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}




// \brief  POST /v1/matches/{matchId}/results . Reference Blaze Server psnmatches.rpc's report results rpc
function post_match_results(response, requestHeaders, requestBodyJSON, matchId) {
    try {
        if (shouldSimMatchHttp409(response, matchId)) {
            return __HTTP409_ERR_RSP;//response.statusCode set
        }

        mDoLog && LOG("[Mat]POST MATCH RESULTS " + matchId);

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[Mat]post_match_results: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecifiedOrNullNested(requestBodyJSON, 'matchResults', 'version')) {
            ERR("[Mat]post_match_results: failed to get request JSON body or matchResults version");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON or matchResults version");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);

        var mps = get_match(response, matchId, npServiceLabel);
        if (util.isUnspecifiedOrNull(mps)) {
            mDoLog && WARN("[Mat]post_match_results: can't update non-existent match");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        if (util.isUnspecifiedOrNull(mps.status)) {
            ERR("[Mat]post_match_results: warning: unexpected state, match(" + matchId+") didn't yet have a 'status' when trying to submit results!");
            response.statusCode = 400;
            return makeHttp400ErrRsp("unexpected state, match has no 'status' when trying to submit results!");
        }
        else {
            if ((mps.status !== PsnMatchStatus.PLAYING) && (mps.status !== PsnMatchStatus.CANCELLED)) {
                mDoLog && WARN("[Mat]post_match_results: can't update match in status(" + mps.status + ")");
                response.statusCode = 400;
                return makeHttp400ErrRsp("already in status(" + mps.status + ")");
            }
            if (mps.status === PsnMatchStatus.CANCELLED) {// By PSN docd spec:  match will remain CANCELLED without changing.
                mDoLog && WARN("[Mat]post_match_results: can't update match in status(" + mps.status + ")");
                response.statusCode = 204;
                return "";
            }
        }

        var reqCompetitiveResult = requestBodyJSON.matchResults.competitiveResult;
        var reqCooperativeResult = requestBodyJSON.matchResults.cooperativeResult;
        if (util.isUnspecifiedOrNull(reqCompetitiveResult) && util.isUnspecifiedOrNull(reqCompetitiveResult)) {
            ERR("[Mat]post_match_results: Blaze err, req didn't specify any competitiveResult NOR cooperativeResult");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req missing competitiveResult/cooperativeResult");
        }
        // validate req matchResults
        if (mps.competitionType === PsnCompetitionType.COOPERATIVE) {
            if (!util.isUnspecifiedOrNull(reqCompetitiveResult)) {
                ERR("[Mat]post_match_results: activity configured to competitionType(" + mps.competitionType + "), but req has a competitiveResult");
                response.statusCode = 400;
                return makeHttp400ErrRsp("activity configured to competitionType(" + mps.competitionType + "), but req has a competitiveResult");
            }
            if (!util.isString(reqCooperativeResult)) {
                ERR("[Mat]post_match_results: req cooperativeResult not a string");
                response.statusCode = 400;
                return makeHttp400ErrRsp("req invalid cooperativeResult");
            }
            if (mps.hasOwnProperty('matchResults') && mps.matchResult.hasOwnProperty('cooperativeResult')) {
                ERR("[Mat]post_match_results: unexpected state/Blaze-req: cooperativeResult already submitted/exists for match:" + matchId);
                response.statusCode = 400;
                return makeHttp400ErrRsp("a cooperativeResult already exists/submitted");
            }
            if (reqCooperativeResult !== "SUCCESS" && reqCooperativeResult !== "UNFINISHED" && reqCooperativeResult !== "FAILED") {
                ERR("[Mat]post_match_results: req cooperativeResult(" + reqCooperativeResult + ") not one of the valid poss values");
                response.statusCode = 400;
                return makeHttp400ErrRsp("req invalid cooperativeResult");
            }
        }
        else {//else  (mps.competitionType === PsnCompetitionType.COMPETITIVE) 
            if (!util.isUnspecifiedOrNull(reqCooperativeResult)) {
                ERR("[Mat]post_match_results: activity configured to competitionType(" + mps.competitionType + "), but req has a cooperativeResult");
                response.statusCode = 400;
                return makeHttp400ErrRsp("activity configured to competitionType(" + mps.competitionType + "), but req has a cooperativeResult");
            }
            if (mps.hasOwnProperty('matchResults') && mps.matchResult.hasOwnProperty('competitiveResult')) {
                ERR("[Mat]post_match_results: unexpected state/Blaze-req: competitiveResult already submitted/exists for match:" + matchId);
                response.statusCode = 400;
                return makeHttp400ErrRsp("a competitiveResult already exists/submitted");
            }
            if (mps.groupingType === PsnGroupingType.TEAM_MATCH) {
                if (!util.isUnspecifiedOrNull(reqCompetitiveResult.playerResults)) {
                    ERR("[Mat]post_match_results: activity configured to groupingType(" + mps.groupingType + "), but req has a playerResults");
                    response.statusCode = 400;
                    return makeHttp400ErrRsp("req unexpected playerResults for groupingType");
                }
                if (!util.isArray(reqCompetitiveResult.teamResults)) {
                    ERR("[Mat]post_match_results: req teamResults format invalid");
                    response.statusCode = 400;
                    return makeHttp400ErrRsp("req invalid teamResults");
                }
                //GEN5_TODO: add more detailed validations based on Sony specs! For now just assuming rest of it valid!
            }
            else { // (mps.groupingType === NON_TEAM_MATCH)
                if (!util.isUnspecifiedOrNull(reqCompetitiveResult.teamResults)) {
                    ERR("[Mat]post_match_results: activity configured to groupingType(" + mps.groupingType + "), but req has a teamResults");
                    response.statusCode = 400;
                    return makeHttp400ErrRsp("req unexpected teamResults for groupingType");
                }
                if (!util.isArray(reqCompetitiveResult.playerResults)) {
                    ERR("[Mat]post_match_results: req playerResults format invalid");
                    response.statusCode = 400;
                    return makeHttp400ErrRsp("req invalid playerResults");
                }
                //GEN5_TODO: add more detailed validations based on Sony specs! For now just assuming rest of it valid!
            }
        }
        //GEN5_TODO: handle/validate matchStatistics updates once we implement the mid game report flows

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        mps['matchResults'] = requestBodyJSON.matchResults;
        mps.status = PsnMatchStatus.COMPLETED;//By PSN spec
        mDoLog && LOG("[Mat]POSTED MATCH RESULTS " + matchId);
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[Mat]post_match_results: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}




// helpers



// \brief Add, or update requested members to the match
// \param[in,out] mps pre-created match to join to. May include pre-existing members if updating. Summary report will be updated, based on orig vs end member counts etc.
// \return true on success, false on failure
function upsertPlayersToMatch(response, mps, reqPlayersList, isPatchReq) {
    try {
        if (util.isUnspecifiedOrNull(reqPlayersList)) {
            return true;
        }

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        var matchId = (util.isUnspecifiedOrNull(mps.matchId) ? "<ErrN/A>" : mps.matchId);

        if (!assertMatchHasPlayersList(response, mps)) {
            return false;
        }
        if (isPatchReq) {
            //For PATCH PSN marks all those missing in the req as joinFlag=false. Do that 1st then upsert to mark those in req as true.
            for (var p in mps.inGameRoster.players) {
                if (mps.inGameRoster.players[p].joinFlag === true) {
                    mps.inGameRoster.players[p].joinFlag = false;
                    processSummaryReport(-1);
                }
            }
        }

        for (var i in reqPlayersList) {

            var reqPlayerId = reqPlayersList[i].playerId;
            var reqAccountId = (util.isUnspecifiedOrNull(reqPlayersList[i].accountId) ? "" : reqPlayersList[i].accountId);
            var reqPlayerName = (util.isUnspecifiedOrNull(reqPlayersList[i].playerName) ? "" : reqPlayersList[i].playerName);

            var existingPlayer = findPlayerInMatch(mps, reqPlayerId, reqAccountId);
            if (existingPlayer === "") {

                //A. NEW PLAYER:
                mps.inGameRoster.players.push(
                    {
                        "playerId": reqPlayerId,
                        "accountId": reqAccountId,
                        "onlineId": reqAccountId,
                        "playerType": reqPlayersList[i].playerType,
                        "joinFlag": true,//all start as this
                        "playerName": reqPlayerName
                    }
                );
                mDoLog && LOG("[Mat]upsertPlayersToMatch: successfully added user(" + reqAccountId + ",'" + reqPlayerName + "') to mock match(" + matchId + ").");
                continue;
            }
            //B. EXISTING PLAYER:

            // below are expected updatable
            var didUpdate = false;
            if ((reqPlayerName !== undefined) && (reqPlayerName !== null) && (reqPlayerName !== existingPlayer.playerName)) {

                mDoLog && LOG("[Mat]upsertPlayersToMatch: playerName(" + (util.isUnspecifiedOrNull(existingPlayer.playerName) ? "<n/a>" : existingPlayer.playerName) + " -> " + reqPlayerName + "), for user(" + reqAccountId + " playerId=" + reqPlayerId + ") in (" + matchId + ")");
                existingPlayer.playerName = reqPlayerName;
                didUpdate = true;
            }
            if (util.isUnspecifiedOrNull(existingPlayer.joinFlag) || (existingPlayer.joinFlag === false)) {// anyone specified in request automatically would toggle joinFlag
                mDoLog && LOG("[Mat]upsertPlayersToMatch: joinFlag(false->true), for user(" + reqAccountId + " playerId=" + reqPlayerId + ") in (" + matchId + ")");
                existingPlayer.joinFlag = true;
                didUpdate = true;
            }

            if (!didUpdate) {
                // if not rejoining, either player is changing Teams, or, Blaze is doing redundant call here. To detect excess Blaze calls, log it in case
                mDoLog && LOG("[Mat]upsertPlayersToMatch: no need to re-add existing player(" + reqAccountId + " playerId=" + reqPlayerId + ") of(" + matchId + "). NOOP.");
                ++mRequestsIgnoredAlreadyJoined;
            }
            else
                processSummaryReport(1);

        }//for

        return true;
    } catch (err) {
        ERR("[Mat]upsertPlayersToMatch: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \brief Add, or update requested teams to the match
// \param[in,out] mps pre-created match to join to. May include pre-existing members if updating.
// \return true on success, false on failure
function upsertTeamsToMatch(response, mps, reqTeamsList, isPatchReq) {
    try {
        if (util.isUnspecifiedOrNull(reqTeamsList)) {
            mDoLog && LOG("[Mat]upsertTeamsToMatch: req had no teams, NOOP.");
            return true;
        }
        var matchId = (util.isUnspecifiedOrNull(mps.matchId) ? "<ErrN/A>" : mps.matchId);

        //sanity check: pre: this helper assumes the match record already has (at least an empty) inGameRoster.teams
        if (!assertMatchHasTeamsList(response, mps)) {
            mDoLog && WARN("[Mat]upsertTeamsToMatch: assert teams list failed for Match(" + matchId + ")");
            return false;
        }

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        if (isPatchReq) {

            //By PSN sepc, given the above req's inGameRoster was specified if here,
            //For PATCH PSN marks all those missing in the req as joinFlag=false.
            //Pre: ok to do this here up front given the entire req was pre-validated already, and shouldn't fail below
            if (!markAllTeamsMembersJoinedFalse(mps)) {
                mDoLog && WARN("[Mat]upsertTeamsToMatch: failed marking members joinFlag false");
                response.statusCode = 500;
                return false;
            }
        }

        for (var i in reqTeamsList) {

            var reqTeamId = reqTeamsList[i].teamId;
            var reqTeamName = reqTeamsList[i].teamName;
            var isReqTeamNameSpecified = !util.isUnspecifiedOrNull(reqTeamName);
            var existingTeam = findTeamInMatch(mps, reqTeamId);

            if (existingTeam === "") {
                //A. CREATE NEW TEAM:
                var newTeam = {
                    "teamId": reqTeamId,
                    "teamName": (isReqTeamNameSpecified ? reqTeamName : ""),
                    "members": []//upsertTeamMembers adds members list below
                };
                if (!upsertTeamMembers(response, mps, newTeam, reqTeamsList[i])) {
                    ERR("[Mat]upsertTeamsToMatch: failed adding members to new team(" + reqTeamId + ") in (" + matchId + ")");
                    return false;
                }
                mps.inGameRoster.teams.push(newTeam);

                mDoLog && LOG("[Mat]upsertTeamsToMatch: added team(" + reqTeamId + ") to match(" + matchId + ").");
                continue;
            }
            //B. UPDATE EXISTING TEAM

            //teamName
            if (isReqTeamNameSpecified && (reqTeamName !== existingTeam.teamName)) {

                mDoLog && LOG("[Mat]upsertTeamsToMatch: teamName(" + (util.isUnspecifiedOrNull(existingTeam.teamName) ? "<n/a>" : existingTeam.teamName) + " -> " + reqTeamName + "), for team(" + reqTeamId + ") in (" + matchId + ")");
                existingTeam.teamName = reqTeamName;
            }
            //members
            if (!util.isUnspecifiedOrNull(reqTeamsList[i].members)) {
                if (!upsertTeamMembers(response, mps, existingTeam, reqTeamsList[i])) {
                    ERR("[Mat]upsertTeamsToMatch: failed updating members in team(" + teamId + ") in (" + matchId + ")");
                    return false;
                }
            }

        }//for

        return true;
    } catch (err) {
        ERR("[Mat]upsertTeamsToMatch: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}


// \brief Add, or update requested members to the team.  
// \param[in,out] outTeam pre-created match to join members to teams. May include pre-existing members if updating.
// \param[in] reqTeam Request's team with members to try to join into outTeam
// \param[in,out] mps If a player switches teams, it will be marked joinFlag=false from the old one
// \return true on success, false on failure
function upsertTeamMembers(response, mps, outTeam, reqTeam) {
    try {
        
        //GEN5_TODO validate vs dupes. docs state cant have dupe playerids
        //GEN5_TODO can reqs change a playerid/accountid combo? does PSN error if you try to? PATCH roster vs POST join match behavioral diffs?

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        var reqMembers = (util.isUnspecifiedOrNull(reqTeam.members) ? null : reqTeam.members);
        if (reqMembers === null || reqMembers.length === 0) {
            return true;//noop
        }

        //sanity check
        if (util.isUnspecifiedOrNull(outTeam.teamId) || (typeof outTeam.teamId !== 'string') || (outTeam.teamId !== reqTeam.teamId)) {
            ERR("[Mat]upsertTeamMembers: invalid teamId(s): outTeam(" + (util.isUnspecifiedOrNull(outTeam.teamId) ? "<N/A>" : outTeam.teamId) +
                ") vs reqTeam(" + (util.isUnspecifiedOrNull(reqTeam.teamId) ? "<N/A>" : reqTeam.teamId) + ")");
            response.statusCode = 500;
            return false;
        }

        // if existing match doesn't have teams yet, init empty team array.
        if (util.isUnspecified(outTeam.members)) {
            outTeam.members = [];
        }

        for (var i in reqMembers) {

            var reqPlayerId = reqMembers[i].playerId;

            var existingMember = findTeamMember(outTeam, reqPlayerId);
            if (existingMember === "") {

                //A. NEW MEMBER (OF THIS TEAM)

                //must mark unjoined from any prior team in this match
                ensureTeamJoinedFalseForPlayer(mps, reqPlayerId);

                outTeam.members.push({
                    "playerId": reqPlayerId,
                    "joinFlag": true//all start as this
                });
                mDoLog && LOG("[Mat]upsertTeamMembers: added new playerId(" + reqPlayerId + ") to team(" + outTeam.teamId + ").");
            }
            else {

                //B. UPDATE EXISTING MEMBER (OF THIS TEAM)

                if (util.isUnspecifiedOrNull(existingMember.joinFlag) || (existingMember.joinFlag !== true)) {
                    // anyone specified in request automatically would toggle joinFlag
                    mDoLog && LOG("[Mat]upsertTeamMembers: joinFlag(false->true), for playerId(" + reqPlayerId + ") in (" + outTeam.teamId + ")");
                    //must mark unjoined from any prior team in this match
                    ensureTeamJoinedFalseForPlayer(mps, reqPlayerId);
                    existingMember.joinFlag = true;
                } else {
                    // didn't need to update anything
                    // if not rejoining, Blaze is doing redundant call here. PSN may not actually return/log error, but to detect excess Blaze calls, log it.
                    mDoLog && WARN("[Mat]upsertTeamMembers: redundant calls to upsert existing member(" + reqPlayerId + ") in team(" + outTeam.teamId + "). Is Blaze making excess calls?");
                }
            }
        }//for

        return true;
    } catch (err) {
        ERR("[Mat]upsertTeamMembers: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function ensureTeamJoinedFalseForPlayer(mps, playerId) {
    if (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams')) {
        mDoLog && LOG("[Mat]ensureTeamJoinedFalseForPlayer: match missing inGameRoster teams. NOOP");
        return;
    }
    for (var t in mps.inGameRoster.teams) {//could be made more efficient, perhaps by caching team ptr on player objs
        var existingMember = findTeamMember(mps.inGameRoster.teams[t], playerId);
        if (!util.isUnspecifiedOrNull(existingMember) &&
            (util.isUnspecifiedOrNull(existingMember.joinFlag) || (existingMember.joinFlag !== false))) {
            mDoLog && LOG("[Mat] .. team joinFlag(true->false), for playerId(" + playerId + ") in (" + mps.inGameRoster.teams[t].teamId + ")");
            existingMember.joinFlag = false;
        }
    }
}

function markAllTeamsMembersJoinedFalse(mps) {
    try {
        if (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams')) {
            mDoLog && LOG("[Mat]markAllTeamsMembersJoinedFalse: match missing inGameRoster teams. NOOP");
            return true;
        }
        //pre: mps's teams valid array. teams.members valid arrays also
        for (var i in mps.inGameRoster.teams) {
            //if (util.isUnspecifiedOrNull(team.members)){//shouldnt be poss
            //    continue;
            //}
            for (var j in mps.inGameRoster.teams[i].members) {
                mps.inGameRoster.teams[i].members[j].joinFlag = false;
            }
        }
        return true;
    } catch (err) {
        ERR("[Mat]markAllTeamsMembersJoinedFalse: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}


// mock PSN's async updates to the match before returning it, as needed
function handleAutomaticUpdates(mps) {
    try {
        if (!util.isUnspecifiedOrNull(mps)) {
            handleAutomaticExpiryTime(mps);
        }
    } catch (err) {
        ERR("[Mat]handleAutomaticUpdates: Exception caught: " + err.message + ", stack: " + err.stack);
    }
}


function handleAutomaticExpiryTime(mps) {
    try {
        var expiryTimeIntS = getExpiryTimeS(mps);
        if (isNaN(expiryTimeIntS) || (expiryTimeIntS === -1)) {
            return;//logged
        }
        var status = mps.status;
        if (!validateReqMatchStatus(status)) {
            ERR("[Mat]handleAutomaticExpiryTime: internal test bug: match status invalid. Can't handle expiry.");
            return;
        }

        if ((status === PsnMatchStatus.WAITING) || (status === PsnMatchStatus.ONHOLD)) {
            // by spec in the above status's there should already have been an expiry countdown started (and shouldn't be -1):
            if (util.isUnspecifiedOrNullNested(mps, '_internal', 'expiryCountdownTimestamp') || !validatePositiveTimeVal(mps._internal.expiryCountdownTimestamp, status)) {
                ERR("[Mat]handleAutomaticExpiryTime: internal error: match internal expiryCountdownTimestamp invalid. Can't handle expiry.");
                return;
            }
            var timeElapsed = (Date.now() - mps._internal.expiryCountdownTimestamp);
            if (!validatePositiveTimeVal(timeElapsed, "time elapsed since " + status)) {
                return;//logged
            }


            // handle expiry:
            //**** NOTE: 
            // As of 2019 Sony docs state PSN will later support automatic updating of match statuses based on the scheduled expiry time. Sample code below is disabled meantime:
            //if ((timeElapsed / 1000) > expiryTimeIntS) {
            //    mps.status = PsnMatchStatus.CANCELLED;
            //    mps._internal.expiryCountdownTimestamp = -1;//clear
            //}
        }
    } catch (err) {
        ERR("[Mat]handleAutomaticExpiryTime: Exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// \brief return the mps's scheduled expiry time, if present.
function getExpiryTimeS(mps) {
    var foundTimeInt = -1;
    if (util.isUnspecifiedOrNullNested(mps, 'expirationTime')) {
        foundTimeInt = mps.expirationTime;
    } else if (!util.isUnspecifiedOrNull(mps)) {
        mps.expirationTime = DEFAULT_MATCH_EXPIRATION_TIME;//To simulate PSN behavior, if the time is unspecified, default to this
        foundTimeInt = mps.expirationTime;
    }
    if (isNaN(foundTimeInt) || foundTimeInt === -1) {
        ERR("[Mat]getExpiryTimeS: invalid expiry time(" + foundTimeInt + ")");
    }
    return foundTimeInt;
}
// \brief (re)start the mps's scheduled expiry time
function startExpiryCountdown(mps) {
    if (!util.isUnspecifiedOrNullNested(mps, '_internal')) {
        mps._internal['expiryCountdownTimestamp'] = Date.now();
    }
}
function clearExpiryCountdown(mps) {
    if (!util.isUnspecifiedOrNullNested(mps, '_internal')) {
        mps._internal['expiryCountdownTimestamp'] = -1;
    }
}

////////// UTIL HELPERS



function findPlayerInMatch(mps, playerId, memberAccountId) {
    try {
        if (!assertMatchHasPlayersList(null, mps)) {
            return "";//logged
        }
        for (var i in mps.inGameRoster.players) {
            if (!util.isUnspecifiedOrNull(mps.inGameRoster.players[i].playerId) && (mps.inGameRoster.players[i].playerId === playerId)) {
                return mps.inGameRoster.players[i];
            }
            if (!util.isUnspecifiedOrNull(mps.inGameRoster.players[i].accountId) && (mps.inGameRoster.players[i].accountId === memberAccountId)) {
                return mps.inGameRoster.players[i];
            }
        }
        return "";
    } catch (err) {
        ERR("[Mat]findPlayerInMatch: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function findTeamInMatch(mps, teamId) {
    try {
        if (util.isUnspecifiedOrNullNested(mps, 'inGameRoster')) {
            mDoLog && LOG("[Mat]findTeamInMatch: match missing inGameRoster. NOOP");
            return "";
        }
        return findTeamInMatchTeamsList(mps.inGameRoster.teams, teamId);
    } catch (err) {
        ERR("[Mat]findTeamInMatch: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}
// return teamId's team in the list, or "" if not found
function findTeamInMatchTeamsList(teamsList, teamId) {
    try {
        if (util.isUnspecifiedOrNull(teamsList)) {
            mDoLog && LOG("[Mat]findTeamInMatchTeamsList: missing teams list in match/req. NOOP");
            return "";
        }
        if (!util.isArray(teamsList)) {
            ERR("[Mat]findTeamInMatchTeamsList: input teams list not array. Check mock test code/log.");
            return "";
        }
        for (var i in teamsList) {
            if (!util.isUnspecifiedOrNull(teamsList[i].teamId) && (teamsList[i].teamId === teamId)) {
                return teamsList[i];
            }
        }
        return "";
    } catch (err) {
        ERR("[Mat]findTeamInMatchTeamsList: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function findTeamMember(team, playerId) {
    try {
        if (util.isUnspecifiedOrNull(team.members)) {
            WARN("[Mat]findTeamMember: internal err team missing members array");
            return "";
        }
        for (var i in team.members) {
            var member = team.members[i];
            if (!util.isUnspecifiedOrNull(member.playerId) && (member.playerId === playerId)) {
                mDoLog && LOG("[Mat]findTeamMember: player(" + playerId + ") found in teamId("+ team.teamId + "), with joinFlag(" + member.joinFlag + ")");
                return member;
            }
        }
        return "";
    } catch (err) {
        ERR("[Mat]findTeamMember: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}



// util
function getMatchPlayerCount(mps, isErrIfNoPlayersList) {//Method may also be used for the mock system's rate limit mechanism in psnserverps5.js
    try {
        if (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'players') || !util.isArray(mps.inGameRoster.players)) {
            if (isErrIfNoPlayersList) {
                var matchId = (util.isUnspecifiedOrNull(mps.matchId) ? "<ErrN/A>" : mps.matchId);
                ERR("[Mat]getMatchPlayerCount: internal mock bug: match(" + matchId + ") missing valid inGameRoster.players. Fix test code.");
                return -1;
            }
            return 0;
        }
        return mps.inGameRoster.players.length;
    } catch (err) {
        ERR("[smgr]getMatchPlayerCount: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}

//Technically PSN docs state there could be no players list available on a match, but Blaze currently always creates with one, so just to simplify code so does our post_match().
//Call this so sanity check for Blaze, the match record has (at least an empty) inGameRoster.players.
function assertMatchHasPlayersList(response, mps) {
    var pcount = getMatchPlayerCount(mps, true);
    if (pcount === -1) {
        if (!util.isUnspecifiedOrNull(response))
            response.statusCode = 500;
        return false;
    }
    return true;
}
//Technically PSN docs state there could be no teams list available on a match, but we create with one (see post_match()), so this should hold:
function assertMatchHasTeamsList(response, mps) {
    if (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams')) {
        var matchId = (util.isUnspecifiedOrNull(mps.matchId) ? "<ErrN/A>" : mps.matchId);
        ERR("[Mat]assertMatchHasTeamsList: internal mock bug: match(" + matchId + ") missing inGameRoster.teams. Fix test code.");
        if (!util.isUnspecifiedOrNull(response))
            response.statusCode = 500;
        return false;
    }
    return true;
}

// \brief validate match scheduled expiry time, if present
function validateReqExpiryTime(response, foundTimeInt) {
    try {
        if (!validatePositiveTimeVal(foundTimeInt, "expiry time")) {
            response.statusCode = 400;
            return false;//logged
        }
        if (foundTimeInt < 60 || foundTimeInt > 2678400) {//PSN docs state these min, max values
            ERR("[Mat]validateReqExpiryTime: specified expiry time(" + foundTimeInt + ") is out of PSN accepted bounds");
            response.statusCode = 400;
            return false;
        }
        mDoLog && LOG("[Mat]validateReqExpiryTime:" + foundTimeInt + "s");
        return true;
    } catch (err) {
        ERR("[Mat]validateReqExpiryTime: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function validatePositiveTimeVal(foundTimeInt, logContextStr) {
    try {
        if (isNaN(foundTimeInt)) {
            ERR("[Mat]validatePositiveTimeVal: " + (!util.isUnspecifiedOrNull(logContextStr) ? (logContextStr+":") : "") + " time(" + foundTimeInt + ") is not a number");
            return false;
        }
        if (foundTimeInt < 0) {
            ERR("[Mat]validatePositiveTimeVal: " + (!util.isUnspecifiedOrNull(logContextStr) ? (logContextStr + ":") : "") + " time(" + foundTimeInt + ") invalid/negative");
            return false;
        }
        return true;
    } catch (err) {
        ERR("[Mat]validatePositiveTimeVal: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \brief validate match status if present
function validateReqMatchStatus(matchStatus) {
    try {
        if (util.isUnspecifiedOrNull(matchStatus)) {
            ERR("[Mat]validateReqMatchStatus: internal err invalid matchStatus(" + (util.isUnspecifiedOrNull(matchStatus) ? "<n/a>" : matchStatus) + ")");
            return false;
        }
        //ensure found in enum
        for (var x in PsnMatchStatus) {
            if (x.toString() === matchStatus) {
                return true;
            }
        }
        ERR("[Mat]validateReqMatchStatus: matchStatus(" + matchStatus + ") invalid");
        return false;
    } catch (err) {
        ERR("[Mat]validateReqMatchStatus: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \param[in] mps Existing match, for comparing req against (validate end counts won't be over limit etc). Pass null if n/a or to ignore
function validateReqPlayersList(response, reqPlayersList, teamsListToCheckVsPlayers, mps, isPlayersListRequired) {
    try {
        if (util.isUnspecifiedOrNull(reqPlayersList)) {
            if (isPlayersListRequired) {
                ERR("[Mat]validateReqPlayersList: request players missing");
                response.statusCode = 400;
                return false;
            }
            return true;
        }
        //PSN docd spec: Up to 100 players can be made to join at once. However, there will be an error if the combined number of currently participating, previously participating, and newly joining players exceeds 500.
        if (!util.isArray(reqPlayersList) || (reqPlayersList.length < 1 || reqPlayersList.length > 100)) {
            ERR("[Mat]validateReqInGameRoster: request players len must be in [1,100], actual:" + (!util.isArray(reqPlayersList) ? "<not-array>" : reqPlayersList.length));
            response.statusCode = 400;
            return false;
        }
        var existingPcount = getMatchPlayerCount(mps, false);
        if (reqPlayersList.length + existingPcount > 500) {
            ERR("[Mat]validateReqPlayersList: req players count(" + reqPlayersList.length + ") + existing player count(" + existingPcount + ") would exceed max(500)");
            response.statusCode = 400;
            return false;
        }
        //GEN5_TODO:validate also: PSN docd spec: cant have dupe playerids

        //validate each player
        for (var i in reqPlayersList) {
            if (!validateReqMatchPlayer(response, reqPlayersList[i], teamsListToCheckVsPlayers)) {
                return false;//rsp set
            }
        }

        return true;
    } catch (err) {
        ERR("[Mat]validateReqInGameRoster: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \brief validate match player member in req
function validateReqMatchPlayer(response, reqPlayer, teamsListToCheckVsPlayers) {
    try {
        //PSN docs state playerId must be present
        if (util.isUnspecifiedOrNull(reqPlayer) || util.isUnspecifiedOrNull(reqPlayer.playerId)) {
            ERR("[Mat]validateReqMatchPlayer: reqPlayer missing required playerId");
            response.statusCode = 400;
            return false;
        }
        var playerId = reqPlayer.playerId;
        if (!validateReqMatchPlayerType(reqPlayer)) {
            ERR("[Mat]validateReqMatchPlayer: reqPlayer(" + playerId + ") failed playerType validation");
            response.statusCode = 400;
            return false;
        }
        if (!util.isUnspecifiedOrNull(reqPlayer.teamId)) {
            if (!findTeamInMatchTeamsList(teamsListToCheckVsPlayers, reqPlayer.teamId)) {
                ERR("[Mat]validateReqMatchPlayer: reqPlayer(" + playerId + ") requested teamId(" + reqPlayer.teamId + ") must be in teams list");
                response.statusCode = 400;
                return false;
            }
        }

        //poss enhancement: return appropriate PSN err code if invalidly tried to change other constants
        //GEN5_TODO: can reqs change a playerid/accountid combo? does PSN error if you try to
        //GEN5_TODO: validate max len for playerId 64, playerName 32. Dcheck if Sony just clips it

        return true;
    } catch (err) {
        ERR("[Mat]validateReqMatchPlayer: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \brief validate match player member's playerType
function validateReqMatchPlayerType(reqPlayer) {
    try {
        //PSN docs state playerType must be present
        if (util.isUnspecifiedOrNull(reqPlayer) || util.isUnspecifiedOrNull(reqPlayer.playerType)) {
            ERR("[Mat]validateReqMatchPlayerType: reqPlayer missing required playerType");
            return false;
        }
        var playerType = reqPlayer.playerType;

        //ensure found in enum
        var validPlayerTypeVal = false;
        for (var x in PsnPlayerType) {
            if (x.toString() === playerType) {
                validPlayerTypeVal = true;
                break;
            }
        }
        if (!validPlayerTypeVal) {
            ERR("[Mat]validateReqMatchPlayerType: reqPlayer invalid playerType:" + playerType);
            return false;
        }
        //PSN docs state if its PSN player, it must have account id specified
        if ((playerType === PsnPlayerType.PSN_PLAYER) && util.isUnspecifiedOrNull(reqPlayer.accountId)) {
            ERR("[Mat]validateReqMatchPlayerType: reqPlayer PSN_PLAYER playerType missing required accountId");
            return false;
        }

        return true;
    } catch (err) {
        ERR("[Mat]validateReqMatchPlayerType: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// use this to validate req's teams updates (before committing anything after this)
// \param[in] reqTeamsList Request's inGameRoster teams field. The players field is cross checked vs the teams field.
// \param[in] mps Existing match, for comparing req against. Pass null if n/a
// \return Empty string on pass. Else error message
function validateReqMatchTeams(response, reqTeamsList, mps, groupingType) {
    try {
        if (util.isUnspecifiedOrNull(reqTeamsList)) {
            return "";
        }

        if (!util.isArray(reqTeamsList)) {
            ERR("[Mat]validateReqMatchTeams: invalid request teams, is not type array");
            response.statusCode = 400;
            return "invalid request teams";
        }
        if (reqTeamsList.length < 1 || reqTeamsList.length > 200) {
            ERR("[Mat]validateReqMatchTeams: invalid request teams array size(" + reqTeamsList.length +"), must be in [1,200]");
            response.statusCode = 400;
            return "invalid request teams array size";
        }
        // PSN docd spec: combining currently participating teams and teams left in the history, a maximum of 200 teams can be registered for a match.An error will occur if either of these maximum values is exceeded.
        var existingTeamCount = (util.isUnspecifiedOrNullNested(mps, 'inGameRoster', 'teams') ? 0 : mps.inGameRoster.teams.length);
        if (reqTeamsList.length + existingTeamCount > 200) {
            ERR("[Mat]validateReqMatchTeams: request teams array size(" + reqTeamsList.length + ") + existing team count(" + existingTeamCount + ") must be < 200");
            response.statusCode = 400;
            return "invalid request teams array size";
        }
        if (groupingType !== PsnGroupingType.TEAM_MATCH) {
            ERR("[Mat]validateReqMatchTeams: groupingType(" + groupingType + ") invalid when specifying teams");
            response.statusCode = 400;
            return "groupingType(" + groupingType + ") invalid when specifying teams";
        }
        for (var i in reqTeamsList) {
            if (!validateReqTeam(response, reqTeamsList[i])) {
                return "invalid req team";//rsp set
            }
        }
        //GEN5_TODO: A player can only belong to one team at a time.
        //GEN5_TODO: check for any needed additional missing validations

        return "";
    } catch (err) {
        ERR("[Mat]validateReqMatchTeams: Exception caught: " + err.message + ", stack: " + err.stack);
        return "Exception caught: " + err.message + ", stack: " + err.stack;
    }
}

// \brief checks lengths, types etc of all the requested team's root level params
function validateReqTeam(response, reqTeam) {
    try {
        if (util.isUnspecifiedOrNull(reqTeam)) {
            ERR("[Mat]validateReqTeam: null or empty team object");
            response.statusCode = 400;
            return false;
        }
        //teamId
        if (!util.isString(reqTeam.teamId) || (reqTeam.teamId.length < 1) || (reqTeam.teamId.length > 64)) {
            ERR("[Mat]validateReqTeam: invalid teamId(" + (util.isString(reqTeam.teamId) ? reqTeam.teamId : "<not-string>") + ") length (" + (util.isString(reqTeam.teamId) ? reqTeam.teamId.length : 0) + "), must be in [1,64]");
            response.statusCode = 400;
            return false;
        }
        //teamName (if present)
        if ((reqTeam.teamName !== undefined) && (reqTeam.teamName !== null)) {
            if (!util.isString(reqTeam.teamName) || (reqTeam.teamName.length < 1) || (reqTeam.teamName.length > 64)) {
                ERR("[Mat]validateReqTeam: invalid teamName(" + (util.isString(reqTeam.teamName) ? reqTeam.teamName : "<not-string>") + ") length (" + (util.isString(reqTeam.teamName) ? reqTeam.teamName.length : 0) + "), must be in [1,64]");
                response.statusCode = 400;
                return false;
            }
        }
        //members (if present)
        if ((reqTeam.members !== undefined) && (reqTeam.members !== null)) {
            if (!util.isArray(reqTeam.members) || (reqTeam.members.length < 1 || reqTeam.members.length > 500)) {
                ERR("[Mat]validateReqTeam: requested team members size(" + (!util.isArray(reqTeam.members) ? "<not-array>" : reqTeam.members.length) + ") must be in [1,500]");
                response.statusCode = 400;
                return false;
            }
        }
        //GEN5_TODO add a validateReqTeamMember()//checks lengths, types etc of all the requested team member's params
        //GEN5_TODO: PSN docd spec: Each teamId within the request must be unique.

        return true;
    } catch (err) {
        ERR("[Mat]validateReqTeam: Exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function LOG(str) { log.LOG(str); }
function ERR(str) { log.ERR(str); }
function WARN(str) { log.WARN(str); }
function setDoLogPSN(val) { mDoLog = val; }

module.exports.get_match = get_match;
module.exports.post_match = post_match;
module.exports.patch_match = patch_match;
module.exports.put_match_status = put_match_status;
module.exports.post_join_match = post_join_match;
module.exports.post_leave_match = post_leave_match;
module.exports.post_match_results = post_match_results;
module.exports.setDoLogPSN = setDoLogPSN;
module.exports.setMaxOpCountBeforeReport = setMaxOpCountBeforeReport;
module.exports.setHttp409matchms = setHttp409matchms;
module.exports.configureActivities = configureActivities;