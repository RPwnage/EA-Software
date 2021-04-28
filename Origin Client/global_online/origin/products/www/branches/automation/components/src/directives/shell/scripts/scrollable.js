/**
 * @file shell/scripts/scrollable.js
 */
(function() {
    'use strict';

    function originShellScrollable() {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            template: '<div class="origin-navigation-scrollable-container" ng-transclude></div>'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellScrollable
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * scrollable container for shell menu, wraps all scrollable items
     *
     *
     */
    angular.module('origin-components')
        .directive('originShellScrollable', originShellScrollable);
}());