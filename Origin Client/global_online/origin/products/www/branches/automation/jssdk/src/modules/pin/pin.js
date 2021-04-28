/*jshint strict: false */
/*jshint unused: false*/
define([
    'core/user',
    'core/dataManager',
    'core/utils',
    'core/urls'
], function(user, dataManager, utils, urls) {

    /**
     * @module module:pin
     * @memberof module:Origin
     */

    /**
     * user info for a single user
     * @typedef trackingRecommendationObject
     * @type {object}
     * @property {string} recoid the id used to identify the recommendation, ctids for news, project id for games
     * @property {string} recoindex the position of the tile in the recommendation (0 = first rec)
     */

    var PIN_EM_CONFIG_DEFAULTS = {
            atype: 'POST',
            headers: [{
                'label': 'Accept',
                'val': 'application/json'
            }, {
                'label': 'Content-Type',
                'val': 'application/json'
            }]
        },
        PIN_EM_BODY_DEFAULTS = {
            clientId: 'origin',
            tId: '196775',
            tIdT: 'projectid'
        },
        GAMEID = '901002',
        DEFAULTS_RECS = 4,
        ORIGIN_NEWS = 'origin news',
        ORIGIN_GAMES = 'origin games',
        NEWS = 'news',
        GAMES = 'games',
        CLICKS = 'clicks',
        IMPRESSIONS = 'impressions';


    /**
     * get recommendations from the pin service
     * @param  {string} recommendationType is this a ORIGIN_NEWS or ORIGIN_GAME recommendation
     * @param  {string[]} inputIds ids used in tracking recommendation, project ids for games ,ctids for news
     * @param  {number} numItems maximum number of recommendations to be returned by api
     * @return {promise} promise that will execute the recommendation api with the parameters
     */
    function getRecommendation(recommendationType, inputIds, numItems) {
        //for recommendations we always want to use pc platform, reco engine currently has no
        //concept of mac
        var endPoint = (recommendationType === ORIGIN_GAMES ? urls.endPoints.pinemPCRecoGames : urls.endPoints.pinemPCRecoNews),
            config = {},
            body = {
                gameId: GAMEID,
                state: {
                    'item_list': inputIds,
                    'num_items': numItems || DEFAULTS_RECS
                },
                pidMap: {
                    nucleus: user.publicObjs.userPid()
                },
                recommendations: [recommendationType]
            };

        utils.mix(config, PIN_EM_CONFIG_DEFAULTS);
        utils.mix(body, PIN_EM_BODY_DEFAULTS);

        config.body = JSON.stringify(body);

        return dataManager.enQueue(endPoint, config);
    }

    /**
     * send information back to the pin recommendation service
     * @param  {string} impressionType          is this a click or impression
     * @param  {string} trackingTagArray        the tracking tags from the recommendation responses
     * @param  {trackingRecommendationObject[]} trackingIdsAndIndices ids/index object used in tracking recommendation, (id = project ids for games ,ctids for news)
     * @return {promise}                        promise that will execute the tracking api with the parameters
     */
    function trackRecommendations(impressionType, trackingTagArray, trackingIdsAndIndices) {
        var endPoint = (impressionType === 'clicks' ? urls.endPoints.pinemTrackClicks : urls.endPoints.pinemTrackImpressions);
        var config = {},
            body = {
                pidMap: {
                    nucleus: user.publicObjs.userPid().toString()
                },
                'tracking-tag-list': trackingTagArray,
                dataList: trackingIdsAndIndices
            };

        utils.mix(config, PIN_EM_CONFIG_DEFAULTS);
        utils.mix(body, PIN_EM_BODY_DEFAULTS);
        config.body = JSON.stringify(body);

        return dataManager.enQueue(endPoint, config);
    }



    return /** @lends module:Origin.module:pin */ {
        /**
         * pass in a set of ctids and return a filterd and ordered set back from the pin recommendation service for news
         *
         * @param  {string[]} ctids used in getting recommendation
         * @param  {number} numItems maximum number of recommendations to be returned by api
         * @return {promise} retval promise that will execute the recommendation api with the parameters and resolve with a recommendation object
         * @method
         */
        getNewsRecommendation: getRecommendation.bind(this, ORIGIN_NEWS),
        /**
         * called to track news impression
         * @param  {string} trackingTag             the tracking tag from the recommendation response
         * @param  {trackingRecommendationObject[]} trackingIdsAndIndices ids/index object used in tracking recommendation, (id = project ids for games ,ctids for news)
         * @return {promise}                        retval promise that will execute the tracking api with the parameters with no return object
         * @method
         */
        trackRecommendations: trackRecommendations,
 
        /**
         * pass in a set of project ids and return a filterd and ordered set back from the pin recommendation service for games
         *
         * @param  {string[]} project ids used in getting recommendation
         * @param  {number} numItems maximum number of recommendations to be returned by api
         * @return {promise} retval promise that will execute the recommendation api with the parameters and resolve with a recommendation object
         * @method
         */
        getGamesRecommendation: getRecommendation.bind(this, ORIGIN_GAMES),
        /**
         * constants used when calling tracking API
         * @type {Object}
         */
        constants: {
            /**
             * equivalent to 'news' 
             * @type {string}
             */
            NEWS: NEWS,
            /**
             * equivalent to 'games' 
             * @type {string}
             */            
            GAMES: GAMES

        }
    };
});