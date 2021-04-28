/**
 * @file common/componentsurl.js
 */
(function() {
    'use strict';

    function ComponentsUrlsFactory(UtilFactory, ComponentsLogFactory, COMPONENTSCONFIG) {

        function init() {
            Origin.utils.replaceTemplatedValuesInConfig(COMPONENTSCONFIG);

            ComponentsLogFactory.info('COMPONENT URLS:', COMPONENTSCONFIG.urls);

        }

        function getUrl(id) {
            return COMPONENTSCONFIG.urls[id];
        }
        /**
         * builds the YouTube Url based on videoId
         * @return {string} url of youtube video to embed.
         */
        function buildYouTubeEmbedUrl(videoId) {
            var hl = Origin.locale.locale().toLowerCase().substring(0, 2);
            return getUrl('youTubeEmbedUrl').replace('{videoId}', videoId).replace('{hl}', hl);
        }

        function buildCMSImageUrl(path) {
            return COMPONENTSCONFIG.hostname.basedata + path;
        }
        return {
            init: init,
            /**
             * returns the set of component service urls we should be using
             * @return {Promise} that resolves with the current set of componentUrls
             */
            getUrl: getUrl,
            getYouTubeEmbedUrl: buildYouTubeEmbedUrl,
            getCMSImageUrl: buildCMSImageUrl
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