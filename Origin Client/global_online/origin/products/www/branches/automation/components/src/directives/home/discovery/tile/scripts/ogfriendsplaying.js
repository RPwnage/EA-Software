/**
 * @file home/discovery/tile/ogfriendsplaying.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogfriendsplaying';

    function customDescriptionFunction() {
        //this is intentionally left blank, as we don't want to set a description for this directive
    }

    function OriginHomeDiscoveryTileOgfriendsplayingCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and description function (if needed)
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: customDescriptionFunction
        });
    }

    function originHomeDiscoveryTileOgfriendsplaying(ComponentsConfigFactory) {

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
            controller: 'OriginHomeDiscoveryTileOgfriendsplayingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgfriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description string for the tile
     * @param {LocalizedString} gamename the game name
     * @param {string} offerid offer id for the tile
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @description
     *
     * friends playing discovery tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ogfriendsplaying offerid="OFB-EAST:49753" image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/discovery/programmed/tile_bejeweled3_long.jpg"></origin-home-discovery-tile-ogfriendsplaying>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileOgfriendsplayingCtrl', OriginHomeDiscoveryTileOgfriendsplayingCtrl)
        .directive('originHomeDiscoveryTileOgfriendsplaying', originHomeDiscoveryTileOgfriendsplaying);
}());
