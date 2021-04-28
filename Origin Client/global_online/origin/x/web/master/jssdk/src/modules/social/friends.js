/*jshint unused: false */
/*jshint strict: false */

define([
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(logger, user, dataManager, urls, errorhandler) {

    /**
     * @module module:friends
     * @memberof module:Origin
     */
    var filterEnums = {
        FRIEND_OF_FRIEND: 1,
        BASED_ON_GAMES_PLAYED: 2,
        LOCATION_BASED: 3,
        BASED_ON_NUCLEUS_DATA: 4
    };

    return /** @lends module:Origin.module:friends */ {

        /**
         * @typedef GOSFriendsReasonObject
         * @type {object}
         * @property {number} r the reason id
         * @property {string} rp a string of comma deliminated values depending on the reason
         * @property {number} w he weight of this reason
         */
        /**
         * @typedef GOSRecommendedFriendObject
         * @type {object}
         * @property {number} id nucleus id of recommended friends
         * @property {module:Origin.module:friends~GOSFriendsReasonObject[]} rs an array of reason objects telling you why the friend was recommended
         * @property {number} s the score rating for the friend rec
         */
        /**
         * @typedef GOSFriendsObject
         * @type {object}
         * @property {module:Origin.module:friends~GOSRecommendedFriendObject[]} rfs array of recommended friends
         * @property {number} _id nucleusId of the user
         */
        /**
         * This will return a list of friend recommendations from the GOS server
         * @return {module:Origin.module:friends~GOSFriendsObject} returnname object containing recommended friends info
         */
        friendRecommendations: function() {

            var endPoint = urls.endPoints.friendRecommendation;

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
                .catch(errorhandler.logAndCleanup('Friends:friendRecommendations FAILED'));

        },
        /**
         * Takes in a list of nucleus ids and adds them to a ignore list for friends recommendations
         * @param  {string} nucleusIds a single/or array of nucleus ids
         * @return {promise} retVal resolves when the post call has completed
         */
        friendRecommendationsIgnore: function(nucleusIds) {

            var endPoint = urls.endPoints.friendRecommendationIgnore;

            var config = {
                atype: 'POST',
                headers: [{
                    'label': 'Content-Type',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            config.body = JSON.stringify({
                ignores: Array.isArray(nucleusIds) ? Number(nucleusIds) : [Number(nucleusIds)]
            });

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('Friends:friendIngoreRecommendation FAILED'));

        },

        filter: filterEnums
    };
});
