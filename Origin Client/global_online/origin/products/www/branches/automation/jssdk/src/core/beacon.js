/*jshint strict: false */


define([
    'core/dataManager',
    'core/urls',
    'generated/jssdkconfig.js'
], function(dataManager, urls, jssdkconfig) {

    /**
     * Contains methods to query the Origin client for installation and version
     * @module module:beacon
     * @memberof module:Origin
     */

    /**
     * The property in which the beacon response value is stored in the JSON beacon response.
     * @constant
     * @default
     * @type {string}
     */
    var BEACON_RESPONSE_PROPERTY = 'resp';

    /**
     * The beacon response pong string
     * @constant
     * @default
     * @type {string}
     */
    var BEACON_PONG = 'pong';

    /**
     * The configuration used for dataManager to query the Origin beacon service.
     */
    var beaconQueryConfig = {
        atype: 'GET',
        headers: [],
        parameters: [],
        reqauth: false,
        requser: false
    };

    /**
     * Installed clients will expose a version number.
     *
     * @param  {Object} response the response from data manager
     * @return {string} the version number or undefined if the property is empty
     */
    function handleVersionResponse(response) {
        if (response.hasOwnProperty(BEACON_RESPONSE_PROPERTY)) {
            return response[BEACON_RESPONSE_PROPERTY];
        }

        return undefined;
    }

    /**
     * In the case of an HTTP error, resolve to an undefined client version value
     *
     * @return {Promise.<undefined, Error>}
     */
    function handleVersionError() {
        return Promise.resolve(undefined);
    }

    /**
     * Determine if the client is actively running
     *
     * @param  {Object} response the response from data manager
     * @return {Boolean} true if running
     */
    function handleRunningResponse(response) {
        if (response.hasOwnProperty(BEACON_RESPONSE_PROPERTY) && response[BEACON_RESPONSE_PROPERTY] === BEACON_PONG) {
            return true;
        }

        return false;
    }

    /**
     * In the case of an HTTP error, resolve to false for running status
     *
     * @return {Promise.<Boolean, Error>}
     */
    function handleRunningError() {
        return Promise.resolve(false);
    }

    /**
     * use the provided user agent string override or fallback to window navigator
     *
     * @param  {string} userAgent the user agent string override
     * @return {string} the user agent string
     */
    function getUserAgent(userAgent) {
        return userAgent || window.navigator.userAgent;
    }

    /**
     * Check if the user agent matches the minimum Operating system requirments for Origin to run correctly on Windows
     *
     * @param {string} userAgent the user agent string to use
     * @return {Boolean}
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832%28v=vs.85%29.aspx - list of NT Versions
     */
    function isInstallableOnWindows(userAgent) {
        var windowsCheck = new RegExp('\\bWindows NT ([\\d\\.]+)').exec(userAgent);
        if (windowsCheck instanceof Array && windowsCheck.length === 2) {
            var versionNumber = parseFloat(windowsCheck[1], 10); //Windows NT
            return (versionNumber >= jssdkconfig.osminversion.windowsnt) ? true : false; //6.1 is Windows 7
        }

        return false;
    }

    /**
     * Check if the user agent matches the minimum Operating system requirments for Origin to run correctly on Mac OS
     *
     * @param {string} userAgent the user agent string to use
     * @return {Boolean}
     * @see http://www.useragentstring.com/pages/Safari
     */
    function isInstallableOnMac(userAgent) {
        var macCheck = new RegExp('\\bMacintosh; Intel Mac OS X 10[\\._]([\\d_\\.]+)').exec(userAgent);
        if (macCheck instanceof Array && macCheck.length === 2) {
            var versionNumber = parseFloat(macCheck[1].replace('_','.'), 10); //Macintosh; Intel Mac OS X 10_11_2 (trim off the 10_ and use remaining number)
            return (versionNumber >= jssdkconfig.osminversion.macosx) ? true : false; //10.7 or higher
        }

        return false;
    }

    /**
     * Check both platforms
     *
     * @param {string} userAgent the user agent string to use
     * @return {Boolean}
     */
    function isInstallableOnMacOrPc(userAgent) {
        return (isInstallableOnWindows(userAgent) || isInstallableOnMac(userAgent));
    }

    function installed() {
        var endpoint = urls.endPoints.localOriginClientBeaconVersion;

        return dataManager.dataREST(endpoint, beaconQueryConfig)
            .then(handleVersionResponse)
            .catch(handleVersionError);
    }

    function installable(userAgent) {
        userAgent = getUserAgent(userAgent);

        return new Promise(function(resolve) {
            if (isInstallableOnMacOrPc(userAgent)) {
                resolve(true);
            } else {
                resolve(false);
            }
        });
    }

    function installableOnPlatform(platform, userAgent) {
        var catalogPlatforms = {
                'PCWIN': isInstallableOnWindows,
                'MAC': isInstallableOnMac
            };

        userAgent = getUserAgent(userAgent);

        return new Promise(function(resolve) {
            if (catalogPlatforms[platform] && catalogPlatforms[platform].apply(undefined, [userAgent])) {
                resolve(true);
            } else {
                resolve(false);
            }
        });
    }

    function running() {
        var endpoint = urls.endPoints.localOriginClientPing;

        return dataManager.dataREST(endpoint, beaconQueryConfig)
            .then(handleRunningResponse)
            .catch(handleRunningError);
    }

    return  {
        /**
         *
         * Returns a promise that resolves if Origin client is installed, rejects otherwise
         *
         * @method
         * @static
         * @returns {Promise.<String, Error>} resolves to a boolean
         */
        installed: installed,

        /**
         *
         * Returns whether Origin can be installed on the current OS
         *
         * @method
         * @static
         * @param {string} userAgent optional user agent override
         * @returns {Promise.<Boolean, Error>} resolves to a boolean
         */
        installable: installable,

        /**
         *
         * Returns whether the client is installable for a specific platform
         *
         * @method
         * @static
         * @param {string} platform the catalog platform to query eg. PCWIN/MAC
         * @param {string} userAgent optional useragent override
         * @returns {Promise.<Boolean, Error>} resolves to a boolean
         */
        installableOnPlatform: installableOnPlatform,

        /**
         *
         * Returns whether Origin is running on the current machine
         *
         * @method
         * @static
         * @returns {Promise.<Boolean, Error>} resolves to a boolean
         */
        running: running
    };
});
