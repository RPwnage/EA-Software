/**
 * @file home/discovery/tile/ognotplayedrecently.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ognotplayedrecently';

    function originHomeDiscoveryTileOgnotplayedrecently(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileOgnotplayedrecentlyLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
            scope.description = description ? description + ' ' : description;
            ctrl.init();
        }

        return {
            restrict: 'E',
            scope: {
                descriptionstr: '@description',
                image: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileOgnotplayedrecentlyLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgnotplayedrecently
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description string for the tile
     * @param {ImageUrl} image  image for the tile
     * @param {string} offerid offer id for the tile
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
        .directive('originHomeDiscoveryTileOgnotplayedrecently', originHomeDiscoveryTileOgnotplayedrecently);
}());