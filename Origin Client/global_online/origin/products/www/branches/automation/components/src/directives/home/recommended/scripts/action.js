/**
 * @file home/recommended/action.js
 */
(function() {
    'use strict';

    function originHomeRecommendedAction() {
        return {
            restrict: 'E'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedAction
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * recommended next action cms container
     *
     */
    angular.module('origin-components')
        .directive('originHomeRecommendedAction', originHomeRecommendedAction);
}());