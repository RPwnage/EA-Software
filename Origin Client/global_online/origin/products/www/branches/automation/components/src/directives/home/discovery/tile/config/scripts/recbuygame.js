/**
 * @file home/discovery/tile/config/Recbuygame.js
 */

(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigRecbuygameCtrl($scope, $attrs, $element, $controller, PinRecommendationGamesFactory) {
        var controlGames = $element.children(),
            tileConfig = {
                directiveName: 'origin-home-discovery-tile-programmable-game',
                feedFunctionName: 'getRecommendedGamesToBuy',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit),
                numrecs: parseInt($attrs.numrecs),
                controlGames: controlGames
            };

        $scope.pinRecommendation = true;

        //let the bucket know of the filter function to use
        $scope.$emit('bucketFilter', {
            filterFunction: PinRecommendationGamesFactory.filterControlGames
        });

        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: tileConfig,
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigRecbuygame() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigRecbuygameCtrl',
            scope: true // allows the children of 'recbuygame' to see $scope.pinRecommendation
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigRecbuygame
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} numrecs The maximum number of recommendations we want returned
     * @description
     *
     * tile config for recommended game to buy
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigRecbuygameCtrl', OriginHomeDiscoveryTileConfigRecbuygameCtrl)
        .directive('originHomeDiscoveryTileConfigRecbuygame', originHomeDiscoveryTileConfigRecbuygame);
}());