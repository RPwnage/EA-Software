/**
 * @file home/discovery/tile/config/ogfriendsplaying.js
 */
(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigOgfriendsplayingCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-ogfriendsplaying',
                feedFunctionName: 'getFriendsPlayingOwned',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit)
            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigOgfriendsplaying() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigOgfriendsplayingCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgfriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @description
     *
     * tile config for friends playing discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigOgfriendsplayingCtrl', OriginHomeDiscoveryTileConfigOgfriendsplayingCtrl)
        .directive('originHomeDiscoveryTileConfigOgfriendsplaying', originHomeDiscoveryTileConfigOgfriendsplaying);
}());