/**
* URL Helper utlilities
* @file url-helper-factory.js
*/
(function() {
    'use strict';

    function LocUrlHelper() {

        /**
        * Utility method to get a parameter value from the url
        * @param {String} str - the param name in the url you want the value of
        * @return {String}
        */
        function getParamValue(str) {
            var boom = window.location.search.slice(1).split('&'), //headshot
                smallerBoom;
            for (var i=0, j=boom.length; i<j; i++) {
                smallerBoom = boom[i].split('='); // smaller headshot
                if (smallerBoom.length && decodeURIComponent(smallerBoom[0]) === str) {
                    return smallerBoom[1] ? decodeURIComponent(smallerBoom[1]) : '';
                }
            }
            return '';
        }

        return {
            getParamValue: getParamValue
        };
    }

    /**
     * @ngdoc service
     * @name origin-i18n.factories.LocUrlHelper
     *
     * @description
     *
     * Url helper utilities
     */
    angular.module('origin-i18n')
        .factory('LocUrlHelper', LocUrlHelper);

}());
