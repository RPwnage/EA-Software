/**
 * @file home/recommended/area.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedAreaCtrl($scope) {
        $scope.isLoading = true;
    }

    function originHomeRecommendedArea($compile, RecommendedStoryFactory, ComponentsConfigFactory, ComponentsLogFactory) {

        function originHomeRecommendedAreaLink(scope) {
            RecommendedStoryFactory.getStoryDirective().then(function(response) {
                scope.isLoading = false;
                $('.l-origin-home-recommended-tile').html($compile(response)(scope));
                console.timeEnd('timeToFirstAction');
            }).catch(function(error) {
                ComponentsLogFactory.error('[origin-home-recommended-area] RecommendedStoryFactory.getStoryDirective', error.stack);
            });
        }

        return {
            restrict: 'E',
            scope: {
                anonymous: '@',
            }, //I too, live dangerously.
            controller: 'OriginHomeRecommendedAreaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/views/area.html'),
            link: originHomeRecommendedAreaLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedArea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * recommended next action area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-area>
     *
     *         </origin-home-recommended-area>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedAreaCtrl', OriginHomeRecommendedAreaCtrl)
        .directive('originHomeRecommendedArea', originHomeRecommendedArea);
}());