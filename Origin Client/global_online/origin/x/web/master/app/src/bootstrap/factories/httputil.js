/**
 * @file bootstrap/factories/httputil.js
 */
(function() {
    'use strict';

    function HttpUtilFactory($http, LogFactory) {
        function extractData(response) {
            var status = response.status,
                data = response.data;

            if (status !== 200 && data !== 'object') {
                data = {};
                LogFactory.warn('ERROR: error retrieving JSON data from: ' + response.config.url);
            }

            return data;
        }

        function getJsonFromUrl(url) {
            return $http.get(url).then(extractData, extractData);
        }
        
        return {
            getJsonFromUrl: getJsonFromUrl
        };

    }

    /**
     * @ngdoc service
     * @name originApp.factories.HttpUtilFactory
     * @requires $http
     * @description
     *
     * HttpUtilFactory
     */
    angular.module('bootstrap')
        .factory('HttpUtilFactory', HttpUtilFactory);

}());