/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(Promise, logger, user, dataManager, urls, errorhandler) {

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

    /** @namespace
     * @memberof Origin
     * @alias atom
     */
    return {

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
         * @return {promise<atomUserInfoObject[]>}  array of atomUserInfoObjects
         */
        atomUserInfoByUserIds: function(userIdList) {

            var endPoint = urls.endPoints.atomUsers;

            var config = {
                atype: 'GET',
                headers: [],
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
         * @return {promise<atomGameUsageObject[]>}  array of atomUserInfoObjects
         */
        atomGameUsage: function(masterTitleId, multiplayerId) {
            var endPoint = urls.endPoints.atomGameUsage;

            var config = {
                atype: 'GET',
                headers: [],
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
         * @return {promise<atomGameLastPlayedObject>}  atomGameLastPlayedObject
         */
        atomGameLastPlayed: function() {
            var endPoint = urls.endPoints.atomGameLastPlayed;

            var config = {
                atype: 'GET',
                headers: [],
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
    };
});