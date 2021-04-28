/**
 * @file factories/appurls.js
 */
(function() {
    'use strict';

    function AppUrlsFactory(APPCONFIG) {
        return {
            getUrl: function(id) {
                return APPCONFIG.urls[id];
            }

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
    angular.module('originApp')
        .factory('AppUrlsFactory', AppUrlsFactory);

}());