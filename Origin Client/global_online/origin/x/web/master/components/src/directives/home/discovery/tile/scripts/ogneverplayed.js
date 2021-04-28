/**
 * @file home/discovery/tile/ogneverplayed.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogneverplayed';

    function OriginHomeDiscoveryTileOgneverplayedCtrl($scope, $controller) {
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: null
        });
    }

    function originHomeDiscoveryTileOgneverplayed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionRaw: '@description',
                imageRaw: '@image',
                gamename: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileOgneverplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgneverplayed
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
     * never played discovery tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ogneverplayed offerid="OFB-EAST:57198" image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png"></origin-home-discovery-tile-ogneverplayed>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileOgneverplayedCtrl', OriginHomeDiscoveryTileOgneverplayedCtrl)
        .directive('originHomeDiscoveryTileOgneverplayed', originHomeDiscoveryTileOgneverplayed);
}());