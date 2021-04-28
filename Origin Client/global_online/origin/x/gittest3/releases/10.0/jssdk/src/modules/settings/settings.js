/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/defines',
    'core/logger',
    'core/user',
    'core/auth',
    'core/events',
    'core/dataManager',
    'core/config',
    'core/urls',
    'core/errorhandler',
], function(Promise, defines, logger, user, auth, events, dataManager, config, urls, errorhandler) {


    function retrieveUserSettings(category) {
        /* 
        possible categories:
        ACCOUNT,
        GENERAL,
        NOTIFICATION,
        INGAME,
        HIDDENGAMES,
        FAVORITEGAMES,
        TELEMETRYOPTOUT,
        ENABLEIGO,
        ENABLECLOUDSAVING,
        BROADCASTING;
        */
        var env = config.environment(),
            endPoint = urls.endPoints.settingsData,
            token = user.publicObjs.accessToken(),
            requestConfig = {
                atype: 'GET',
                headers: [],
                parameters: [],
                appendparams: [],
                reqauth: true,
                requser: false
            };

        //we need to substitute
        endPoint = endPoint.replace('{userId}', user.publicObjs.userPid());
        if(category) {
            endPoint += '/' + category;            
        }
        if (token.length > 0) {
            dataManager.addHeader(requestConfig, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, requestConfig, 0)
            .catch(errorhandler.logAndCleanup('SETTINGS:retrieveUserSettings FAILED'));
    }

    function postUserSettings(category, value) {
        /* 
        possible categories:
        ACCOUNT,
        GENERAL,
        NOTIFICATION,
        INGAME,
        HIDDENGAMES,
        FAVORITEGAMES,
        TELEMETRYOPTOUT,
        ENABLEIGO,
        ENABLECLOUDSAVING,
        BROADCASTING;
        */
        var env = config.environment();
        var endPoint = urls.endPoints.settingsData;

        //we need to substitute
        endPoint = endPoint.replace('{userId}', user.publicObjs.userPid());

        if(category) {
            endPoint += '/' + category;            
        }

        var requestConfig = {
            atype: 'POST',
            headers: [],
            parameters: [],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        var token = user.publicObjs.accessToken();

        if (token.length > 0) {
            dataManager.addHeader(requestConfig, 'AuthToken', token);
        }


        requestConfig.body = value;


        var promise = dataManager.enQueue(endPoint, requestConfig, 0);
        return promise
            .catch(errorhandler.logAndCleanup('SETTINGS:postUserSettings FAILED'));
    }

    /** @namespace
     * @memberof Origin
     * @alias settings
     */
    return {

        /**
         * This will return a promise for the requested user settings from server
         *
         * @return {promise}  response returns the current settings as key value pairs in JSON format
         * @method
         */
        retrieveUserSettings: retrieveUserSettings,

        /**
         * This will return a promise for the posted user settings
         *
         * @return {promise}  response returns the current settings as key value pairs in JSON format
         * @method
         */
        postUserSettings: postUserSettings

    };
});