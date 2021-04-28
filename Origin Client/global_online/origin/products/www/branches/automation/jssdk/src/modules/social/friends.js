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
       * @typedef PlayHistory
       * @type {object}
       * @property {string} game title of the game
       * @property {number} no_of_matches the number of matches played
       * @property {number} most_recent_ts timestamp of most recently played game.
       */
      /**
         * @typedef FriendsReasonObject
         * @type {object}
         * @property {array} mf an array of mutual friends nucleus ids
         * @property {module:Origin.module:friends~PlayHistory[]} play_history an array of play histories
         */
        /**
         * @typedef RecommendedFriendObject
         * @type {object}
         * @property {number} id nucleus id of recommended friends
         * @property {number} wt weight of the recommendation
         * @property {module:Origin.module:friends~FriendsReasonObject[]} reasons a reason object telling you why the friend was recommended
         */
        /**
         * @typedef FriendsObject
         * @type {object}
         * @property {module:Origin.module:friends~RecommendedFriendObject[]} recs array of recommended friends
         * @property {number} page_size number of records returned in the current response
         * @property {boolean} more_recs a flag indicating if there are more records which can be retrieved for the user
         */
        /**
         * This will return a list of friend recommendations from EDAP Social API
         * @see {@link https://developer.ea.com/pages/viewpage.action?pageId=163746092|See EADP Social API doc}
         * @param {number} start start page Number. default is 1
         * @param {number} pageSize the size of the page. default is 10
         * @return {module:Origin.module:friends~FriendsObject} return object containing recommended friends info
         */
        friendRecommendations: function(start, pageSize) {

            var endPoint = urls.endPoints.friendRecommendation;

            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                  'label': 'pagestart',
                  'val': start ? start : 1
                },{
                  'label': 'pagesize',
                  'val': pageSize ? pageSize : 10
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'Authorization', 'Bearer ' + token);
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
                atype: 'DELETE',
                headers: [{
                    'label': 'Content-Type',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label' : 'disableUserId',
                    'val' : nucleusIds
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'Authorization', 'Bearer ' + token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('Friends:friendIngoreRecommendation FAILED'));

        },

        filter: filterEnums
    };
});
