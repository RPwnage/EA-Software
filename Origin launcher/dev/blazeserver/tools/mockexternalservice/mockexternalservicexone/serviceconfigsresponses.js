const ON_BEHALF_OF_USERS_HEADER = 'x-xbl-onbehalfof-users';

var util = require("../common/responseutil");
var log = require("../common/logutil");

// map holding active activity handle infos
var mActivityInfoByXuidMap = {};

// map holding active MultiplayerSessions
var mMpsByNameMapGame = {};
var mMpsByNameMapMm = {};
var mTeamMpsListByXuidMap = {};
var reqMembersList = {};

var CONST_MPSD_CONTRACT_VERSION = 107;
var CONST_SESSIONTEMPLATE_MAXMEMBERSCOUNT = 100;
var CONST_MAX_EXTERNALSESSIONACTIVITYHANDLEID_LEN = 39; // see Blaze Server's ExternalSessionTypes
// session templates no longer expected to contain constants like maxMembersCount
var CONST_GET_SESSION_TEMPLATE_RESPONSE = {
    "fixed": {},
    "contractVersion": CONST_MPSD_CONTRACT_VERSION
};

// Blaze Server's externalsessiontypes.h's enum of the same name
var ExternalSessionType = {
    EXTERNAL_SESSION_GAME : 0x1,
    EXTERNAL_SESSION_MATCHMAKING_SESSION : 0x2,
    EXTERNAL_SESSION_GAME_GROUP : 0x3,
    EXTERNAL_SESSION_MATCHMAKING_SCENARIO : 0x4
};

// whether to log MPSD specific info. for performance by default disabled
var mDoLog = false;

// Metrics
var mGuageMembersGame = 0;
var mGuageMembersMatchmaking = 0;
var mRequestsIgnoredAlreadyJoined = 0;
var mRequestsIgnoredAlreadyLeft = 0;

// number of update ops before the next logged summary report
var mMaxOpCountBeforeNextReport = 1;
var mOpCountBeforeNextReport = 0;
function setMaxOpCountBeforeReport(val) { mMaxOpCountBeforeNextReport = val; }

function processSummaryReport(guageMembersChange, externalSessionType) {
    try {
        // update guageMembersChange
        if ((guageMembersChange !== 0) && !util.isUnspecified(externalSessionType)) {
            switch (externalSessionType) {
                case ExternalSessionType.EXTERNAL_SESSION_MATCHMAKING_SESSION:
                case ExternalSessionType.EXTERNAL_SESSION_MATCHMAKING_SCENARIO:
                    mGuageMembersMatchmaking += guageMembersChange;
                    break;
                case ExternalSessionType.EXTERNAL_SESSION_GAME:
                case ExternalSessionType.EXTERNAL_SESSION_GAME_GROUP:
                    mGuageMembersGame += guageMembersChange;
                    break;
                case 0:
                    mDoLog && LOG("[MPSD]processSummaryReport: Internal test/mock service: mps unspecified external session type " + externalSessionType + ". This is possible value for team MPSes.");
                    break;
                default:
                    ERR("[MPSD]processSummaryReport: Internal test/mock service error: mps external session type " + externalSessionType + " unhandled! Using default map type game session.");
            }
        }
    
        if (++mOpCountBeforeNextReport >= mMaxOpCountBeforeNextReport) {
            LOG("[MPSD] SUMMARY REPORT:\n   Games/members: " + util.count(mMpsByNameMapGame) + "/" + mGuageMembersGame +
                "\n   MmSess/members:" + util.count(mMpsByNameMapMm) + "/" + mGuageMembersMatchmaking +
                "\n   Activities:" + util.count(mActivityInfoByXuidMap) +
                "\n   ReqIgnored: AlreadyLeft:" + mRequestsIgnoredAlreadyLeft + ", AlreadyJoined:" + mRequestsIgnoredAlreadyJoined);
            mOpCountBeforeNextReport = 0;
        }
    } catch (err) {
        ERR("[MPSD]processSummaryReport: exception caught: " + err.message + ", stack: " + err.stack);
    }
}



