/**
 * @file bootstrap/factories/appurls.js
 */
(function() {
    'use strict';

    function AppUrlsFactory(HttpUtilFactory, APPCONFIG) {
        var commonConfig = APPCONFIG.common;

        /**
         * takes a list of token values and recursive traverses and object and replaces all occurances
         * @param  {object} data            a json object
         * @param  {array} replacementObject a object of token and values to replace
         */
        function replaceInObject(data, replacementObject) {
            for (var key in data) {
                if (!data.hasOwnProperty(key)) {
                    continue;
                }
                if (typeof data[key] === 'object') {
                    replaceInObject(data[key], replacementObject);
                } else {
                    for (var prop in replacementObject) {
                        if (replacementObject.hasOwnProperty(prop)) {
                            data[key] = data[key].replace(prop, replacementObject[prop]);
                        }
                    }
                }
            }
        }


        /**
         * merges data (urls pulled from server) with the fallback urls and override urls
         * @param  {object} data            the urls/config returned from server
         * @returns {object} the current config object for app
         */
        function mergeUrlsWithFallbacksAndOverrides(data) {
            //first we mix in any urls we get from the server
            Origin.utils.mix(APPCONFIG.app, data);
            //then we mix in any overriden urls for the app
            Origin.utils.mix(APPCONFIG.app, APPCONFIG.overrides);

            return APPCONFIG.app;
        }


        /**
         * apply the response from the retrieve app urls to our current set of urls
         * @param  {object} response the response from the config url call
         * @param  {[type]} resolve  the resolve function of the promise
         */
        function replaceTokens(locale) {
            return function(data) {
                commonConfig.cmsOrigin = commonConfig.cmsOrigin.replace('{stage}', commonConfig.cmsstage);
                replaceInObject(data, {
                    '{cmsOrigin}': commonConfig.cmsOrigin,
                    '{locale}': locale
                });
                return data;
            };
        }

        /**
         * builds the dynamic service url to use
         * @returns {string} the dynamic service url to use
         */
        function buildDynServiceUrl() {
            return commonConfig.dynServicesOrigin + commonConfig.dynServicesPath.replace('{module}', APPCONFIG.app.filename).replace('{env}', commonConfig.env);
        }

        /**
         * grabs the config url from the ConfigurationFactory then retrives the service urls from the config url
         * @return {Promise} promise which resolves with the list of app service urls we should use
         */
        function retrieveUrls(locale) {
            var configUrl = buildDynServiceUrl();
            return HttpUtilFactory.getJsonFromUrl(configUrl)
                .then(mergeUrlsWithFallbacksAndOverrides)
                .then(replaceTokens(locale));

        }

        return {
            retrieveUrls: retrieveUrls
        };

    }

    /**
     * @ngdoc service
     * @name originApp.factories.AppUrlsFactory
     * @requires HttpUtilFactory
     * @requires APPCONFIG
     * @description
     *
     * AppUrlsFactory
     */
    angular.module('bootstrap')
        .factory('AppUrlsFactory', AppUrlsFactory);

}());