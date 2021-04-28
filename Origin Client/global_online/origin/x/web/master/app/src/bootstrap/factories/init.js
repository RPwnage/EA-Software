/**
 * @file bootstrap/factories/init.js
 */
(function() {
    'use strict';

    function BootstrapInitFactory($cookies, APPCONFIG, HttpUtilFactory, PreloadFactory, OverridesFactory, LogFactory, LOCDICTIONARY) {
        var locale,
            promiseMap = {
                initLocDictionaryPromise: 0,
                initAppUrlsPromise: 1,
                initJSSDKPromise: 2
            };

        /**
         * get the locale from cookie if one exists
         * @return {string} locale
         */
        function determineLocale() {
            return $cookies.locale || 'en_US';
        }

        /**
         * writes an entry to localstorage to state that the client has the data to run offline mode [EMBEDDED BROWSER ONLY]
         * @param {boolean} offlineGoodToGo flag to state whether we have the files we need to run offline
         */
        function setOfflineCapable(offlineGoodToGo) {
            if (Origin.client.isEmbeddedBrowser() && Origin.client.onlineStatus.isOnline()) {
                var jsonS = JSON.stringify(offlineGoodToGo);
                localStorage.setItem('offlineCapable', jsonS);
                Origin.client.settings.writeSetting('SPAPreloadSuccess', offlineGoodToGo);
                LogFactory.log('BootstrapInitFactory: setting SPAPreloadSuccess=', offlineGoodToGo);
            }
        }

        /**
         * writes the returned dictionary data from server to LOCDICTIONARY
         * @param {object} data the data returned by the http request
         */
        function handleLocDictionaryResponse(data) {
            LOCDICTIONARY.defaults.dictionary = data;
            return data;
        }

        /**
         * init the dictionary needed for locale
         * @returns {promise}
         */
        function initLocDictionary() {
            //we may want to move this to within the loc module and just passin the LOCDICTIONARY.defaults.dictionary to initLocale

            //dictonary can be either url or dictionaryObject
            var dictionary = LOCDICTIONARY.defaults.dictionary,
                dictionaryIsLocal = angular.isObject(dictionary),
                dictionaryIsHttpUrl = angular.isString(dictionary),
                promiseObject = null;


            //if its a url lets grab the information
            if (dictionaryIsHttpUrl) {
                promiseObject = HttpUtilFactory.getJsonFromUrl(dictionary.replace('{locale}', locale))
                    .then(handleLocDictionaryResponse, handleLocDictionaryResponse);
            } else if (dictionaryIsLocal) {
                promiseObject = Promise.resolve(dictionary);
            } else {
                //cause an exception if we don't have a dictionary
                throw 'No dictionary found';
            }
            return promiseObject;

        }

        /**
         * returns a boolean for the preload result
         * @param {boolean} true if preload was successful, false if not
         */
        function handlePreloadResult() {
            LogFactory.log('[INIT:preload COMPLETED]');
            return true;
        }

        /**
         * returns a boolean for the preload result
         * @param {boolean} true if preload was successful, false if not
         */
        function handlePreloadFailure(error) {
            LogFactory.error('[INIT:preload FAILED]', error.stack);
            return false;
        }

        /**
         * init the appurls and preload any templates we need for offline
         * @returns {promise}
         */
        function initAppUrlsAndOfflineCache() {
            //replace the tokens with the actual values
            Origin.utils.replaceTemplatedValuesInConfig(APPCONFIG);   

            //temp: log out the urls easier for testing         
            LogFactory.info('APP URLS:', APPCONFIG.urls);

            return PreloadFactory.preload(APPCONFIG)
                .then(handlePreloadResult, handlePreloadFailure);
        }

        function initJSSDK() { 
            if(window.opener && window.opener.Origin) { 

                Origin = window.opener.Origin; 
                return Promise.resolve(); 

            } 
            else { 
                return Origin.init(); 
            } 
        }


        /**
         * function waits for all promises to resolve then resolves itself
         * @returns {promise}
         */
        function getAsynchronousDependencies() {
            var asyncDependencyArray = [];
            asyncDependencyArray[promiseMap.initLocDictionaryPromise] = initLocDictionary();
            asyncDependencyArray[promiseMap.initAppUrlsPromise] = initAppUrlsAndOfflineCache();
            asyncDependencyArray[promiseMap.initJSSDKPromise] = initJSSDK();
            return Promise.all(asyncDependencyArray);
        }

        /**
         * handles the results of the batched async dependencies requests
         * @param  {object} results the results of the async requests are stored in an object under properties that correspond to the object used in getAsynchronousDependencies.
         */
        function handleAsyncDependencyResults(results) {
            //set offline capable with the result of the preload promise.
            setOfflineCapable(results[promiseMap.initAppUrlsPromise]);
        }
        /**
         * the init function to kick off all the async requests
         * @returns {promise}
         */
        function init() {

            locale = determineLocale();

            //first parse any overrides, then grab the asynchronouse data
            return OverridesFactory.getOverrides()
                .then(getAsynchronousDependencies)
                .then(handleAsyncDependencyResults);
        }

        return {
            /**
             * the init function to kick off all the async requests
             * @returns {promise}
             */
            init: init
        };

    }

    /**
     * @ngdoc service
     * @name bootstrapInit.factories.BootstrapInitFactory
     * @requires $q
     * @requires HttpUtilFactory
     * @requires AppUrlsFactory
     * @requires PreloadFactory
     * @requires OverridesFactory
     * @requires LOCDICTIONARY
     *
     * @description
     *
     * This module handles parsing query parameters to setup up the proper config urls and cms origins
     * It also handles parsing any individual overrides from the EACoreIniUtil and hardcoded override urls
     */
    angular.module('bootstrap')
        .factory('BootstrapInitFactory', BootstrapInitFactory);

}());