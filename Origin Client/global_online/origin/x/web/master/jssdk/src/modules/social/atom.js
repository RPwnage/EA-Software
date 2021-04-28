/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
    'core/utils'
], function(Promise, logger, user, dataManager, urls, errorhandler, utils) {

    /**
     * @module module:atom
     * @memberof module:Origin
     */
    function processAtomUserInfoResponse(response) {
        //if only one user was specified, then it returns the response as users.user.avatar
        //but if more than one user was specified, it returns the response as users.user[].avatar
        //so we need to convert the single user case into returning it as an array (of one element) too.
        if (!(response.users.user instanceof Array)) {
            response.users.user = [response.users.user];
        }

        return response.users.user;
    }

    function processAtomGameUsageResponse(response) {
        return response.usage;
    }

    function processAtomLastPlayedResponse(response) {
        var returnObj = response.lastPlayedGames.lastPlayed;

        //when converting from xml to JSON we don't figure out its an array so we check here
        if (Object.prototype.toString.call(response.lastPlayedGames.lastPlayed) !== '[object Array]') {
            returnObj = [response.lastPlayedGames.lastPlayed];
        }

        return returnObj;
    }

    function processAtomGamesOwnedForUserResponse(response) {
        if (!!response && !!response.productInfoList && !!response.productInfoList.productInfo) {
            if (!(response.productInfoList.productInfo instanceof Array)) {
                response.productInfoList.productInfo = [response.productInfoList.productInfo];
            }
            return response.productInfoList.productInfo;
        } else {
            return [];
        }
    }

    function processAtomFriendsForUserResponse(response) {
        if (!!response && !!response.users && !!response.users.user) {
            if (!(response.users.user instanceof Array)) {
                response.users.user = [response.users.user];
            }
            return response.users.user;
        } else {
            return [];
        }
    }

    return /** @lends module:Origin.module:atom */ {

        /**
         * user info for a single user
         * @typedef atomUserInfoObject
         * @type {object}
         * @property {string} userId
         * @property {string} personaId
         * @property {string} originId
         * @property {string} firstName
         * @property {string} lastName
         */



        /**
         * This will return a promise for the atom user info for each user in the userId list
         *
         * @param {Object} list of userIds, separate by ;
         * @return {promise<module:Origin.module:atom~atomUserInfoObject[]>} name array of atomUserInfoObjects
         */
        atomUserInfoByUserIds: function(userIdList) {

            var endPoint = urls.endPoints.atomUsers;

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userIdList',
                    'val': userIdList
                }],
                appendparams: [],
                reqauth: true,
                requser: false
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processAtomUserInfoResponse, errorhandler.logAndCleanup('ATOM:atomUserInfoByUserIds FAILED'));

        },

        /**
         * game usage info
         * @typedef atomGameUsageObject
         * @type {object}
         * @property {string} gameId
         * @property {string} total
         * @property {string} lastSession
         * @property {string} lastSessionEndTimeStamp in epoch time
         */

        /**
         * This will return a promise for the atom user info for each user in the userId list
         *
         * @param {String} masterTitleId
         * @param {String} multiplayerId
         * @return {promise<module:Origin.module:atom~atomGameUsageObject[]>} name array of atomUserInfoObjects
         */
        atomGameUsage: function(masterTitleId, multiplayerId) {
            var endPoint = urls.endPoints.atomGameUsage;

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'masterTitleId',
                    'val': masterTitleId
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            if (multiplayerId) {
                config.headers.push({
                    'label': 'MultiplayerId',
                    'val': multiplayerId
                });
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processAtomGameUsageResponse, errorhandler.logAndCleanup('ATOM:processAtomGameUsageResponse FAILED'));
        },

        /**
         * game lastplayed info
         * @typedef atomGameLastPlayedObject
         * @type {object}
         * @property {string} userId
         * @property {string} masterTitleId
         * @property {string} timestamp
         */

        /**
         * This will return a promise for the last played game of the current user
         *
         * @return {promise<module:Origin.module:atom~atomGameLastPlayedObject>}  atomGameLastPlayedObject
         */
        atomGameLastPlayed: function() {
            var endPoint = urls.endPoints.atomGameLastPlayed;

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processAtomLastPlayedResponse, errorhandler.logAndCleanup('ATOM:processAtomLastPlayedResponse FAILED'));

        },


        /**
         * owned games for user
         * @typedef atomGamesOwnedForUserObject
         * @type {object}
         * @property {string} productId
         * @property {string} displayProductName
         * @property {string} cdnAssetRoot
         * @property {string} packArtSmall
         * @property {string} packArtMedium
         * @property {string} packArtLarge
         */

        /**
         * This will return a promise for the games owned by the specified userId
         *
         * @param {String} userId
         * @return {promise<module:Origin.module:atom~atomGamesOwnedForUserObject[]>} name array of atomGamesOwnedForUserObjects
         */
        atomGamesOwnedForUser: function(userId) {
            var endPoint = urls.endPoints.atomGamesOwnedForUser;

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'otherUserId',
                    'val': userId
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processAtomGamesOwnedForUserResponse, errorhandler.logAndCleanup('ATOM:processAtomGamesOwnedForUserResponse FAILED'));
        },

        /**
         * friends for user
         * @typedef atomFriendsForUserObject
         * @type {object}
         * @property {string} userId
         * @property {string} personaId
         * @property {string} EAID
         * @property {string} firstName (optional)
         * @property {string} lastName (optional)
         */

        /**
         * This will return a promise for the friends of the specified userId
         *
         * @param {String} userId
         * @return {promise<module:Origin.module:atom~atomFriendsForUserObject[]>} name array of atomFriendsForUserObjects
         */
        atomFriendsForUser: function(userId) {
            var endPoint = urls.endPoints.atomFriendsForUser;

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'otherUserId',
                    'val': userId
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processAtomFriendsForUserResponse, errorhandler.logAndCleanup('ATOM:processAtomFriendsForUserResponse FAILED'));
        },

        /**
         * This will return a promise for the user abuse report response
         *
         * @param {String} userId userId to report
         * @param {String} location Location of the offense (ie "In Game")
         * @param {String} reason Reason for the report (ie "Cheating")
         * @param {String} comment User-specified comment (optional)
         * @param {String} masterTitle Master Title of the game that the offense occured if it occurred in-game (optional)
         * @return {promise} response raw response
         */
        atomReportUser: function(userId, location, reason, comment, masterTitle) {
            var endPoint = urls.endPoints.atomReportUser;

            var config = {
                atype: 'POST',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'otherUserId',
                    'val': userId
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            comment = !!comment ? comment : '';
            masterTitle = !!masterTitle ? masterTitle : '';

            var contentTypeEl = ' <contentType>%1</contentType>\n'.replace('%1', location);
            var reportReasonEl = ' <reportReason>%1</reportReason>\n'.replace('%1', reason);
            var commentsEl = comment.length ? ' <comments>%1</comments>\n'.replace('%1', comment) : '';
            var locationEl = masterTitle.length ? ' <location>%1</location>\n'.replace('%1', masterTitle) : '';

            var reportUser = '<reportUser>\n%1%2%3%4</reportUser>'.replace('%1', contentTypeEl)
                .replace('%2', reportReasonEl)
                .replace('%3', commentsEl)
                .replace('%4', locationEl);

            config.body = reportUser;

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            // No response processing function needed here
            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('ATOM:processAtomReportUserResponse FAILED'));
        },
        /**
         * Gets a list of mastertitle ids a user owns
         * @param  {string} friendsIds a string of common deliminated nucleus ids (max 5)
         * @return {promise} retVal returns a promise that resovles with an array of objects containing masterTitleIds
         */
        atomCommonGames: function(friendsIds) {
            var endPoint = urls.endPoints.atomCommonGames;

            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'friendsIds',
                    'val': friendsIds
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('ATOM:atomCommonGames FAILED'));
        }



    };
});