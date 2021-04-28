/**
 * @file home/discovery/tile/ugfriendsplaying.js
 */
(function() {
    'use strict';


    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ugfriendsplaying';

    function customDescriptionFunction() {
        //this is intentionally left blank, as we don't want to set a description for this directive
    }

    function OriginHomeDiscoveryTileUgfriendsplayingCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and description function (if needed)        
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: customDescriptionFunction
        });
    }

    function originHomeDiscoveryTileUgfriendsplaying(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionRaw: '@description',
                imageRaw: '@image',
                gamename: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileUgfriendsplayingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileUgfriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} gamename the game name
     * @param {ImageUrl|OCD} image tile image 1000x250
     * @param {string} offerid offer id for the tile
     * @description
     *
     * friends playing unowned game discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ugfriendsplaying offerid="OFB-EAST:49753" image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/discovery/programmed/tile_bejeweled3_long.jpg"></origin-home-discovery-tile-ugfriendsplaying>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileUgfriendsplayingCtrl', OriginHomeDiscoveryTileUgfriendsplayingCtrl)
        .directive('originHomeDiscoveryTileUgfriendsplaying', originHomeDiscoveryTileUgfriendsplaying);
}());