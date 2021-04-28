/**
 * @file home/discovery/tile/newdlc.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-newdlc';

    function OriginHomeDiscoveryTileNewdlcCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and description function (if needed)
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: null
        });
    }

    function originHomeDiscoveryTileNewdlc(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionRaw: '@description',
                gamename: '@',
                offerId: '@offerid',
                discoverTileImage: '@',
                discoverTileColor: '@'
            },
            controller: 'OriginHomeDiscoveryTileNewdlcCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileNewdlc
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description string for the tile
     * @param {LocalizedString} gamename the game name
     * @param {string} offerid offer id for the tile
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     *
     * @description
     *
     * new dlc discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-newdlc offerid="OFB-EAST:109550761" image="https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/231.0x326.0/1004841_LB_231x326_en_US_^_2014-01-23-13-45-06_be769fbd3057bde6a82b49b2ab20e2ddeb628f90.jpg"></origin-home-discovery-tile-newdlc>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileNewdlcCtrl', OriginHomeDiscoveryTileNewdlcCtrl)
        .directive('originHomeDiscoveryTileNewdlc', originHomeDiscoveryTileNewdlc);
}());
