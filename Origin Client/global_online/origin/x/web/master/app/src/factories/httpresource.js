/* HttpResourceFactory
 *
 * generic way of retrieving JSON resources over HTTP.
 * @todo we should probably replace it with ngResource module in the future
 * @file factories/httpresource.js
 */
(function() {
    'use strict';

    function HttpResourceFactory($http, LogFactory) {
        /**
        * Get the endpoint for the JSON file for a locale.
        * @return {String} - the resource endpoint
        */
        function getEndpoint(endpointTemplate, locale) {
            var normalizedLocale = locale.toLowerCase().replace('_', '-'),
                endpoint = endpointTemplate.replace('%locale%', normalizedLocale);

            return endpoint;
        }

        /**
        * Gets CMS resource for given locale
        * @param {String} endpointTemplate - the resource endpoint
        * @param {String} locale - locale
        * @param {Function} callback - function to call when the data is ready
        * @return {Object} - the resource
        */
        function get(endpointTemplate, locale, callback) {
            var endpoint = getEndpoint(endpointTemplate, locale);

            $http.get(endpoint)
                .success(callback)
                .error(function() {
                    LogFactory.log('Error getting HTTP resource: ' + endpoint);
                    callback({});
                });
        }

        return {
            get: get
        };

    }

    /**
     * @ngdoc service
     * @name originApp.factories.HttpResourceFactory
     * @requires $http
     * @description
     *
     * Wrapper for HTTP resources. To be replaced by $resource in the future
     */
    angular.module('originApp')
        .factory('HttpResourceFactory', HttpResourceFactory);
}());
