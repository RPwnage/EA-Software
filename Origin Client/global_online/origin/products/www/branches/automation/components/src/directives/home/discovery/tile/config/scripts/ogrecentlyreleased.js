/**
 * @file home/discovery/tile/config/ogrecentlyreleased.js
 */

(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigOgrecentlyreleasedCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-ogrecentlyreleased',
                feedFunctionName: 'getRecentlyReleased',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit),
                dayssincerelease: parseInt($attrs.dayssincerelease)

            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigOgrecentlyreleased() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigOgrecentlyreleasedCtrl',
            scope: true
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgrecentlyreleased
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} dayssincerelease the max number a days a game has been released to show in this list
     * @description
     *
     * tile config for recently released / preload discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigOgrecentlyreleasedCtrl', OriginHomeDiscoveryTileConfigOgrecentlyreleasedCtrl)
        .directive('originHomeDiscoveryTileConfigOgrecentlyreleased', originHomeDiscoveryTileConfigOgrecentlyreleased);
}());