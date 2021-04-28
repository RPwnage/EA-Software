/**
 * @file home/discovery/tile/config/newdlc.js
 */
(function() {
    'use strict';


    function OriginHomeDiscoveryTileConfigNewdlcCtrl($scope, $attrs, $controller) {
        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            tileConfig: {
                directiveName: 'origin-home-discovery-tile-newdlc',
                feedFunctionName: 'getUnownedDLC',
                priority: parseInt($attrs.priority),
                diminish: parseInt($attrs.diminish),
                limit: parseInt($attrs.limit)
            },
            $scope: $scope
        });
    }

    function originHomeDiscoveryTileConfigNewdlc() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigNewdlcCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigNewdlc
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @description
     *
     * tile config for new dlc discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigNewdlcCtrl', OriginHomeDiscoveryTileConfigNewdlcCtrl)
        .directive('originHomeDiscoveryTileConfigNewdlc', originHomeDiscoveryTileConfigNewdlc);
}());