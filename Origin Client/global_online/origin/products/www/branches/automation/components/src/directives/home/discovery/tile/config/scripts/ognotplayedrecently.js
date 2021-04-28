/**
 * @file home/discovery/tile/config/ognotplayedrecently.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileConfigOgnotplayedrecentlyCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-ognotplayedrecently',
                feedFunctionName: 'getNotRecentlyPlayed',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit),
                notplayedlow: parseInt($attrs.notplayedlow),
                notplayedhigh: parseInt($attrs.notplayedhigh)

            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigOgnotplayedrecently() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigOgnotplayedrecentlyCtrl',
            scope: true
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgnotplayedrecently
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} notplayedlow the lower bound of days not played
     * @param {number} notplayedhigh the upper bound of days not played
     * @description
     *
     * tile config for not recently played discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigOgnotplayedrecentlyCtrl', OriginHomeDiscoveryTileConfigOgnotplayedrecentlyCtrl)
        .directive('originHomeDiscoveryTileConfigOgnotplayedrecently', originHomeDiscoveryTileConfigOgnotplayedrecently);
}());