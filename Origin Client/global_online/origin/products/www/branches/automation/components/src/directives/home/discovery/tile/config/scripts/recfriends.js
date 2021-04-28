/**
 * @file home/discovery/tile/config/recfriends.js
 */

(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigRecfriendsCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-recfriends',
                feedFunctionName: 'getRecommendedFriends',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit)
            },
            $scope: $scope
        });

    }

    function originHomeDiscoveryTileConfigRecfriends() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigRecfriendsCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigRecfriends
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @description
     *
     * tile config for not recommended friends discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigRecfriendsCtrl', OriginHomeDiscoveryTileConfigRecfriendsCtrl)
        .directive('originHomeDiscoveryTileConfigRecfriends', originHomeDiscoveryTileConfigRecfriends);
}());
