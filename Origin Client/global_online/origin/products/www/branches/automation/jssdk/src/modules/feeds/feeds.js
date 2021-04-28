/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(Promise, dataManager, urls, errorhandler) {

    /**
     * @module module:feeds
     * @memberof module:Origin
     */

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

    function retrieveStoryData(feedType, startingNdx, numStories, locale) {

        //url is composited
        //{env}.{feedType}.feeds.dm.origin.com/stories/{startinIndex}/
        //and if numStories is defined and not 1, then size=numStories is appended
        var endPoint = urls.endPoints.feedStories;

        //we need to substitute
        endPoint = endPoint.replace('{locale}', locale);
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

    return/** @lends module:Origin.module:feeds */ {

        /**
         * This will return a promise for the requested feed from ({env}.{feedType}.feeds.dm.origin.com/stories/{startinIndex}/)
         *
         * @param  {string} feedType    The feed type
         * @param  {number} startingNdx The starting index
         * @param  {number} numStories  Num stories to retrieve
         * @param  {string} locale      The locale
         * @return {promise} name response dependent on feedtype but returns an array of responsetype
         * @method
         */
        retrieveStoryData: retrieveStoryData
    };
});