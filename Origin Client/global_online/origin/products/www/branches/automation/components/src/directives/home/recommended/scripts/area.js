/**
 * @file home/recommended/area.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedAreaCtrl($scope, $stateParams) {
        $scope.isLoading = true;
        //if this is triggered from ITO do not show the RNA area
        $scope.showArea = ($stateParams.source === 'ITO') ? false : true;
    }

    function originHomeRecommendedArea($compile, $timeout, RecommendedStoryFactory, ComponentsConfigFactory, ComponentsLogFactory) {

        function originHomeRecommendedAreaLink(scope, element) {
            RecommendedStoryFactory.getStoryDirective().then(function(response) {
                var recommendedTile;

                //response maybe null if its an underage child with no games, in that case we won't show any recommended action
                if (response) {
                    recommendedTile = element.find('.l-origin-home-recommended-tile');
                    //wrap the response in a div as compiling the directive that has targeting at the root level doesn't work 
                    //(same issue as if you tried to compile an ng-if at the root directive)
                    //seems to be with how ng-if and targeting place a comment as a place holder first
                    recommendedTile.html($compile('<div>'+response.outerHTML+'</div>')(scope));
                } else {
                    scope.showArea = false;
                }
                scope.isLoading = false;
                scope.$digest();

                if (response) {
                    var tealiumPayload = {
                        type: 'recommended',
                        tile: recommendedTile
                    };

                    $timeout(function() {
                        Origin.telemetry.events.fire('origin-tealium-discovery-tile-loaded', tealiumPayload);
                    }, 0, false);
                }
            }).catch(function(error) {
                ComponentsLogFactory.error('[origin-home-recommended-area] RecommendedStoryFactory.getStoryDirective', error);
            });

            /**
             * hides the RNA area
             */
            function hideRNA () {
                scope.showArea = false;
            }

            scope.$on('origin-home-RNA-hide', hideRNA);
        }

        return {
            restrict: 'E',
            scope: {}, //I too, live dangerously.
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
     * recommended next action area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-area>
     *         </origin-home-recommended-area>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedAreaCtrl', OriginHomeRecommendedAreaCtrl)
        .directive('originHomeRecommendedArea', originHomeRecommendedArea);
}());