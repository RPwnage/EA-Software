/**
 * @file app.js
 */
(function() {
    'use strict';

    function setComponentsBasePath(ComponentsConfigFactory) {
        ComponentsConfigFactory.setBasePath('bower_components/origin-components/');
    }

    /**
    * Add the language to the html element
    */
    function initHtmlLang() {
        var originLocaleApi = OriginLocale.parse(window.opener.location.href, 'en-us', 'usa'); // jshint ignore:line
        angular.element('html').attr('lang', originLocaleApi.getLanguageCode());
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
        .run(setComponentsBasePath)
        .run(initHtmlLang);
}());
