/**
 * file gamelibrary/ogdcontentpanel.js
 */
(function() {

    'use strict';


    function originGamelibraryOgdContentPanel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdcontentpanel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdContentPanel
     * @restrict E
     * @element ANY
     * @scope
     */
    angular.module('origin-components')
        .directive('originGamelibraryOgdContentPanel', originGamelibraryOgdContentPanel);
})();