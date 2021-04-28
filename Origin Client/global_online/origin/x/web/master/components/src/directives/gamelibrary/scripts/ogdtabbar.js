/**
 * @file gamelibrary/ogdtabbar.js
 */
(function(){
    'use strict';

    function originGamelibraryOgdTabBar(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdtabbar.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdTabBar
     * @restrict E
     * @element ANY
     * @scope
     */
    angular.module('origin-components')
        .directive('originGamelibraryOgdTabBar', originGamelibraryOgdTabBar);
})();