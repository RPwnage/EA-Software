/**
 * @file home/discovery/tile/config/ogrecentlyplayed.js
 */
(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigOgrecentlyplayedCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-ogrecentlyplayed',
                feedFunctionName: 'getRecentlyPlayedNoFriends',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit),
                dayssinceplayed: parseInt($attrs.dayssinceplayed)
            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigOgrecentlyplayed() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigOgrecentlyplayedCtrl',
            scope: true
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} dayssinceplayed the number of days since the user last played
     * @description
     *
     * tile config for owned game recently played discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigOgrecentlyplayedCtrl', OriginHomeDiscoveryTileConfigOgrecentlyplayedCtrl)
        .directive('originHomeDiscoveryTileConfigOgrecentlyplayed', originHomeDiscoveryTileConfigOgrecentlyplayed);
}());