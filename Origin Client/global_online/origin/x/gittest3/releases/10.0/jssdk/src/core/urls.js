/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'core/defines',
    'core/dataManager',
    'core/utils',
    'core/logger',
    'generated/jssdkconfig.js'
], function(defines, dataManager, utils, logger, jssdkconfig) {

    function buildDynServiceUrl() {
        return jssdkconfig.common.dynServicesOrigin + jssdkconfig.common.dynServicesPath.replace('{module}', jssdkconfig.jssdk.filename).replace('{env}', jssdkconfig.common.env);
    }

    function requestUrlsForEnv() {
        var endPoint = buildDynServiceUrl(),
            config = {
                atype: 'GET',
                headers: [],
                parameters: [],
                reqauth: false,
                requser: false
            };

        return dataManager.dataREST(endPoint, config);
    }

    function mergeUrlsWithFallbacksAndOverrides(data) {
        //first we mix in any urls we get from the server
        utils.mix(jssdkconfig.jssdk, data);
        //then we mix in any overriden urls for the jssdk
        utils.mix(jssdkconfig.jssdk, jssdkconfig.overrides);

        return jssdkconfig.jssdk;
    }

    function handleError(error) {
        logger.warn('JSSDK: could not retrieve dynamic urls -- using defaults - Error:', error);
        return mergeUrlsWithFallbacksAndOverrides({});
    }

    return {

        init: function() {
            return requestUrlsForEnv().then(mergeUrlsWithFallbacksAndOverrides, handleError);
        },

        /**
         * endpoints used
         */
        endPoints: jssdkconfig.jssdk.urls
    };
});