//Author: Sze Ben
//Date: 2016

var util = require("../common/responseutil");
var log = require("../common/logutil");

// map holding each user's active NP session
var mActivityInfoByUserMap = {};

// map holding active NP sessions
var mNpsBySessionIdMap = {};

// for next NP Session Id
var mNpsSessionIdCounter = 1;

// whether to log NP sessions specific info. for performance by default disabled
var mDoLog = false;

// Metrics
var mGuageMembers = 0;
var mRequestsIgnoredAlreadyJoined = 0;
var mRequestsIgnoredAlreadyLeft = 0;

// number of membership updates before the next logged summary report
var mMaxOpCountBeforeNextReport = 1;
var mOpCountBeforeNextReport = 0;
function setMaxOpCountBeforeReport(val) { mMaxOpCountBeforeNextReport = val; }

function LOG(str) { log.LOG(str); }
function ERR(str) { log.ERR(str); }
function WARN(str) { log.WARN(str); }
function setDoLogSessions(val) { mDoLog = val; }

function processSummaryReport(guageMembersChange) {
    try {
        if (guageMembersChange !== 0) {
            mGuageMembers += guageMembersChange;
        }
        if (++mOpCountBeforeNextReport >= mMaxOpCountBeforeNextReport) {
            LOG("[NPS] SUMMARY REPORT:\n   Games/members: " + util.count(mNpsBySessionIdMap) + "/" + mGuageMembers +
                "\n   Activities:" + util.count(mActivityInfoByUserMap) +
                "\n   ReqIgnored: AlreadyLeft:" + mRequestsIgnoredAlreadyLeft + ", AlreadyJoined:" + mRequestsIgnoredAlreadyJoined);
            mOpCountBeforeNextReport = 0;
        }
    } catch (err) {
        ERR("[NPS]processSummaryReport: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

//////////////////////////
// NP Session Create, Join & Leave RPCs

// constants for eff
const _CONST_POSTSESSION_CONTENT_TYPE = "multipart/mixed; boundary=boundary_parameter";
const _CONST_POSTSESSION_BOUNDARY = "--boundary_parameter";
const _CONST_POSTSESSIONIMG_CONTENTDESCN = "Content-Description:session-image";
const _CONST_POSTSESSIONREQ_CONTENTDESCN = "Content-Description:session-request";
const _CONST_POSTSESSIONDATA_CONTENTDESCN = "Content-Description:session-data";
const _CONST_POSTSESSIONCHANGEABLEDATA_CONTENTDESCN = "Content-Description:changeable-session-data";

// create session, and caller to it (POST v1/sessions). Reference Blaze Server psnsessioninvitation.rpc's postNpSession.
function post_session(requestHeaders, requestBodyBuf, response) {
    try {
        
        //node js version must be at least 1.5.0, for Buffer.indexOf(), used in post_session()
        var nodeVerSplit = process.versions.node.split('.');
        if (nodeVerSplit[0] < 1 || (nodeVerSplit[0] === 1 && nodeVerSplit[1] < 5)) {
            ERR("[NPS]Min Node ver is 1.5! Current:" + process.versions.node);
            response.statusCode = 500;
            return "";
        }

        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs4::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]post_session: unauthorized");
            return "";//status set
        }
        mDoLog && LOG("[NPS]CREATING NEW NPS with user: " + requestHeaders.authorization);

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (util.isUnspecified(requestBodyBuf)) {
            ERR("[NPS]post_session: failed to get valid request body");
            response.statusCode = 400;
            return "";
        }
        if (util.isUnspecified(requestHeaders) || !requestHeaders.hasOwnProperty('content-type') ||
            requestHeaders['content-type'].substr(0, _CONST_POSTSESSION_CONTENT_TYPE.length) !== _CONST_POSTSESSION_CONTENT_TYPE) {
            ERR("[NPS]post_session: failed to get valid content type header from request");
            response.statusCode = 400;
            return "";
        }

        // NOTE: Blaze Server is coded to send session-image part last for efficiency reasons (see ExternalSessionUtilPs4::createNpSession).
        // For mock service eff below, split off rest of request body, to separately process
        var sessImgIndex = requestBodyBuf.indexOf(_CONST_POSTSESSIONIMG_CONTENTDESCN);
        if (sessImgIndex === -1) {
            ERR("[NPS]post_session: failed to get valid request session-image");
            response.statusCode = 400;
            return "";
        }
        var requestBodyNoImg = requestBodyBuf.slice(0, sessImgIndex);
        
        // Get 'session-request' section
        var sessReqDescnIndex = requestBodyNoImg.indexOf(_CONST_POSTSESSIONREQ_CONTENTDESCN);
        if (sessReqDescnIndex === -1) {
            ERR("[NPS]post_session: failed to get request's session-request");
            response.statusCode = 400;
            return "";
        }
        sessReqDescnIndex += _CONST_POSTSESSIONREQ_CONTENTDESCN.length;//advance to end of it
        var sessReqBegIndex = indexOfNextPartContent(sessReqDescnIndex, requestBodyNoImg);
        
        var sessReqLen = (sessReqBegIndex !== -1) ? lengthOfNextPart(sessReqDescnIndex, requestBodyNoImg) : -1;
        if (sessReqLen === -1) {
            ERR("[NPS]post_session: failed to get request's session-request length");
            response.statusCode = 400;
            return "";
        }
        var sessReqContent = JSON.parse(requestBodyNoImg.slice(sessReqBegIndex, sessReqBegIndex + sessReqLen));
        
        // Get 'session-data' section
        var sessDataDescnIndex = requestBodyNoImg.indexOf(_CONST_POSTSESSIONDATA_CONTENTDESCN);
        if (sessDataDescnIndex === -1) {
            ERR("[NPS]post_session: failed to get request's session-data");
            response.statusCode = 400;
            return "";
        }
        sessDataDescnIndex += _CONST_POSTSESSIONDATA_CONTENTDESCN.length;
        var sessDataBegIndex = indexOfNextPartContent(sessDataDescnIndex, requestBodyNoImg);
        var sessDataLen = lengthOfNextPart(sessDataDescnIndex, requestBodyNoImg);
        if (sessDataLen === -1) {
            ERR("[NPS]post_session: failed to get request's session-data length");
            response.statusCode = 400;
            return "";
        }

        var sessDataContent = requestBodyNoImg.slice(sessDataBegIndex, sessDataBegIndex + sessDataLen);
        
        // Get 'changeable-session-data' section
        var changeableDataDescnIndex = requestBodyNoImg.indexOf(_CONST_POSTSESSIONCHANGEABLEDATA_CONTENTDESCN);
        if (changeableDataDescnIndex === -1) {
            ERR("[NPS]post_session: failed to get request's changeable-session-data");
            response.statusCode = 400;
            return "";
        }
        changeableDataDescnIndex += _CONST_POSTSESSIONCHANGEABLEDATA_CONTENTDESCN.length;
        var changeableDataBegIndex = indexOfNextPartContent(changeableDataDescnIndex, requestBodyNoImg);
        var changeableDataLen = lengthOfNextPart(changeableDataDescnIndex, requestBodyNoImg);
        var changeableDataContent = requestBodyNoImg.slice(changeableDataBegIndex, changeableDataBegIndex + changeableDataLen);

        // Note: session-image part is handled as no op
        
        var sessionId = mNpsSessionIdCounter++;

        // Init the new NP session
        // note: sessionData and changeableData are accessed in get_sessiondata(), put_changeablesessiondata()
        var npsInfo = {
            "nps" : sessReqContent, //NP session returned by GET
            "sessionData" : new Buffer(sessDataContent),
            "changeableData" : new Buffer(changeableDataContent)
        };
        npsInfo.nps['sessionCreateTimestamp'] = 123;
        npsInfo.nps['sessionCreator'] = { "accountId" : requestHeaders.authorization, "onlineId" : requestHeaders.authorization };
        // add creator. Sony/Blaze spec: pre-multiple NP session membership (Sony DevNet ticket 58807), also becomes user's active session
        if (!addUserToSession(sessionId, npsInfo.nps, requestHeaders.authorization, response)) {
            return "";//status set
        }

        mNpsBySessionIdMap[sessionId] = npsInfo;

        response.statusCode = 200;
        mDoLog && LOG("[NPS]CREATED NEW NPS: " + JSON.stringify(npsInfo));
        return { "sessionId" : sessionId.toString() };
    } catch (err) {
        ERR("[NPS]post_session: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// post_session() helper for multi-part request. Gets index at end of the next content length header, i.e. before start of next content
const CONST_CONTENTLENGTH = "Content-Length:";
function indexOfNextPartContent(searchStartIndex, requestBodyBufStr) {
    var index = requestBodyBufStr.indexOf(CONST_CONTENTLENGTH, searchStartIndex);
    if (index !== -1) {
        index = requestBodyBufStr.indexOf("\n".charCodeAt(0), index + CONST_CONTENTLENGTH.length);
        if (index !== -1) {
            // advance past new line. note: there may be bytes in a row that eval to "\n"
            while (requestBodyBufStr[index] === "\n".charCodeAt(0) && index < requestBodyBufStr.length)
                ++index;
        }   
    }
    return index;
}
function lengthOfNextPart(searchStartIndex, requestBodyBufStr) {
    try {
        var lengthStrBegIndex = requestBodyBufStr.indexOf(CONST_CONTENTLENGTH, searchStartIndex);
        if (lengthStrBegIndex === -1) {
            ERR("[NPS]lengthOfNextPart: request part missing content length");
            return -1;
        }
        lengthStrBegIndex += CONST_CONTENTLENGTH.length;

        var lengthStrEndIndex = lengthStrBegIndex;
        for (; lengthStrEndIndex < requestBodyBufStr.length; ++lengthStrEndIndex) {
            if (requestBodyBufStr[lengthStrEndIndex] === "\n".charCodeAt(0))
                break;
        }

        if (lengthStrEndIndex <= lengthStrBegIndex) {
            ERR("[NPS]lengthOfNextPart: request part content length empty");
            return -1;
        }

        var len = parseInt(requestBodyBufStr.slice(lengthStrBegIndex, lengthStrEndIndex));
        if (isNaN(len)) {
            ERR("[NPS]lengthOfNextPart: failed to parse content length from request data(" + requestBodyBufStr.slice(lengthStrBegIndex, lengthStrEndIndex) + ").");
            return -1;
        }
        return len;
    } catch (err) {
        ERR("[NPS]lengthOfNextPart: exception caught: " + err.message + ", stack: " + err.stack);
        return -1;
    }
}


// \brief Join caller to session (POST v1/sessions/{sessionId}/members?npTitleId={npTitleId}).
//  Reference Blaze Server psnsessioninvitation.rpc' s postNpSessionMember.
// \param[in] requestHeaders - headers with authorization info for determining caller to remove
function post_session_member(sessionId, requestHeaders, response) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs4::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]post_session_member: unauthorized");
            return "";//status set
        }
        mDoLog && LOG("[NPS]JOINING NPS");
    
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        var nps = get_session(sessionId, response);
        if (util.isUnspecified(nps)) {
            response.statusCode = 404;//PSN's code
            return "";
        }

        var origSize = util.count(nps.members);
        if (!addUserToSession(sessionId, nps, requestHeaders.authorization, response)) {
            return "";//status set
        }

        // Successful join
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[NPS]post_session_member: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// \brief Leave caller from session (DELETE v1/sessions/{sessionId}/members/{accountId}).
//  Reference Blaze Server psnsessioninvitation.rpc' s deleteNpSessionMember.
function delete_session(sessionId, requestHeaders, response) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs4::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]delete_session: unauthorized");
            return "";//status set
        }
        mDoLog && LOG("[NPS]LEAVING NPS");
    
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        var nps = get_session(sessionId, response);
        if (util.isUnspecified(nps)) {
            mDoLog && WARN("[NPS]ignoring request to leave non-existent sesssion");
            ++mRequestsIgnoredAlreadyLeft;
            response.statusCode = 404;//PSN's code
            return "";
        }
    
        // override this success code as needed in remUserFromSession()
        response.statusCode = 204;
        remUserFromSession(sessionId, nps, requestHeaders.authorization, response, true);
        return "";
    } catch (err) {
        ERR("[NPS]delete_session: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// NP Session helpers

// \brief Add calling user to the session
// \param[in] sessionId - id of nps
// \param[in,out] nps - existing session to add member to. Pre: includes any pre-existing members already.
// \param[in] accountId - user to remove
// \return whether user was successfully added
function addUserToSession(sessionId, nps, accountId, response) {
    try {
        var existingMember = findMemberInNps(accountId, nps);
        if (existingMember !== "") {
            // Blaze is doing redundant call here. PSN would not actually return/log error, but to detect excess Blaze calls, log it
            ++mRequestsIgnoredAlreadyJoined;
            if (response !== null)
                response.statusCode = 200;//bstod dcheck correct err code
            return false;
        }
        
        if (!nps.hasOwnProperty('members')) {
            nps['members'] = [];
        }
        nps.members.push({
            "onlineId" : accountId,
            "accountId" : accountId,
            "platform" : "PS4"
        });
        
        // Sony/Blaze spec: pre-multiple NP session membership (Sony DevNet ticket 58807):
        // 1) latest joined session is automatically set as user 's active session.
        // 2) The user will be automatically removed from any prior NP sessions, upon joining another one. This will be handled inside setActivityForUser, for eff
        setActivityForUser(sessionId, accountId);

        processSummaryReport(1);

        mDoLog && LOG("[NPS]addUserToSession: session(" + sessionId + ") added user:" + accountId);
        return true;
    } catch (err) {
        ERR("[NPS]addUserToSession: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \param[in] sessionId - id of nps
// \param[in,out] nps - session to remove member from
// \return end member count of nps. 0 if deleted as result of remove
function remUserFromSession(sessionId, nps, accountId, response, shouldClearActivity) {
    try {
        var memberCountOrig = util.count(nps.members);
        for (var i = 0; i < memberCountOrig; ++i) {
            if (nps.members[i].accountId === accountId) {

                // upon leaving the session, clear user's active session mapping to it
                if (shouldClearActivity === true) {
                    clearActivityForUser(sessionId, nps.members[i].accountId, nps);
                }
                mDoLog && LOG("[NPS]remUserFromSession: session(" + sessionId + ") removed user:" + accountId);
                nps.members.splice(i, 1);
                break;
            }
        }
        
        var memberCountEnd = util.count(nps.members);
        if (memberCountOrig === memberCountEnd) {
            mDoLog && WARN("[NPS]remUserFromSession: not in session(" + sessionId + "), no op, user:" + accountId);
            if (response !== null)
                response.statusCode = 204;//PSN's code
            ++mRequestsIgnoredAlreadyLeft;
            return memberCountEnd;
        }

        // PSN behavior: if NPS became empty, delete it
        if (memberCountEnd === 0) {
            delete mNpsBySessionIdMap[sessionId];//mNpsBySessionIdMap[sessionId] = null;//don't try setting null for GC, won't work and causes mem growth
        }
        
        mDoLog && LOG("[NPS]Removed " + (memberCountOrig - memberCountEnd) + " user(s). " + (memberCountEnd === 0 ? "Session(" + sessionId + ") was empty, removed" : ""));
        processSummaryReport(memberCountEnd - memberCountOrig);
        return memberCountEnd;
    } catch (err) {
        ERR("[NPS]remUserFromSession: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}

function findMemberInNps(memberAccountId, nps) {
    try {
        if (!util.isUnspecifiedNested(nps, 'members') && !util.isUnspecified(memberAccountId)) {
            for (var k in nps.members) {
                if (util.isUnspecifiedNested(nps.members[k], 'accountId')) {
                    ERR("[NPS]findMemberInNps: internal test issue detected: nps member at index(" + k + ") was missing/invalid. Sess: " + JSON.stringify(nps));
                    continue;
                }
                if (nps.members[k].accountId === memberAccountId) {
                    return nps.members[k];
                }
            }
        }
        return "";
    } catch (err) {
        ERR("[NPS]findMemberInNps: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// util
function getGameMemberCount(sessionId) {
    try {
        var npsInfo = getNpSessionInfoInternal(sessionId, null);
        if (util.isUnspecified(npsInfo)) {
            mDoLog && LOG("[NPS]getGameMemberCount: non-existent session!");
            return 0;
        }
        return util.count(npsInfo.nps.members);
    } catch (err) {
        ERR("[NPS]getGameMemberCount: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}

//////////////////////////
// NP Session Update RPCs

// \brief Update session basic properties (PUT /v1/sessions/{sessionId}). Reference Blaze Server psnsessioninvitation.rpc's putNpSessionUpdate().
function put_session(sessionId, requestHeaders, requestBodyJSON, response) {
    try {
        mDoLog && LOG("[NPS]UPDATING BASIC PROPS");

        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]put_session: unauthorized");
            return "";//status set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[NPS]put_session: failed to get request's valid JSON body");
            response.statusCode = 400;
            return "";
        }
        var nps = get_session(sessionId, response);
        if (util.isUnspecified(nps)) {
            mDoLog && WARN("[NPS]put_session: can't update non-existent session");
            response.statusCode = 404;
            return "";
        }
        // PSN: if user isn't a member, access denied
        var existingMember = findMemberInNps(requestHeaders.authorization, nps);
        if (util.isUnspecified(existingMember)) {
            mDoLog && WARN("[NPS]put_session: can't update NPS caller isn't member of");
            response.statusCode = 403;
            return "";
        }

        // currently Blaze only updates the below. (poss enhancement: return appropriate PSN err code if invalidly tried to change constants)
        if (nps.sessionLockFlag !== requestBodyJSON.sessionLockFlag)
            nps.sessionLockFlag = requestBodyJSON.sessionLockFlag;
        if (nps.sessionMaxUser !== requestBodyJSON.sessionMaxUser)
            nps.sessionMaxUser = requestBodyJSON.sessionMaxUser;
        if (nps.sessionName !== requestBodyJSON.sessionName)
            nps.sessionName = requestBodyJSON.sessionName;
        if (nps.sessionStatus !== requestBodyJSON.sessionStatus)
            nps.sessionStatus = requestBodyJSON.sessionStatus;
        // Future enhancement: allow testing localized statuses that may exist!

        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[NPS]put_session: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// \brief Update session image (PUT /v1/sessions/{sessionId}/sessionImage). Reference Blaze Server psnsessioninvitation.rpc 's putNpSessionImage().
function put_sessionimage(sessionId, requestHeaders, requestBodyBuf, response) {
    try {
        
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]put_sessionimage: unauthorized");
            return "";//status set
        }
        var nps = get_session(sessionId, response);
        if (util.isUnspecified(nps)) {
            mDoLog && WARN("[NPS]put_sessionimage: can't update non-existent session");
            response.statusCode = 404;
            return "";
        }
        // PSN: if user isn't a member, access denied
        var existingMember = findMemberInNps(requestHeaders.authorization, nps);
        if (util.isUnspecified(existingMember)) {
            mDoLog && WARN("[NPS]put_sessionimage: can't update NPS caller isn't member of");
            response.statusCode = 403;
            return "";
        }

        mDoLog && LOG("[NPS]UPDATING IMAGE");
        response.statusCode = 200;
        return "";
    } catch (err) {
        ERR("[NPS]put_sessionimage: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// \brief Update session changeable data (PUT /v1/sessions/{sessionId}/changeableSessionData). Reference Blaze Server psnsessioninvitation.rpc's putNpSessionChangeableData().
function put_changeablesessiondata(sessionId, requestHeaders, requestBodyBuf, response) {
    try {
        mDoLog && LOG("[NPS]UPDATING CHANGEABLE DATA");
        
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[NPS]put_changeablesessiondata: unauthorized");
            return "";//status set
        }
        if (util.isUnspecified(requestBodyBuf)) {
            ERR("[NPS]put_changeablesessiondata: failed to get request's valid body");
            response.statusCode = 400;
            return "";
        }
        var npsInfo = getNpSessionInfoInternal(sessionId, response);
        if (response.statusCode !== 200 || util.isUnspecified(npsInfo)) {
            mDoLog && WARN("[NPS]put_changeablesessiondata: can't update non-existent session");
            response.statusCode = 404;
            return "";
        }
        
        // PSN: if user isn't a member, access denied
        var existingMember = findMemberInNps(requestHeaders.authorization, npsInfo.nps);
        if (util.isUnspecified(existingMember)) {
            mDoLog && WARN("[NPS]put_changeablesessiondata: can't update NPS caller isn't member of");
            response.statusCode = 403;
            return "";
        }
        
        // Pre: changeableData was added in post_session()
        if (requestBodyBuf !== npsInfo.changeableData) {
            npsInfo.changeableData = requestBodyBuf;
        }
        
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[NPS]put_changeablesessiondata: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


//////////////////////////
// NP Session Get RPCs

// get info on a session (GET /v1/sessions/{sessionId}?fields=@default,members,sessionLockFlag&npLanguage={language}). Reference Blaze Server psnsessioninvitation.rpc's getNpSession.
function get_session(sessionId, response, isServiceLabelsApi) {
    try {
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        response.statusCode = 200;
        var npsInfo = getNpSessionInfoInternal(sessionId, response);
        if (response.statusCode !== 200) {
            return "";
        }
        return (isServiceLabelsApi ? { "sessions": [npsInfo.nps] } : npsInfo.nps);
    } catch (err) {
        ERR("[NPS]get_session: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// get session's changeable data  (GET /v1/sessions/{sessionId}/changeableSessionData). Reference Blaze Server psnsessioninvitation.rpc's getNpSessionChangeableData.
function get_changeablesessiondata(sessionId, response) {
    try {
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        response.statusCode = 200;
        var npsInfo = getNpSessionInfoInternal(sessionId, response);
        if (response.statusCode !== 200) {
            return;
        }
        // Pre: changeableData was added in post_session()
        return npsInfo.changeableData;
    } catch (err) {
        ERR("[NPS]get_changeablesessiondata: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// get session's binary data (GET /v1/sessions/{sessionId}/sessionData). Reference Blaze Server psnsessioninvitation.rpc's getNpSessionData.
function get_sessiondata(sessionId, response) {
    try {
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        response.statusCode = 200;
        var npsInfo = getNpSessionInfoInternal(sessionId, response);
        if (response.statusCode !== 200) {
            return;
        }
        // Pre: sessionData was added in post_session()
        return npsInfo.sessionData;
    } catch (err) {
        ERR("[NPS]get_sessiondata: exception caught: " + err.message + ", stack: " + err.stack);
    }
}



function getNpSessionInfoInternal(sessionId, response) {
    try {
        if (util.isUnspecified(sessionId)) {
            ERR("[NPS]getNpSessionInfoInternal: failed to get request's sessionId.");
            if (response !== null)
                response.statusCode = 400;
            return "";
        }
        if (!mNpsBySessionIdMap.hasOwnProperty(sessionId)) {
            // NPS not found, return appropriate code (see Blaze Server's PSNSESSIONINVITATION_RESOURCE_NOT_FOUND BlazeRpcError, and ExternalSessionUtilPs4::getNpSession())
            if (response !== null)
                response.statusCode = 404;
            return "";
        }
        return mNpsBySessionIdMap[sessionId];
    } catch (err) {
        ERR("[NPS]getNpSessionInfoInternal: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


//////////////////////////
// Activities Get RPCs

const emptyActivityInfo = {
    "start": 0,
    "size": 1,
    "totalResults": 1,
    "sessions": []
};

// \brief retrieve user's primary/active session (GET /v1/users/{accountId}/sessions). Reference Blaze Server psnsessioninvitation.rpc's getNpSessionIds.
// \param[in] accountId the user's online id
function get_users_sessions_activity(accountId, response) {
    try {
        if (util.isUnspecified(accountId)) {
            mDoLog && WARN("[NPS]get_users_sessions_activity: request missing accountId");
            response.statusCode = 400;
            return "";
        }

        var activityInfo = (mActivityInfoByUserMap.hasOwnProperty(accountId) ? mActivityInfoByUserMap[accountId] : emptyActivityInfo);
        activityInfo = (activityInfo === null ? emptyActivityInfo : activityInfo);
        mDoLog && LOG("[NPS]get_users_sessions_activity: activity " + (activityInfo === "" ? "not" : JSON.stringify(activityInfo)) + " found for user:" + accountId);
        response.statusCode = 200;
        return activityInfo;
    } catch (err) {
        ERR("[NPS]get_users_sessions_activity: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// helpers

// \brief set user's primary/active session
function setActivityForUser(sessionId, userAccountId) {
    try {
        mDoLog && LOG("[NPS]setActivityForUser: setting activity(" + sessionId + ") for user:" + userAccountId);
        var activity = mActivityInfoByUserMap[userAccountId];
        if (typeof activity !== 'undefined') {
            if ((typeof activity.sessions === 'undefined') || (typeof activity.sessions[0] === 'undefined') || (typeof activity.sessions[0].sessionId === 'undefined')) {
                ERR("[NPS]setActivityForUser: existing activity missing sessionId, making new one for user:" + userAccountId);
            }
            else {
                if (activity.sessions[0].sessionId === sessionId)
                    return;//no op
            
                // Sony/Blaze spec: pre-multiple NP session membership, user will be automatically removed from any prior sessions, upon advertising another (Sony DevNet ticket 58807)
                var npsInfo = getNpSessionInfoInternal(activity.sessions[0].sessionId, null);
                if (!util.isUnspecified(npsInfo)) {
                    remUserFromSession(activity.sessions[0].sessionId, npsInfo.nps, userAccountId, null, false);// false here avoids clearing activity, see below
                }

                // For efficiency, if had a pre existing activity, no need to reallocate, just replace sessionId
                activity.sessions[0].sessionId = sessionId.toString();
                return;
            }        
        }
    
        // add an activity for user. side: Sony/Blaze spec supports at most session in the 'sessions' list
        var newActivityInfo = {
            "start": 0,
            "size": 1,
            "totalResults": 1,
            "sessions": [{ "platform": "PS4", "sessionId": sessionId.toString() }]
        };
        mActivityInfoByUserMap[userAccountId] = newActivityInfo;
        mDoLog && LOG("[NPS]setActivityForUser: successfully set activity(" + newActivityInfo.sessions[0].sessionId + ") for user:" + userAccountId);
    } catch (err) {
        ERR("[NPS]setActivityForUser: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

// \brief clear user's primary/active session
function clearActivityForUser(sessionId, userAccountId, nps) {
    try {
        var activityInfo = mActivityInfoByUserMap.hasOwnProperty(userAccountId) ? mActivityInfoByUserMap[userAccountId] : null;
        if (activityInfo === null) {
            mDoLog && LOG("[NPS]clearActivityForUser: no op, no activity found for user:" + userAccountId);
            return;
        }    
        if (util.isUnspecifiedNested(activityInfo, 'sessions')) {
            ERR("[NPS]clearActivityForUser: mock service error, no op, as activity for nps(" + sessionId + ") was invalid/empty for user:" + userAccountId);
            return;
        }

        var activityList = activityInfo.sessions;
        if (activityList.length > 1) {
            mDoLog && WARN("[NPS]clearActivityForUser: likely mock service error, expected at most 1 activity, actual number " + activityList.length + ", for user:" + userAccountId);
        }
        // remove user's activity
        for (var i in activityList) {
            if (activityList[i].sessionId === sessionId) {
                //note: to clean up on error, if there's somehow > 1 in list (unsupported by Sony/Blaze spec), we'll remove all activities
                mDoLog && LOG("[NPS]clearActivityForUser: removing activity(" + sessionId + ") for user:" + userAccountId);
                activityList = [];
                break;
            }
        }
        if (activityList.length === 0) {
            delete mActivityInfoByUserMap[userAccountId];//mActivityInfoByUserMap[userAccountId] = null;//don't try setting null for GC, won't work and causes mem growth
        }
        else {
            mDoLog && LOG("[NPS]clearActivityForUser: no op, as activity(" + sessionId + ") was not mapped as primary for user:" + userAccountId);
        }
    } catch (err) {
        ERR("[NPS]clearActivityForUser: exception caught: " + err.message + ", stack: " + err.stack);
    }
}

module.exports.get_session = get_session;
module.exports.get_sessiondata = get_sessiondata;
module.exports.get_changeablesessiondata = get_changeablesessiondata;
module.exports.post_session = post_session;
module.exports.post_session_member = post_session_member;
module.exports.delete_session = delete_session;
module.exports.put_session = put_session;
module.exports.put_sessionimage = put_sessionimage;
module.exports.put_changeablesessiondata = put_changeablesessiondata;
module.exports.get_users_sessions_activity = get_users_sessions_activity;
module.exports.setMaxOpCountBeforeReport = setMaxOpCountBeforeReport;
module.exports.setDoLogSessions = setDoLogSessions;
module.exports.getGameMemberCount = getGameMemberCount;

