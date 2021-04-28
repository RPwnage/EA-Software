/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to online status with the C++ client
 */


define([
    'core/urls',
    'core/logger',
    'core/beacon',
    'core/dataManager'
], function(urls, logger, beacon, dataManager) {
    /**
     * Contains client remote communication methods
     * @module module:remote
     * @memberof module:Origin.module:client
     * @private
     *
     */

    /**
     * the local host version of the command
     * @param  {object} config   configuration information for the command
     * @param  {string} endpoint endpoint for the command
     */
    function localHostCommand(config, endpoint) {
        return function() {
            return dataManager.dataREST(endpoint, config);
        };
    }

    /**
     * the origin2 version of the command
     * @param  {object} config   configuration information for the command
     * @param  {string} endpoint endpoint for the command
     */

    function origin2Command(config, endpoint) {
        return function() {
            config.parameters.forEach(function(param) {
                endpoint = endpoint.replace('{' + param.label + '}', param.val);
            });

            window.location.href = endpoint;

            //return a promise to keep consistent with local host call
            return Promise.resolve();
        };
    }

    /**
     * checks to see if origin client is running if it is then issue local host command
     * otherwise use origin2
     * @param  {object} config   configuration information for the command
     * @param  {string} origin2url the origin2:// endpoint for the command
     * @param  {string} localHostUrl the localhost endpoint for the command
     */
    function remoteCommand(config, localHostUrl, origin2Url) {
        return beacon.running()
            .then(localHostCommand(config, localHostUrl))
            .catch(origin2Command(config, origin2Url));
    }

    /**
     * launches a game remotely, initiates a download if the game is not installed and the autodownload flag is set to 1
     * @param {string} offerIds a comma delimited list of offers to look for
     * @param {boolean} autoDownload true if you want to download if not installed
     */
    function gameLaunch(offerIds, autoDownload) {
        var autoDownloadVal = autoDownload ? 1 : 0,
            config = {
                atype: 'POST',
                headers: [],
                parameters: [{
                    'label': 'offerIds',
                    'val': offerIds
                }, {
                    'label': 'autoDownload',
                    'val': autoDownloadVal
                }],
                reqauth: false,
                requser: false,
                body: ''
            };

        return remoteCommand(config, urls.endPoints.localHostClientGameLaunch, urls.endPoints.origin2ClientGameLaunch);
    }


    return /** @lends module:Origin.module:client.module:remote */ {
        /**
         * launches a game remotely, initiates a download if the game is not installed and the autodownload flag is set to 1
         * @param {string} offerIds a comma delimited list of offers to look for
         * @param {boolean} autoDownload true if you want to download if not installed
         * @static
         * @method
         */
        gameLaunch: gameLaunch
    };
});