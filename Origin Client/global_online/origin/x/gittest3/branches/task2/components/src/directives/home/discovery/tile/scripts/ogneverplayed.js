/**
 * @file home/discovery/tile/ogneverplayed.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogneverplayed';

    function originHomeDiscoveryTileOgneverplayed(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileOgneverplayedLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            scope.description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description') + ' ';
            ctrl.init();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionstr: '@description',
                image: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileOgneverplayedLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgneverplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description string for the tile
     * @param {ImageUrl|OCD} image  image for the tile
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
        .directive('originHomeDiscoveryTileOgneverplayed', originHomeDiscoveryTileOgneverplayed);
}());