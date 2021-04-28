/**
 * @file home/discovery/tile/ogfriendsplaying.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileOgfriendsplaying(ComponentsConfigFactory) {

        function originHomeDiscoveryTileOgfriendsplayingLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            ctrl.init();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                image: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileOgfriendsplayingLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgfriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image image for the tile
     * @param {string} offerid offer id for the tile
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
        .directive('originHomeDiscoveryTileOgfriendsplaying', originHomeDiscoveryTileOgfriendsplaying);
}());