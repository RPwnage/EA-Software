/**
 * @file home/discovery/tile/ogrecentlyplayed.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogrecentlyplayed';

    function OriginHomeDiscoveryTileOgrecentlyplayedCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and description function (if needed)
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: null
        });
    }

    function originHomeDiscoveryTileOgrecentlyplayed(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionRaw: '@description',
                imageRaw: '@image',
                gamename: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileOgrecentlyplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description string for the tile
     * @param {LocalizedString|OCD} gamename the game name
     * @param {ImageUrl|OCD} image tile image 1000x250
     * @param {string} offerid offer id for the tile
     *
     * @description
     *
     * recently played discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ogrecentlyplayed image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_dai_long.png" offerid="OFB-EAST:1000032"></origin-home-discovery-tile-ogrecentlyplayed>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileOgrecentlyplayedCtrl', OriginHomeDiscoveryTileOgrecentlyplayedCtrl)
        .directive('originHomeDiscoveryTileOgrecentlyplayed', originHomeDiscoveryTileOgrecentlyplayed);
}());