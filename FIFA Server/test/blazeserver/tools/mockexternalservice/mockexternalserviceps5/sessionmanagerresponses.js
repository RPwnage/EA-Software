//Author: Sze Ben
//Date: 2020

var util = require("../common/responseutil");
var log = require("../common/logutil");

const _SMGR_ACCOUNTIDS_HEADERNAME = "X-PSN-SESSION-MANAGER-ACCOUNT-IDS";
const __SMGR_SESSIONIDS_HEADERNAME = "X-PSN-SESSION-MANAGER-SESSION-IDS";

var PsnSupportedPlatformType = {
    PS5: 'PS5',
    PROSPERO: 'PROSPERO',
    PS4: 'PS4'
};
const _PsnSupportedPlatformTypeMaxStringlen = PsnSupportedPlatformType.PROSPERO.length;
function isPsnSupportedPlatformType(obj) {
    for (var x in PsnSupportedPlatformType) {
        if (x.toString() === obj)
            return true;
    }
    return false;
}

var PsnJoinableUserType = {
    NO_ONE: 'NO_ONE',
    FRIENDS: 'FRIENDS',
    FRIENDS_OF_FRIENDS: 'FRIENDS_OF_FRIENDS',
    ANYONE: 'ANYONE',
    SPECIFIED_USERS: 'SPECIFIED_USERS'
};
function isPsnJoinableUserType(obj) {
    for (var x in PsnJoinableUserType) {
        if (x.toString() === obj)
            return true;
    }
    return false;
}

// map holding each user's active 1p session
var mActiveSessInfoByUserMap = {};

// map holding active 1p sessions
var mPlayerSessionBySessionIdMap = {};

// for next 1P Session Id
var mPlayerSessionSessionIdCounter = 1;

// whether to log 1p sessions specific info. for performance by default disabled
var mDoLog = false;

// Metrics
var mGuageMembers = 0;
var mRequestsIgnoredAlreadyJoined = 0;
var mRequestsIgnoredAlreadyLeft = 0;

// number of membership updates before the next logged summary report
var mMaxOpCountBeforeNextReport = 1;
var mOpCountBeforeNextReport = 0;
function setMaxOpCountBeforeReport(val) { mMaxOpCountBeforeNextReport = val; }

function makeHttp400ErrRsp(reasonStr) {
    return { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269185, "message": "Bad request ('Mock PSN: " + reasonStr + "')" } };
}
const __HTTP404_ERR_RSP = { "error": { "referenceId": "1111-1111-1111-1111-1111", "code": 2269197, "message": "Resource not found (Mock PSN Service does not have specified PlayerSession resource)" } };

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
            LOG("[smgr] SUMMARY REPORT:\n   Games/members: " + util.count(mPlayerSessionBySessionIdMap) + "/" + mGuageMembers +
                "\n   Activities:" + util.count(mActiveSessInfoByUserMap) +
                "\n   ReqIgnored: AlreadyLeft:" + mRequestsIgnoredAlreadyLeft + ", AlreadyJoined:" + mRequestsIgnoredAlreadyJoined);
            mOpCountBeforeNextReport = 0;
        }
    } catch (err) {
        ERR("[smgr]processSummaryReport: exception caught: " + err.message + ", stack: " + err.stack);
    }
}



//////////////////////////
// 1P Session Create, Join & Leave RPCs

