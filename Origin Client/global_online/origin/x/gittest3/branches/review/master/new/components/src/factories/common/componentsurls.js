/**
 * @file common/componentsurl.js
 */
(function() {
    'use strict';

    function ComponentsUrlsFactory(UtilFactory, ComponentsLogFactory, COMPONENTSCONFIG) {
        var myEvents = new Origin.utils.Communicator(),
            commonConfig = COMPONENTSCONFIG.common,
            urlsInitialized = false,
            initInProgress = false,

            //default urls used by the component in case we cannot reach the config service
            componentUrls = COMPONENTSCONFIG.components;
        /**
         * retrieves the set of service urls we should be using for the components
         * @param  {string} url the config url
         * @return {object} a json object of service urls
         */

        function passOnResponse(response) {
            return response;
        }

        function handleFailedComponentUrls() {
            ComponentsLogFactory.warn('[ComponentsUrl: getComponentsUrls] retrieving urls failed');
            return {};
        }

        /**
         * takes a list of token values and recursive traverses and object and replaces all occurances
         * @param  {object} data            a json object
         * @param  {array} replacementList a list of token and values to replace
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
         * apply the response from the retrieve component urls to our current set of urls
         * @param  {object} response the response from the config url call
         * @param  {[type]} resolve  the resolve function of the promise
         */
        function mixUrlWithDefaultsAndOverrides(response) {


            if (response) {
                //first we mix in any urls we get from the server
                Origin.utils.mix(componentUrls, response);
            }
            //then we mix in any overriden urls for the components
            Origin.utils.mix(componentUrls, COMPONENTSCONFIG.overrides);
            return componentUrls;
        }

        function replaceTokensInResponse(urls) {

            var locale = Origin.locale.locale().toLowerCase().replace('_', '-');
            commonConfig.cmsOrigin = commonConfig.cmsOrigin.replace('{stage}', commonConfig.cmsstage);

            replaceInObject(urls, {
                '{cmsOrigin}': commonConfig.cmsOrigin,
                '{locale}': locale
            });

            return urls;
        }

        function notifyRetrieveFinished(urls) {
            initInProgress = false;
            urlsInitialized = true;

            myEvents.fire('initFinished', urls);
            return urls;
        }

        function urlForId(id) {
            return function(urls) {
                return urls[id];
            };
        }

        function buildDynServiceUrl() {
            return commonConfig.dynServicesOrigin + commonConfig.dynServicesPath.replace('{module}', COMPONENTSCONFIG.components.filename).replace('{env}', commonConfig.env);
        }

        /**
         * builds the YouTube Url based on videoId
         * @return {string} url of youtube video to embed.
         */
        function buildYouTubeEmbedUrl(videoId) {
            var hl = Origin.locale.locale().toLowerCase().substring(0,2);
            return componentUrls.youTubeEmbedUrl.replace('{videoId}', videoId).replace('{hl}', hl);
        }

        /**
         * grabs the config url from the ConfigurationFactory then retrives the service urls from the config url
         * @return {Promise} promise which resolves with the list of app service urls we should use
         */
        function retrieveUrls() {
            if (!initInProgress) {
                initInProgress = true;

                return UtilFactory.getJsonFromUrl(buildDynServiceUrl())
                    .then(passOnResponse, handleFailedComponentUrls)
                    .then(mixUrlWithDefaultsAndOverrides)
                    .then(replaceTokensInResponse)
                    .then(notifyRetrieveFinished);
            } else {
                return new Promise(function(resolve) {
                    //everytime this function gets called we want to register a one time listener
                    //if we already have a call in progress, don't make another call, but instead let the listener trigger the resolve
                    myEvents.once('initFinished', resolve);
                });
            }
        }

        return {
            /**
             * returns the set of component service urls we should be using
             * @return {Promise} that resolves with the current set of componentUrls
             */
            getUrl: function(id) {

                //if we've already initialized just resolve the current set of componentUrls
                if (urlsInitialized) {
                    return Promise.resolve(componentUrls[id]);
                } else {
                    //if not then lets go grab them
                    return retrieveUrls().then(urlForId(id));
                }
            },
            getYouTubeEmbedUrl: buildYouTubeEmbedUrl
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ComponentsUrlsFactorySingleton(UtilFactory, ComponentsLogFactory, COMPONENTSCONFIG, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ComponentsUrlsFactory', ComponentsUrlsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ComponentsUrlsFactory
     * @requires $http
     * @description
     *
     * ComponentsUrlsFactory
     */
    angular.module('origin-components')
        .factory('ComponentsUrlsFactory', ComponentsUrlsFactorySingleton);

}());