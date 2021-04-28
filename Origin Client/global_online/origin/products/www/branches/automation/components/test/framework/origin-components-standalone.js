/**
* @file test/framework/origin-components-standalone.js
*/
(function() {
    'use strict';

    /**
     * @ngdoc object
     * @name origin-components
     * @description
     * @requires  origin-i18n
     * @requires angular-toArrayFilter
     * @requires origin-components-config
     * @description
     *
     * component package for testing
     */
    angular.module('origin-components', ['origin-i18n', 'angular-toArrayFilter', 'origin-components-config']);
}());
