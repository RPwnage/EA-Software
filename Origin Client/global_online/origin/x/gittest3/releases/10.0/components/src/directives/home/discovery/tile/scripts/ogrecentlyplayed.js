/**
 * @file home/discovery/tile/ogrecentlyplayed.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogrecentlyplayed';

    function originHomeDiscoveryTileOgrecentlyplayed(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileOgrecentlyplayedLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description') + ' ';
            scope.description = description ? description + ' ' : description;
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
            link: originHomeDiscoveryTileOgrecentlyplayedLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description string for the tile
     * @param {ImageUrl|OCD} image image for the tile
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
        .directive('originHomeDiscoveryTileOgrecentlyplayed', originHomeDiscoveryTileOgrecentlyplayed);
}());