/**
 * @file shell/navigation.js
 */
(function() {
    'use strict';

    function originShellNavigation() {
        return {
            restrict: 'E',
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellNavigation
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * discovery tile container
     *
     */
    angular.module('origin-components')
        .directive('originShellNavigation', originShellNavigation);
}());