function sessions_get_sessiontemplate(templateName, response) {
    try {
        if (util.isUnspecified(templateName)) {
            response.statusCode = 404;
            return "";
        }
        response.statusCode = 200;
        return CONST_GET_SESSION_TEMPLATE_RESPONSE;
    } catch (err) {
        ERR("[MPSD]sessions_get_sessiontemplate: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function sessions_get_response(sessionName, requestHeaders, response) {
    try {
        var isReqCertBased = util.isUnspecified(requestHeaders.authorization);
        var isReqOnBehalfOfUsers = !util.isUnspecifiedOrNullNested(requestHeaders, ON_BEHALF_OF_USERS_HEADER);
        var mps = sessions_get_response_internal(sessionName, response);
        if (!util.isUnspecifiedOrNullNested(mps, 'members') && isLargeMps(mps.constants)) {
            return largeSessionRspCopy(mps, requestHeaders, isReqCertBased, isReqOnBehalfOfUsers, response);
        }
        return mps;
    } catch (err) {
        ERR("[MPSD]sessions_get_response: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function sessions_get_response_internal(sessionName, response) {
    try {
        if (util.isUnspecified(sessionName)) {
            ERR("[MPSD]sessions_get_response: unexpected internal state: request missing some session name.");
            if (!util.isUnspecifiedOrNull(response)) {
                response.statusCode = 400;
            }
            return "";
        }
        var mps = "";
        if (mMpsByNameMapMm.hasOwnProperty(sessionName)) {
            if (!util.isUnspecifiedOrNull(response)) {
                response.statusCode = 200;
            }
            mps = mMpsByNameMapMm[sessionName];
        }
        else if (mMpsByNameMapGame.hasOwnProperty(sessionName)) {
            if (!util.isUnspecifiedOrNull(response)) {
                response.statusCode = 200;
            }   
            mps = mMpsByNameMapGame[sessionName];
        }
        if (!util.isUnspecifiedOrNull(mps)) {
            // mock XBL's async updates to the MPS before returning it, as needed.
            handleAutomaticUpdates(mps);
            return mps;
        }
        // MPS not found, return appropriate code (see Blaze Server's XBLCLIENTSESSIONDIRECTORY_NO_CONTENT BlazeRpcError, and ExternalSessionUtilXboxOne::getExternalSession())
        if (!util.isUnspecifiedOrNull(response)) {
            response.statusCode = 204;
        }
        return "";
    } catch (err) {
        ERR("[MPSD]sessions_get_response: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function sessions_getsessionsxuid_response(xuidStr, response) {
    try {
        if (util.isUnspecified(xuidStr)) {
            response.statusCode = 404;
            return "";
        }
        response.statusCode = 200;

        // for efficiency, we only track team MPSes currently, for each user
        return getTeamMpsList(xuidStr);
    } catch (err) {
        ERR("[MPSD]sessions_getsessionsxuid_response: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function sessions_put_response(scid, templateName, sessionName, requestHeaders, requestBodyJSON, response, noCommit) {
    try {

        // store whether this was a certificate based call (Blaze Server's xblclientsessiondirectory rpcs), as opposed to xbl token based (Blaze Server's xblserviceconfigs.rpc's rpcs)
        var isReqCertBased = util.isUnspecified(requestHeaders.authorization);
        var isReqOnBehalfOfUsers = !util.isUnspecifiedOrNullNested(requestHeaders, ON_BEHALF_OF_USERS_HEADER);
        var hasReqBody = !util.isUnspecified(requestBodyJSON);
        var hasReqPropertyUpdates = (hasReqBody && !util.isUnspecified(requestBodyJSON.properties));
        var hasReqMemberPropUpdates = (hasReqBody && hasReqMemberPropertyUpdates(requestBodyJSON));
        var hasReqServersPropUpdates = (hasReqBody && hasReqServersPropertyUpdates(requestBodyJSON));
        var isTeam = isTournamentTeamMps(requestBodyJSON.servers, requestBodyJSON.constants);
        var origSize = 0;

        var reqMemberLen = (hasReqBody ? util.count(requestBodyJSON.members) : 0);
        // store whether req is leaving members (i.e. member[x] === null).
        var isLeaveRequest = false;
        if (reqMemberLen > 0) {
            for (var k in requestBodyJSON.members) {
                if (requestBodyJSON.members[k] === null) {
                    // Side: XBL/Blaze impln doesn't allow creating/joining + leaving in same call. Can assume if one member is leaving, all are.
                    isLeaveRequest = true;
                    break;
                }
            }
        }

        var mps = sessions_get_response_internal(sessionName, response);
        var mpsExisted = (util.isUnspecified(mps) ? false : true);
    
        // if req has no properties specified, its not an update call. If members also empty, call might be for an invalid join/leave - early out
        if (!hasReqPropertyUpdates && !hasReqMemberPropUpdates && !hasReqServersPropUpdates && (reqMemberLen === 0)) {
            mDoLog && WARN("[MPSD]ignoring unneeded request which specified no updates nor members to join/leave.");
            response.statusCode = 200;
            return "";
        }
        // if req is a simple update call, and MPS doesn't exist, no op. Note: we identify simple update calls by checking if req uses certificate
        if ((hasReqPropertyUpdates || hasReqServersPropUpdates) && (reqMemberLen === 0) && isReqCertBased && !mpsExisted) {
            mDoLog && LOG("[MPSD]ignoring unneeded request to update a non-existing session");
            response.statusCode = 200;
            return "";
        }

        var isLargeSessionRequest = (isLargeMps(requestBodyJSON.constants) || (mpsExisted && isLargeMps(mps.constants)));


        if (!mpsExisted && !isLeaveRequest) {
            // CREATING MPS
            mDoLog && ((noCommit === 'false') ? LOG("[MPSD]CREATING NEW MPS") : LOG("[MPSD]SIMULATE CREATING NEW MPS"));

            if (isReqCertBased && !isReqOnBehalfOfUsers) {
                ERR("[MPSD]XBL create request cannot be certificate based without xbl tokens");
                response.statusCode = 401;
                return "";
            }
            if (!hasReqBody || util.isUnspecified(requestBodyJSON.constants) ||
                (!isTeam && (util.isUnspecifiedNested(requestBodyJSON.constants, 'custom', 'externalSessionId')))) {
                // On tight edge cases, likely join to about-to-be-removed MPS. To mimic XBL, don't error out. To avoid issues in this script, we won't bother saving the MPS
                mDoLog && LOG("[MPSD]Warning: Likely join attempt to MPS(" + (util.isUnspecified(sessionName) ? "<sessionName missing>" : sessionName) + "), for already-removed Blaze game. Blaze will likely remove MPS soon, returning dummy create response. Orig request body: " + (hasReqBody ? JSON.stringify(requestBodyJSON) : "<missing>"));
                response.statusCode = 201;
                return "";
            }

            // init new MPS, and add creator
            mps = {
                "correlationId" : requestBodyJSON.constants.custom.externalSessionId,
                "constants": requestBodyJSON.constants,
                "properties": requestBodyJSON.properties,
                "servers": requestBodyJSON.servers,
                "members": {
                }
            };
            // XBL inits these defaults if req didn't specify:
            if (util.isUnspecifiedOrNullNested(mps.constants, 'system')) { mps.constants['system'] = {}; } //below fills:
            if (util.isUnspecifiedNested(mps.constants.system, 'maxMembersCount')) { mps.constants.system['maxMembersCount'] = CONST_SESSIONTEMPLATE_MAXMEMBERSCOUNT; }
            if (util.isUnspecifiedNested(mps.constants.system, 'capabilities')) { mps.constants.system['capabilities'] = {}; }
            if (util.isUnspecifiedNested(mps.constants.system.capabilities, 'large')) { mps.constants.system.capabilities['large'] = false; } //see isLargeMps()
            if (util.isUnspecifiedNested(mps.constants.system.capabilities, 'crossplay')) { mps.constants.system.capabilities['crossplay'] = false; }

            if (isTournamentGameMps(requestBodyJSON.servers, requestBodyJSON.constants) && !validateTournamentGameStartTime(mps, response)) {
                return "";//rsp status set
            }
            // init Arena arbitration.status etc, as needed
            handleAutomaticUpdates(mps);

            if (!addMembersToMultiplayerSession(scid, templateName, sessionName, requestHeaders, requestBodyJSON.members, mps, response, isLargeSessionRequest)) {
                return "";//rsp status set
            }

            // if not committing, don't add to persisted map
            if (noCommit === 'false') {
                if (isMatchmakingMps(mps)) {
                    mMpsByNameMapMm[sessionName] = mps;
                } else {
                    mMpsByNameMapGame[sessionName] = mps;
                }
            }

            // Successful create
            response.statusCode = 201;
            processSummaryReport(((noCommit === 'false') ? util.count(mps.members) : 0), (isTeam ? 0 : mps.constants.custom.externalSessionType));
        }
        else if (reqMemberLen > 0 && !isLeaveRequest) {
            // JOINING MPS

            mDoLog && LOG("[MPSD]JOINING AN MPS");
            // team MPSes currently untested for join restrictions
            if (!isTeam && !checkJoinRestrictions(requestHeaders, requestBodyJSON.members, mps, response)) {
                return "";//rsp status set
            }

            // if not committing, clone a copy for response and update that instead
            if (noCommit === 'true') {
                mps = JSON.parse(JSON.stringify(mps));
            }
            origSize = util.count(mps.members);
            if (!addMembersToMultiplayerSession(scid, templateName, sessionName, requestHeaders, requestBodyJSON.members, mps, response, isLargeSessionRequest)) {
                return "";
            }

            // Successful join
            response.statusCode = 200;
            processSummaryReport(((noCommit === 'false') ? util.count(mps.members) - origSize : 0), (isTeam ? 0 : mps.constants.custom.externalSessionType));
        }
        else if (isLeaveRequest) {
            // LEAVING MPS
            mDoLog && LOG("[MPSD]LEAVING AN MPS");

            // if this was a (late arriving) update-properties or leave attempt, for an already removed MPS, ignore call.
            if (!mpsExisted) {
                mDoLog && WARN("[MPSD]ignoring request to leave non-existent MPS.");
                ++mRequestsIgnoredAlreadyLeft;
                response.statusCode = 200;
                return "";
            }
            if (!isReqCertBased) {
                ERR("[MPSD]invalid non certificate based leave. XBL only allows PUT calls to be leaving users if call was certificate based. (Non - certificate based leaves use DELETE).");
                response.statusCode = 401;
                return "";
            }

            // cache external session type before potentially deleting
            var extSessType = (isTeam ? 0 : mps.constants.custom.externalSessionType);
            origSize = util.count(mps.members);
            var endCount = remMembersFromMultiplayerSession(sessionName, requestBodyJSON.members, mps, response, isLargeSessionRequest);

            // Successful leave
            response.statusCode = 200;
            mDoLog && LOG("[MPSD]Left " + (origSize - endCount) + " of req's user(s). " + (endCount === 0 ? " Session was empty, removed" : ""));
            processSummaryReport(endCount - origSize, extSessType);
        }
        // side: below is not an 'else if' since technically, XBL allows updating + join/leaving MPS in *same* call.
        if (mpsExisted && hasReqPropertyUpdates) {
            // UPDATING MPS

            mDoLog && ((noCommit === 'false') ? LOG("[MPSD]UPDATING AN MPS") : LOG("[MPSD]SIMULATE UPDATING AN MPS"));

            // currently Blaze only updates the below. (poss enhancement: return appropriate XBL err code if invalidly tried to change constants)
            for (var sysPropKey in requestBodyJSON.properties.system) {
                mps.properties.system[sysPropKey] = requestBodyJSON.properties.system[sysPropKey];
            }
        }
        if (mpsExisted && hasReqMemberPropUpdates) {
            // UPDATING MPS MEMBERS

            mDoLog && ((noCommit === 'false') ? LOG("[MPSD]UPDATING MPS MEMBERS") : LOG("[MPSD]SIMULATE UPDATING MPS MEMBERS"));
            var result = doReqMemberPropertyUpdates(requestHeaders, requestBodyJSON.members, mps, response, isLargeSessionRequest);
            if (result !== "") {
                return result;//rsp status set
            }
        }
        if (mpsExisted && hasReqServersPropUpdates) {
            // UPDATING MPS SERVERS

            mDoLog && ((noCommit === 'false') ? LOG("[MPSD]UPDATING MPS SERVERS") : LOG("[MPSD]SIMULATE UPDATING MPS SERVERS"));
            var reslt = doReqServersPropertyUpdates(requestBodyJSON, mps, response);
            if (reslt !== "") {
                return reslt;//rsp status set
            }
        }
    
        // note: due to MS behavior (see GOS-29065) if visibility was private, join restriction now always is 'none' (regardless of orig args). Fix it up here as needed.
        if (!util.isUnspecifiedNested(mps, 'constants', 'system') &&
            mps.constants.system.visibility === "private" && !util.isUnspecifiedNested(mps, 'properties', 'system')) {
            mps.properties.system.joinRestriction = "none";
        }

        if (isLargeSessionRequest && !util.isUnspecified(mps.members)) {
            return largeSessionRspCopy(mps, requestHeaders, isReqCertBased, isReqOnBehalfOfUsers, response);
        }
        return mps;
    } catch (err) {
        ERR("[MPSD]sessions_put_response: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// helpers

// \brief Adding requested members to the multiplayer session.
// \param[in,out] mps pre-created session to join to. Pre: includes any pre-existing members already.
function addMembersToMultiplayerSession(scid, templateName, sessionName, requestHeaders, reqMembers, mps, response, isLargeSessionRequest) {
    try {
        if (true === isLargeSessionRequest) {
            if (util.count(reqMembers) !== 1) {
                ERR("[MPSD]addMembersToMultiplayerSession: large session req should have 1 member, but had(" + util.count(reqMembers) + ").");
                response.statusCode = 403;
                return "";
            }
            if (!('me' in reqMembers)) {
                ERR("[MPSD]addMembersToMultiplayerSession: internal error, missing member \"me\" in request.");
                response.statusCode = 400;
                return "";
            }
            reqMembersList = {};
            reqMembersList['me'] = reqMembers['me'];
        }
        else {
            reqMembersList = reqMembers;
        }
        for (var reqMemberKey in reqMembersList) {

            var xuid = getXuidForMemberInPutRequest(requestHeaders, reqMembers, reqMemberKey, response);
            if (xuid === "") {
                ERR("[MPSD]addMembersToMultiplayerSession: xuid in request was bad. Can't add user");
                return false;//rsp status set
            }

            var existingMember = findMemberInMps(xuid, mps);
            if (existingMember !== "") {
                
                var isUpdateMemberReq = false;
                if (isLargeMps(mps.constants) && !util.isUnspecifiedOrNullNested(reqMembers[reqMemberKey], 'properties', 'system', 'groups')) { // update a Large MPS member's recent player group id
                    isUpdateMemberReq = true;
                    if (util.isUnspecifiedOrNull(existingMember, 'properties')) {
                        existingMember['properties'] = {};
                    }
                    if (util.isUnspecifiedOrNull(existingMember.properties, 'system')) {
                        existingMember.properties['system'] = {};
                    }
                    existingMember.properties.system['groups'] = reqMembers[reqMemberKey].properties.system.groups;
                    mDoLog && LOG("[MPSD]addMembersToMultiplayerSession: updated user(" + xuid + ")'s recent player group id to(" + JSON.stringify(reqMembers[reqMemberKey].properties.system.groups) + "), in MPS(" + sessionName + ").");
                }

                // if not updating, and not claiming a reservation, somehow Blaze is doing redundant call here. XBL would not actually return/log error, but to detect excess Blaze calls, log it
                if (!isUpdateMemberReq &&
                    ((reqMemberKey.substr(0, "reserve_".length) !== "reserve_") || existingMember.properties.system.active === true)) {
                    mDoLog && WARN("[MPSD]addMembersToMultiplayerSession: '" + xuid + "' was already member of '" + sessionName + "'. Is Blaze making excess calls?");
                    ++mRequestsIgnoredAlreadyJoined;
                }
                //poss enhancement: return appropriate XBL err code if invalidly tried to change constants
                continue;
            }

            //side: real XBL keeps an running 'next' index counter on MPS as the new member key, but we won't bother, just key by xuid for simplicity.
            mps.members[xuid] = reqMembers[reqMemberKey];
            mps.members[xuid].constants.system.xuid = xuid;//'me' may have omitted xuid in request. Copy it now to result.
            mps.members[xuid].arbitrationStatus = "joining";

            //if this is a team MPS track in user's list for the GET sessions by xuid calls
            if (isTournamentTeamMps(mps.servers, mps.constants)) {
                addToTeamMpsList(xuid, scid, templateName, sessionName);
            }

            mDoLog && LOG("[MPSD]addMembersToMultiplayerSession: successfully completed(or simulated) adding user(" + xuid + ") to mock MPS(" + sessionName + "), correlationId(" + mps.correlationId + "), team(" + (!util.isUnspecifiedNested(mps.members[xuid], 'constants', 'system', 'team') ? mps.members[xuid].constants.system.team : "<none>") + ").");
        }
        return true;
    } catch (err) {
        ERR("[MPSD]addMembersToMultiplayerSession: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function remMembersFromMultiplayerSession(sessionName, reqMembers, mps, response, isLargeSessionRequest) {
    try {
        if (true === isLargeSessionRequest) {
            if (util.count(reqMembers) !== 1) {
                ERR("[MPSD]remMembersFromMultiplayerSession: large session req should have 1 member, but had(" + util.count(reqMembers) + ").");
                response.statusCode = 403;
                return "";
            }
            // (poss slightly simpler refactor could just move the below checks into the for (var reqMemberKey in reqMembersList) loop below (including resolving of the 'xuid')):
            reqMembersList = {};
            let mePrefix = 'me_';
            for (let k in reqMembers) {
                if (!k.startsWith(mePrefix)) {
                    ERR("[MPSD]remMembersFromMultiplayerSession: member's key(" + k + ") should start with(" + mePrefix + "), for large session leave req.");
                    response.statusCode = 400;
                    return "";
                }
                let xuid = k.substr(mePrefix.length);
                reqMembersList[xuid] = reqMembers[k];
            }
        }
        else {
            reqMembersList = reqMembers;
        }
        for (var reqMemberKey in reqMembersList) {
            if (!mps.members.hasOwnProperty(reqMemberKey)) {
                mDoLog && WARN("[MPSD]remMembersFromMultiplayerSession: user with index key '" + reqMemberKey + "' was not in the MPS '" + sessionName + "', not removing the user.");
                ++mRequestsIgnoredAlreadyLeft;
                continue;
            }
            // upon leaving the session, if it was the user's primary, as XBL does, automatically clear user's primary activity
            handlesClearActivity(sessionName, mps.members[reqMemberKey].constants.system.xuid, mps);

            if (isTournamentTeamMps(mps.servers, mps.constants)) {
                removeFromTeamMpsList(mps.members[reqMemberKey].constants.system.xuid, sessionName);
            }        
            mDoLog && LOG("[MPSD]remMembersFromMultiplayerSession: leaving member " + mps.members[reqMemberKey].constants.system.xuid + ", reqKey: " + reqMemberKey + ", teamMps: " + (isTournamentTeamMps(mps.servers, mps.constants) ? "true" : "false"));
            delete mps.members[reqMemberKey];
        }
        // XBL behavior: if MPS is empty, delete it
        var endCount = util.count(mps.members);
        if (endCount === 0) {
            if (isMatchmakingMps(mps)) {
                delete mMpsByNameMapMm[sessionName];
            }
            else {
                delete mMpsByNameMapGame[sessionName];//mMpsByNameMapGame[sessionName] = null;//don't try setting null for GC, won't work and causes mem growth
            }
        }
        return endCount;
    } catch (err) {
        ERR("[MPSD]remMembersFromMultiplayerSession: Error exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}

function findMemberInMps(memberXuid, mps) {
    try {
        if (util.isUnspecifiedNested(mps, 'members'))
            return "";
        for (var k in mps.members) {
            if (!util.isUnspecifiedNested(mps.members[k], 'constants', 'system') && (mps.members[k].constants.system.xuid === memberXuid)) {
                return mps.members[k];
            }
        }
        return "";
    } catch (err) {
        ERR("[MPSD]findMemberInMps: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// helper to extract and validate user's xuid exists in PUT request. Returns XBL based errors as needed.
function getXuidForMemberInPutRequest(requestHeaders, reqMembers, reqMemberKey, response) {
    try {
        if (util.isUnspecified(reqMemberKey) || util.isUnspecified(reqMembers)) {
            ERR("[MPSD]getXuidForMemberInPutRequest: internal error, invalid/missing members in request. Unable to get xuid.");
            response.statusCode = 400;
            return "";
        }
        var isMeKey = (reqMemberKey === "me");
        var isReserveKey = (reqMemberKey.substr(0, "reserve_".length) === "reserve_");
        if (!isMeKey && !isReserveKey) {
            ERR("[MPSD]getXuidForMemberInPutRequest: Bad request:user attempting to join MPS, has an invalid member key '" + reqMemberKey + "'.");
            response.statusCode = 400;
            return "";
        }
        // validate 'me' members rely on auth token to identify the user. Pre: we stashed xuid in the xbl token in ExternalSessionUtilXon::getAuthToken().
        if (isMeKey && (util.isUnspecifiedNested(requestHeaders, 'authorization')) && util.isUnspecifiedOrNullNested(requestHeaders, ON_BEHALF_OF_USERS_HEADER)) {
            mDoLog && WARN("[MPSD]getXuidForMemberInPutRequest: user attempting to join MPS, as 'me' had missing/invalid 'auth token' header. Member key was '" + reqMemberKey + "', request headers '" + (!util.isUnspecified(requestHeaders) ? JSON.stringify(requestHeaders) : "<missing>") + "'.");
            response.statusCode = 401;
            return "";
        }
        // validate for 'reserve_' key, xuid is specified in the member system constants
        if (isReserveKey && (util.isUnspecifiedNested(reqMembers[reqMemberKey], 'constants', 'system', 'xuid'))) {
            mDoLog && WARN("[MPSD]getXuidForMemberInPutRequest: user attempting to join MPS, as reserved had missing/invalid xuid specified. Member key was '" + reqMemberKey + "', request members specified was '" + (!util.isUnspecified(memberRequest) ? JSON.stringify(memberRequest) : "<missing>") + "'.");
            response.statusCode = 400;
            return "";
        }
        var xuid = "";
        if (!isMeKey) {
            xuid = reqMembers[reqMemberKey].constants.system.xuid;
        }
        if (isMeKey && !util.isUnspecifiedOrNullNested(requestHeaders, 'authorization')) {
            xuid = requestHeaders.authorization;
        }
        if (isMeKey && !util.isUnspecifiedOrNullNested(requestHeaders, ON_BEHALF_OF_USERS_HEADER)) {
            //Only take first xuid. Don't handle multiple users in one request on mock, this can be changed in future if needed
            if (requestHeaders[ON_BEHALF_OF_USERS_HEADER].includes(",")) {
                response.statusCode = 403;
                return "";
            }
            xuid = requestHeaders[ON_BEHALF_OF_USERS_HEADER].split(";")[0];
            
        }
        // validate xuid isn't Blaze::INVALID_EXTERNAL_ID
        if (xuid === "0") {
            ERR("[MPSD]getXuidForMemberInPutRequest: xuid in request was INVALID");
            response.statusCode = 400;
            return "";
        }
        return xuid;
        
    } catch (err) {
        ERR("[MPSD]getXuidForMemberInPutRequest: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// Check 'join restrictions'. If join restrictions checks fail, returns false, and updates headers in the response. Reference BlazeServer's ExternalSessionUtilXboxOne::checkRestrictions and ExternalSessionUtilXboxOne::mpsJoinErrorToSpecificError
function checkJoinRestrictions(reqHeaders, reqMembers, mps, response) {
    try {
        // Blaze uses MS's bypass for non-join restriction checks, header deny-scope Multiplayer.Manage
        if (util.count(reqMembers) === 0 || !reqHeaders.hasOwnProperty("x-xbl-deny-scope") || reqHeaders["x-xbl-deny-scope"] !== "Multiplayer.Manage") {
            return true;
        }
        for (var reqMemberKey in reqMembers) {
            var xuid = getXuidForMemberInPutRequest(reqHeaders, reqMembers, reqMemberKey, response);
            if (xuid === "") {
                ERR("[MPSD]checkJoinRestrictions: xuid in request was bad");
                return false;
            }
            //if already member, no check done.
            if (findMemberInMps(xuid, mps) !== "")
                continue;
            //check visibility.
            if (mps.constants.system.visibility === "visible") {
                util.setResponseXblHeaders(response, "reason=not-open;", "");
                response.statusCode = 403;
                return false;
            }
            if (mps.constants.system.visibility === "private") {
                response.statusCode = 403;
                return false;
            }
            // check join restrictions
            if (mps.properties.system.joinRestriction === "local") {
                util.setResponseXblHeaders(response, "reason=not-local;", "");
                response.statusCode = 403;
                return false;
            }
            if (mps.properties.system.joinRestriction === "followed") {
                mDoLog && WARN("[MPSD]checkJoinRestrictions: Friends lists are not implemented for mock external service. Request to check restrictions for external session id '" + mps.constants.custom.externalSessionId + "' cannot be done on 'followed' join restriction.");
            }
            // check closed
            if (mps.properties.system.closed === true) {
                util.setResponseXblHeaders(response, "reason=closed;", "");
                response.statusCode = 403;
                return false;
            }
        }
        return true;
    } catch (err) {
        ERR("[MPSD]checkJoinRestrictions: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}



// Activities call handlers

function handles_post_setactivity(request, requestBodyJSON, response) {
    try {
        // side: not making a var sessionName here, for efficiency, to avoid copying strings
        var sessRef = (util.isUnspecifiedNested(requestBodyJSON, 'sessionRef') ? null : requestBodyJSON.sessionRef);
        if (sessRef === null) {
            ERR("[MPSD]handles_post_setactivity: internal error, missing expected session info or name in request. No op.");
            response.statusCode = 400;
            return "";
        }
        var mps = sessions_get_response(sessRef.name, request.headers, response);
        if (util.isUnspecified(mps) || (response.statusCode === 404)) {
            response.statusCode = 404;//NO_CONTENT
            return "";
        }
        if (isMatchmakingMps(mps)) {
            ERR("[MPSD]handles_post_setactivity: internal error, tried to set a MM session MPS as primary activity. No op.");
            response.statusCode = 200;
            return "";
        }

        //pre: xuid stashed in authorization heder by Blaze mock test code
        var callingMember = findMemberInMps(request.headers.authorization, mps);
        if (util.isUnspecified(callingMember)) {
            response.statusCode = 403;//ACCESS_FORBIDDEN
            return "";
        }

        var newActivityInfo = {
            "results" : [{
                    "id": sessionNameToActivityHandleId(sessRef.name),
                    "sessionRef": sessRef
                }]
        };
        mActivityInfoByXuidMap[request.headers.authorization] = newActivityInfo;
        processSummaryReport(0, mps.constants.custom.externalSessionType);

        mDoLog && LOG("[MPSD]handles_post_setactivity: successfully set activity (handle:'" + newActivityInfo.results[0].id + "', sessionName:'" + sessRef.name + "') for user( " + request.headers.authorization + ").");
        response.statusCode = 200;
        return { "id": newActivityInfo.results[0].id };
    } catch (err) {
        ERR("[MPSD]handles_post_setactivity: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function handles_post_getactivity(requestBodyJSON, response) {
    try {
        var xuidsLen = (util.isUnspecifiedNested(requestBodyJSON, 'owners', 'xuids') ? 0 : requestBodyJSON.owners.xuids.length);
        if (xuidsLen !== 1) {
            mDoLog && WARN("[MPSD]handles_post_getactivity: possible mock service or Blaze issue: expected request's owner xuids list to have 1 item, had " + xuidsLen + ". Request body's owner xuids list: " + ((!util.isUnspecified(requestBodyJSON) && !util.isUnspecified(requestBodyJSON.owners)) ? JSON.stringify(requestBodyJSON.owners) : "<missing>"));
            response.statusCode = 400;
            return "";
        }

        var activityResult = (mActivityInfoByXuidMap.hasOwnProperty(requestBodyJSON.owners.xuids[0]) ? mActivityInfoByXuidMap[requestBodyJSON.owners.xuids[0]] : null);
        if (activityResult === null) {
            activityResult = {
                "results": [{}]
            };
        }
        mDoLog && LOG("[MPSD]handles_post_getactivity: for user(" + requestBodyJSON.owners.xuids[0] + ") activity handle id was " + (activityResult === "" ? "not found" : JSON.stringify(activityResult)) + ".");
        response.statusCode = 200;
        return activityResult;
    } catch (err) {
        ERR("[MPSD]handles_post_getactivity: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// helpers

function handlesClearActivity(sessionName, mpsMemberXuid, mps) {
    try {
        var handleId = sessionNameToActivityHandleId(sessionName);
        var activityInfo = mActivityInfoByXuidMap.hasOwnProperty(mpsMemberXuid) ? mActivityInfoByXuidMap[mpsMemberXuid] : null;
        if (activityInfo === null) {
            // if not private and not a MM session external session type, log warning
            if (mps.constants.system.visibility!== "private" && !isMatchmakingMps(mps)) {
                mDoLog && LOG("[MPSD]handlesClearActivity: possible mock service/test issue, no activity handle map-entry found for user(xuid " + mpsMemberXuid + "). Not clearing any activity(" + handleId + ") for user.");
            }
            return;
        }    
        if (util.isUnspecifiedNested(activityInfo, 'results')) {
            ERR("[MPSD]handlesClearActivity: likely mock service error, no activity handle info found for user(xuid " + mpsMemberXuid + "). Not clearing any activity(" + handleId + ") for user.");
            return;
        }

        var activityList = activityInfo.results;
        if (activityList.length > 1) {
            mDoLog && WARN("[MPSD]handlesClearActivity: likely mock service error, expected at most 1 activity handle for user(xuid " + mpsMemberXuid + "), actual number was " + activityList.length + ".");
        }
        // remove user's activity
        for (var i in activityList) {
            if (activityList[i].id === handleId) {
                //note: to clean up on error, if there's > 1 in list (unexpected by MS spec), we'll remove all activities
                mDoLog && LOG("[MPSD]handlesClearActivity: removing found activity (handle: '" + handleId + "', sessionName: '" + sessionName + "') for user(xuid " + mpsMemberXuid + ").");
                activityList = [];
                break;
            }
        }
        if (activityList.length === 0) {
            delete mActivityInfoByXuidMap[mpsMemberXuid];//mActivityInfoByXuidMap[mpsMemberXuid] = null;//don't try setting null for GC, won't work and causes mem growth
            processSummaryReport(0, mps.constants.custom.externalSessionType);
        }
        else {
            mDoLog && LOG("[MPSD]handlesClearActivity: activity handle '" + handleId + "' (for sessionName: '" + sessionName + "') for user(" + mpsMemberXuid + ") was not its primary, not cleared.");
        }
    } catch (err) {
        ERR("[MPSD]handlesClearActivity: Error exception caught: " + err.message + ", stack: " + err.stack);
    }
}

function sessionNameToActivityHandleId(sessionName) {
    // don't use more than the max length for a handle. Take tail of max length
    return sessionName.substring((sessionName.length - CONST_MAX_EXTERNALSESSIONACTIVITYHANDLEID_LEN + 1), sessionName.length);
}

function isMatchmakingMps(mps) {
    try {
        return !util.isUnspecifiedNested(mps, 'constants', 'custom', 'externalSessionType') &&
            ((mps.constants.custom.externalSessionType === ExternalSessionType.EXTERNAL_SESSION_MATCHMAKING_SESSION) ||
             (mps.constants.custom.externalSessionType === ExternalSessionType.EXTERNAL_SESSION_MATCHMAKING_SCENARIO));
    } catch (err) {
        ERR("[MPSD]isMatchmakingMps: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function isLargeMps(mpsConstantsJson) {
    try {
        return (!util.isUnspecifiedNested(mpsConstantsJson, 'system', 'capabilities', 'large') && (mpsConstantsJson.system.capabilities.large === true));
    } catch (err) {
        ERR("[MPSD]isLargeMps: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}


// Tournament / Arena

function isTournamentGameMps(mpsServersJson, mpsConstantsJson) {
    try {
        return isTournamentIdSpecified(mpsServersJson) && !util.isUnspecifiedNested(mpsConstantsJson, 'system', 'capabilities') &&
            (mpsConstantsJson.system.capabilities.arbitration === true);
    } catch (err) {
        ERR("[MPSD]isTournamentGameMps: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function isTournamentTeamMps(mpsServersJson, mpsConstantsJson) {
    try {
        return isTournamentIdSpecified(mpsServersJson) &&
            !util.isUnspecifiedNested(mpsConstantsJson, 'system', 'capabilities') && (mpsConstantsJson.system.capabilities.team === true);

    } catch (err) {
        ERR("[MPSD]isTournamentTeamMps: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function isTournamentIdSpecified(mpsServersJson) {
    try {
        return !util.isUnspecifiedNested(mpsServersJson, 'tournaments', 'constants', 'system', 'tournamentRef', 'tournamentId');
    } catch (err) {
        ERR("[MPSD]isTournamentIdSpecified: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// Get Tournament Team Multiplayer Sessions for user
function getTeamMpsList(xuid) {
    try {
        var userTeamList = (mTeamMpsListByXuidMap.hasOwnProperty(xuid) ? mTeamMpsListByXuidMap[xuid] : null);
        userTeamList = (userTeamList === null ? "" : userTeamList);
        mDoLog && LOG("[MPSD]getTeamMpsList: for user(" + xuid + "): " + (userTeamList === "" ? "not found" : JSON.stringify(userTeamList)) + ".");
        return userTeamList;
    } catch (err) {
        ERR("[MPSD]getTeamMpsList: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function addToTeamMpsList(xuid, scid, templateName, sessionName) {
    try {
        var userTeamList = getTeamMpsList(xuid);
        if (util.isUnspecifiedOrNullNested(userTeamList, 'results')) {
            var newUserTeamList = { "results": [{ "sessionRef": { "scid": scid, "templateName": templateName, "name": sessionName } }] };
            mTeamMpsListByXuidMap[xuid] = newUserTeamList;
        }
        else {
            userTeamList.results.push({ "sessionRef": { "scid": scid, "templateName": templateName, "name": sessionName } });
        }
        mDoLog && LOG("[MPSD]addToTeamMpsList: added team(" + sessionName + ") for user( " + xuid + "): " + JSON.stringify(getTeamMpsList(xuid)));
    } catch (err) {
        ERR("[MPSD]addToTeamMpsList: Error exception caught: " + err.message + ", stack: " + err.stack);
    }
}

function removeFromTeamMpsList(xuid, sessionName) {
    try {
        var userTeamList = getTeamMpsList(xuid);
        if (!util.isUnspecifiedOrNullNested(userTeamList, 'results')) {
            for (var i = 0, len = userTeamList.results.length; i < len; ++i) {
                if (!util.isUnspecifiedNested(userTeamList.results[i], 'sessionRef', 'name') && (userTeamList.results[i].sessionRef.name === sessionName)) {
                    userTeamList.results.splice(i, 1);
                    mDoLog && LOG("[MPSD]removeFromTeamMpsList: removed team(" + sessionName + ") for user( " + xuid + ")");
                    return;
                }
            }
        }
        mDoLog && LOG("[MPSD]removeFromTeamMpsList: NO team(" + sessionName + ") found to remove for user( " + xuid + ").");
    } catch (err) {
        ERR("[MPSD]removeFromTeamMpsList: Error exception caught: " + err.message + ", stack: " + err.stack);
    }
}

function hasReqMemberPropertyUpdates(requestBodyJSON) {
    try {
        if (util.isUnspecified(requestBodyJSON.members))
            return false;
        for (var k in requestBodyJSON.members) {

            var reqNextMember = requestBodyJSON.members[k];

            // check updatable properties for Blaze titles:
            var hasArbitrationUpdate = !util.isUnspecifiedOrNullNested(reqNextMember, 'properties', 'system', 'arbitration');
            if (hasArbitrationUpdate) {
                return true;
            }
        }
        return false;
    } catch (err) {
        ERR("[MPSD]hasReqMemberPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

const BASIC_COMPLETED_MPS_SERVERS_ARBITRATION_PROPERTIES = {
        "system": {
            "resultSource": "arbitration",
            "resultConfidenceLevel": 100,
            "resultState": "completed",
            "results": {}
        }
};

// Pre: hasReqMemberPropertyUpdates() returned true. Returns a non-empty debug string for response, if there was an error (like XBL does)
function doReqMemberPropertyUpdates(reqHeaders, reqMembers, mps, response, isLargeSessionRequest) {
    try {
        if (true === isLargeSessionRequest) {
            if (util.count(reqMembers) !== 1) {
                ERR("[MPSD]doReqMemberPropertyUpdates: large session req should have 1 member, but had(" + util.count(reqMembers) + ").");
                response.statusCode = 403;
                return "";
            }
            if (!("me" in reqMembers)) {
                ERR("[MPSD]doReqMemberPropertyUpdates: internal error, missing member \"me\" in request.");
                response.statusCode = 400;
                return "";
            }
            reqMembersList = {};
            reqMembersList["me"] = reqMembers["me"];
        }
        else {
            reqMembersList = reqMembers;
        }
        for (var k in reqMembersList) {

            var reqNextMember = reqMembers[k];
            if (util.isUnspecifiedOrNull(reqNextMember, 'properties')) {
                continue;
            }

            var xuid = getXuidForMemberInPutRequest(reqHeaders, reqMembers, k, response);
            if (xuid === "") {
                ERR("[MPSD]doReqMemberPropertyUpdates: xuid in request was bad. Can't update user");
                return "[MPSD]doReqMemberPropertyUpdates: xuid in request was bad. Can't update user";//rsp status set
            }
            if (util.isUnspecifiedOrNull(findMemberInMps(xuid, mps))) {
                // for Blaze, just to detect issues:
                mDoLog && WARN("[MPSD]doReqMemberPropertyUpdates: '" + xuid + "'  ' not MPS member, shouldn't update");
                response.statusCode = 400;
                return "[MPSD]doReqMemberPropertyUpdates: '" + xuid + "'  ' not MPS member, shouldn't update";
            }

            // add member.properties as needed
            if (util.isUnspecified(mps.members[xuid].properties)) {
                mps.members[xuid].properties = reqNextMember.properties;
            }
            // add member.properties.system as needed
            else if (util.isUnspecified(mps.members[xuid].properties.system) && !util.isUnspecifiedNested(reqNextMember.properties, 'system')) {
                mps.members[xuid].properties.system = reqNextMember.properties.system;
            }
            // add member.properties.system.arbitration as needed
            else if (!util.isUnspecifiedNested(reqNextMember.properties.system, 'arbitration')) {
                if (util.isUnspecified(mps.members[xuid].properties.system.arbitration)) {
                    mps.members[xuid].properties.system.arbitration = reqNextMember.properties.system.arbitration;
                }
                else if (util.isUnspecified(mps.members[xuid].properties.system.arbitration.results)) {
                    mps.members[xuid].properties.system.arbitration.status = reqNextMember.properties.system.arbitration.results;
                }
            }

            // simplified/efficient simulation: assuming results in, set all to complete:
            if (!util.isUnspecifiedNested(mps.members[xuid], 'properties', 'system', 'arbitration', 'results')) {
                mps.arbitration = { "status": "complete" };
                mps.members[xuid].arbitrationStatus = "complete";

                if (util.isUnspecified(mps.servers))
                    mps.servers = {};
                if (util.isUnspecified(mps.servers.arbitration))
                    mps.servers.arbitration = {};
                if (util.isUnspecified(mps.servers.arbitration.properties))
                    mps.servers.arbitration.properties = BASIC_COMPLETED_MPS_SERVERS_ARBITRATION_PROPERTIES;
                //unneeded by Blaze, can omit for eff:
                //mps.servers.arbitration.properties.system.results = mps.members[xuid].properties.system.arbitration.results;
            }
            mDoLog && LOG("[MPSD]doReqMemberPropertyUpdates: successfully completed update of user '" + xuid + "' properties. MPS arbitration.status(" + (util.isUnspecifiedOrNullNested(mps, 'arbitration', 'status') ? "<N/A>" : mps.arbitration.status) + ")");
        }
        return "";
    } catch (err) {
        ERR("[MPSD]hasReqMemberPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "[MPSD]hasReqMemberPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack;
    }
}



function hasReqServersPropertyUpdates(requestBodyJSON) {
    try {
        // check updatable properties for Blaze titles:
        if (!util.isUnspecifiedNested(requestBodyJSON, 'servers', 'tournaments', 'properties'))
            return true;
        if (!util.isUnspecifiedNested(requestBodyJSON, 'servers', 'arbitration', 'properties'))
            return true;
        return false;
    } catch (err) {
        ERR("[MPSD]hasReqServersPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}
// Pre: hasReqServersPropertyUpdates() returned true. Returns a non-empty debug string for response, if there was an error (like XBL does)
// \return response body error message if there was error. empty string otherwise
function doReqServersPropertyUpdates(requestBodyJSON, mps, response) {
    try {
        if (util.isUnspecified(mps.servers)) {
            // Poss future enhancement: return XBL err if try to update constants
            mps.servers = requestBodyJSON.servers;
            return "";
        }
        // update servers.tournaments.properties*
        if (!util.isUnspecifiedNested(requestBodyJSON, 'servers', 'tournaments', 'properties')) {
            if (util.isUnspecified(mps.servers.tournaments)) {
                mps.servers.tournaments = requestBodyJSON.servers.tournaments;
            }
            else if (util.isUnspecified(mps.servers.tournaments.properties)) {
                mps.servers.tournaments.properties = requestBodyJSON.servers.tournaments.properties;
            }
            else if (util.isUnspecified(mps.servers.tournaments.properties.system)) {
                mps.servers.tournaments.properties.system = requestBodyJSON.servers.tournaments.properties.system;
            }
            else {
                // Poss future enhancement: update each properties.system entry's sub-members below individually (currently assuming all or nothing)
                for (var sysPropKey in requestBodyJSON.servers.tournaments.properties.system) {
                    mps.servers.tournaments.properties.system[sysPropKey] = requestBodyJSON.servers.tournaments.properties.system[sysPropKey];
                }
            }
        }
        // update servers.arbitration.properties*
        if (!util.isUnspecifiedNested(requestBodyJSON, 'servers', 'arbitration', 'properties')) {
            if (util.isUnspecified(mps.servers.arbitration)) {
                mps.servers.arbitration = requestBodyJSON.servers.arbitration;
            }
            else if (util.isUnspecified(mps.servers.arbitration.properties)) {
                mps.servers.arbitration.properties = requestBodyJSON.servers.arbitration.properties;
            }
            else if (util.isUnspecified(mps.servers.arbitration.properties.system)) {
                mps.servers.arbitration.properties.system = requestBodyJSON.servers.arbitration.properties.system;
            }
            else {
                // Poss future enhancement: update each properties.system entry's sub-members below individually as needed (currently assuming all or nothing)
                for (var arbPropKey in requestBodyJSON.servers.arbitration.properties.system) {
                    mps.servers.tournaments.arbitration.system[arbPropKey] = requestBodyJSON.servers.arbitration.properties.system[arbPropKey];
                }
            }
        }   

        return "";
    } catch (err) {
        ERR("[MPSD]doReqServersPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
        return "[MPSD]doReqServersPropertyUpdates: Error exception caught: " + err.message + ", stack: " + err.stack;
    }
}

// mock XBL's async updates to the MPS before returning it, as needed
function handleAutomaticUpdates(mps) {
    try {
        if (!util.isUnspecifiedOrNull(mps) && isTournamentGameMps(mps.servers, mps.constants)) {
            handleAutomaticUpdatesByStartTime(mps);
        }
    } catch (err) {
        ERR("[MPSD]handleAutomaticUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// mock Arena XBL's automatic updating of game/match MPS status based on the scheduled start time
function handleAutomaticUpdatesByStartTime(mps) {
    try {
        var startTimeInt = getStartTimeUtcMs(mps);
        if (isNaN(startTimeInt) || (startTimeInt === -1)) {
            return;//logged if tournament game
        }

        var isStarted = (startTimeInt < Date.now());

        if (isStarted && (util.isUnspecifiedNested(mps, 'arbitration', 'status') || (mps.arbitration.status === "waiting"))) {
            mps.arbitration = { "status": "inprogress" };
            mDoLog && LOG("[MPSD]handleAutomaticUpdatesByStartTime: arbitration.status(waiting->" + mps.arbitration.status + ") after start time(" + startTimeInt + "), currTime(" + Date.now() + ")");
        }
        else if (util.isUnspecifiedNested(mps, 'arbitration', 'status')) {
            mps.arbitration = { "status": "waiting" };
            mDoLog && LOG("[MPSD]handleAutomaticUpdatesByStartTime: arbitration.status(" + mps.arbitration.status + "), start time(" + startTimeInt + "), currTime(" + Date.now() + ")");
        }

        // update members
        if (!util.isUnspecified(mps.members)) {
            for (var k in mps.members) {
                var origStatus = mps.members[k].arbitrationStatus;
                if (isStarted && (util.isUnspecified(origStatus) || (origStatus === "joining"))) {
                    mps.members[k].arbitrationStatus = "playing";
                }
                else if (util.isUnspecified(origStatus)) {
                    m.arbitrationStatus = "joining";
                }
                else
                    continue;
                mDoLog && LOG("[MPSD]handleAutomaticUpdatesByStartTime: member(" + k + ") arbitrationStatus(" + origStatus + "->" + mps.members[k].arbitrationStatus + ") after start time(" + startTimeInt + "), currTime(" + Date.now() + ")");
            }
        }
    } catch (err) {
        ERR("[MPSD]handleAutomaticUpdates: Error exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// \brief validate scheduled start time, if present. To simulate XBL behavior, if the start time is in the past, advance it to current time.
function validateTournamentGameStartTime(mps, response) {
    try {
        var startTimeInt = getStartTimeUtcMs(mps);
        if ((isNaN(startTimeInt) || (startTimeInt === -1))) {
            response.statusCode = 400;
            return false;//logged
        }
        if (startTimeInt <= Date.now()) {
            var now = new Date();
            mps.servers.arbitration.constants.system.startTime = now.toISOString().split('.')[0]+"Z";
        }
        mDoLog && LOG("[MPSD]validateTournamentGameStartTime:" + mps.servers.arbitration.constants.system.startTime);
        return true;
    } catch (err) {
        ERR("[MPSD]validateTournamentGameStartTime: Error exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \brief return the mps's scheduled start time, if present. If not present, returns -1. If failed, NaN
function getStartTimeUtcMs(mps) {
    var startTimeInt = -1;
    if (!util.isUnspecifiedNested(mps, 'servers', 'arbitration', 'constants', 'system', 'startTime')) {
        startTimeInt = Date.parse(mps.servers.arbitration.constants.system.startTime);
    }
    if ((isNaN(startTimeInt) || (startTimeInt === -1)) && isTournamentGameMps(mps.servers, mps.constants)) {
        ERR("[MPSD]getStartTimeUtcMs: bad tournament MPS start time(" + (startTimeInt === -1 ? "<missing>" : mps.servers.arbitration.constants.system.startTime) + ")");
    }
    return startTimeInt;
}

//Function to make a copy of response for large session MPS. Only return 'me' in responses instead of the enitre player list
function largeSessionRspCopy(mps, requestHeaders, isReqCertBased, isReqOnBehalfOfUsers, response) {
    var xuid = "";
    var mpsLargeSession = Object.assign({}, mps);
    if (!isReqCertBased) {
        xuid = requestHeaders.authorization;
    }
    else if (isReqOnBehalfOfUsers) {
        if (requestHeaders[ON_BEHALF_OF_USERS_HEADER].includes(",")) {
            ERR("[MPSD]largeSessionRspCopy: multiple on-behalf-of users unsupported!");
            response.statusCode = 403;
            return "";
        }
        xuid = requestHeaders[ON_BEHALF_OF_USERS_HEADER].split(";")[0];
    }
    if (xuid in mps.members) {
        var me = mpsLargeSession.members[xuid];
        mpsLargeSession.members = {};
        mpsLargeSession.members["me"] = me;
        return mpsLargeSession;
    }
    else {
        mpsLargeSession.members = {};
        return mpsLargeSession;
    }

}

function LOG(str) { log.LOG(str); }
function ERR(str) { log.ERR(str); }
function WARN(str) { log.WARN(str); }
function setDoLogMpsd(val) { mDoLog = val; }

module.exports.sessions_put_response = sessions_put_response;
module.exports.sessions_get_response = sessions_get_response;
module.exports.sessions_get_sessiontemplate = sessions_get_sessiontemplate;
module.exports.handles_post_setactivity = handles_post_setactivity;
module.exports.handles_post_getactivity = handles_post_getactivity;
module.exports.setDoLogMpsd = setDoLogMpsd;
module.exports.setMaxOpCountBeforeReport = setMaxOpCountBeforeReport;
module.exports.sessions_getsessionsxuid_response = sessions_getsessionsxuid_response;
