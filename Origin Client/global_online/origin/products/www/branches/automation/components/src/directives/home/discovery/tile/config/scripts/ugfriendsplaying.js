/**
 * @file home/discovery/tile/config/ugfriendsplaying.js
 */

(function() {
    'use strict';

    function OriginHomeDiscoveryTileConfigUgfriendsplayingCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-ugfriendsplaying',
                feedFunctionName: 'getFriendsPlayingUnowned',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit)
            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigUgfriendsplaying() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigUgfriendsplayingCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigUgfriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @description
     *
     * tile config for unowned games friends playing discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigUgfriendsplayingCtrl', OriginHomeDiscoveryTileConfigUgfriendsplayingCtrl)
        .directive('originHomeDiscoveryTileConfigUgfriendsplaying', originHomeDiscoveryTileConfigUgfriendsplaying);
}());