// create session, and caller to it (POST v1/playerSessions). Reference Blaze Server psnsessionmanager.rpc's postPlayerSession. largely
function post_playersession(response, requestHeaders, requestBodyJSON) {
    try {
        
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken())
        // This maybe used to replace 'me' entries with accountId//GEN5_TODO: valdiate input token vs body's acctid as real PSN would
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]post_playersession: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[smgr]post_playersession: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        var npServiceLabel = (util.isUnspecifiedOrNull(requestBodyJSON.npServiceLabel) ? 0 : requestBodyJSON.npServiceLabel);
        if (!util.validateReqNpServiceLabel(response, npServiceLabel)) {
            WARN("[smgr]post_playersession: Poss BlazeServer setup issue detected: S2S requires a valid NP service label for req.");
        }
        mDoLog && LOG("[smgr]CREATING NEW playerSession, user: " + requestHeaders.authorization);
        //GEN5_TODO: These checks from docs are aribtrarily ordered currently,  test/confirm order of checks PSN actually does://GEN5_TODO add more validations!
        
        var reqSes = requestBodyJSON.playerSessions[0];

        // validate entire req, players lists etc
        // This also populates required defaults for req members if unspecified, as per PSN:
        var isCreate = true;
        if (!validateReqPlayerSess(response, requestHeaders, reqSes, "post_playersession", isCreate, null)) {
            return makeHttp400ErrRsp("req invalid session");//response.statusCode set
        }

        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        var sessionId = mPlayerSessionSessionIdCounter++;
        
        // Init new sess, fill in defaulted fields based on PSN.
        //GEN5_TODO: confirm whether these timestamps even appear in rsps if not (yet) set. Maybe omittable here?: thot he docs state the getMatchDetail req always has em?
        //GEN5_TODO: audit/confirm all defaults below/above correct
        var sessInfo = {
            "sess": reqSes//1p session returned by GET. Start w/req's contents, then populate auto-set values
        };
        sessInfo.sess['sessionId'] = sessionId.toString();
        sessInfo.sess['createdTimestamp'] = Date.now();

        upsertLocalizedSessionName(sessInfo.sess, reqSes.localizedSessionName);

        var reqPlayer = sessInfo.sess.member.players[0];//SHOULD be set and validated exists above!

        // add creator NOT needed, as it should have been INCLUDED as part of req by PS5 spec (unlike PS4, X1)
        //addUserToPlayerSess(response, sessInfo.sess, reqPlayer, false)
        setActiveSessForUser(sessionId, reqPlayer.accountId);//but still need to set as primary
        reqPlayer['joinTimestamp'] = sessInfo.sess['createdTimestamp'];

        // set creator as leader (as per Sony spec)
        sessInfo.sess['leader'] = reqPlayer;

        mPlayerSessionBySessionIdMap[sessionId] = sessInfo;

        response.statusCode = 200;
        mDoLog && LOG("[smgr]CREATED NEW SESS " + sessInfo.sess.sessionId + ": " + JSON.stringify(sessInfo.sess));
        return { "playerSessions": [{ "sessionId": sessInfo.sess.sessionId, "member": { "players": [{ "accountId": reqPlayer.accountId, "platform": reqPlayer.platform }] } }] };
    } catch (err) {
        ERR("[smgr]post_playersession: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// see similar validateReqLocalizedSessionName()
function upsertLocalizedSessionName(sess, reqLocalizedSessionName) {
    if (util.isUnspecifiedOrNull(sess)) {
        mDoLog && WARN("upsertLocalizedSessionName: no target sess(" + sess + ") specified. NOOP.");
        return;
    }
    //upsert localizedSessionName
    if (!util.isUnspecifiedOrNull(reqLocalizedSessionName)
        && (sess.localizedSessionName !== reqLocalizedSessionName)) {//for eff can skip, if (create flow) already set us to point to req

        if (util.isUnspecifiedOrNull(sess.localizedSessionName)) {
            mDoLog && LOG("[smgr]upsertLocalizedSessionName: init:   " + JSON.stringify(reqLocalizedSessionName));
            sess['localizedSessionName'] = reqLocalizedSessionName;
        }
        else {
            //upsert localizedSessionName.localizedTest
            if (!util.isUnspecifiedOrNull(reqLocalizedSessionName.localizedText)) {
                if (util.isUnspecifiedOrNull(sess.localizedSessionName.localizedText)) {
                    mDoLog && LOG("[smgrupsertLocalizedSessionName: init localizedText:   " + JSON.stringify(reqLocalizedSessionName.localizedText));
                    sess.localizedSessionName['localizedText'] = reqLocalizedSessionName.localizedText;
                }
                else {
                    for (var k in reqLocalizedSessionName.localizedText) {
                        mDoLog && LOG("[smgr]upsertLocalizedSessionName: localizedText." + k + ":   " + sess.localizedSessionName.localizedText[k] + " -> " + reqLocalizedSessionName.localizedText[k]);
                        sess.localizedSessionName.localizedText[k] = reqLocalizedSessionName.localizedText[k];
                    }
                }
            }
            //upsert localizedSessionName.defaultLanguage
            if (!util.isUnspecifiedOrNull(reqLocalizedSessionName.defaultLanguage)) {
                mDoLog && LOG("[smgr]upsertLocalizedSessionName: defaultLanguage:   " + sess.localizedSessionName.defaultLanguage + " -> " + reqLocalizedSessionName.defaultLanguage);
                sess.localizedSessionName['defaultLanguage'] = reqLocalizedSessionName.defaultLanguage;
            }
        }
    }
    //update cached sessionName returned to GET reqs for eff (by default Blaze tests use same en-US)
    var defaultLang = sess.localizedSessionName.defaultLanguage;
    if (util.isString(defaultLang) && !util.isUnspecifiedOrNullNested(sess.localizedSessionName, 'localizedText', defaultLang)) {
        mDoLog && LOG("[smgr]upsertLocalizedSessionName: sessionName cached:   " + sess.sessionName + " -> " + sess.localizedSessionName.localizedText[defaultLang]);
        sess['sessionName'] = sess.localizedSessionName.localizedText[defaultLang];
    }
}


// \brief Join caller to session as player (POST /v1/playerSessions/{sessionId}/member/players).
//  Reference Blaze Server psnsessionmanager.rpc' s postPlayerSessionMember, in Sony docs 'joinPlayerSessionAsPlayer'.
// \param[in] requestHeaders - headers with authorization info for determining caller
function post_playersession_player(response, requestHeaders, requestBodyJSON, sessionId) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]post_playersession_player: unauthorized");
            return "";//rsp set
        }
        mDoLog && LOG("[smgr]JOINING SESS " + sessionId);

        // PSN spec requires players array size to be 1 currently
        if (!util.isArray(requestBodyJSON.players) || (requestBodyJSON.players.length !== 1)) {
            ERR("[smgr]post_playersession_player: playerSessions array size must be 1, actual:" + (util.isArray(requestBodyJSON.players) ? requestBodyJSON.players.length : "<bad-type>"));
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid playerSessions array size");
        }

        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            response.statusCode = 404;//PSN's code
            ERR("[smgr]post_playersession_player: not found: sessionId: " + sessionId);
            return __HTTP404_ERR_RSP;
        }

        //to mimic PSN, this converts 'me' to appropriate accountId up front here:
        var maxPlayersToCheck = sess.maxPlayers;
        if (!validateReqMembersList(response, requestHeaders, requestBodyJSON.players, "post_playersession_player", maxPlayersToCheck, sess, false)) {
            ERR("[smgr]post_playersession_player: req players list validation failed");
            return makeHttp400ErrRsp("req invalid players list");//response.statusCode set
        }

        var reqPlayer = requestBodyJSON.players[0];
        if (!validateReqMember(response, reqPlayer, requestHeaders.authorization, sess, true)) {
            ERR("[smgr]post_playersession_player: failed: request's player invalid");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid player");
        }

        if (!addUserToPlayerSess(response, sess, reqPlayer, false)) {
            ERR("[smgr]post_playersession_player: failed: add player to pses: " + sessionId);
            return makeHttp400ErrRsp("failed to add player to PlayerSession");//response.statusCode set
        }

        // Successful join
        response.statusCode = 201;
        mDoLog && LOG("[smgr]JOINED SESS " + sessionId);
        return sess;
    } catch (err) {
        ERR("[smgr]post_playersession_player: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// \brief Join caller to session as spectator (POST /v1/playerSessions/{sessionId}/member/spectators).
//  Reference Blaze Server psnsessionmanager.rpc' s postPlayerSessionMember, in Sony docs 'joinPlayerSessionAsSpectator'.
// \param[in] requestHeaders - headers with authorization info for determining caller
function post_playersession_spectator(response, requestHeaders, requestBodyJSON, sessionId) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]post_playersession_spectator: unauthorized");
            return "";//rsp set
        }
        mDoLog && LOG("[smgr]JOINING AS SPECT Sess " + sessionId);

        // PSN spec requires playerSessions array size to be 1 currently
        if (!util.isArray(requestBodyJSON.spectators) || (requestBodyJSON.spectators.length !== 1)) {
            ERR("[smgr]post_playersession_spectator: spectators array size must be 1, actual:" + (util.isArray(requestBodyJSON.spectators) ? requestBodyJSON.spectators.length : "<bad-type>"));
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid spectators list len");
        }

        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            mDoLog && WARN("[smgr]post_playersession_spectator: not found sessionId: " + sessionId);
            response.statusCode = 404;//PSN's code
            return __HTTP404_ERR_RSP;
        }

        var reqSpectator = requestBodyJSON.spectators[0];
        //to mimic PSN, this converts 'me' to appropriate accountId up front here:
        if (!validateReqMember(response, reqSpectator, requestHeaders.authorization, sess, true)) {
            ERR("[smgr]post_playersession_spectator: failed: request's spectator invalid");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid spectator");
        }

        if (!addUserToPlayerSess(response, sess, reqSpectator, true)) {
            ERR("[smgr]post_playersession_spectator: addUserToPlayerSess failed");
            return makeHttp400ErrRsp("failed to add players");//response.statusCode set
        }

        // Successful join
        response.statusCode = 201;
        mDoLog && LOG("[smgr]JOINED AS SPECT");
        return sess;
    } catch (err) {
        ERR("[smgr]post_playersession_spectator: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// \brief Leave the member (player OR spectator) from session (DELETE v1/playersessions/{sessionId}/members/{accountId}).
//  Reference Blaze Server psnsessionmanager.rpc' s deletePlayerSessionMember, Sony doc's 'leavePlayerSession'.
//  PSN specs:          //GEN5_TODO: handle these cases edge leave cases, priv checks:
//  Leaving Sessions:
//      If me or the accountId of the user linked to the access token is specified for accountId, the member linked to the access token will leave.
//  Kicking Out:
//      Limited to cases in which the member linked to the access token is the leader, a member other than the leader can be specified for accountId and forced to leave(kicked out) Users who have been kicked out are afterward unable to rejoin the session.
//  Transfer of Leader Privileges:
//      If the leader of the session leaves, leader privileges are transferred to the player who joined at the oldest date and time.
//      If there are multiple players with the same join date and time, the player with the smallest Account ID becomes the new leader.
function delete_playersession_member(response, requestHeaders, sessionId, accountId) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken()).
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]delete_playersession_member: unauthorized");
            return "";//rsp set
        }
        if (!util.isString(accountId) || !util.isString(sessionId)) {
            ERR("[smgr]delete_playersession_member: invalid requested sessionId or accountId");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid accountId or sessionId");
        }
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)

        // PSN would cross check the auth token vs accountId, for the cases of leaving and kicking//GEN5_TODO validations requestHeaders.authorization vs accountId

        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            mDoLog && WARN("[smgr]ignoring request to rem (" + accountId+") from non-existent session's joinable users list");
            response.statusCode = 404;//PSN's code
            return __HTTP404_ERR_RSP;
        }
        
        mDoLog && LOG("[smgr]LEAVING Sess user:" + accountId + " from sess:" + sessionId);

        // override this success code as needed in remUserFromPlayerSess()
        response.statusCode = 204;
        remUserFromPlayerSess(response, sess, accountId, true);//auto-migrates leader as need

        return "";
    } catch (err) {
        ERR("[smgr]delete_playersession_member: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

//POST /v1/playerSessions/{sessionId}/joinableSpecifiedUsers
//PSN spec: Register the specified user to the joinableSpecifiedUsers list for the Player Session.
// When the joinableUserType setting for the Player Session is SPECIFIED_USERS, the joinableSpecifiedUsers indicates the users who can join the Player Session.
// No error will occur, even if a user who has already been registered is specified by his or her accountId.
function post_playersession_joinableusers(response, requestHeaders, requestBodyJSON, sessionId) {
    try {
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]post_playersession_joinableusers: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[smgr]post_playersession_joinableusers: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        if (!util.isArray(requestBodyJSON.joinableSpecifiedUsers)) {
            ERR("[smgr]post_playersession_joinableusers: invalid/missing joinableSpecifiedUsers in req");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req missing joinableSpecifiedUsers");
        }
        // validate format of the joinable users list in req b4 committing
        for (var i in requestBodyJSON.joinableSpecifiedUsers) {
            if (util.isUnspecifiedOrNullNested(requestBodyJSON.joinableSpecifiedUsers[i], "accountId") || !util.isString(requestBodyJSON.joinableSpecifiedUsers[i].accountId)) {
                ERR("[smgr]post_playersession_joinableusers: invalid user in joinableSpecifiedUsers in req");
                response.statusCode = 400;
                return makeHttp400ErrRsp("req invalid user in joinableSpecifiedUsers");
            }
        }
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        mDoLog && LOG("[smgr]ADD joinableSpecifiedUsers");
        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            mDoLog && WARN("[smgr]post_playersession_joinableusers: ignoring request to add to non-existent session's joinable users list");
            response.statusCode = 404;//PSN's code
            return __HTTP404_ERR_RSP;
        }
        //add if not yet in list
        if (util.isUnspecifiedOrNull(sess.joinableSpecifiedUsers)) {
            sess['joinableSpecifiedUsers'] = [];
        }
        for (var a in requestBodyJSON.joinableSpecifiedUsers) {
            addUserToJoinableUsers(sess.joinableSpecifiedUsers, requestBodyJSON.joinableSpecifiedUsers[a].accountId);
        }
        response.statusCode = 201;//sony docs state this is returned regardless of whether already was in list
        return sess.joinableSpecifiedUsers;
    } catch (err) {
        ERR("[smgr]post_playersession_joinableusers: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


//DELETE /v1/playerSessions/{sessionId}/joinableSpecifiedUsers  .  Delete the specified users from the joinableSpecifiedUsers list for the Player Session.
function delete_playersession_joinableusers(response, requestHeaders, sessionId) {
    try {
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]delete_playersession_joinableusers: unauthorized");
            return "";//rsp set
        }
        mDoLog && LOG("[smgr]DELETE joinableSpecifiedUsers");
        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecifiedOrNull(sess)) {
            mDoLog && WARN("[smgr]ignoring request to leave non-existent session");
            ++mRequestsIgnoredAlreadyLeft;
            response.statusCode = 404;//PSN's code
            return __HTTP404_ERR_RSP;
        }
        var accountIds = requestHeaders[_SMGR_ACCOUNTIDS_HEADERNAME];
        if (util.isUnspecifiedOrNull(accountIds)) {
            accountIds = requestHeaders[_SMGR_ACCOUNTIDS_HEADERNAME.toLowerCase()];//in case lc
        }
        if (!util.isString(accountIds)) {
            mDoLog && WARN("[smgr]delete_playersession_joinableusers: invalid/missing values for header:" + _SMGR_ACCOUNTIDS_HEADERNAME);
            response.statusCode = 400;//PSN's code//GEN5_TODO confirm
            return makeHttp400ErrRsp("req invalid accountIds header");
        }
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        // GEN5_TODO: confirm types of errors would be checked for here by PSN? Empty accountIds? What happens if the removed user is already a member?

        if (!util.isArray(sess.joinableSpecifiedUsers)) {
            mDoLog && WARN("[smgr]delete_playersession_joinableusers: sess(" + sessionId + ") doesn't have its joinableSpecifiedUsers list (yet?), NOOP for: " + accountIds);
            response.statusCode = 204;
            return "";
        }
        // else has a joinableSpecifiedUsers to update
        var accountIdArr = accountIds.split(',');

        //PSN spec: The request will succeed even if a specified user is not registered on the joinableSpecifiedUsers list.
        if (util.isArray(accountIdArr)) {
            for (var i in accountIdArr) {
                remUserFromJoinableUsers(sess.joinableSpecifiedUsers, accountIdArr[i]);
            }
        }
        response.statusCode = 204;
        return "";
    } catch (err) {
        ERR("[smgr]delete_playersession_joinableusers: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// Session helpers


function addUserToJoinableUsers(joinableSpecifiedUsers, accountId) {
    try {
        //Pre: args pre validated already
        if (util.isUnspecifiedOrNull(accountId) || !util.isString(accountId) || !util.isArray(joinableSpecifiedUsers)) {//sanity gaurd
            ERR("[smgr]addUserToJoinableUsers: internal err: invalid accountId(" + (accountId !== null ? accountId : "<null>") + ") or joinableSpecifiedUsers(" + (joinableSpecifiedUsers !== null ? "<not-array>" : "<null>") + "), fix mock code.");
            return false;
        }
        var i = joinableSpecifiedUsers.length;
        while (i--) {
            if (joinableSpecifiedUsers[i].accountId === accountId) {
                mDoLog && LOG("[smgr]addUserToJoinableUsers: already added user(" + accountId + ") to joinable users list. NOOP");
                return true;
            }
        }
        joinableSpecifiedUsers.push({ "accountId": accountId });
        mDoLog && LOG("[smgr]addUserToJoinableUsers: added user(" + accountId + ") to joinable users list.");
        return true;
    } catch (err) {
        ERR("[smgr]addUserToJoinableUsers: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

function remUserFromJoinableUsers(joinableSpecifiedUsers, accountId) {
    try {
        //Pre: args pre validated already
        if (util.isUnspecifiedOrNull(accountId) || !util.isString(accountId) || !util.isArray(joinableSpecifiedUsers)) {//sanity gaurd
            ERR("[smgr]remUserFromJoinableUsers: internal err: invalid accountId(" + (accountId !== null ? accountId : "<null>") + ") or joinableSpecifiedUsers(" + (joinableSpecifiedUsers !== null ? "<not-array>" : "<null>") + "), fix mock code.");
            return false;
        }
        var i = joinableSpecifiedUsers.length;
        while (i--) {
            if (joinableSpecifiedUsers[i].accountId === accountId) {
                joinableSpecifiedUsers.splice(i, 1);
                mDoLog && LOG("[smgr]remUserFromJoinableUsers: removed user(" + accountId + ") from joinable users list");
                break;
            }
        }
        return true;
    } catch (err) {
        ERR("[smgr]remUserFromJoinableUsers: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}



// \brief Add calling user to the session
// \param[in] sessionId - id of sess
// \param[in,out] sess - existing session to add member to. Pre: includes any pre-existing members already.
// \param[in] accountId - user to add
// \return whether user was successfully added
function addUserToPlayerSess(response, sess, reqUser, asSpectator) {
    try {
        // PRE: If here, req fully validated and is otherwise comittable (don't expect rolling back anything!)

        // add in case lists not both specified in req
        if (!sess.hasOwnProperty('member')) {
            sess['member'] = { "players": [], "spectators": [] };
        }
        if (!sess.member.hasOwnProperty('spectators')) {
            sess.member['spectators'] = [];
        }
        if (!sess.member.hasOwnProperty('players')) {
            sess.member['players'] = [];
        }

        var sessionId = sess.sessionId;
        var accountId = reqUser.accountId;

        var pInd = findPlayerIndexInPlayerSess(sess, accountId);
        var sInd = findSpectatorIndexInPlayerSess(sess, accountId);
        if ((pInd !== -1 && !asSpectator) || (sInd !== -1 && asSpectator)) {

            // Blaze is doing redundant call here. PSN would not actually return/log error, but to detect excess Blaze calls, log it
            ++mRequestsIgnoredAlreadyJoined;
            if (response !== null)
                response.statusCode = 200;//GEN5_TODO dcheck correct err code
            mDoLog && LOG("[smgr]addUserToPlayerSess: user(" + accountId + ") already joined ses(" + sessionId + "). NOOP.");
            return false;
        }

        reqUser['joinTimestamp'] = Date.now();

        if (asSpectator) {
            if (pInd !== -1) {
                // remove from players list. We'll readd to the spectators list below
                sess.member.players.splice(pInd, 1);
            }
            sess.member.spectators.push(reqUser);
        }
        else {
            //player
            if (sInd !== -1) {
                // remove from spectators list. We'll readd to the players list below
                sess.member.spectators.splice(sInd, 1);
            }
            sess.member.players.push(reqUser);
        }
        
        // Sony/Blaze spec: single session membership:
        // 1) latest joined session is automatically set as user 's active session.
        // 2) The user will be automatically removed from any prior 1p sessions, upon joining another one. This will be handled inside setActiveSessForUser, for eff
        setActiveSessForUser(sessionId, accountId);

        processSummaryReport(1);

        mDoLog && LOG("[smgr]addUserToPlayerSess: session(" + sessionId + ") added user:" + accountId);
        return true;
    } catch (err) {
        ERR("[smgr]addUserToPlayerSess: exception caught: " + err.message + ", stack: " + err.stack);
        return false;
    }
}

// \param[in,out] sess - session to remove member from
// \return end member count of sess. 0 if deleted as result of remove. -1 if user wasn't in session
function remUserFromPlayerSess(response, sess, accountId, shouldClearActiveSess) {
    try {
        var sInd = -1;
        var pInd = findPlayerIndexInPlayerSess(sess, accountId);
        if (pInd === -1) {
            sInd = findSpectatorIndexInPlayerSess(sess, accountId);
        }

        if ((pInd === -1) && (sInd === -1)) {
            mDoLog && WARN("[smgr]remUserFromPlayerSess: not in session(" + sess.sessionId + "), no op, user:" + accountId);
            if (response !== null)
                response.statusCode = 204;//PSN's code(?)
            ++mRequestsIgnoredAlreadyLeft;
            return -1;
        }
        //if here, we're removing
        
        // upon leaving the session, clear user's active session mapping to it
        if (shouldClearActiveSess === true) {
            clearActiveSessForUser(sess, accountId);
        }

        var origLeaderAccountId = sess.leader.accountId;
        var memberListCountOrig = 0;
        var memberListCountEnd = 0;
        //remove from spectators
        if (sInd !== -1) {
            memberListCountOrig = sess.member.spectators.length;

            mDoLog && LOG("[smgr]remUserFromPlayerSess: session(" + sess.sessionId + ") removed spectator:" + accountId);
            sess.member.spectators.splice(sInd, 1);

            memberListCountEnd = sess.member.spectators.length;
        }
        //remove from players
        if (pInd !== -1) {
            if ((pInd !== -1) && (sInd !== -1)) {
                //shouldn't be poss unless bug. just cleanup in case, but log ERR:
                ERR("[smgr]remUserFromPlayerSess: unexpected mock tool state, user:" + accountId + " was in both players and spectators list of session(" + sess.sessionId + ")");
            }

            memberListCountOrig = sess.member.players.length;

            // To mimic PSN, immediately do leader auto-migrations (tho blaze *might* override and make a player admin always be the leader after).
            // PSN grabs the earliest (poss enhancement: PSN spec is if somehow tied would grab shortest accountId str, could do that too here).
            if ((origLeaderAccountId === accountId) && (memberListCountOrig > 1)) {
                sess['leader'] = (pInd !== 0 ? sess.member.players[0] : sess.member.players[1]);
            }
            mDoLog && LOG("[smgr]remUserFromPlayerSess: session(" + sess.sessionId + ") removed participant:" + accountId);
            sess.member.players.splice(pInd, 1);

            memberListCountEnd = sess.member.players.length;

            // PSN doc'd behavior: If participating players after a player leaves is 0, Player Session is deleted. https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0008.html
            if (memberListCountEnd === 0) {
                delete mPlayerSessionBySessionIdMap[sess.sessionId];//mPlayerSessionBySessionIdMap[sess.sessionId] = null;//don't try setting null for GC, won't work and causes mem growth
            }
        }
      
        processSummaryReport(memberListCountEnd - memberListCountOrig);
        return memberListCountEnd;
    } catch (err) {
        ERR("[smgr]remUserFromPlayerSess: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}



//////////////////////////
// 1P Session Update RPCs

// \brief Update session basic properties (PATCH /v1/playersessions/{sessionId}). Reference Blaze Server psnsessionmanager.rpc's putPlayerSessionUpdate().
function patch_session(response, requestHeaders, requestBodyJSON, sessionId) {
    //GEN5_TODO: validations, complete this..:
    //Modify one item of information specified, out of all the information concerning the Player Session(it is not possible to update multiple items, and specifying multiple such items will result in an error).
    //If maxPlayers is being updated, it cannot be modified to a value smaller than the number of players already currently participating.
    //If maxSpectators is being updated, it cannot be modified to a value smaller than the number of spectators already currently participating.
    //If updating joinableUserType or invitableUserType, the access token linked to the user who is the leader of the Player Session must be specified.
    if (!util.validateAuthorization(requestHeaders, response)) {
        ERR("[smgr]patch_session: unauthorized");
        return "";//rsp set
    }
    if (util.isUnspecified(requestBodyJSON)) {
        ERR("[smgr]patch_session: failed to get request's valid JSON body");
        response.statusCode = 400;
        return makeHttp400ErrRsp("req invalid JSON");
    }
    var reqSes = requestBodyJSON;
    // PSN spec is you're only allowed to specify max ONE thing to change!
    if (util.count(reqSes) !== 1) {
        ERR("[smgr]patch_session: request specified more than 1 item, PSN disallows");
        response.statusCode = 400;
        return makeHttp400ErrRsp("req specified more than 1 item, PSN disallows");
    }
    var propToUpdate = Object.keys(reqSes)[0];//validated above has 1
    mDoLog && LOG("[smgr]UPDATING SESS " + sessionId + "(" + propToUpdate + ")");

    var sess = getPlayerSessionInternal(response, sessionId);
    if (util.isUnspecified(sess)) {
        mDoLog && WARN("[smgr]patch_session: can't update non-existent session");
        return "";
    }
    //// PSN: if user isn't a member, access denied//GEN5_TODO: confirm if this needed etc
    //var existingMember = findMemberInPlayerSess(requestHeaders.authorization, sess);
    //if (util.isUnspecified(existingMember)) {
    //    mDoLog && WARN("[smgr]put_session: can't update Sess caller isn't member of");
    //    response.statusCode = 403;
    //    return "";
    //}

    // validate entire req, players lists etc
    // This also populates required defaults for req members if unspecified, as per PSN:
    if (!validateReqPlayerSess(response, requestHeaders, reqSes, "patch_session", false, sess)) {
        return makeHttp400ErrRsp("req invalid contents");//response.statusCode set
    }

    // currently Blaze only updates the below. (poss enhancement: return appropriate PSN err code if invalidly tried to change constants)
    if ((propToUpdate === 'joinDisabled') && (sess.joinDisabled !== reqSes.joinDisabled))
        sess.joinDisabled = reqSes.joinDisabled;
    if ((propToUpdate === 'maxPlayers') && (sess.maxPlayers !== reqSes.maxPlayers))
        sess.maxPlayers = reqSes.maxPlayers;
    if ((propToUpdate === 'maxSpectators') && (sess.maxSpectators !== reqSes.maxSpectators))
        sess.maxSpectators = reqSes.maxSpectators;
    if ((propToUpdate === 'joinableUserType') && (sess.joinableUserType !== reqSes.joinableUserType))
        sess.joinableUserType = reqSes.joinableUserType;
    if ((propToUpdate === 'invitableUserType') && (sess.invitableUserType !== reqSes.invitableUserType))
        sess.invitableUserType = reqSes.invitableUserType;
    if ((propToUpdate === 'leaderPrivileges') && (sess.leaderPrivileges !== reqSes.leaderPrivileges))
        sess.leaderPrivileges = reqSes.leaderPrivileges;
    if ((propToUpdate === 'customData1') && (sess.customData1 !== reqSes.customData1))
        sess.customData1 = reqSes.customData1;
    if ((propToUpdate === 'customData2') && (sess.customData2 !== reqSes.customData2))
        sess.customData2 = reqSes.customData2;
    if ((propToUpdate === 'swapSupported') && (sess.swapSupported !== reqSes.swapSupported))
        sess.swapSupported = reqSes.swapSupported;
    if (propToUpdate === 'localizedSessionName')
        upsertLocalizedSessionName(sess, reqSes.localizedSessionName);

    response.statusCode = 200;
    mDoLog && LOG("[smgr]UPDATED SESS " + sessionId + "(" + propToUpdate + ")");
    return "";
}


// \brief Transfer leader privileges for the Player Session (PUT /v1/playerSessions/{sessionId}/leader). Reference Blaze Server psnsessionmanager.rpc's 
function put_session_leader(response, requestHeaders, requestBodyJSON, sessionId) {
    try {
        mDoLog && LOG("[smgr]UPDATING LEADER");

        // PSN spec: The access token linked to the user who is the leader of the Player Session must be specified.
        // Pre: we stashed accountId/onlineId in requestHeaders.authorization (see Blaze Server ExternalSessionUtilPs5::getAuthToken())
        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]put_session_leader: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[smgr]put_session_leader: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        if (util.isUnspecifiedOrNull(requestBodyJSON.accountId) || requestBodyJSON.accountId !== requestHeaders.authorization) {//GEN5_TODO: this  accountId !== requestHeaders.authorization check may not be needed if Sony's s2s api ends up being client credentialed
            ERR("[smgr]put_session_leader: failed req accountId(" + requestBodyJSON.accountId + ") not one in authorization token(" + requestHeaders.authorization +")");
            response.statusCode = 403;
            return "";
        }

        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            mDoLog && WARN("[smgr]put_session_leader: can't update non-existent session");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        // PSN: if user isn't a player, access denied. Note: Sony docs appear to indicate leaders cant be spectators: https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0010.html
        var existingMember = findMemberInPlayerSess(requestBodyJSON.accountId, sess);
        if (util.isUnspecified(existingMember)) {
            mDoLog && WARN("[smgr]put_session_leader: can't update Sess caller isn't member of");
            response.statusCode = 403;
            return "";
        }
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)
        
        if (util.isUnspecifiedOrNull(sess.leader) || (sess.leader.accountId !== existingMember.accountId)) {// NOOP if already
            sess['leader'] = existingMember;
        }
        response.statusCode = 204;
        mDoLog && LOG("[smgr]UPDATED LEADER");
        return "";
    } catch (err) {
        ERR("[smgr]put_session_leader: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// \brief Update Player Session member's properties (PATCH /v1/playerSessions/{sessionId}/members/{accountId}). Reference Blaze Server psnsessionmanager.rpc's, 'setPlayerSessionMemberSystemProperties' in Sony docs
function patch_session_member_properties(response, requestHeaders, requestBodyJSON, sessionId, accountId) {
    try {
        mDoLog && LOG("[smgr]UPDATING SESS MEMBER PROPS");

        if (!util.validateAuthorization(requestHeaders, response)) {
            ERR("[smgr]patch_session_member_properties: unauthorized");
            return "";//rsp set
        }
        if (util.isUnspecified(requestBodyJSON)) {
            ERR("[smgr]patch_session_member_properties: failed to get request's valid JSON body");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        // Sony docd spec as of 0.90:  For accountId, you can specify only either me, indicating the user linked to access token, or the accountId of the user linked to the access token.//GEN5_TODO: this  accountId !== requestHeaders.authorization check may not be needed if Sony's s2s api ends up being client credentialed
        if (util.isUnspecifiedOrNull(accountId) || accountId !== requestHeaders.authorization) {
            ERR("[smgr]put_session_leader: failed req accountId(" + accountId + ") not one in authorization token(" + requestHeaders.authorization + ")");
            response.statusCode = 403;
            return "";
        }

        var sess = getPlayerSessionInternal(response, sessionId);
        if (util.isUnspecified(sess)) {
            mDoLog && WARN("[smgr]put_session_leader: can't update non-existent session");
            response.statusCode = 404;
            return __HTTP404_ERR_RSP;
        }
        // PSN: if user isn't a member, access denied
        var existingMember = findMemberInPlayerSess(accountId, sess);
        if (util.isUnspecified(existingMember)) {
            mDoLog && WARN("[smgr]put_session_leader: can't update " + accountId+" not member of sess");
            response.statusCode = 403;
            return "";
        }
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)

        if (util.isUnspecified(requestBodyJSON.customData1)) {
            mDoLog && WARN("[smgr]put_session_leader: Blaze issue? attempt to update non-existent/empty player customdata1. *Blaze* impln isn't expected to hit this, hence this WARN.");
            // PSN: docs as of 0.90 state the customData1 field is optional, tho its the only thing potentially updatable on a player.//GEN5_TODO confirm behavior or dcheck docs in final Sony release: https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0009.html
            response.statusCode = 204;
            return "";
        }

        if (util.isUnspecifiedOrNull(existingMember.customData1) || (existingMember.customData1 !== requestBodyJSON.customData1)) {// NOOP if already
            existingMember['customData1'] = requestBodyJSON.customData1;
        }
        response.statusCode = 204;
        mDoLog && LOG("[smgr]UPDATED SESS MEMBER PROPS");
        return "";
    } catch (err) {
        ERR("[smgr]patch_session_member_properties: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


//////////////////////////
// 1P Session Get RPCs

// get info on a session (GET /v1/playerSessions?fields=...). Reference Blaze Server psnsessionmanager.rpc's getPlayerSession.
//  Blaze currently returns same fields regardless (fields query params ignored). 
//  (Actual poss options: GET /v1/playerSessions?fields=@default, member, member(players), member(spectators), member(players(customData1)), member(spectators(customData1)), createdTimestamp, maxPlayers, maxSpectators, joinDisabled, supportedPlatforms, sessionName, leader, joinableUserType, joinableSpecifiedUsers, invitableUserType, leaderPrivileges, customData1, customData2, swapSupported, localizedSessionName )
function get_playersession(response, requestHeaders, notFoundIsStatusOk) {
    try {
        // (Poss future enhancement: fully validate request and return errs as PSN would, here)

        //It is not currently possible to specify multiple Player Session IDs; specifying multiple IDs will result in an error.//GEN5_TODO can add validation when time
        response.statusCode = 200;
        var sessionId = requestHeaders[__SMGR_SESSIONIDS_HEADERNAME];
        if (util.isUnspecifiedOrNull(sessionId)) {
            sessionId = requestHeaders[__SMGR_SESSIONIDS_HEADERNAME.toLowerCase()];//in case lc
        }
        var sessInfo = getPlayerSessionInfoInternal(response, sessionId);//todo can replace with call to wrapping method 'getPlayerSessionInternal'

        //If a Player Session with a specified sessionId does not exist, information for that session information will not be included in the response.
        //Sony spec: GET rpc may simply return empty list if not found//GEN5_TODO: confirm this is correct PSN behavior (docs imply)
        if (notFoundIsStatusOk && (response.statusCode === 404)) {
            response.statusCode === 200;
            return ({ "playerSessions": [] });
        }
        if (response.statusCode !== 200) {
            mDoLog && WARN("[smgr]get_playersession: could not find in map sessionId: " + sessionId);
            return "";//rsp set
        }
        if (util.isUnspecifiedOrNullNested(sessInfo, 'sess')) {
            ERR("[smgr]get_playersession: internal err: found sessionId in map but sess data N/A. Check mock test code");
            response.statusCode === 500;
            return "";
        }

        return ({ "playerSessions": [sessInfo.sess] });
    } catch (err) {
        ERR("[smgr]get_playersession: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}



// helper to grab the PlayerSession out of the mPlayerSessionBySessionIdMap[sessionId] sessionInfo
function getPlayerSessionInternal(response, sessionId) {
    try {
        if (util.isUnspecified(sessionId)) {
            ERR("[smgr]getPlayerSessionInternal: failed to get request's sessionId " + sessionId);
            return "";
        }
        var sessInfo = getPlayerSessionInfoInternal(response, sessionId);
        //response.statusCode propagated up for caller to handle
        if (util.isUnspecifiedOrNullNested(sessInfo, 'sess')) {
            mDoLog && LOG("[smgr]getPlayerSessionInternal: no session:" + sessionId);
            return "";
        }
        return sessInfo.sess;
    } catch (err) {
        ERR("[smgr]getPlayerSessionInternal: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

function getPlayerSessionInfoInternal(response, sessionId) {
    try {
        if (util.isUnspecified(sessionId)) {
            ERR("[smgr]getPlayerSessionInfoInternal: failed to get request's sessionId " + sessionId);
            if (response !== null)
                response.statusCode = 400;
            return "";
        }
        if (!mPlayerSessionBySessionIdMap.hasOwnProperty(sessionId)) {
            // Sess not found, return appropriate code (see Blaze Server's PSNSESSIONMANAGER_RESOURCE_NOT_FOUND BlazeRpcError, and ExternalSessionUtilPs5::getPlayerSession())//GEN5_TODO test or confirm this
            if (response !== null)
                response.statusCode = 404;
            return "";
        }
        return mPlayerSessionBySessionIdMap[sessionId];
    } catch (err) {
        ERR("[smgr]getPlayerSessionInfoInternal: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


//////////////////////////
// More Get RPCs

const emptyActiveSessInfo = {
    "playerSessions": []
};


// \brief retrieve user's primary/active session (GET /v1/users/{accountId}/playerSessions?memberFilter=player,spectator&platformFilter=PS4,PS5). Reference Blaze Server psnsessionmanager.rpc's getPsnPlayerSessionIds.
//   (On PS5 this is currently the only active session the user is in)
// \param[in] accountId the user's online id
function get_users_playersessions(response, accountId, urlquerystring) {
    try {
        if (util.isUnspecified(accountId)) {
            mDoLog && WARN("[smgr]get_users_playersessions: request missing accountId");
            response.statusCode = 400;
            return makeHttp400ErrRsp("req invalid JSON");
        }
        //Poss enhancement: validate query string platformFilter etc

        var activeSessInfo = (mActiveSessInfoByUserMap.hasOwnProperty(accountId) ? mActiveSessInfoByUserMap[accountId] : emptyActiveSessInfo);
        activeSessInfo = (activeSessInfo === null ? emptyActiveSessInfo : activeSessInfo);
        mDoLog && LOG("[smgr]get_users_playersessions: activeSess " + (activeSessInfo === "" ? "not" : JSON.stringify(activeSessInfo)) + " found for user:" + accountId);
        response.statusCode = 200;
        return activeSessInfo;
    } catch (err) {
        ERR("[smgr]get_users_playersessions: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}

// helpers


// \brief set user's primary/active session.  (On PS5 this is currently the only active session the user is in)
function setActiveSessForUser(sessionId, userAccountId) {
    try {
        mDoLog && LOG("[smgr]setActiveSessForUser: setting activeSess(" + sessionId + ") for user:" + userAccountId);
        var activeSess = mActiveSessInfoByUserMap[userAccountId];
        if (typeof activeSess !== 'undefined') {
            if ((typeof activeSess.playerSessions === 'undefined') || (typeof activeSess.playerSessions[0] === 'undefined') || (typeof activeSess.playerSessions[0].sessionId === 'undefined')) {
                ERR("[smgr]setActiveSessForUser: existing activeSess missing sessionId, making new one for user:" + userAccountId);
            }
            else {
                if (activeSess.playerSessions[0].sessionId === sessionId)
                    return;//no op
            
                // Sony/Blaze spec: single session membership: user will be automatically removed from any prior sessions, upon advertising another
                var sessInfo = getPlayerSessionInfoInternal(null, activeSess.playerSessions[0].sessionId);//todo can replace with call to wrapping method 'getPlayerSessionInternal'
                if (!util.isUnspecified(sessInfo)) {
                    remUserFromPlayerSess(activeSess.playerSessions[0].sessionId, sessInfo.sess, userAccountId, null, false);// false here avoids clearing activeSess, see below
                }

                // For efficiency, if had a pre existing activeSess, no need to reallocate, just replace sessionId
                activeSess.playerSessions[0].sessionId = sessionId.toString();
                return;
            }        
        }
    
        // add an activeSess for user. side: Sony/Blaze spec supports at most 1 session in the 'playerSessions' list
        var newActiveSessInfo = {
            "playerSessions": [{ "platform": PsnSupportedPlatformType.PS5, "sessionId": sessionId.toString() }]
        };
        mActiveSessInfoByUserMap[userAccountId] = newActiveSessInfo;
        mDoLog && LOG("[smgr]setActiveSessForUser: successfully set activeSess(" + newActiveSessInfo.playerSessions[0].sessionId + ") for user:" + userAccountId);
    } catch (err) {
        ERR("[smgr]setActiveSessForUser: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


// \brief clear user's primary/active session
function clearActiveSessForUser(sess, userAccountId) {
    try {
        if (util.isUnspecifiedOrNull(sess.sessionId)) {
            ERR("[smgr]clearActiveSessForUser: mock service error, no op, missing valid sessionId! for user:" + userAccountId);
            return;
        }

        var activeSessInfo = mActiveSessInfoByUserMap.hasOwnProperty(userAccountId) ? mActiveSessInfoByUserMap[userAccountId] : null;
        if (activeSessInfo === null) {
            mDoLog && LOG("[smgr]clearActiveSessForUser: no op, no activeSess found for user:" + userAccountId);
            return;
        }    
        if (util.isUnspecifiedNested(activeSessInfo, 'playerSessions')) {
            ERR("[smgr]clearActiveSessForUser: mock service error, no op, as activeSess for sess(" + sess.sessionId + ") was invalid/empty for user:" + userAccountId);
            return;
        }

        var activeSessList = activeSessInfo.playerSessions;
        if (activeSessList.length > 1) {
            mDoLog && WARN("[smgr]clearActiveSessForUser: likely mock service error, expected at most 1 activeSess, actual number " + activeSessList.length + ", for user:" + userAccountId);
        }
        // remove user's activeSess
        for (var i in activeSessList) {
            if (activeSessList[i].sessionId === sess.sessionId) {
                //note: to clean up on error, if there's somehow > 1 in list (unsupported by Sony/Blaze spec), we'll remove all activities
                mDoLog && LOG("[smgr]clearActiveSessForUser: removing activeSess(" + sess.sessionId + ") for user:" + userAccountId);
                activeSessList = [];
                break;
            }
        }
        if (activeSessList.length === 0) {
            delete mActiveSessInfoByUserMap[userAccountId];//mActiveSessInfoByUserMap[userAccountId] = null;//don't try setting null for GC, won't work and causes mem growth
        }
        else {
            mDoLog && LOG("[smgr]clearActiveSessForUser: no op, as activeSess(" + sess.sessionId + ") was not mapped as primary for user:" + userAccountId);
        }
    } catch (err) {
        ERR("[smgr]clearActiveSessForUser: exception caught: " + err.message + ", stack: " + err.stack);
    }
}



//// UTIL HELPERS

// util to return index of a user in the PlayerSession's players list. returns -1 if not found in players list
function findPlayerIndexInPlayerSess(sess, userAccountId) {
    try {
        if (!util.isUnspecifiedNested(sess, 'member', 'players') && !util.isUnspecified(userAccountId)) {
            for (var k in sess.member.players) {
                if (util.isUnspecifiedNested(sess.member.players[k], 'accountId')) {
                    ERR("[smgr]findPlayerIndexInPlayerSess: internal test issue detected: sess member at index(" + k + ") was missing/invalid. Sess: " + JSON.stringify(sess));
                    continue;
                }
                if (sess.member.players[k].accountId === userAccountId) {
                    return k;
                }
            }
        }
        return -1;
    } catch (err) {
        ERR("[smgr]findPlayerIndexInPlayerSess: exception caught: " + err.message + ", stack: " + err.stack);
        return -1;
    }
}
// util to return index of a user in the PlayerSession's spectators list. returns -1 if not found in spectators list
function findSpectatorIndexInPlayerSess(sess, userAccountId) {
    try {
        if (!util.isUnspecifiedNested(sess, 'member', 'spectators') && !util.isUnspecified(userAccountId)) {
            for (var k in sess.member.spectators) {
                if (util.isUnspecifiedNested(sess.member.spectators[k], 'accountId')) {
                    ERR("[smgr]findSpectatorIndexInPlayerSess: internal test issue detected: sess member at index(" + k + ") was missing/invalid. Sess: " + JSON.stringify(sess));
                    continue;
                }
                if (sess.member.spectators[k].accountId === userAccountId) {
                    return k;
                }
            }
        }
        return -1;
    } catch (err) {
        ERR("[smgr]findSpectatorIndexInPlayerSess: exception caught: " + err.message + ", stack: " + err.stack);
        return -1;
    }
}

// util to find a member in the PlayerSession, trying first the players list, then spectators list.
function findMemberInPlayerSess(sess, userAccountId) {
    try {
        var pInd = findPlayerIndexInPlayerSess(sess, userAccountId);
        if (pInd !== -1) {
            return sess.member.players[pInd];
        }
        var sInd = findSpectatorIndexInPlayerSess(sess, userAccountId);
        if (sInd !== -1) {
            return sess.member.spectators[sInd];
        }
        return "";
    } catch (err) {
        ERR("[smgr]findMemberInPlayerSess: exception caught: " + err.message + ", stack: " + err.stack);
        return "";
    }
}


// util
function getPlayerSessPlayerCount(sessionId) {//Method may also be used for the mock system's rate limit mechanism in psnserverps5.js
    try {
        var sessInfo = getPlayerSessionInfoInternal(null, sessionId);//todo can replace with call to wrapping method 'getPlayerSessionInternal'
        if (util.isUnspecifiedOrNullNested(sessInfo, 'sess', 'member', 'players')) {
            mDoLog && LOG("[smgr]getPlayerSessPlayerCount: non-existent session players list!");
            return 0;
        }
        return sessInfo.sess.member.players.length;
    } catch (err) {
        ERR("[smgr]getPlayerSessPlayerCount: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}
function getPlayerSessSpectatorCount(sessionId) {
    try {
        var sessInfo = getPlayerSessionInfoInternal(null, sessionId);//todo can replace with call to wrapping method 'getPlayerSessionInternal'
        if (util.isUnspecifiedOrNullNested(sessInfo, 'sess', 'member', 'spectators')) {
            mDoLog && LOG("[smgr]getPlayerSessPlayerCount: non-existent session spectators list!");
            return 0;
        }
        return sessInfo.sess.member.spectators.length;
    } catch (err) {
        ERR("[smgr]getPlayerSessSpectatorCount: exception caught: " + err.message + ", stack: " + err.stack);
        return 0;
    }
}






// validates the PlayerSession for the request.
// Adds appropriate Sony spec'd defaults for unspecified/missing fields for create requests as appropriate.
function validateReqPlayerSess(response, requestHeaders, reqSes, logContext, isCreateReq, existingSess) {

    var logPrefix = (util.isUnspecifiedOrNull(logContext) ? "validateReqPlayerSess" : "(" + logContext + ")");

    // create requests require some fields. Handle checking these below:

    //supportedPlatforms
    var shouldCheckSupportedPlatforms = isCreateReq || !util.isUnspecifiedOrNull(reqSes.supportedPlatforms);//Sony create needs
    if (shouldCheckSupportedPlatforms && (!util.validateReqListIsOfStrings(response, reqSes.supportedPlatforms, 1, _PsnSupportedPlatformTypeMaxStringlen, (logContext + " supportedPlatforms")) ||
        (reqSes.supportedPlatforms.length === 0) || !isPsnSupportedPlatformType(reqSes.supportedPlatforms[0]))) {
        ERR("[smgr]" + logPrefix + ": req supportedPlatforms invalid.");
        response.statusCode = 400;
        return false;
    }
    //maxPlayers
    var shouldCheckMaxPlayersArg = isCreateReq || !util.isUnspecifiedOrNull(reqSes.maxPlayers);//Sony create needs
    if (shouldCheckMaxPlayersArg && (util.isUnspecifiedOrNull(reqSes.maxPlayers) || isNaN(reqSes.maxPlayers) || (reqSes.maxPlayers < 1) || (reqSes.maxPlayers > 100))) {
        ERR("[smgr]" + logPrefix + ": maxPlayers must be in [1,100], actual:" + (util.isUnspecifiedOrNull(reqSes.maxPlayers) ? "<n/a>" : reqSes.maxPlayers));
        response.statusCode = 400;
        return false;
    }
    //member.players (basic check. detailed checks below)
    var shouldCheckPlayersListArg = isCreateReq || !util.isUnspecifiedOrNull(reqSes.member);//Sony create needs
    if (shouldCheckPlayersListArg && (util.isUnspecifiedOrNull(reqSes.member) || !util.isArray(reqSes.member.players) ||
        (isCreateReq && reqSes.member.players.length !== 1))) {// PSN spec requires playerSessions array size to be 1 currently
        ERR("[smgr]" + logPrefix + ": failed: request's player session players count must be 1");
        response.statusCode = 400;
        return false;
    }

    // create requests auto-default some fields. Handle defaulting below:

    //maxSpectators
    if (util.isUnspecifiedOrNull(reqSes.maxSpectators)) {
        if (isCreateReq)
            reqSes.maxSpectators = 0;//Sony default
    }
    else if (isNaN(reqSes.maxSpectators) || (reqSes.maxSpectators < 0) || (reqSes.maxSpectators > 50)) {
        ERR("[smgr]" + logPrefix + ": maxSpectators must be in [0,50], actual:" + (util.isUnspecifiedOrNull(reqSes.maxSpectators) ? "<n/a>" : reqSes.maxSpectators));
        response.statusCode = 400;
        return false;
    }
    //joinableUserType
    if (util.isUnspecifiedOrNull(reqSes.joinableUserType)) {
        if (isCreateReq)
            reqSes.joinableUserType = PsnJoinableUserType.FRIENDS_OF_FRIENDS;//Sony default
    }
    else if (!isPsnJoinableUserType(reqSes.joinableUserType)) {
        ERR("[smgr]" + logPrefix + ": req joinableUserType invalid");
        response.statusCode = 400;
        return false;
    }
    //joinDisabled
    if (util.isUnspecifiedOrNull(reqSes.joinDisabled)) {
        if (isCreateReq)
            reqSes.joinDisabled = false;//Sony default
    }
    else if (reqSes.joinDisabled !== true && reqSes.joinDisabled !== false) {
        ERR("[smgr]" + logPrefix + ": req joinDisabled invalid");
        response.statusCode = 400;
        return false;
    }
    //swapSupported
    if (util.isUnspecifiedOrNull(reqSes.swapSupported)) {
        if (isCreateReq)
            reqSes.swapSupported = false;//Sony default
    }
    else if (reqSes.swapSupported !== true && reqSes.swapSupported !== false) {//GEN5_TODO: implement actually using this: PSN spec:     Flag indicating whether members who have joined can switch from players to spectators, or from spectators to players, without leaving. When true, swapping is allowed. (Users can swap using the system software UI.)When false, swapping is not allowed. 
        ERR("[smgr]" + logPrefix + ": req swapSupported invalid");
        response.statusCode = 400;
        return false;
    }
    //leaderPrivileges
    if (util.isUnspecifiedOrNull(reqSes.leaderPrivileges)) {
        if (isCreateReq)
            reqSes.leaderPrivileges = ["KICK", "UPDATE_JOINABLE_USER_TYPE"];//Sony default
    }
    else if (!util.validateReqListIsOfStrings(response, reqSes.leaderPrivileges, 1, 2, "leaderPrivileges")) {//GEN5_TODO: implement actually using this: PSN spec:     Flag indicating whether members who have joined can switch from players to spectators, or from spectators to players, without leaving. When true, swapping is allowed. (Users can swap using the system software UI.)When false, swapping is not allowed. 
        ERR("[smgr]" + logPrefix + ": req leaderPrivileges invalid");
        response.statusCode = 400;
        return false;
    }
    //customData1
    if (util.isUnspecifiedOrNull(reqSes.customData1)) {
        if (isCreateReq)
            WARN("[smgr]" + logPrefix + ": req customData1 missing. For Blaze, poss error!");
    }
    else if (!util.isString(reqSes.customData1)) {
        ERR("[smgr]" + logPrefix + ": req customData1 invalid");
        response.statusCode = 400;
        return false;
    }
    //customData2
    if (util.isUnspecifiedOrNull(reqSes.customData2)) {
        //this is ok
    }
    else if (!util.isString(reqSes.customData2)) {
        ERR("[smgr]" + logPrefix + ": req customData2 invalid");
        response.statusCode = 400;
        return false;
    }
    //localizedSessionName
    if (!validateReqLocalizedSessionName(response, reqSes.localizedSessionName, logContext, isCreateReq)) {
        ERR("[smgr]" + logPrefix + ": req localizedSessionName validation failed. req value was: " + JSON.parse(reqSes.localizedSessionName).toString());
        return false;
    }
    //member.players (detailed, deep checks)
    if (!util.isUnspecifiedOrNullNested(reqSes, 'member', 'players') /*&& (reqSes.member.players.length > 0)*/) {

        // PSN docd specs when trying to join, check req:
        if (reqSes.joinDisabled === true) {
            response.statusCode = 400;
            ERR("[smgr]" + logPrefix + ": cannot join, when joinDisabled true in req");
            return false;
        }

        var maxPlayersToCheck = (!util.isUnspecifiedOrNull(reqSes.maxPlayers) ? reqSes.maxPlayers : (util.isUnspecifiedOrNull(existingSess) ? null : existingSess.maxPlayers));

        if (!validateReqMembersList(response, requestHeaders, reqSes.member.players, logContext, maxPlayersToCheck, existingSess, false)) {
            ERR("[smgr]" + logPrefix + ": req players list validation failed");
            return false;
        }
    }
    //member.spectators (detailed, deep checks)
    if (!util.isUnspecifiedOrNullNested(reqSes, 'member', 'spectators') && (reqSes.member.spectators.length > 0)) {

        if (reqSes.joinDisabled === true) {
            response.statusCode = 400;
            ERR("[smgr]" + logPrefix + ": cannot join, when joinDisabled true in req");
            return false;
        }

        var maxSpectatorsToCheck = (!util.isUnspecifiedOrNull(reqSes.maxSpectators) ? reqSes.maxSpectators : (util.isUnspecifiedOrNull(existingSess) ? null : existingSess.maxSpectators));

        if (!validateReqMembersList(response, requestHeaders, reqSes.member.spectators, logContext, maxSpectatorsToCheck, existingSess, true)) {
            ERR("[smgr]" + logPrefix + ": req spectators list validation failed");
            return false;
        }       
    }

    return true;
}

function validateReqLocalizedSessionName(response, reqLocalizedSesName, logPrefix, isCreateReq) {

    var shouldCheckLocalizedSessionName = isCreateReq || !util.isUnspecifiedOrNull(reqLocalizedSesName);//Sony create needs
    if (!shouldCheckLocalizedSessionName) {
        return true;
    }
    if (isCreateReq && util.isUnspecifiedOrNull(reqLocalizedSesName)) {
        ERR("[smgr]" + logPrefix + ": req localizedSessionName missing. Sony requires for create");
        response.statusCode = 400;
        return false;
    }
    if (!util.isString(reqLocalizedSesName.defaultLanguage) || reqLocalizedSesName.defaultLanguage.length < 1) {
        ERR("[smgr]" + logPrefix + ": req localizedSessionName.defaultLanguage invalid");
        response.statusCode = 400;
        return false;
    }
    if (util.count(reqLocalizedSesName.localizedText) < 1) {//GEN5_TODO: verify whether Sony really doesn't allow empty names. If that's true, and GameName is empty, then at least add a dummy name in the create call
        ERR("[smgr]" + logPrefix + ": req localizedSessionName.localizedText array invalid/empty");
        response.statusCode = 400;
        return false;
    }
    for (var k in reqLocalizedSesName.localizedText) {
        if (!util.isString(k) || k.length < 1) {
            ERR("[smgr]" + logPrefix + ": req localizedSessionName.localizedText has an invalid/empty key string");
            response.statusCode = 400;
            return false;
        }
        //var reqTextItem = reqLocalizedSesName.localizedText[k];
        //if (!util.isString(reqTextItem) || reqTextItem.length < 1) {//GEN5_TODO: verify whether Sony really doesn't allow empty names. If that's true, and GameName is empty, then at least add a dummy name in the blaze server side create call
        //    ERR("[smgr]" + logPrefix + ": req localizedSessionName.localizedText has an invalid/empty value string");
        //    response.statusCode = 400;
        //    return false;
        //}
    }
    return true;
}


// Validate PSN docd specs when trying to join a players or spectators list
// To mimic PSN, this converts 'me' to appropriate accountId in list as needed
// \param[in] existingSess If there was an existing session, pass it in, to validate against it. Else null.
function validateReqMembersList(response, requestHeaders, reqMembersList, logContext, maxMembers, existingSess, asSpectator) {
    var logPrefix = (util.isUnspecifiedOrNull(logContext) ? "validateReqMembersList" : "(" + logContext + ")");

    if (!util.isArray(reqMembersList)) {
        response.statusCode = 400;
        ERR("[smgr]" + logPrefix + ": players list not valid array in req");
        return false;
    }
    if (!(reqMembersList.length > 0)) {
        response.statusCode = 400;
        ERR("[smgr]" + logPrefix + ": players list empty req");//at least for Blaze this should never happen
        return false;
    }
    // (note checks for already-joined done in addUsers method)
    // GEN5_TODO: should for completeness check vs dupes up front
    
    var existingSessPlayerCount = (util.isUnspecifiedOrNull(existingSess) ? 0 :
        (asSpectator ? getPlayerSessSpectatorCount(existingSess.sessionId) : getPlayerSessPlayerCount(existingSess.sessionId)));

    if (maxMembers < existingSessPlayerCount + reqMembersList.length) {
        response.statusCode = 400;
        ERR("[smgr]" + logPrefix + ": cannot add (" + reqMembersList.length + ") user(s) due to maxMembers");
        return false;
    }

    // validate each member
    for (var i in reqMembersList) {

        //to mimic PSN, this converts 'me' to appropriate accountId up front here:
        if (!validateReqMember(response, reqMembersList[i], requestHeaders.authorization, existingSess, asSpectator)) {
            ERR("[smgr]" + logPrefix + ": a request's user to add invalid");
            return false;//rsp set
        }
    }
    return true;
}

// validates request player/spectator. Also, will replace 'me' entries with actual account ids, as appropriate//GEN5_TODO done for now, but can validate in more detail when time
// \param[in] existingSess If there was an existing session, pass it in, to validate against it. Else null.
function validateReqMember(response, reqMember, callerId, existingSess, asSpectator) {

    //GEN5_TODO: BLOCK LIST ETC https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0010.html : "The user linked to the access token must not have been kicked out of the requested session in the past- The users linked to the access token must not be included in the block list of a member participating in the requested Player Session- A member participating in the requested Player Session must not be included in the block list of the user linked to the access token"

    if (util.isUnspecifiedOrNullNested(reqMember, 'accountId') || !util.isString(reqMember.accountId)) {
        ERR("[smgr]validateReqMember: request's players must have valid accountId");
        response.statusCode = 400;
        return false;
    }
    if (util.isUnspecifiedOrNull(reqMember.platform) || !isPsnSupportedPlatformType(reqMember.platform)) {
        ERR("[smgr]validateReqMember: request's players must have valid platform");
        response.statusCode = 400;
        return false;
    }
    if (!util.isArray(reqMember.pushContexts) || reqMember.pushContexts.length !== 1 || !util.isString(reqMember.pushContexts[0].pushContextId)) {//sony docs state can only req w/1
        ERR("[smgr]validateReqMember: (Blaze) request's members must have one valid push context");//Sony docs don't technically require this, but for *Blaze* we should always have push context ids
        response.statusCode = 400;
        return false;
    }

    if (!util.isUnspecifiedOrNull(existingSess)) {

        if (existingSess.joinDisabled === true) {
            response.statusCode = 400;//GEN5_TODO: dcheck code
            ERR("[smgr]validateReqMember: cannot join, when joinDisabled true");
            return false;
        }

        if (asSpectator === true) {

            // PSN docd specs when trying to switch 'player' to 'spectator':
            var pInd = findPlayerIndexInPlayerSess(existingSess, (reqMember.accountId === "me" ? callerId : reqMember.accountId));
            if (pInd !== -1) {
                if (existingSess.swapSupported === false) {
                    response.statusCode = 400;
                    ERR("[smgr]validateReqMember: cannot switch a player to spectator, when swapSupported false");
                    return false;
                }
                // The (to-be-spectator) must not be the leader https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0011.html
                if (existingSess.leader.accountId === accountId) {
                    response.statusCode = 403;
                    ERR("[smgr]validateReqMember: cannot switch a leader to spectator");
                    return false;
                }
                // The (to-be-spectator) must not be the only 'player' left https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0011.html
                if (getPlayerSessPlayerCount(existingSess.sessionId) === 1) {
                    response.statusCode = 400;//GEN5_TODO: dcheck code
                    ERR("[smgr]validateReqMember: cannot switch a user to spectator if its the last 'player'");
                    return false;
                }   
            }
        }
        else { //as player

            // PSN docd specs when trying to switch 'spectator' to 'player':
            var sInd = findSpectatorIndexInPlayerSess(existingSess, (reqMember.accountId === "me" ? callerId : reqMember.accountId));
            if (sInd !== -1) {
                if (existingSess.swapSupported === false) {
                    response.statusCode = 400;//GEN5_TODO: dcheck code
                    ERR("[smgr]validateReqMember: cannot switch a spectator to player, when swapSupported false");
                    return false;
                }
            }
        }
    }

    if (reqMember.accountId === "me") {
        reqMember.accountId = callerId;
    }
    return true;
}




module.exports.post_playersession = post_playersession;
module.exports.post_playersession_player = post_playersession_player;
module.exports.post_playersession_spectator = post_playersession_spectator;
module.exports.post_playersession_joinableusers = post_playersession_joinableusers;
module.exports.patch_session = patch_session;
module.exports.delete_playersession_member = delete_playersession_member;
module.exports.delete_playersession_joinableusers = delete_playersession_joinableusers;
module.exports.put_session_leader = put_session_leader;
module.exports.patch_session_member_properties = patch_session_member_properties;
module.exports.get_playersession = get_playersession;
module.exports.get_users_playersessions = get_users_playersessions;
module.exports.setMaxOpCountBeforeReport = setMaxOpCountBeforeReport;
module.exports.setDoLogSessions = setDoLogSessions;
//module.exports.getPlayerSessPlayerCount = getPlayerSessPlayerCount;

