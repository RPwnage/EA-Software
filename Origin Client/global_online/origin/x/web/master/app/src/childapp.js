/**
 * @file app.js
 */
(function() {
    'use strict';

    function setComponentsBasePath(ComponentsConfigFactory) {
        ComponentsConfigFactory.setBasePath('bower_components/origin-components/');
    }
    

    /**
     * @ngdoc object
     * @name originApp
     * @requires origin-components
     * @requires origin-i18n
     * @description
     *
     * simple app for popup windows
     *
     */

    angular.module('childapp', ['origin-components'])
        .run(setComponentsBasePath);
}());