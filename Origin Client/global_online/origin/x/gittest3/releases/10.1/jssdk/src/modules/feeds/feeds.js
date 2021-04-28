/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/dataManager',
    'core/urls',
    'core/config',
    'core/errorhandler',
], function(Promise, dataManager, urls, config, errorhandler) {



    /**
     * handles the story response
     * @param  {object} story response
     */
    function processResponseStories(response) {
        if (typeof response.stories !== 'undefined') {
            return response.stories;
        } else {
            return errorhandler.promiseReject('unexpected response from retrieveFeeds');
        }
    }

    /**
     * This will return a promise for the requested feed
     *
     * @param {String} feedtype
     * @param {String} index of the feed
     * @return {promise}  response dependent on feedtype but returns an array of responsetype
     * @method
     */
    function retrieveStoryData(feedType, startingNdx, numStories, locale) {

        //url is composited
        //{env}.{feedType}.feeds.dm.origin.com/stories/{startinIndex}/
        //and if numStories is defined and not 1, then size=numStories is appended
        var env = config.environment();
        var endPoint = urls.endPoints.feedStories;

        //we need to substitute
        endPoint = endPoint.replace('{locale}', locale);
        endPoint = endPoint.replace('{env}', env);
        endPoint = endPoint.replace(/{feedType}/g, feedType); //replace all occurrences
        endPoint = endPoint.replace('{index}', startingNdx);

        if (typeof numStories !== 'undefined') {
            endPoint += '/?size=' + numStories;
        }

        var requestConfig = {
            atype: 'GET',
            headers: [],
            parameters: [],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        return dataManager.enQueue(endPoint, requestConfig, 0)
            .then(processResponseStories)
            .catch(errorhandler.logAndCleanup('FEEDS:retrieveStoryData FAILED')); //added cause we want to trap the reject from processResponseStories

    }

    /** @namespace
     * @memberof Origin
     * @alias feeds
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
         * This will return a promise for the requested feed
         *
         * @param {String} feedtype
         * @param {String} index of the feed
         * @return {promise}  response dependent on feedtype but returns an array of responsetype
         * @method
         */
        retrieveStoryData: retrieveStoryData
    };
});