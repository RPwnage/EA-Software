/**
 * @file home/discovery/tile/ognotplayedrecently.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ognotplayedrecently';

    function OriginHomeDiscoveryTileOgnotplayedrecentlyCtrl($scope, $controller) {
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: null
        });
    }

    function originHomeDiscoveryTileOgnotplayedrecently(ComponentsConfigFactory) {
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
            controller: 'OriginHomeDiscoveryTileOgnotplayedrecentlyCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgnotplayedrecently
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
     * not played recently discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ognotplayedrecently offerid="OFB-EAST:57198" descriptionstr='Get back into the game.' image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png"></origin-home-discovery-tile-ognotplayedrecently>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileOgnotplayedrecentlyCtrl', OriginHomeDiscoveryTileOgnotplayedrecentlyCtrl)
        .directive('originHomeDiscoveryTileOgnotplayedrecently', originHomeDiscoveryTileOgnotplayedrecently);
}());
